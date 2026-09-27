#ifndef FIELD_ACCESS_REQUEST_STORE_H
#define FIELD_ACCESS_REQUEST_STORE_H

#include "provider_permissions.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace field {

struct AccessRequest {
    std::string caller_key;
    std::string module_name;
    std::string module_instance;
    std::set<Capability> capabilities;
};

class AccessRequestStore {
public:
    bool requestCapability(
        const std::string& caller_key,
        const std::string& module_name,
        const std::string& module_instance,
        Capability capability)
    {
        if (caller_key.empty() || module_name.empty())
            return false;

        const auto it = requests_.find(caller_key);

        if (it == requests_.end()) {
            AccessRequest request;
            request.caller_key = caller_key;
            request.module_name = module_name;
            request.module_instance = module_instance;
            request.capabilities.insert(capability);

            requests_.emplace(
                caller_key,
                std::move(request));

            return true;
        }

        AccessRequest& request = it->second;

        // A canonical caller key must never silently acquire
        // different display identity metadata.
        if (request.module_name != module_name ||
            request.module_instance != module_instance)
            return false;

        request.capabilities.insert(capability);
        return true;
    }

    std::optional<AccessRequest> find(
        const std::string& caller_key) const
    {
        const auto it = requests_.find(caller_key);

        if (it == requests_.end())
            return std::nullopt;

        return it->second;
    }

    bool remove(
        const std::string& caller_key)
    {
        return requests_.erase(caller_key) != 0;
    }

    std::size_t size() const
    {
        return requests_.size();
    }

    std::vector<AccessRequest> all() const
    {
        std::vector<AccessRequest> result;
        result.reserve(requests_.size());

        for (const auto& [key, request] : requests_)
            result.push_back(request);

        return result;
    }

private:
    std::map<std::string, AccessRequest> requests_;
};

} // namespace field

#endif
