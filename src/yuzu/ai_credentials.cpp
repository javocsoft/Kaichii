// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/ai_credentials.h"

#include <string>

#include "common/logging.h"

#if defined(_WIN32)
#include <windows.h>
#include <wincred.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#else
#include <dlfcn.h>
#endif

namespace AiCredentials {
namespace {

constexpr std::string_view Service = "Kaichii AI assistant";

[[maybe_unused]] QString Label(std::string_view provider) {
    return QStringLiteral("%1 (%2)").arg(QString::fromUtf8(Service.data(), Service.size()),
                                         QString::fromUtf8(provider.data(), provider.size()));
}

#if defined(_WIN32)

std::wstring Target(std::string_view provider) {
    return Label(provider).toStdWString();
}

#elif defined(__APPLE__)

struct CfRelease {
    CFTypeRef ref;
    ~CfRelease() {
        if (ref) {
            CFRelease(ref);
        }
    }
};

CFMutableDictionaryRef Query(CFStringRef account) {
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const std::string service{Service};
    CFStringRef service_ref =
        CFStringCreateWithCString(kCFAllocatorDefault, service.c_str(), kCFStringEncodingUTF8);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service_ref);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFRelease(service_ref);
    return query;
}

CFStringRef Account(std::string_view provider) {
    const std::string name{provider};
    return CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8);
}

#else

// libsecret, loaded at run time: Kaichii builds and runs without it, and an AppImage uses the
// host's. Its schema layout and these functions are libsecret's stable ABI.
struct SecretSchemaAttribute {
    const char* name;
    int type; // SECRET_SCHEMA_ATTRIBUTE_STRING = 0
};
struct SecretSchema {
    const char* name;
    int flags; // SECRET_SCHEMA_NONE = 0
    SecretSchemaAttribute attributes[32];
    int reserved;
    void* reserved1;
    void* reserved2;
    void* reserved3;
    void* reserved4;
    void* reserved5;
    void* reserved6;
    void* reserved7;
};
struct GError {
    unsigned int domain;
    int code;
    char* message;
};

using StoreFn = int (*)(const SecretSchema*, const char*, const char*, const char*, void*,
                        GError**, ...);
using LookupFn = char* (*)(const SecretSchema*, void*, GError**, ...);
using ClearFn = int (*)(const SecretSchema*, void*, GError**, ...);
using FreeFn = void (*)(char*);
using ErrorFreeFn = void (*)(GError*);

struct Library {
    StoreFn store = nullptr;
    LookupFn lookup = nullptr;
    ClearFn clear = nullptr;
    FreeFn free_password = nullptr;
    ErrorFreeFn free_error = nullptr;

    bool Loaded() const {
        return store && lookup && clear && free_password && free_error;
    }
};

const Library& Secret() {
    static const Library library = [] {
        Library loaded;
        void* handle = dlopen("libsecret-1.so.0", RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr) {
            LOG_INFO(Frontend, "AI assistant: libsecret is not installed, API keys can't be saved");
            return loaded;
        }
        loaded.store = reinterpret_cast<StoreFn>(dlsym(handle, "secret_password_store_sync"));
        loaded.lookup = reinterpret_cast<LookupFn>(dlsym(handle, "secret_password_lookup_sync"));
        loaded.clear = reinterpret_cast<ClearFn>(dlsym(handle, "secret_password_clear_sync"));
        loaded.free_password = reinterpret_cast<FreeFn>(dlsym(handle, "secret_password_free"));
        loaded.free_error = reinterpret_cast<ErrorFreeFn>(dlsym(handle, "g_error_free"));
        return loaded;
    }();
    return library;
}

const SecretSchema& Schema() {
    static const SecretSchema schema{
        "io.github.javocsoft.kaichii.ai",
        0,
        {{"application", 0}, {"provider", 0}, {nullptr, 0}},
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    };
    return schema;
}

// Logs and frees a libsecret error. True if there was one.
bool Failed(const Library& library, GError* error, const char* what) {
    if (error == nullptr) {
        return false;
    }
    LOG_WARNING(Frontend, "AI assistant: the Secret Service could not {} the API key: {}", what,
                error->message ? error->message : "unknown error");
    library.free_error(error);
    return true;
}

#endif

} // Anonymous namespace

bool Available() {
#if defined(_WIN32) || defined(__APPLE__)
    return true;
#else
    return Secret().Loaded();
#endif
}

