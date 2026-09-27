#ifndef FIELD_LEZ_TRANSFER_RESULT_H
#define FIELD_LEZ_TRANSFER_RESULT_H

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace field {

struct LezTransferResult {
    bool success = false;
    std::string tx_hash;
    std::string error;
};

inline std::optional<LezTransferResult>
parseLezTransferResult(const std::string& payload)
{
    const auto doc =
        nlohmann::json::parse(
            payload,
            nullptr,
            false);

    if (doc.is_discarded() ||
        !doc.is_object() ||
        !doc.contains("success") ||
        !doc["success"].is_boolean() ||
        !doc.contains("tx_hash") ||
        !doc["tx_hash"].is_string() ||
        !doc.contains("error") ||
        !doc["error"].is_string()) {
        return std::nullopt;
    }

    LezTransferResult result;
    result.success =
        doc["success"].get<bool>();
    result.tx_hash =
        doc["tx_hash"].get<std::string>();
    result.error =
        doc["error"].get<std::string>();

    return result;
}

inline bool isSuccessfulLezTransferResult(
    const LezTransferResult& result)
{
    return result.success &&
           !result.tx_hash.empty() &&
           result.error.empty();
}

} // namespace field

#endif
