#include "tcp-pipe-server-env.h"
#include "tcp-pipe-endpoint.h"

#include <common-lib/timer/multiple-timer.h>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <optional>
#include <mutex>
#include <list>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

using namespace boost::asio::ip;

namespace vshalygin::rpc {
    using pipe_endpoint_future = tcp_pipe_server_env::pipe_endpoint_future;
    using pipe_endpoint_promise = promise<ftuple<pipe_wait_res, std::shared_ptr<ipipe_endpoint>>,
                                          pipe_wait_res, std::shared_ptr<ipipe_endpoint>>;

    namespace {
        class create_operation
            : public std::enable_shared_from_this<create_operation>
        {
            create_operation(cl::thread_pool *thread_pool,
                             tcp::acceptor &acceptor)
                : m_thread_pool(thread_pool)
                , m_acceptor(acceptor)
                , m_promise(make_promise(m_thread_pool,
                                         [](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> e) {
                                             return ftuple(r, std::move(e));
                                         }))
            {}

        public:
            inline static std::shared_ptr<create_operation> create(cl::thread_pool *thread_pool,
                                                                   tcp::acceptor &acceptor)
            {
                return std::shared_ptr<create_operation>(new create_operation(thread_pool, acceptor));
            }

            create_operation(const create_operation &) = delete;
            create_operation &operator=(const create_operation &) = delete;

            pipe_endpoint_future get_future()
            {
                return m_promise.get_future();
            }

            void start()
            {
                std::lock_guard guard(m_mtx);
                assert(!m_was_started);
                assert(!m_was_canceled);

                m_was_started = true;

                m_acceptor.async_accept([self = shared_from_this()]
                                        (const boost::system::error_code &ec, tcp::socket socket) mutable {
                    std::lock_guard guard(self->m_mtx);
                    if(self->m_was_canceled) {
                        auto r = self->m_canceled_by_timer ? pipe_wait_res::timeout : pipe_wait_res::canceled;
                        self->m_promise.resolve(r, {});
                    } else if (ec) {
                        self->m_promise.resolve(pipe_wait_res::failed, {});
                    } else {
                        boost::system::error_code option_ec;

                        socket.set_option(tcp::no_delay(true), option_ec);
                        if(option_ec) {
                            self->m_promise.resolve(pipe_wait_res::failed, {});
                            return;
                        }

                        socket.set_option(boost::asio::socket_base::keep_alive(true), option_ec);
                        if(option_ec) {
                            self->m_promise.resolve(pipe_wait_res::failed, {});
                            return;
                        }

                        auto endpoint = std::make_shared<tcp_pipe_endpoint>(self->m_thread_pool, std::move(socket));
                        self->m_promise.resolve(pipe_wait_res::success, std::move(endpoint));
                    }
                });
            }

            void cancel(bool by_timer)
            {
                std::lock_guard guard(m_mtx);
                if(!m_was_canceled) {
                    m_was_canceled = true;
                    m_canceled_by_timer = by_timer;
                    if(m_was_started) {
                        boost::system::error_code ignored;
                        m_acceptor.cancel(ignored);
                    } else {
                        m_promise.resolve(m_canceled_by_timer ? pipe_wait_res::timeout : pipe_wait_res::canceled, {});
                    }
                }
            }

        private:
            cl::thread_pool *m_thread_pool;

            std::mutex m_mtx;
            bool m_was_started = false;
            bool m_was_canceled = false;
            bool m_canceled_by_timer = false;
            tcp::acceptor &m_acceptor;

            pipe_endpoint_promise m_promise;
        };
    }

    class tcp_pipe_server_env::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        impl(cl::thread_pool *thread_pool,
             const std::string &ip4_address,
             uint32_t port);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        pipe_endpoint_future create_pipe(uint64_t client_id);
        pipe_endpoint_future create_pipe(uint64_t client_id, std::chrono::milliseconds timeout);

