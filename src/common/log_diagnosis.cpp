// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/log_diagnosis.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>

namespace Common::LogDiagnosis {
namespace {

bool Contains(std::string_view text, std::string_view part) {
    return text.find(part) != std::string_view::npos;
}

bool ContainsNoCase(std::string_view text, std::string_view part) {
    return std::ranges::search(text, part, [](char a, char b) {
               return std::tolower(static_cast<unsigned char>(a)) ==
                      std::tolower(static_cast<unsigned char>(b));
           }).begin() != text.end();
}

std::vector<std::string_view> SplitLines(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        const size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        lines.push_back(line);
        if (end == std::string_view::npos) {
            break;
        }
        text.remove_prefix(end + 1);
    }
    return lines;
}

bool IsProblem(std::string_view line) {
    return Contains(line, "<Critical>") || Contains(line, "<Error>");
}

// The line without its timestamp, so repeats of one message count as one.
std::string_view Message(std::string_view line) {
    if (line.starts_with('[')) {
        if (const size_t end = line.find(']'); end != std::string_view::npos) {
            line.remove_prefix(end + 1);
        }
    }
    while (!line.empty() && line.front() == ' ') {
        line.remove_prefix(1);
    }
    return line;
}

// The text after `marker` up to the next space, from the first line that has it.
std::string After(std::string_view line, std::string_view marker) {
    const size_t at = line.find(marker);
    if (at == std::string_view::npos) {
        return {};
    }
    std::string_view rest = line.substr(at + marker.size());
    return std::string{rest.substr(0, rest.find_first_of(" |"))};
}

} // Anonymous namespace

std::string Redact(std::string_view line) {
    std::string out{line};
    // The folder after these prefixes is the user's name.
    constexpr std::array prefixes{std::string_view{"/home/"}, std::string_view{"/Users/"},
                                  std::string_view{"\\Users\\"}, std::string_view{"/users/"}};
    for (const std::string_view prefix : prefixes) {
        size_t at = 0;
        while ((at = out.find(prefix, at)) != std::string::npos) {
            const char separator = prefix.back();
            const size_t name_end = out.find(separator, at + prefix.size());
            size_t start = at;
            // "C:\Users\name": take the drive letter too.
            if (separator == '\\' && start >= 2 && out[start - 1] == ':') {
                start -= 2;
            }
            if (name_end == std::string::npos) {
                out.replace(start, out.size() - start, "~");
                break;
            }
            out.replace(start, name_end - start, "~");
            at = start + 1;
        }
    }
    return out;
}

