#include "server-endpoint.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/channel/channel.h"

#include <vector>

namespace vshalygin::rpc {
    server_endpoint::impl::impl(std::unique_ptr<ilistener> listener,
                                std::shared_ptr<iservice> service,
                                std::shared_ptr<cl::thread_pool> thread_pool,
                                creator)
        : m_listener(std::move(listener))
        , m_service(std::move(service))
        , m_thread_pool(std::move(thread_pool))
    {
        assert(m_listener);
        assert(m_service);
        assert(m_thread_pool);
    }

    void server_endpoint::impl::start_listen()
    {
        m_listener->start();
    }

    bool server_endpoint::impl::is_listening() const
    {
        return !m_listener->is_stopped();
    }

    void server_endpoint::impl::stop_listen()
    {
        return m_listener->stop();
    }

    void server_endpoint::impl::set_connection_change_state_handler
        (connection_change_state_handler_t &&handler)
    {
        auto [guard, connection_change_state_handler] = m_connection_change_state_handler.get();
        connection_change_state_handler = std::move(handler);
    }

    void server_endpoint::impl::set_listener_change_state_handler
        (listener_change_state_handler_t &&handler)
    {
        m_listener->set_change_state_handler(std::move(handler));
    }

    void server_endpoint::impl::drop_connection(uint64_t id)
    {
        auto [guard, map] = m_channel_map.get();
        auto it = map.find(id);
        if(it != map.end()) {
            it->second->get_connection()->stop_transport();
        }
    }

    void server_endpoint::impl::drop_all_connections()
    {
        auto [guard, map] = m_channel_map.get();
        for(auto &el : map) {
            el.second->get_connection()->stop_transport();
        }
    }

    void server_endpoint::impl::set_new_connection_handler()
    {
        auto handler = [self = weak_from_this()](std::unique_ptr<itransport> transport) {
            assert(transport);
            if(auto s = self.lock()) {
                s->handle_new_connection(std::move(transport), self);
            }
        };
        
        m_listener->set_connect_handler(std::move(handler));
    }

    size_t server_endpoint::impl::get_inactive_connections_count() const
    {
        auto [guard, map] = m_inactive_connection_map.get();
        return map.size();
    }

    size_t server_endpoint::impl::get_channels_count() const
    {
        auto [guard, map] = m_channel_map.get();
        return map.size();
    }

    void server_endpoint::impl::handle_new_connection(std::unique_ptr<itransport> transport,
                                                      std::weak_ptr<impl> self)
    {
        const auto id = m_next_connection_id.fetch_add(1);
        auto new_connection = std::make_shared<connection>(m_thread_pool);

        auto change_state_handler = create_connection_change_state_handler(self, id);
        auto request_handler = create_request_handler();

        new_connection->set_change_state_handler(std::move(change_state_handler));
        new_connection->set_request_handler(std::move(request_handler));

        {
            auto [guard, inactive_map] = m_inactive_connection_map.get();
            inactive_map[id] = new_connection;
        }

        try {
            new_connection->set_and_start_transport(std::move(transport));
        } catch (...) {
            //TODO log
            auto [guard, inactive_map] = m_inactive_connection_map.get();
            assert(inactive_map.count(id));
            inactive_map.erase(id);
        }
    }

    std::function<void(connection_state)> server_endpoint::impl::create_connection_change_state_handler
        (std::weak_ptr<impl> self, uint64_t connection_id) const
    {
        return [self, connection_id](connection_state state) mutable {
            if(auto s = self.lock()) {
                if(state == connection_state::connected) {
                    auto [guard, map] = s->m_channel_map.get();
                    auto [guard2, inactive_map] = s->m_inactive_connection_map.get();
                    assert(map.count(connection_id) == 0);
                    assert(inactive_map.count(connection_id));
                    auto c = std::make_shared<channel>();
                    c->set_connection(std::move(inactive_map[connection_id]));
                    map[connection_id] = std::move(c);
                    inactive_map.erase(connection_id);
                } else if(state == connection_state::disconnected) {
                    auto [guard, map] = s->m_channel_map.get();
                    assert(map.count(connection_id));
                    map.erase(connection_id);
                }
                s->call_connection_state_change_handler(connection_id, state);
            }
        };
    }

    std::function<void(cl::buffer &&, iconnection::response_handler_t &&)> 
        server_endpoint::impl::create_request_handler() const
    {
        return [service = m_service](cl::buffer &&buff, iconnection::response_handler_t &&res_handler) {
            service->process_request(std::move(buff), std::move(res_handler));
        };
    }

    std::shared_ptr<ichannel> server_endpoint::impl::find_channel_or_throw(uint64_t connection_id) const
    {
        auto [guard, map] = m_channel_map.get();
        auto it = map.find(connection_id);
        if(it == map.end()) {
            throw std::runtime_error("no channel with id " + std::to_string(connection_id));
        }
        assert(it->second);
        return it->second;
    }

    std::vector<std::pair<uint64_t, std::shared_ptr<ichannel>>>
        server_endpoint::impl::get_all_channels_with_id() const
    {
        std::vector<std::pair<uint64_t, std::shared_ptr<ichannel>>> ans;
        auto [guard, map] = m_channel_map.get();
        ans.reserve(map.size());
        for(auto el : map) {
            ans.push_back(el);
        }
        return ans;
    }

    void server_endpoint::impl::call_connection_state_change_handler(uint64_t connection_id,
                                                                     connection_state state)
    {
        auto [guard, handler] = m_connection_change_state_handler.get();
        if(handler) {
            try {
                handler(connection_id, state);
            } catch(...) {
                //TODO log
            }
        }
    }

    server_endpoint::server_endpoint(std::unique_ptr<ilistener> listener,
                                     std::shared_ptr<iservice> service,
                                     std::shared_ptr<cl::thread_pool> thread_pool)
        : m_impl(impl::create(std::move(listener), std::move(service), std::move(thread_pool)))
    {
        m_impl->set_new_connection_handler();
    }

    void server_endpoint::start_listen()
    {
        m_impl->start_listen();
    }

    bool server_endpoint::is_listening() const
    {
        return m_impl->is_listening();
    }

    void server_endpoint::stop_listen()
    {
        m_impl->stop_listen();
    }

    void server_endpoint::set_connection_change_state_handler
        (connection_change_state_handler_t &&handler)
    {
        m_impl->set_connection_change_state_handler(std::move(handler));
    }

    void server_endpoint::set_listener_change_state_handler
        (listener_change_state_handler_t &&handler)
    {
        m_impl->set_listener_change_state_handler(std::move(handler));
    }

    void server_endpoint::drop_connection(uint64_t id)
    {
        m_impl->drop_connection(id);
    }

    void server_endpoint::drop_all_connections()
    {
        m_impl->drop_all_connections();
    }

    size_t server_endpoint::get_inactive_connections_count() const
    {
        return m_impl->get_inactive_connections_count();
    }

    size_t server_endpoint::get_channels_count() const
    {
        return m_impl->get_channels_count();
    }
}
