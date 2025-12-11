#include "strand.h"

namespace vsh::cl {
    strand::strand(boost::asio::io_context &io_context)
        : m_strand(io_context.get_executor())
        , m_executing_thread_id(std::make_shared<thread_id_t>())
    {}

    bool strand::is_in_executing_context() const
    {
        auto executing_thread_id = m_executing_thread_id->load(std::memory_order_acquire);
        return executing_thread_id == std::this_thread::get_id();
    }

    strand::thread_id_guard::thread_id_guard(std::shared_ptr<thread_id_t> thread_id)
        : m_thread_id(std::move(thread_id))
    {
        m_thread_id->store(std::this_thread::get_id(), std::memory_order_release);
    }

    strand::thread_id_guard::~thread_id_guard()
    {
        m_thread_id->store(std::thread::id(), std::memory_order_release);
    }
}