Analysis Analyze(std::string_view log) {
    Analysis analysis;
    const std::vector<std::string_view> all = SplitLines(log);

    // The last game session starts at its "Booting game" line; with none, the whole log is the
    // problem (Kaichii or Vulkan didn't start).
    size_t session_start = 0;
    for (size_t i = all.size(); i-- > 0;) {
        if (Contains(all[i], "Booting game: ")) {
            session_start = i;
            analysis.game_booted = true;
            analysis.program_id = After(all[i], "Booting game: ");
            break;
        }
    }

    size_t last_failure = all.size();
    for (size_t i = all.size(); i-- > 0;) {
        if (Contains(all[i], "Failed to load ROM")) {
            last_failure = i;
            break;
        }
    }
    const bool load_failed =
        last_failure != all.size() && (!analysis.game_booted || last_failure > session_start);

    std::vector<Finding> found;
    const auto add = [&found](Finding finding) {
        if (std::ranges::find(found, finding) == found.end()) {
            found.push_back(finding);
        }
    };
    bool missing_keys = false;
    bool no_firmware = false;
    std::vector<std::string_view> keep;
    // The start of the log says which build and computer it is.
    for (size_t i = 0; i < std::min<size_t>(all.size(), session_start); ++i) {
        const std::string_view line = all[i];
        if (i == last_failure && load_failed) {
            keep.push_back(line);
        }
        if (Contains(line, "Version: ") || Contains(line, "Host CPU") || Contains(line, "Host OS") ||
            Contains(line, "GPU: ") || Contains(line, "Vulkan driver")) {
            keep.push_back(line);
        }
        if (Contains(line, "Version: ") && analysis.build.empty()) {
            analysis.build = std::string{Message(line)};
        }
        if (Contains(line, "Decryption keys are missing")) {
            missing_keys = true;
            keep.push_back(line);
        }
        if (Contains(line, "Installed firmware: No firmware available")) {
            no_firmware = true;
            keep.push_back(line);
        }
        if (Contains(line, "Vulkan initialization failed") ||
            Contains(line, "Failed to load Vulkan")) {
            add(Finding::VulkanUnavailable);
            keep.push_back(line);
        }
    }
    bool errors = false;
    for (size_t i = session_start; i < all.size(); ++i) {
        const std::string_view line = all[i];
        errors = errors || IsProblem(line);
        bool interesting = IsProblem(line) || Contains(line, "Booting game: ");
        // Without a game the whole log is the session, and its start still names the build.
        if (!analysis.game_booted &&
            (Contains(line, "Version: ") || Contains(line, "Host CPU") ||
             Contains(line, "Host OS") || Contains(line, "GPU: ") ||
             Contains(line, "Vulkan driver"))) {
            interesting = true;
        }
        if (Contains(line, "Version: ") && analysis.build.empty()) {
            analysis.build = std::string{Message(line)};
        }
        if (Contains(line, "Decryption keys are missing") || Contains(line, "missing from prod.keys") ||
            Contains(line, "(0007-000D)") || Contains(line, "(0007-000E)")) {
            missing_keys = true;
            interesting = true;
        }
        if (Contains(line, "Installed firmware: No firmware available")) {
            no_firmware = true;
            interesting = true;
        }
        if (Contains(line, "Vulkan initialization failed") ||
            Contains(line, "Failed to load Vulkan")) {
            add(Finding::VulkanUnavailable);
        }
        if (Contains(line, "Device loss occurred")) {
            add(Finding::DeviceLost);
            interesting = true;
        }
        if (Contains(line, "Device fault")) {
            interesting = true;
        }
        if (ContainsNoCase(line, "out of device memory")) {
            add(Finding::OutOfVideoMemory);
            interesting = true;
        }
        if (Contains(line, "shader rejected") ||
            (Contains(line, "native stage") && Contains(line, "rejected:"))) {
            add(Finding::ShadersRejected);
        }
        if (Contains(line, "Stopping execution due to unmapped memory access")) {
            add(Finding::GuestCrashed);
            interesting = true;
        }
        if (interesting) {
            keep.push_back(line);
        }
    }
    // Keys missing are only the cause when a game then failed to load: a player with Switch 2 .nup
    // files never needs them, and every start without keys logs it.
    if (missing_keys && load_failed) {
        add(Finding::MissingKeys);
    } else if (load_failed) {
        add(Finding::LoadFailed);
    }
    if (no_firmware && load_failed) {
        add(Finding::NoFirmware);
    }
    // In the order of the enum: what to fix first comes first.
    std::ranges::sort(found);
    analysis.findings = std::move(found);
    analysis.has_problems = errors || !analysis.findings.empty();

    // One copy of each message, with how often it repeated, at most MaxLines.
    std::map<std::string_view, size_t> repeats;
    for (const std::string_view line : keep) {
        ++repeats[Message(line)];
    }
    std::map<std::string_view, bool> written;
    for (const std::string_view line : keep) {
        const std::string_view message = Message(line);
        if (written[message]) {
            continue;
        }
        written[message] = true;
        std::string text = Redact(line.substr(0, MaxLineLength));
        if (const size_t count = repeats[message]; count > 1) {
            text += " (x" + std::to_string(count) + ")";
        }
        analysis.lines.push_back(std::move(text));
        if (analysis.lines.size() == MaxLines) {
            break;
        }
    }
    return analysis;
}

} // namespace Common::LogDiagnosis
