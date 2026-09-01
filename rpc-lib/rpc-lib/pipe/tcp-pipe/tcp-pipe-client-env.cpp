#include "tcp-pipe-client-env.h"
#include "tcp-pipe-endpoint.h"

#include <common-lib/timer/multiple-timer.h>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>

#include <unordered_map>
#include <mutex>
#include <optional>

namespace vshalygin::rpc {
    using socket = boost::asio::ip::tcp::socket;

    using pipe_endpoint_future = tcp_pipe_client_env::pipe_endpoint_future;
    using pipe_endpoint_promise =
        cl::promise<cl::thread_pool,
                    cl::ftuple<pipe_wait_res, std::shared_ptr<ipipe_endpoint>>>;

    namespace {
        bool is_valid_ipv4(const std::string &value)
        {
            boost::system::error_code ec;
            boost::asio::ip::make_address_v4(value, ec);
            return !ec;
        }

        class open_operation
            : public std::enable_shared_from_this<open_operation>
        {
            open_operation(uint64_t client_id, cl::thread_pool *thread_pool)
                : m_client_id(client_id)
                , m_thread_pool(thread_pool)
                , m_socket(m_thread_pool->get_io_context())
            {}

        public:
            inline static std::shared_ptr<open_operation> create(uint64_t client_id,
                                                                 cl::thread_pool *thread_pool)
            {
                return std::shared_ptr<open_operation>(new open_operation(client_id, thread_pool));
            }

            open_operation(const open_operation &) = delete;
            open_operation &operator=(const open_operation &) = delete;

            pipe_endpoint_future start(const std::string &ip4_address, uint32_t port)
            {
                using namespace boost::asio::ip;

                std::lock_guard guard(m_socket_mtx);
                if(m_was_canceled) {
                    auto r = m_canceled_by_timer ? pipe_wait_res::timeout : pipe_wait_res::canceled;
                    return pipe_endpoint_future(m_thread_pool,
                                                cl::ftuple(r, std::shared_ptr<ipipe_endpoint>{}));
                }

                boost::system::error_code ec;
                m_socket.open(tcp::v4(), ec);
                if(ec) {
                    return pipe_endpoint_future(m_thread_pool,
                                                cl::ftuple(pipe_wait_res::failed, std::shared_ptr<ipipe_endpoint>{}));
                }
                m_socket.set_option(tcp::no_delay(true), ec);
                if(ec) {
                    return pipe_endpoint_future(m_thread_pool,
                                                cl::ftuple(pipe_wait_res::failed, std::shared_ptr<ipipe_endpoint>{}));
                }

                m_socket.set_option(boost::asio::socket_base::keep_alive(true), ec);
                if(ec) {
                    return pipe_endpoint_future(m_thread_pool,
                                                cl::ftuple(pipe_wait_res::failed, std::shared_ptr<ipipe_endpoint>{}));
                }

                pipe_endpoint_promise promise(m_thread_pool);
                auto future = promise.get_future();

                m_socket.async_connect(
                    tcp::endpoint(make_address(ip4_address), static_cast<uint16_t>(port)),
                    [promise = std::move(promise), self = shared_from_this()](const boost::system::error_code &ec) mutable {
                        pipe_wait_res result;
                        std::shared_ptr<ipipe_endpoint> endpoint;
                        {
                            std::lock_guard guard(self->m_socket_mtx);
                            if(self->m_was_canceled) {
                                result = self->m_canceled_by_timer
                                    ? pipe_wait_res::timeout
                                    : pipe_wait_res::canceled;
                            } else if(ec) {
                                result = pipe_wait_res::failed;
                            } else {
                                self->m_is_socket_valid = false;
                                result = pipe_wait_res::success;
                                endpoint =
                                std::make_shared<tcp_pipe_endpoint>(
                                    self->m_thread_pool,
                                    std::move(self->m_socket));
                            }
                        }

                        promise.set_value(cl::ftuple(result, std::move(endpoint)));
                    });

                return future;
            }

            void cancel(bool by_timer)
            {
                std::lock_guard guard(m_socket_mtx);
                if(!m_was_canceled) {
                    m_was_canceled = true;
                    m_canceled_by_timer = by_timer;

                    if(m_is_socket_valid) {
                        boost::system::error_code ec;
                        m_socket.close(ec);
                    }
                }
            }

            uint64_t get_client_id() const noexcept
            {
                return m_client_id;
            }

        private:
            const uint64_t m_client_id;
            cl::thread_pool *m_thread_pool;

            std::mutex m_socket_mtx;
            bool m_is_socket_valid = true;
            bool m_was_canceled = false;
            bool m_canceled_by_timer = false;
            socket m_socket;
        };
    }

