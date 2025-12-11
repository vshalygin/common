#include "pipe-environment.h"

namespace vsh::cl {
    using pipe_endpoint_sp = pipe_environment::pipe_endpoint_sp;

    pipe_environment::pipe_info::pipe_info(std::shared_ptr<thread_pool> thread_pool)
        : m_client_to_server_buffer(pipe_buffer::create(thread_pool))
        , m_server_to_client_buffer(pipe_buffer::create(thread_pool))
    {}

    pipe_environment::pipe_info::~pipe_info()
    {
        try {
            disable_buffers();
        } catch(...) {
            //TODO safe log
        }
    }

    std::shared_ptr<pipe_buffer> pipe_environment::pipe_info::get_client_to_server_buffer() const
    {
        return m_client_to_server_buffer;
    }

    std::shared_ptr<pipe_buffer> pipe_environment::pipe_info::get_server_to_client_buffer() const
    {
        return m_server_to_client_buffer;
    }

    bool pipe_environment::pipe_info::is_client_endpoint_exists() const
    {
        return !m_client_endpoint.expired();
    }

    void pipe_environment::pipe_info::set_client_endpoint(std::shared_ptr<pipe_endpoint> client_enpoint)
    {
        m_client_endpoint = client_enpoint;
    }

    void pipe_environment::pipe_info::enable_buffers()
    {
        m_client_to_server_buffer->enable();
        m_server_to_client_buffer->enable();
    }

    void pipe_environment::pipe_info::disable_buffers()
    {
        m_client_to_server_buffer->disable();
        m_server_to_client_buffer->disable();
    }

    std::shared_ptr<pipe_environment> 
        pipe_environment::create(std::shared_ptr<thread_pool> thread_pool)
    {
        return std::make_shared<pipe_environment>(std::move(thread_pool), creator());
    }

    pipe_environment::pipe_environment(std::shared_ptr<thread_pool> thread_pool,
                                       creator)
        : m_thread_pool(std::move(thread_pool))
    {}

    pipe_endpoint_sp pipe_environment::create_pipe(const std::string &pipe_name)
    {
        auto info = std::make_shared<pipe_info>(m_thread_pool);
        auto destruction_callback = [self = weak_from_this(), pipe_name]() {
            if(auto s = self.lock()) {
                std::lock_guard lock(s->m_mtx);
                s->m_named_pipes_map.erase(pipe_name);
            }
        };
        auto server_endpoint = std::make_shared<pipe_endpoint>(info->get_client_to_server_buffer(),
                                                               info->get_server_to_client_buffer(),
                                                               std::move(destruction_callback));

        {
            std::lock_guard lock(m_mtx);
            if(m_named_pipes_map.count(pipe_name)) {
                throw std::runtime_error("pipe '" + pipe_name + "' already exists");
            }
            m_named_pipes_map.insert({ pipe_name, std::move(info) });
        }
        m_cv.notify_one();

        return server_endpoint;
    }

    pipe_endpoint_sp pipe_environment::open_pipe(const std::string &pipe_name)
    {
        return open_pipe_timed(pipe_name, std::chrono::milliseconds(0));
    }

    pipe_endpoint_sp pipe_environment::open_pipe_timed(const std::string &pipe_name,
                                                       const std::chrono::milliseconds &timeout)
    {
        std::unique_lock lock(m_mtx);
        const auto is_success = m_cv.wait_for(lock, timeout, [&]() {
            auto it = m_named_pipes_map.find(pipe_name);
            if(it == m_named_pipes_map.end()) {
                return false;
            }
            return !it->second->is_client_endpoint_exists();
        });

        if(!is_success) {
            throw std::runtime_error("pipe server is not available");
        }

        auto info = m_named_pipes_map[pipe_name];
        auto destruction_callback = [info_wp = std::weak_ptr<pipe_info>(info),
                                     self = weak_from_this()]() {
            if(auto info = info_wp.lock()) {
                info->disable_buffers();
            }
            if(auto s = self.lock()) {
                s->m_cv.notify_one();
            }
        };
        auto client_endpoint = std::make_shared<pipe_endpoint>(info->get_server_to_client_buffer(),
                                                               info->get_client_to_server_buffer(),
                                                               std::move(destruction_callback));
        info->set_client_endpoint(client_endpoint);
        info->enable_buffers();

        return client_endpoint;
    }
}
