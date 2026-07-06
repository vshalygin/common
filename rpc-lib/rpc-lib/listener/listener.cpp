#include "listener.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/channel/channel.h"

#include <cassert>

namespace vshalygin::rpc {
    //TODO
//    namespace {
//        using connection_change_callback_t = std::function<void(uint64_t, connection_state)>;
//
//        auto create_on_connection_change(auto &&callback)
//        {
//            return std::make_shared<connection_change_callback_t>(std::move(callback));
//        }
//    }
//
//    listener::listener(std::shared_ptr<iconnector> connector,
//                       std::shared_ptr<iservice> service,
//                       std::shared_ptr<cl::thread_pool> thread_pool,
//                       connection_change_callback_t &&on_conn_change_callback,
//                       const std::chrono::milliseconds &request_timeout)
//        : m_request_timeout(request_timeout)
//        , m_connector(std::move(connector))
//        , m_service(std::move(service))
//        , m_thread_pool(std::move(thread_pool))
//        , m_on_connection_change(create_on_connection_change(std::move(on_conn_change_callback)))
//        , m_channel_map(std::make_shared<guarded_channel_map_t>())
//    {}
//
//    listener::~listener()
//    {
//        stop();
//    }
//
//    void listener::start()
//    {
//        std::lock_guard guard(m_mtx);
//        if(m_is_running) {
//            return;
//        }
//        
//        m_listen_thread = std::jthread([this](std::stop_token st) {
//            while(!st.stop_requested()) {
//                try {
//                    create_new_active_connection(); //TODO если не смогли подключиться?
//                } catch(const interrupt_exception &) {
//                    //normal situation
//                } catch(...) {
//                    //TODO log
//                }
//            }
//        });
//
//        m_is_running = true;
//    }
//
//    void listener::stop()
//    {
//        std::lock_guard guard(m_mtx);
//        if(m_is_running) {
//            m_listen_thread.request_stop();
//            if(m_current_connection) {
//                m_current_connection->deactivate();
//                m_current_connection.reset();
//            }
//            m_is_running = false;
//        }
//    }
//
//    bool listener::is_stopped() const
//    {
//        std::lock_guard guard(m_mtx);
//        return !m_is_running;
//    }
//
//    std::shared_ptr<ichannel> listener::get_channel(uint64_t id) const
//    {
//        auto [guard, map] = m_channel_map->get();
//        auto &el = map.at(id);
//        if(!el.first) {
//            throw std::runtime_error("connection is not activated");
//        }
//        return el.second;
//    }
//
//    listener::channels listener::get_all_channels() const
//    {
//        channels ans;
//        auto [guard, map] = m_channel_map->get();
//        ans.reserve(map.size());
//        for(auto el : map) {
//            bool is_activated = el.second.first;
//            if(is_activated) {
//                ans.push_back({ el.first, el.second.second });
//            }
//        }
//        return ans;
//    }
//
//    void listener::drop_connection(uint64_t id)
//    {
//        auto [guard, map] = m_channel_map->get();
//        auto it = map.find(id);
//        if(it != map.end()) {
//            bool is_activated = it->second.first;
//            if(is_activated) {
//                auto &channel = it->second.second;
//                channel->get_connection()->deactivate(); //TODO Действительно нужен метод get_connection()
//                channel->drop_connection();
//            }
//        }
//    }
//    void listener::drop_all_connections()
//    {
//        auto [guard, map] = m_channel_map->get();
//        for(auto &el : map) {
//            bool is_activated = el.second.first;
//            if(is_activated) {
//                auto &channel = el.second.second;
//                channel->get_connection()->deactivate();
//                channel->drop_connection();
//            }
//        }
//    }
//
//    void listener::create_new_active_connection()
//    {
//        std::unique_lock guard(m_mtx);
//        if(!m_is_running) {
//            return;
//        }
//
//        auto id = m_next_connection_id++;
//        auto callback =
//        [cb = m_on_connection_change, map = m_channel_map, id] (connection_state state) mutable {
//            if(state == connection_state::connected) {
//                auto [guard, m] = map->get();
//                assert(m.count(id));
//                m[id].first = true;
//            } else if(state == connection_state::disconnected) {
//                auto [guard, m] = map->get();
//                assert(m.count(id));
//                m.erase(id);
//            }
//
//            (*cb)(id, state);
//        };
//        auto con_change_state_cb =
//            std::make_shared<connection::change_state_callback_t>(std::move(callback));
//        m_current_connection = connection::create(m_thread_pool,
//                                                  std::move(con_change_state_cb),
//                                                  m_connector,
//                                                  m_service,
//                                                  m_request_timeout);
//
//        {
//            auto [g, m] = m_channel_map->get();
//            auto channel0 = std::make_shared<channel>();
//            channel0->set_connection(m_current_connection);
//            m.insert({ id, { false, channel0 } });
//        }
//
//        guard.unlock();
//        try {
//            m_current_connection->activate();
//            guard.lock();
//            m_current_connection.reset();
//        } catch(...) {
//            {
//                auto [g, m] = m_channel_map->get();
//                m.erase(id);
//            }
//
//            guard.lock();
//            m_current_connection.reset();
//            throw;
//        }
//    }
}
