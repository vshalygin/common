#include <common-lib/mpl/type-traits.h>

using namespace vshalygin::cl;

namespace {
    class full_lockable
    {
    public:
        void lock();
        void unlock();
    };

    class only_lockable
    {
    public:
        void lock();
    };

    class only_unlockable
    {
    public:
        void unlock();
    };

    class not_lockable
    {
    };
}

static_assert(is_lockable_v<full_lockable>);
static_assert(!is_lockable_v<only_lockable>);
static_assert(!is_lockable_v<only_unlockable>);
static_assert(!is_lockable_v<not_lockable>);
