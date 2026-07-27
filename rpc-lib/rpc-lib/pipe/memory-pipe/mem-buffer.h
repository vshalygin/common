#pragma once
#include "../pipe-op-res.h"

#include <rpc-lib/types/future.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/timer/multiple-timer.h>
#include <common-lib/synchronization/value-locker.h>
#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/thread/thread-pool/strand.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <chrono>
#include <memory>
#include <queue>
#include <optional>

namespace vshalygin::rpc {
    class mem_buffer final
        : public std::enable_shared_from_this<mem_buffer>
    {
        explicit mem_buffer(std::shared_ptr<cl::thread_pool> thread_pool);

    public:
        using read_promise = promise<ftuple<pipe_op_res, cl::buffer>, pipe_op_res, cl::buffer>;
        using read_future = future<ftuple<pipe_op_res, cl::buffer>>;
        using write_future = future<pipe_op_res>;

        static std::shared_ptr<mem_buffer> create(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_buffer(mem_buffer &) = delete;
        mem_buffer &operator=(mem_buffer &) = delete;

        ~mem_buffer();

        future<pipe_op_res> write_async(cl::buffer &&data,
                                        const std::optional<std::chrono::milliseconds> &timeout);
        future<ftuple<pipe_op_res, cl::buffer>>
            read_async(const std::optional<std::chrono::milliseconds> &timeout);

        void invalidate(bool cancel_read);
        bool is_valid() const;

        size_t get_pending_messages_count() const;
        size_t get_pending_read_handlers_count() const;

    private:
        pipe_op_res write_impl(cl::buffer &&data, const auto &timeout_point);
        void read_impl(read_promise promise,
                       const std::optional<std::chrono::milliseconds> &timeout);

        void resolve_read_promise();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        cl::strand m_read_strand;
        cl::strand m_write_strand;

        mutable std::mutex m_mtx;
        bool m_is_valid = true;

        std::queue<cl::buffer> m_buffer;

        struct read_promise_data
        {
            uint64_t id;
            std::optional<uint64_t> timer_id;
            read_promise promise;
        };

        using read_promise_container = boost::multi_index::multi_index_container<
            read_promise_data,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
                boost::multi_index::ordered_unique<
                    boost::multi_index::member<read_promise_data, uint64_t, &read_promise_data::id>>>>;

        std::shared_ptr<cl::value_locker<read_promise_container>> m_read_promises;
        uint64_t m_next_read_promise_id = 0;

        cl::multiple_timer m_timer;
    };
}