std::optional<QString> Load(std::string_view provider, Result* result) {
    const auto report = [result](Result value) {
        if (result) {
            *result = value;
        }
    };
#if defined(_WIN32)
    PCREDENTIALW credential = nullptr;
    const std::wstring target = Target(provider);
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential)) {
        report(GetLastError() == ERROR_NOT_FOUND ? Result::NotFound : Result::Failed);
        return std::nullopt;
    }
    QString key = QString::fromUtf8(reinterpret_cast<const char*>(credential->CredentialBlob),
                                    static_cast<qsizetype>(credential->CredentialBlobSize));
    SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
    CredFree(credential);
    report(Result::Ok);
    return key;
#elif defined(__APPLE__)
    CfRelease account{Account(provider)};
    CFMutableDictionaryRef query = Query(static_cast<CFStringRef>(account.ref));
    CfRelease query_ref{query};
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef data = nullptr;
    const OSStatus status = SecItemCopyMatching(query, &data);
    if (status != errSecSuccess || data == nullptr) {
        report(status == errSecItemNotFound ? Result::NotFound : Result::Failed);
        return std::nullopt;
    }
    CfRelease data_ref{data};
    const auto bytes = static_cast<CFDataRef>(data);
    report(Result::Ok);
    return QString::fromUtf8(reinterpret_cast<const char*>(CFDataGetBytePtr(bytes)),
                             static_cast<qsizetype>(CFDataGetLength(bytes)));
#else
    const Library& library = Secret();
    if (!library.Loaded()) {
        report(Result::Unavailable);
        return std::nullopt;
    }
    const std::string name{provider};
    GError* error = nullptr;
    char* password = library.lookup(&Schema(), nullptr, &error, "application", "kaichii",
                                    "provider", name.c_str(), nullptr);
    if (Failed(library, error, "read")) {
        report(Result::Failed);
        return std::nullopt;
    }
    if (password == nullptr) {
        report(Result::NotFound);
        return std::nullopt;
    }
    QString key = QString::fromUtf8(password);
    library.free_password(password); // Overwrites the copy before freeing it
    report(Result::Ok);
    return key;
#endif
}

Result Save(std::string_view provider, const QString& key) {
    QByteArray secret = key.toUtf8();
#if defined(_WIN32)
    const std::wstring target = Target(provider);
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(target.c_str());
    credential.CredentialBlobSize = static_cast<DWORD>(secret.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(secret.data());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<LPWSTR>(L"Kaichii");
    const bool saved = CredWriteW(&credential, 0) != FALSE;
    secret.fill('\0');
    return saved ? Result::Ok : Result::Failed;
#elif defined(__APPLE__)
    Forget(provider);
    CfRelease account{Account(provider)};
    CFMutableDictionaryRef item = Query(static_cast<CFStringRef>(account.ref));
    CfRelease item_ref{item};
    CfRelease data{CFDataCreate(kCFAllocatorDefault,
                                reinterpret_cast<const UInt8*>(secret.constData()),
                                secret.size())};
    CFDictionarySetValue(item, kSecValueData, data.ref);
    const OSStatus status = SecItemAdd(item, nullptr);
    secret.fill('\0');
    return status == errSecSuccess ? Result::Ok : Result::Failed;
#else
    const Library& library = Secret();
    if (!library.Loaded()) {
        return Result::Unavailable;
    }
    const std::string name{provider};
    const QByteArray label = Label(provider).toUtf8();
    GError* error = nullptr;
    const int saved = library.store(&Schema(), "default", label.constData(), secret.constData(),
                                    nullptr, &error, "application", "kaichii", "provider",
                                    name.c_str(), nullptr);
    secret.fill('\0');
    if (Failed(library, error, "save") || !saved) {
        return Result::Failed;
    }
    return Result::Ok;
#endif
}

Result Forget(std::string_view provider) {
#if defined(_WIN32)
    const std::wstring target = Target(provider);
    if (CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) {
        return Result::Ok;
    }
    return GetLastError() == ERROR_NOT_FOUND ? Result::NotFound : Result::Failed;
#elif defined(__APPLE__)
    CfRelease account{Account(provider)};
    CFMutableDictionaryRef query = Query(static_cast<CFStringRef>(account.ref));
    CfRelease query_ref{query};
    const OSStatus status = SecItemDelete(query);
    if (status == errSecSuccess) {
        return Result::Ok;
    }
    return status == errSecItemNotFound ? Result::NotFound : Result::Failed;
#else
    const Library& library = Secret();
    if (!library.Loaded()) {
        return Result::Unavailable;
    }
    const std::string name{provider};
    GError* error = nullptr;
    const int cleared = library.clear(&Schema(), nullptr, &error, "application", "kaichii",
                                      "provider", name.c_str(), nullptr);
    if (Failed(library, error, "remove")) {
        return Result::Failed;
    }
    return cleared ? Result::Ok : Result::NotFound;
#endif
}

} // namespace AiCredentials
