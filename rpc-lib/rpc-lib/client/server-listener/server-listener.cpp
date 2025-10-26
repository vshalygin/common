#include "server-listener.h"

#include "rpc-lib/common/transfer-entry/transfer-entry.h"

namespace vsh::rpc {
    server_listener::server_listener(std::shared_ptr<guarded_cb_map> cb_map,
                                     std::shared_ptr<iclient_transport> transport,
                                     std::shared_ptr<ithread_pool> thread_pool)
        : cb_map_(std::move(cb_map))
        , transport_(std::move(transport))
        , thread_pool_(std::move(thread_pool))
    {}

    void server_listener::start()
    {
        listen_thread_ = std::jthread([this](std::stop_token st) {
                                          while(!st.stop_requested() && transport_->is_active()) {
                                              listen();
                                          }
                                      });
    }


    bool server_listener::listen()
    {
        common_lib::buffer buffer;
        transport_->recv(buffer); //TODO check fail

        auto task = [buffer = std::move(buffer), cb_map = cb_map_]() {
            auto entry_number = get_entry_number_res(buffer);
            callback_type callback;
            {
                auto [guard, m] = cb_map->get();
                auto it = m.find(entry_number);
                if(it != m.end()) {
                    callback = std::move(it->second);
                    m.erase(it);
                }
            }
            if(callback) {
                callback(buffer);
            }
        };

        thread_pool_->post(std::move(task));

        return true;
    }
}
