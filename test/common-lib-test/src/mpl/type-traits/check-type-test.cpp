#include <common-lib/mpl/type-traits.h>

using namespace vshalygin::cl;

static_assert(!is_const_v<int>);
static_assert(is_const_v<const int>);
static_assert(is_const_v<const int&>);
static_assert(is_const_v<const int&&>);
static_assert(is_const_v<volatile const int>);
static_assert(is_const_v<volatile const int&>);
static_assert(is_const_v<volatile const int&&>);

static_assert(is_value_v<int>);
static_assert(!is_value_v<int &>);

static_assert(is_lvalue_ref_v<int &>);
static_assert(!is_lvalue_ref_v<int &&>);

static_assert(is_rvalue_ref_v<int &&>);
static_assert(!is_rvalue_ref_v<int &>);

static_assert(is_const_value_v<const int>);
static_assert(!is_const_value_v<int &>);

static_assert(is_const_lvalue_ref_v<const int&>);
static_assert(!is_const_lvalue_ref_v<int &>);

static_assert(is_const_rvalue_ref_v<const int &&>);
static_assert(!is_const_rvalue_ref_v<int &>);

static_assert(is_volatile_value_v<volatile int>);
static_assert(!is_volatile_value_v<int &>);

static_assert(is_volatile_lvalue_ref_v<volatile int &>);
static_assert(!is_volatile_lvalue_ref_v<int &>);

static_assert(is_volatile_rvalue_ref_v<volatile int &&>);
static_assert(!is_volatile_rvalue_ref_v<int &>);

static_assert(is_const_volatile_value_v<volatile const int>);
static_assert(!is_const_volatile_value_v<int &>);

static_assert(is_const_volatile_lvalue_ref_v<volatile const int &>);
static_assert(!is_const_volatile_lvalue_ref_v<int &>);

static_assert(is_const_volatile_rvalue_ref_v<volatile const int &&>);
static_assert(!is_const_volatile_rvalue_ref_v<int &>);
