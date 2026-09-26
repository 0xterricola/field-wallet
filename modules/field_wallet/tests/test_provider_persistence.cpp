#include <logos_test.h>

#include "provider_persistence.h"

LOGOS_TEST(permission_store_json_round_trip) {
    field::PermissionStore store;

    field::PermissionGrant first;
    first.caller_key = "module|5:app_a|0:";
    first.account_id = "account-a";
    first.account_kind = field::AccountKind::Public;
    first.capabilities.insert(
        field::Capability::AccountIdentityRead);
    first.capabilities.insert(
        field::Capability::TransactionPropose);

    field::PermissionGrant second;
    second.caller_key = "module|5:app_b|0:";
    second.account_id = "account-b";
    second.account_kind = field::AccountKind::Private;
    second.capabilities.insert(
        field::Capability::AccountBalanceRead);

    LOGOS_ASSERT_TRUE(store.put(first));
    LOGOS_ASSERT_TRUE(store.put(second));

    const auto restored = field::permissionStoreFromJsonString(
        field::permissionStoreToJsonString(store));

    LOGOS_ASSERT_TRUE(restored.has_value());
    LOGOS_ASSERT_EQ(
        restored->size(),
        static_cast<std::size_t>(2));

    LOGOS_ASSERT_TRUE(restored->allows(
        "module|5:app_a|0:",
        "account-a",
        field::Capability::TransactionPropose));

    LOGOS_ASSERT_TRUE(restored->allows(
        "module|5:app_b|0:",
        "account-b",
        field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_json_preserves_revocation) {
    field::PermissionStore store;

    field::PermissionGrant grant;
    grant.caller_key = "module|5:app_a|0:";
    grant.account_id = "private-account";
    grant.account_kind = field::AccountKind::Private;
    grant.capabilities.insert(
        field::Capability::AccountBalanceRead);
    grant.revoked = true;

    LOGOS_ASSERT_TRUE(store.put(grant));

    const auto restored = field::permissionStoreFromJsonString(
        field::permissionStoreToJsonString(store));

    LOGOS_ASSERT_TRUE(restored.has_value());

    const auto loaded = restored->find(
        "module|5:app_a|0:",
        "private-account");

    LOGOS_ASSERT_TRUE(loaded.has_value());
    LOGOS_ASSERT_TRUE(
        loaded->account_kind == field::AccountKind::Private);
    LOGOS_ASSERT_TRUE(loaded->revoked);

    LOGOS_ASSERT_FALSE(restored->allows(
        "module|5:app_a|0:",
        "private-account",
        field::Capability::AccountBalanceRead));
}

LOGOS_TEST(permission_store_rejects_malformed_json) {
    const auto restored =
        field::permissionStoreFromJsonString("{not-json");

    LOGOS_ASSERT_FALSE(restored.has_value());
}

LOGOS_TEST(permission_store_rejects_unknown_schema_version) {
    field::PermissionStore store;

    auto json = field::permissionStoreToJson(store);
    json["schemaVersion"] = 2;

    LOGOS_ASSERT_FALSE(
        field::permissionStoreFromJson(json).has_value());
}

LOGOS_TEST(permission_store_rejects_unknown_capability) {
    nlohmann::json json = {
        {"schemaVersion", 1},
        {"grants", nlohmann::json::array({
            {
                {"callerKey", "module|5:app_a|0:"},
                {"accountId", "account-a"},
                {"accountKind", "public"},
                {"capabilities",
                    nlohmann::json::array({"wallet.superuser"})},
                {"revoked", false},
            }
        })}
    };

    LOGOS_ASSERT_FALSE(
        field::permissionStoreFromJson(json).has_value());
}

LOGOS_TEST(permission_store_rejects_duplicate_grants) {
    nlohmann::json grant = {
        {"callerKey", "module|5:app_a|0:"},
        {"accountId", "account-a"},
        {"accountKind", "public"},
        {"capabilities",
            nlohmann::json::array({"account.identity.read"})},
        {"revoked", false},
    };

    nlohmann::json json = {
        {"schemaVersion", 1},
        {"grants", nlohmann::json::array({grant, grant})}
    };

    LOGOS_ASSERT_FALSE(
        field::permissionStoreFromJson(json).has_value());
}
