// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "common/logging.h"
#include "common/thread_worker.h"
#include "common/unique_function.h"

namespace Vulkan {

/// Pipelines the game needs now, built on worker threads while the GPU thread goes on (unlike
/// PipelinePrebuild, which only takes work before it starts). The GPU thread polls for a result
/// and skips the draws that may be skipped meanwhile; for one that may not, Wait builds it on the
/// calling thread if no worker has started it, instead of queueing behind the others.
template <typename Key, typename Pipeline>
class RuntimePipelineBuilds {
public:
    using Build = Common::UniqueFunction<std::unique_ptr<Pipeline>>;

    enum class State {
        None,    ///< Not queued (never, or already taken)
        Pending, ///< Queued or building
        Ready,   ///< Done: the result was moved out (null when the build failed)
    };

    explicit RuntimePipelineBuilds(size_t threads_) : threads{threads_} {}

    ~RuntimePipelineBuilds() {
        Stop();
    }

    RuntimePipelineBuilds(const RuntimePipelineBuilds&) = delete;
    RuntimePipelineBuilds& operator=(const RuntimePipelineBuilds&) = delete;

    /// Queues `build` for `key`. False when stopped or when `key` is queued already.
    bool Submit(const Key& key, Build build) {
        auto entry = std::make_shared<Entry>();
        entry->build = std::move(build);
        {
            std::scoped_lock lock{mutex};
            if (stopped || entries.contains(key)) {
                return false;
            }
            entries.emplace(key, entry);
            if (!workers) {
                workers.reset(new Common::ThreadWorker(threads, "VkPipelineRuntime", {},
                                                       Common::ThreadPlacement::Background));
            }
        }
        workers->QueueWork([this, entry] { Run(*entry); });
        return true;
    }

    /// For a queued key: Pending while it builds, or Ready with the result moved to `out`.
    State Poll(const Key& key, std::unique_ptr<Pipeline>& out) {
        std::scoped_lock lock{mutex};
        const auto it = entries.find(key);
        if (it == entries.end()) {
            return State::None;
        }
        if (!it->second->done) {
            return State::Pending;
        }
        out = std::move(it->second->pipeline);
        entries.erase(it);
        return State::Ready;
    }

    /// The result for a queued key, building it here when no worker has started it yet. Null when
    /// the build failed or nothing is queued for `key`.
    std::unique_ptr<Pipeline> Wait(const Key& key) {
        std::shared_ptr<Entry> entry;
        {
            std::scoped_lock lock{mutex};
            const auto it = entries.find(key);
            if (it == entries.end()) {
                return nullptr;
            }
            entry = it->second;
        }
        Run(*entry);
        std::unique_lock lock{mutex};
        done_cv.wait(lock, [&] { return entry->done; });
        std::unique_ptr<Pipeline> result = std::move(entry->pipeline);
        entries.erase(key);
        return result;
    }

    [[nodiscard]] size_t PendingCount() {
        std::scoped_lock lock{mutex};
        size_t pending = 0;
        for (const auto& [key, entry] : entries) {
            pending += entry->done ? 0 : 1;
        }
        return pending;
    }

    /// Drops what no worker started and waits for the builds running now.
    void Stop() {
        std::unique_ptr<Common::ThreadWorker> joined;
        {
            std::scoped_lock lock{mutex};
            stopped = true;
            joined = std::move(workers);
        }
        joined.reset();
        std::scoped_lock lock{mutex};
        entries.clear();
    }

private:
    struct Entry {
        Build build;
        std::unique_ptr<Pipeline> pipeline;
        bool started{};
        bool done{};
    };

    /// Builds the entry unless another thread started it (a worker, or a Wait).
    void Run(Entry& entry) {
        Build build;
        {
            std::scoped_lock lock{mutex};
            if (entry.started) {
                return;
            }
            entry.started = true;
            if (!stopped) {
                build = std::move(entry.build);
            }
        }
        std::unique_ptr<Pipeline> pipeline;
        if (build) {
            try {
                pipeline = build();
            } catch (const std::exception& exception) {
                LOG_ERROR(Render_Vulkan, "Runtime pipeline build failed: {}", exception.what());
            } catch (...) {
                LOG_ERROR(Render_Vulkan, "Runtime pipeline build failed");
            }
        }
        build = {};
        {
            std::scoped_lock lock{mutex};
            entry.pipeline = std::move(pipeline);
            entry.done = true;
        }
        done_cv.notify_all();
    }

    const size_t threads;
    std::mutex mutex;
    std::condition_variable done_cv;
    bool stopped{};
    std::unordered_map<Key, std::shared_ptr<Entry>> entries;
    std::unique_ptr<Common::ThreadWorker> workers;
};

} // namespace Vulkan
