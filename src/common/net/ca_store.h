// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#ifdef YUZU_BUNDLED_OPENSSL
#include <openssl/cert.h>
#endif

namespace Common::Net {

[[nodiscard]] inline std::string FindSystemCaBundle() {
#if !defined(_WIN32)
    if (std::getenv("SSL_CERT_FILE") || std::getenv("SSL_CERT_DIR")) {
        return {};
    }
    constexpr std::array candidates{
        "/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem", "/etc/ssl/cert.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/etc/ca-certificates/extracted/tls-ca-bundle.pem",
    };
    for (const char* path : candidates) {
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error)) {
            return path;
        }
    }
#endif
    return {};
}

#ifdef YUZU_BUNDLED_OPENSSL
[[nodiscard]] std::string BundledCaFile();
#endif

template <typename Client>
void ConfigureCaStore(Client& client) {
    // OpenSSL handles explicit user CA overrides itself.
    if (std::getenv("SSL_CERT_FILE") || std::getenv("SSL_CERT_DIR")) {
        return;
    }
    if (const std::string path = FindSystemCaBundle(); !path.empty()) {
        client.set_ca_cert_path(path);
        return;
    }
#ifdef YUZU_BUNDLED_OPENSSL
    if (const std::string path = BundledCaFile(); !path.empty()) {
        client.set_ca_cert_path(path);
    } else {
        client.load_ca_cert_store(kCert, sizeof(kCert));
    }
#endif
}

} // namespace Common::Net
