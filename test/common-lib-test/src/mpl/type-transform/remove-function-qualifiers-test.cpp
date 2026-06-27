#include "common-lib/mpl/type-transform.h"
#include <type_traits>

using namespace vshalygin::cl;

static_assert(std::is_same_v<remove_function_qualifiers_t<void()>, void()>);
static_assert(std::is_same_v<remove_function_qualifiers_t<void() noexcept>, void()>);
