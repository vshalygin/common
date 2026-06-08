#include "strand.h"

namespace vshalygin::cl {
    strand::strand(boost::asio::io_context &io_context)
        : m_strand(io_context.get_executor())
    {}
}