        void cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id);

    private:
        pipe_endpoint_future create_pipe(uint64_t client_id, const std::optional<std::chrono::milliseconds> &timeout);

        void on_timeout(uint64_t id);
        void on_completed(uint64_t id);

    private:
        cl::thread_pool *m_thread_pool;

        std::mutex m_mtx;
        uint64_t m_next_id = 0;
        struct create_operation_data
        {
            create_operation_data(uint64_t id_,
                                  uint64_t client_id_,
                                  std::optional<uint64_t> timer_id_,
                                  std::shared_ptr<create_operation> op_)
                : id(id_)
                , timer_id(timer_id_)
                , client_id(client_id_)
                , op(std::move(op_))
            {}

            uint64_t id;
            uint64_t client_id;
            std::optional<uint64_t> timer_id;
            std::shared_ptr<create_operation> op;
        };
        std::list<create_operation_data> m_create_operations;

        tcp::acceptor m_acceptor;

        cl::multiple_timer m_timer;
    };

    tcp_pipe_server_env::impl::impl(cl::thread_pool *thread_pool,
                                    const std::string &ip4_address,
                                    uint32_t port)
        : m_thread_pool(thread_pool)
        , m_acceptor(m_thread_pool->get_io_context())
        , m_timer(m_thread_pool->get_io_context())
    {
        if(port > 65535) {
            throw std::invalid_argument("exceed max port value");
        }

        tcp::endpoint endpoint(make_address_v4(ip4_address), static_cast<boost::asio::ip::port_type>(port));

        m_acceptor.open(endpoint.protocol());
        m_acceptor.bind(endpoint);
        m_acceptor.listen(boost::asio::socket_base::max_listen_connections);
    }

    pipe_endpoint_future tcp_pipe_server_env::impl::create_pipe(uint64_t client_id,
                                                                const std::optional<std::chrono::milliseconds> &timeout)
    {
        std::lock_guard guard(m_mtx);
        auto id = m_next_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([self = weak_from_this(), id]() {
                if(auto s = self.lock()) {
                    s->on_timeout(id);
                }
            }, *timeout);
        }

        m_create_operations.emplace_front(id, client_id, timer_id, create_operation::create(m_thread_pool, m_acceptor));
        auto future = m_create_operations.front().op->get_future();
        if(m_create_operations.size() == 1) {
            m_create_operations.back().op->start();
        }

        return future.finally([self = shared_from_this(), id]() { self->on_completed(id);});
    }

    void tcp_pipe_server_env::impl::on_timeout(uint64_t id)
    {
        std::lock_guard guard(m_mtx);
        auto it = std::find_if(m_create_operations.begin(), m_create_operations.end(),
                               [id](const auto &v) { return v.id == id; });
        if(it != m_create_operations.end()) {
            it->op->cancel(true);
            if(it->id != m_create_operations.back().id) {
                m_create_operations.erase(it);
            }
        }
    }

    void tcp_pipe_server_env::impl::on_completed(uint64_t id)
    {
        std::lock_guard guard(m_mtx);
        if(!m_create_operations.empty() && m_create_operations.back().id == id) {
            if(m_create_operations.back().timer_id) {
                m_timer.cancel(*m_create_operations.back().timer_id);
            }
            m_create_operations.pop_back();

            if(!m_create_operations.empty()) {
                m_create_operations.back().op->start();
            }
        }
    }

    pipe_endpoint_future tcp_pipe_server_env::impl::create_pipe(uint64_t client_id)
    {
        return create_pipe(client_id, std::nullopt);
    }

    pipe_endpoint_future tcp_pipe_server_env::impl::create_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return create_pipe(client_id, std::optional(timeout));
    }

    void tcp_pipe_server_env::impl::cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id)
    {
        std::lock_guard guard(m_mtx);
        if(m_create_operations.empty()) {
            return;
        }

        auto it = m_create_operations.begin();
        auto it_back = --m_create_operations.end();
        while(it != it_back) {
            if(!client_id || it->client_id == *client_id) {
                if(it->timer_id) {
                    m_timer.cancel(*it->timer_id);
                }
                it->op->cancel(false);
                it = m_create_operations.erase(it);
            } else {
                ++it;
            }
        }

        if(!m_create_operations.empty() && (!client_id || m_create_operations.back().client_id == *client_id)) {
            if(m_create_operations.back().timer_id) {
                m_timer.cancel(*m_create_operations.back().timer_id);
            }
            m_create_operations.back().op->cancel(false);
        }
    }

    tcp_pipe_server_env::tcp_pipe_server_env(cl::thread_pool *thread_pool,
                                             const std::string &ip4_address,
                                             uint32_t port)
        : m_impl(std::make_shared<impl>(thread_pool, ip4_address, port))
    {}

    tcp_pipe_server_env::~tcp_pipe_server_env()
    {
        cancel_all_pending_server_endpoints();
    }

    pipe_endpoint_future tcp_pipe_server_env::create_pipe(uint64_t client_id)
    {
        return m_impl->create_pipe(client_id);
    }

    pipe_endpoint_future tcp_pipe_server_env::create_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return m_impl->create_pipe(client_id, timeout);
    }

    void tcp_pipe_server_env::cancel_pending_server_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_server_endpoints(client_id);
    }

    void tcp_pipe_server_env::cancel_all_pending_server_endpoints()
    {
        m_impl->cancel_pending_server_endpoints(std::nullopt);
    }
}