    class tcp_pipe_client_env::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        impl(cl::thread_pool *thread_pool,
             const std::string &ip4_address,
             uint32_t port);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        pipe_endpoint_future open_pipe(uint64_t client_id);
        pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout);

        void cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id);

    private:
        pipe_endpoint_future open_pipe(uint64_t client_id, const std::optional<std::chrono::milliseconds> &timeout);

    private:
        cl::thread_pool *m_thread_pool;
        const std::string m_ip4_address;
        const uint32_t m_port;

        std::mutex m_mtx;
        uint64_t m_next_id = 0;
        std::unordered_map<uint64_t, std::shared_ptr<open_operation>> m_open_operations;

        cl::multiple_timer m_timer;
    };

    tcp_pipe_client_env::impl::impl(cl::thread_pool *thread_pool,
                                    const std::string &ip4_address,
                                    uint32_t port)
        : m_thread_pool(thread_pool)
        , m_ip4_address(ip4_address)
        , m_port(port)
        , m_timer(m_thread_pool->get_io_context())
    {
        if(!is_valid_ipv4(m_ip4_address)) {
            throw std::invalid_argument("invalid ip4 address");
        }
        if(m_port > 65535) {
            throw std::invalid_argument("exceed max port value");
        }
    }

    pipe_endpoint_future tcp_pipe_client_env::impl::open_pipe(uint64_t client_id,
                                                              const std::optional<std::chrono::milliseconds> &timeout)
    {
        std::lock_guard guard(m_mtx);
        auto id = m_next_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([self = weak_from_this(), id]() {
                if(auto s = self.lock()) {
                    std::lock_guard guard(s->m_mtx);
                    auto it = s->m_open_operations.find(id);
                    if(it != s->m_open_operations.end()) {
                        it->second->cancel(true);
                    }
                }
            }, *timeout);
        }
        auto &op = m_open_operations.emplace(id, open_operation::create(client_id, m_thread_pool)).first->second;

        return op->start(m_ip4_address, m_port)
                   .finally([self = shared_from_this(), id, timer_id]() {
                                if(timer_id) self->m_timer.cancel(*timer_id);

                                std::lock_guard guard(self->m_mtx);
                                self->m_open_operations.erase(id);
                            });
    }

    pipe_endpoint_future tcp_pipe_client_env::impl::open_pipe(uint64_t client_id)
    {
        return open_pipe(client_id, std::nullopt);
    }

    pipe_endpoint_future tcp_pipe_client_env::impl::open_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return open_pipe(client_id, std::optional(timeout));
    }

    void tcp_pipe_client_env::impl::cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id)
    {
        std::lock_guard guard(m_mtx);
        for(auto &op : m_open_operations) {
            if(!client_id || op.second->get_client_id() == client_id) {
                op.second->cancel(false);
            }
        }
    }

    tcp_pipe_client_env::tcp_pipe_client_env(cl::thread_pool *thread_pool,
                                             const std::string &ip4_address,
                                             uint32_t port)
        : m_impl(std::make_shared<impl>(thread_pool, ip4_address, port))
    {}

    tcp_pipe_client_env::~tcp_pipe_client_env()
    {
        cancel_all_pending_client_endpoints();
    }

    pipe_endpoint_future tcp_pipe_client_env::open_pipe(uint64_t client_id)
    {
        return m_impl->open_pipe(client_id);
    }

    pipe_endpoint_future tcp_pipe_client_env::open_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return m_impl->open_pipe(client_id, timeout);
    }

    void tcp_pipe_client_env::cancel_pending_client_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_client_endpoints(client_id);
    }

    void tcp_pipe_client_env::cancel_all_pending_client_endpoints()
    {
        m_impl->cancel_pending_client_endpoints(std::nullopt);
    }
}
