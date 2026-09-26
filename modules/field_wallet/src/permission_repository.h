#ifndef FIELD_PERMISSION_REPOSITORY_H
#define FIELD_PERMISSION_REPOSITORY_H

#include "provider_persistence.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace field {

enum class PermissionLoadStatus {
    Loaded,
    NotFound,
    Invalid,
    IoError,
};

struct PermissionLoadResult {
    PermissionLoadStatus status = PermissionLoadStatus::IoError;
    PermissionStore store;
};

class PermissionRepository {
public:
    explicit PermissionRepository(std::filesystem::path path)
        : path_(std::move(path))
    {
    }

    const std::filesystem::path& path() const
    {
        return path_;
    }

    PermissionLoadResult load() const
    {
        std::error_code ec;

        if (!std::filesystem::exists(path_, ec)) {
            return {
                ec ? PermissionLoadStatus::IoError
                   : PermissionLoadStatus::NotFound,
                {}
            };
        }

        std::ifstream input(path_, std::ios::binary);

        if (!input)
            return {PermissionLoadStatus::IoError, {}};

        const std::string raw{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };

        if (input.bad())
            return {PermissionLoadStatus::IoError, {}};

        const auto store = permissionStoreFromJsonString(raw);

        if (!store.has_value())
            return {PermissionLoadStatus::Invalid, {}};

        return {PermissionLoadStatus::Loaded, *store};
    }

    bool save(const PermissionStore& store) const
    {
        std::error_code ec;

        const auto parent = path_.parent_path();

        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);

            if (ec)
                return false;
        }

        std::filesystem::path temporary = path_;
        temporary += ".tmp";

        {
            std::ofstream output(
                temporary,
                std::ios::binary | std::ios::trunc);

            if (!output)
                return false;

            output << permissionStoreToJsonString(store);
            output.flush();

            if (!output) {
                std::filesystem::remove(temporary, ec);
                return false;
            }
        }

        std::filesystem::rename(temporary, path_, ec);

        if (ec) {
            std::filesystem::remove(temporary, ec);
            return false;
        }

        return true;
    }

private:
    std::filesystem::path path_;
};

} // namespace field

#endif
