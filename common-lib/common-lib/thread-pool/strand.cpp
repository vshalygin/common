#include "strand.h"

namespace vsh::cl {
    strand::strand(boost::asio::io_context &io_context)
        : m_strand(io_context.get_executor())
    {}

    void strand::post(std::function<void()> &&task)
    {
        boost::asio::post(m_strand, std::move(task));
    }
}
