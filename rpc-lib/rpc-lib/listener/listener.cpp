#include "listener.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/types/interrupt-exception.h"
#include "rpc-lib/channel/channel.h"

#include <cassert>

namespace vshalygin::rpc {
    listener::listener(std::shared_ptr<iconnector> connector,
                       std::shared_ptr<iservice> service,
                       std::shared_ptr<cl::thread_pool> thread_pool,
                       connection_change_handler_t &&handler)
        : m_connector(std::move(connector))
        , m_service(std::move(service))
        , m_thread_pool(std::move(thread_pool))
        , m_connection_change_handler(std::make_shared<connection_change_handler_t>(std::move(handler)))
        , m_channel_map(std::make_shared<guarded_channel_map_t>())
    {}

    listener::~listener()
    {
        try {
            stop();
        } catch(...) {
            //TODO log
            std::terminate();
        }
    }

    void listener::start()
    {
        std::lock_guard guard(m_mtx);
        if(m_is_running) {
            return;
        }
        
        m_listen_thread = std::jthread([this](std::stop_token st) {
            while(!st.stop_requested()) {
                try {
                    create_new_active_connection();
                } catch(const interrupt_exception &) {
                    //normal situation
                } catch(...) {
                    //TODO log
                }
            }
        });

        m_is_running = true;
        auto [g, state_change_handler] = m_state_change_handler.get();
        if(state_change_handler) {
            state_change_handler(listener_state::started);
        }
    }

    void listener::stop()
    {
        std::lock_guard guard(m_mtx);
        if(m_is_running) {
            m_listen_thread.request_stop();
            if(m_current_connection) {
                m_current_connection->deactivate();
                m_current_connection.reset();
            }
            m_is_running = false;

            //TODO освободить мьютекс?
            auto [g, state_change_handler] = m_state_change_handler.get();
            if(state_change_handler) {
                state_change_handler(listener_state::stopped);
            }
        }
    }

    bool listener::is_stopped() const
    {
        std::lock_guard guard(m_mtx);
        return !m_is_running;
    }

    void listener::set_change_state_handler(change_state_handler_t &&handler)
    {
        auto [guard, state_change_handler] = m_state_change_handler.get();
        state_change_handler = std::move(handler);
    }

    std::shared_ptr<ichannel> listener::get_channel(uint64_t id) const
    {
        auto [guard, map] = m_channel_map->get();
        auto &el = map.at(id);
        if(!el.first) {
            throw std::runtime_error("connection is not activated");
        }
        return el.second;
    }

    listener::channels listener::get_all_channels() const
    {
        channels ans;
        auto [guard, map] = m_channel_map->get();
        ans.reserve(map.size());
        for(auto el : map) {
            bool is_activated = el.second.first;
            if(is_activated) {
                ans.push_back({ el.first, el.second.second });
            }
        }
        return ans;
    }

    void listener::drop_connection(uint64_t id)
    {
        auto [guard, map] = m_channel_map->get();
        auto it = map.find(id);
        if(it != map.end()) {
            bool is_activated = it->second.first;
            if(is_activated) {
                auto &channel = it->second.second;
                channel->get_connection()->deactivate(); //TODO Действительно нужен метод get_connection()
                channel->drop_connection();
            }
        }
    }
    void listener::drop_all_connections()
    {
        auto [guard, map] = m_channel_map->get();
        for(auto &el : map) {
            bool is_activated = el.second.first;
            if(is_activated) {
                auto &channel = el.second.second;
                channel->get_connection()->deactivate();
                channel->drop_connection();
            }
        }
    }

    void listener::create_new_active_connection()
    {
        std::unique_lock guard(m_mtx);
        if(!m_is_running) {
            return;
        }

        auto id = m_next_connection_id++;
        auto handler =
        [handler = m_connection_change_handler, map = m_channel_map, id] (connection_state state) mutable {
            if(state == connection_state::connected) {
                auto [guard, m] = map->get();
                assert(m.count(id));
                m[id].first = true;
            } else if(state == connection_state::disconnected) {
                auto [guard, m] = map->get();
                assert(m.count(id));
                m.erase(id);
            }

            (*handler)(id, state);
        };

        m_current_connection = std::make_shared<connection>(m_connector,
                                                            m_thread_pool,
                                                            m_service,
                                                            std::move(handler));

        guard.unlock();
        //TODO ужасно выглядит
        try {
            {
                auto [g, m] = m_channel_map->get();
                auto channel0 = std::make_shared<channel>(); //TODO сделать прототип?
                channel0->set_connection(m_current_connection);
                m.insert({ id, { false, channel0 } });
            }

            m_current_connection->activate();

        } catch(...) {
            {
                auto [g, m] = m_channel_map->get();
                m.erase(id);
            }

            guard.lock();
            m_current_connection.reset();
            throw;
        }

        guard.lock();
        m_current_connection.reset();
    }
}
