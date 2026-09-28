#ifndef FIELD_TRANSACTION_REQUEST_REPOSITORY_H
#define FIELD_TRANSACTION_REQUEST_REPOSITORY_H

#include "transaction_request_persistence.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace field {

enum class TransactionLoadStatus {
    Loaded,
    NotFound,
    Invalid,
    IoError,
};

struct TransactionLoadResult {
    TransactionLoadStatus status =
        TransactionLoadStatus::IoError;
    TransactionRequestStore store;
};

class TransactionRequestRepository {
public:
    explicit TransactionRequestRepository(
        std::filesystem::path path)
        : path_(std::move(path))
    {
    }

    TransactionLoadResult load() const
    {
        std::error_code ec;

        if (!std::filesystem::exists(path_, ec)) {
            return {
                ec ? TransactionLoadStatus::IoError
                   : TransactionLoadStatus::NotFound,
                {}
            };
        }

        std::ifstream input(
            path_,
            std::ios::binary);

        if (!input)
            return {
                TransactionLoadStatus::IoError,
                {}
            };

        const std::string raw{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };

        if (input.bad())
            return {
                TransactionLoadStatus::IoError,
                {}
            };

        const auto store =
            transactionStoreFromJsonString(raw);

        if (!store.has_value())
            return {
                TransactionLoadStatus::Invalid,
                {}
            };

        return {
            TransactionLoadStatus::Loaded,
            *store
        };
    }

    bool save(
        const TransactionRequestStore& store) const
    {
        std::error_code ec;

        const auto parent =
            path_.parent_path();

        if (!parent.empty()) {
            std::filesystem::create_directories(
                parent,
                ec);

            if (ec)
                return false;
        }

        std::filesystem::path temporary =
            path_;
        temporary += ".tmp";

        {
            std::ofstream output(
                temporary,
                std::ios::binary |
                    std::ios::trunc);

            if (!output)
                return false;

            output <<
                transactionStoreToJsonString(
                    store);

            output.flush();

            if (!output) {
                std::filesystem::remove(
                    temporary,
                    ec);
                return false;
            }
        }

        std::filesystem::rename(
            temporary,
            path_,
            ec);

        if (ec) {
            std::filesystem::remove(
                temporary,
                ec);
            return false;
        }

        return true;
    }

private:
    std::filesystem::path path_;
};

} // namespace field

#endif
