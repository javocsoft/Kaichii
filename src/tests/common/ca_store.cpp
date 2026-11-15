// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "common/fs/path_util.h"
#include "common/net/ca_store.h"
#include "common/scope_exit.h"

namespace {
struct Client {
    std::string path;
    bool loaded_memory{};
    void set_ca_cert_path(const std::string& value) { path = value; }
    void load_ca_cert_store(const char*, std::size_t) { loaded_memory = true; }
};
} // namespace

#ifndef _WIN32
TEST_CASE("Explicit OpenSSL CA settings take precedence over automatic bundles", "[common][net]") {
    const char* old = std::getenv("SSL_CERT_FILE");
    const std::optional<std::string> previous = old ? std::optional<std::string>{old} : std::nullopt;
    SCOPE_EXIT {
        if (previous) {
            setenv("SSL_CERT_FILE", previous->c_str(), 1);
        } else {
            unsetenv("SSL_CERT_FILE");
        }
    };
    setenv("SSL_CERT_FILE", "/synthetic/ca.pem", 1);
    Client client;
    Common::Net::ConfigureCaStore(client);
    CHECK(client.path.empty());
    CHECK_FALSE(client.loaded_memory);
}

TEST_CASE("An available host CA bundle is passed as a path for HTTPS redirects", "[common][net]") {
    if (std::getenv("SSL_CERT_FILE") || std::getenv("SSL_CERT_DIR")) {
        SKIP("OpenSSL CA override is active");
    }
    const std::string bundle = Common::Net::FindSystemCaBundle();
    if (bundle.empty()) {
        SKIP("No known host CA bundle");
    }
    REQUIRE(std::filesystem::is_regular_file(bundle));
    Client client;
    Common::Net::ConfigureCaStore(client);
    CHECK(client.path == bundle);
    CHECK_FALSE(client.loaded_memory);
}
#endif

#ifdef YUZU_BUNDLED_OPENSSL
TEST_CASE("The bundled CA store is a complete reusable file", "[common][net]") {
    const auto previous = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
    const auto root = std::filesystem::temp_directory_path() /
                      ("kaichii-ca-" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count()));
    Common::FS::SetEdenPath(Common::FS::EdenPath::CacheDir, root);
    SCOPE_EXIT {
        Common::FS::SetEdenPath(Common::FS::EdenPath::CacheDir, previous);
        std::error_code error;
        std::filesystem::remove_all(root, error);
    };
    const std::string path = Common::Net::BundledCaFile();
    REQUIRE_FALSE(path.empty());
    CHECK(Common::Net::BundledCaFile() == path);
    std::ifstream input{std::filesystem::path{std::u8string{path.begin(), path.end()}},
                        std::ios::binary};
    const std::string contents{std::istreambuf_iterator<char>{input}, {}};
    CHECK(contents == std::string_view{kCert, sizeof(kCert) - 1});
}
#endif
