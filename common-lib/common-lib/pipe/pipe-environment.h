#pragma once
#include "pipe-endpoint.h"
#include "common-lib/thread-pool/thread-pool.h"

#include <map>
#include <string>
#include <condition_variable>

namespace vsh::cl {
    class pipe_environment final
        : public std::enable_shared_from_this<pipe_environment>
    {
        class creator
        {};

    public:
        using pipe_endpoint_sp = std::shared_ptr<pipe_endpoint>;

        static std::shared_ptr<pipe_environment> create(std::shared_ptr<thread_pool> thread_pool);

        explicit pipe_environment(std::shared_ptr<thread_pool> thread_pool,
                                  creator);

        pipe_environment() = delete;
        pipe_environment &operator=(pipe_environment &) = delete;

        pipe_endpoint_sp create_pipe(const std::string &pipe_name);
        pipe_endpoint_sp open_pipe(const std::string &pipe_name);
        pipe_endpoint_sp open_pipe_timed(const std::string &pipe_name,
                                         const std::chrono::milliseconds &timeout);

        size_t get_existing_pipes_count() const;

    private:
        class pipe_info
        {
        public:
            pipe_info(std::shared_ptr<thread_pool> thread_pool);

            pipe_info(const pipe_info &) = delete;
            pipe_info &operator=(const pipe_info &) = delete;

            ~pipe_info();

            std::shared_ptr<pipe_buffer> get_client_to_server_buffer() const;
            std::shared_ptr<pipe_buffer> get_server_to_client_buffer() const;

            bool is_client_endpoint_exists() const;
            void set_client_endpoint(std::shared_ptr<pipe_endpoint> client_enpoint);

            void enable_buffers();
            void disable_buffers();

        private:
            std::shared_ptr<pipe_buffer> m_client_to_server_buffer;
            std::shared_ptr<pipe_buffer> m_server_to_client_buffer;

            std::weak_ptr<pipe_endpoint> m_client_endpoint;
        };

        std::shared_ptr<thread_pool> m_thread_pool;

        mutable std::mutex m_named_pipes_mtx;
        std::condition_variable m_named_pipes_cv;
        std::map<std::string, std::shared_ptr<pipe_info>> m_named_pipes_map;
    };
}
