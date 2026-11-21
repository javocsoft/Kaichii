// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string_view>

#include <QString>

/// The AI assistant's API keys, kept in the system's credential store: the Secret Service
/// (GNOME Keyring, KWallet) through libsecret on Linux, the Credential Manager on Windows and the
/// Keychain on macOS. Never in Kaichii's configuration files.
namespace AiCredentials {

enum class Result {
    Ok,
    NotFound,
    Unavailable, ///< This system has no credential store Kaichii can use
    Failed,      ///< The store refused, or the user dismissed its unlock prompt
};

/// Whether this system has a credential store Kaichii can use.
[[nodiscard]] bool Available();

/// The key saved for `provider` ("openai", "anthropic").
[[nodiscard]] std::optional<QString> Load(std::string_view provider, Result* result = nullptr);

/// Saves or replaces the key for `provider`.
Result Save(std::string_view provider, const QString& key);

/// Removes the key saved for `provider`. NotFound when there was none.
Result Forget(std::string_view provider);

} // namespace AiCredentials
