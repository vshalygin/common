#include "pipe-environment.h"
#include "common-lib/timer/multiple-timer/multiple-timer.h"
#include "common-lib/syncronization/event/event.h"
#include <list>
#include <string>

using pipe_endpoint_sp = std::shared_ptr<vsh::cl::pipe_endpoint>;

namespace vsh::cl {
    namespace {
        struct prepared_endpoint_info
        {
            uint64_t timer_id;
            pipe_environment::pipe_callback_t callback;
        };
    }

    class pipe_environment::impl final
        : public std::enable_shared_from_this<pipe_environment>
    {
        using prepared_info_map = std::map<std::string, std::list<prepared_endpoint_info>>;

    public:
        explicit impl(std::shared_ptr<thread_pool> thread_pool)
            : m_thread_pool(std::move(thread_pool))
            , m_timer(m_thread_pool->get_io_context())
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        ~impl()
        {
            try {
                m_timer.cancel_all();
                call_prepared_callbacks_with_cancel_code(m_prepared_client_info_map);
                call_prepared_callbacks_with_cancel_code(m_prepared_server_info_map);
            } catch(...) {
                //TODO log
            }
        }

        void create_pipe_async(const std::string &pipe_name,
                               pipe_callback_t &&callback,
                               const std::chrono::milliseconds &timeout)
        {
            process_endpoint_request(m_prepared_server_info_map,
                                     m_prepared_client_info_map,
                                     pipe_name,
                                     std::move(callback),
                                     timeout);
        }

        void open_pipe_async(const std::string &pipe_name,
                             pipe_callback_t &&callback,
                             const std::chrono::milliseconds &timeout)
        {
            process_endpoint_request(m_prepared_client_info_map,
                                     m_prepared_server_info_map,
                                     pipe_name,
                                     std::move(callback),
                                     timeout);
        }

    private:
        void process_endpoint_request(prepared_info_map &target_map,
                                      prepared_info_map &complement_map,
                                      const std::string &pipe_name,
                                      pipe_callback_t &&target_callback,
                                      const std::chrono::milliseconds &timeout)
        {
            assert(target_callback);

            std::lock_guard guard(m_mtx);
            auto complement_endpoints_info_it = complement_map.find(pipe_name);
            if(complement_endpoints_info_it != complement_map.end() &&
               !complement_endpoints_info_it->second.empty())
            {
                auto complement_endpoint_info =
                    exptract_prepared_endpoint_info(complement_endpoints_info_it,
                                                    complement_map);

                cancel_timer(complement_endpoint_info.timer_id);

                auto to_target_buffer = pipe_buffer::create(m_thread_pool);
                auto from_target_buffer = pipe_buffer::create(m_thread_pool);
                to_target_buffer->enable();
                from_target_buffer->enable();

                auto target_endpoint = std::make_shared<pipe_endpoint>(to_target_buffer,
                                                                       from_target_buffer);
                auto complement_endpoint = std::make_shared<pipe_endpoint>(from_target_buffer,
                                                                           to_target_buffer);

                auto complement_callback = std::move(complement_endpoint_info.callback);

                m_thread_pool->post(create_pipe_callback_post_task(std::move(target_endpoint),
                                                                   std::move(target_callback)));
                m_thread_pool->post(create_pipe_callback_post_task(std::move(complement_endpoint),
                                                                   std::move(complement_callback)));
            } else {
                prepared_endpoint_info prepared_info;
                prepared_info.timer_id = start_timer(target_map,
                                                     pipe_name,
                                                     timeout);
                prepared_info.callback = std::move(target_callback);

                auto insert_result = target_map.insert({ pipe_name, {} });
                auto &prepared_infos_it = insert_result.first;
                prepared_infos_it->second.emplace_back(std::move(prepared_info));
            }
        }

        prepared_endpoint_info exptract_prepared_endpoint_info(prepared_info_map::iterator iter,
                                                               prepared_info_map &map)
        {
            auto complement_endpoint_info = std::move(iter->second.front());
            iter->second.pop_front();

            if(iter->second.empty()) {
                map.erase(iter);
            }

            return complement_endpoint_info;
        }

        std::function<void()> create_pipe_callback_post_task(std::shared_ptr<pipe_endpoint> endpoint,
                                                             pipe_callback_t &&pipe_callback)
        {
            return [endpoint = std::move(endpoint),
                    callback = std::move(pipe_callback)]() mutable {
                try {
                    callback(pipe_result::ok, std::move(endpoint));
                } catch(...) {
                    //TODO safe log
                }
            };
        }

        uint64_t start_timer(prepared_info_map &map,
                             const std::string &pipe_name,
                             const std::chrono::milliseconds &timeout)
        {
            auto timer_id = std::make_shared<uint64_t>(-1);
            auto sync_event = std::make_shared<event>();
            auto cancel_task = [&map, pipe_name, timer_id, sync_event, this]() {
                sync_event->wait();
                remove_prepared_info_by_timer(map, pipe_name, *timer_id);
            };
            *timer_id = m_timer.start(std::move(cancel_task), timeout);
            sync_event->set();

            return *timer_id;
        }

        void cancel_timer(uint64_t id)
        {
            m_timer.cancel(id);
        }

        void remove_prepared_info_by_timer(prepared_info_map &map,
                                           const std::string &pipe_name,
                                           uint64_t id)
        {
            std::lock_guard guard(m_mtx);
            pipe_callback_t callback;
            auto named_prepared_infos_it = map.find(pipe_name);
            if(named_prepared_infos_it != map.end()) {
                auto &named_prepared_infos = named_prepared_infos_it->second;
                auto prepared_info_it = std::find_if(named_prepared_infos.begin(),
                                                     named_prepared_infos.end(),
                                                     [id](const auto &info){
                                                         return info.timer_id == id;
                                                     });
                if(prepared_info_it != named_prepared_infos.end()) {
                    callback = std::move(prepared_info_it->callback);
                    named_prepared_infos.erase(prepared_info_it);
                }
                if(named_prepared_infos.empty()) {
                    map.erase(named_prepared_infos_it);
                }
            }

            if(callback) try {
                callback(pipe_result::timeout, {});
            } catch (...) {
                //TODO log
            }
        }

        void call_prepared_callbacks_with_cancel_code(prepared_info_map &map)
        {
            for(auto &prepared_infos : map) {
                for(auto &prepared_info : prepared_infos.second) {
                    auto task = [callback = std::move(prepared_info.callback)]() {
                        try {
                            callback(pipe_result::canceled, {});
                        } catch(...) {
                            //TODO safe log
                        }

                    };
                    m_thread_pool->post(std::move(task));
                }
            }
        }

    private:
        std::shared_ptr<thread_pool> m_thread_pool;
        multiple_timer m_timer;

        std::mutex m_mtx;
        prepared_info_map m_prepared_client_info_map;
        prepared_info_map m_prepared_server_info_map;
    };

    pipe_environment::pipe_environment(std::shared_ptr<thread_pool> thread_pool)
        : m_impl(std::make_unique<impl>(std::move(thread_pool)))
    {}

    pipe_environment::pipe_environment(pipe_environment &&) = default;
    pipe_environment &pipe_environment::operator=(pipe_environment &&) = default;

    void pipe_environment::create_pipe_async(const std::string &pipe_name,
                                             pipe_callback_t &&callback,
                                             const std::chrono::milliseconds &timeout)
    {
        m_impl->create_pipe_async(pipe_name, std::move(callback), timeout);
    }

    void pipe_environment::open_pipe_async(const std::string &pipe_name,
                                           pipe_callback_t &&callback,
                                           const std::chrono::milliseconds &timeout)
    {
        m_impl->open_pipe_async(pipe_name, std::move(callback), timeout);
    }
}
