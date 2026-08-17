#ifdef _WIN32
#include "win-pipe-read-operation.h"
#include <rpc-lib/consts.h>

namespace vshalygin::rpc::internal {
    namespace {
        cl::buffer merge_buffers(const std::vector<cl::buffer> &buffers, size_t size)
        {
            cl::buffer ans(size);
            size_t curr_pos = 0;
            for(size_t i = 0; i < buffers.size() && curr_pos < size; ++i) {
                for(size_t j = 0; j < buffers[i].size() && curr_pos < size; ++j) {
                    ans[curr_pos++] = buffers[i][j];
                }
            }

            return ans;
        }
    }

    std::shared_ptr<win_pipe_read_operation> win_pipe_read_operation::create(
        std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
        cl::thread_pool *thread_pool)
    {
        return std::shared_ptr<win_pipe_read_operation>(new win_pipe_read_operation(std::move(pipe), thread_pool));
    }

    win_pipe_read_operation::win_pipe_read_operation(std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
                                                     cl::thread_pool *thread_pool)
        : win_pipe_overlapped(win_pipe_operation_kind::read)
        , m_pipe(std::move(pipe))
        , m_promise(thread_pool,
                    [](win_pipe_operation_res r, cl::buffer b) { return ftuple(r, std::move(b)); })
    {
        m_buffers.push_back(cl::buffer(8192));
    }

    void win_pipe_read_operation::start(std::error_code &ec) noexcept
    {
        ec.clear();
        BOOL res;
        while(true) {
            auto buffer_size = static_cast<DWORD>(m_buffers.back().size());
            res = ::ReadFile(m_pipe->lock()->get(),
                             m_buffers.back().data(),
                             buffer_size,
                             nullptr,
                             reinterpret_cast<OVERLAPPED *>(this));
            if(res == FALSE && ::GetLastError() == ERROR_MORE_DATA) {
                add_read_bytes(buffer_size);
                add_buffer_chunk();
            } else {
                break;
            }
        }


        if(res == FALSE && ::GetLastError() != ERROR_IO_PENDING) {
            ec.assign(::GetLastError(), std::system_category());
        }
    }

    void win_pipe_read_operation::add_read_bytes(DWORD bytes) noexcept
    {
        m_read_bytes += bytes;
    }

    void win_pipe_read_operation::cancel() noexcept
    {
        ::CancelIo(m_pipe->lock()->get());
    }

    void win_pipe_read_operation::add_buffer_chunk()
    {
        auto prev_size = m_buffers.back().size();
        m_buffers.push_back(cl::buffer(prev_size * 2));
    }

    void win_pipe_read_operation::resolve()
    {
        auto res = m_res.load(std::memory_order_acquire);
        assert(res != win_pipe_operation_res::unknown);

        if(res == win_pipe_operation_res::success) {
            m_promise.resolve(res, merge_buffers(m_buffers, m_read_bytes));
        } else {
            m_promise.resolve(res, cl::buffer{});
        }
    }

    future<ftuple<win_pipe_operation_res, cl::buffer>> win_pipe_read_operation::get_future()
    {
        return m_promise.get_future();
    }

    win_pipe_operation_res win_pipe_read_operation::get_result() const noexcept
    {
        return m_res.load(std::memory_order_acquire);
    }

    void win_pipe_read_operation::set_success() noexcept
    {
        m_res.store(win_pipe_operation_res::success, std::memory_order_release);
    }

    void win_pipe_read_operation::set_canceled_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::canceled,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_read_operation::set_timeout_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::timeout,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_read_operation::set_failed_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::failed,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }
}
#endif
