#include "strand.h"

namespace vsh::cl {
    strand::strand(boost::asio::io_context &io_context)
        : m_strand(io_context.get_executor())
        , m_executing_thread_id(std::make_shared<thread_id_t>())
    {}

    void strand::set_current_thread_id(std::shared_ptr<thread_id_t> thread_id)
    {
        auto [guard, ti] = thread_id->get();
        ti = std::this_thread::get_id();
    }

    void strand::clear_thread_id(std::shared_ptr<thread_id_t> thread_id)
    {
        auto [guard, ti] = thread_id->get();
        ti.reset();
    }

    bool strand::is_in_executing_context() const
    {
        auto [guard, ti] = m_executing_thread_id->get();
        return ti.has_value() && ti.value() == std::this_thread::get_id();
    }
}
