#pragma once
#include <string>

namespace vsh::rpc {
    enum class transfer_type : char
    {
        req = 0,
        res = 1
    };

    unsigned get_entry_id_req(const char *buf, size_t size);
    std::string_view get_client_id_req(const char *buf, size_t size);
    unsigned get_method_idx_req(const char *buf, size_t size);
    std::string_view get_serialized_message_req(const char *buf, size_t size);
}
