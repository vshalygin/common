set(common_lib_sources
    common-lib/synchronization/internal/event/event.cpp
    common-lib/thread/thread-pool/strand.cpp
    common-lib/timer/multiple-timer.cpp
    common-lib/timer/periodic-timer.cpp
    common-lib/utils/internal/buffer-view/buffer-view.cpp
    common-lib/utils/internal/buffer-view/cbuffer-view.cpp
    common-lib/utils/internal/buffer/buffer.cpp
)

set(common_lib_headers
    common-lib/memory/allocator/default-allocator.h
    common-lib/memory/enable-shared-from-this-manual-set.h
    common-lib/memory/memory.h
    common-lib/memory/unique-ptr2.h
    common-lib/mpl/function-traits.h
    common-lib/mpl/function-transform.h
    common-lib/mpl/internal/function-traits/function-traits.h
    common-lib/mpl/internal/function-traits/is-function-pointer.h
    common-lib/mpl/internal/function-traits/is-std-function.h
    common-lib/mpl/internal/function-transform/make-function-type.h
    common-lib/mpl/internal/function-transform/remove-function-qualifiers.h
    common-lib/mpl/internal/function-transform/remove-member-function-qualifiers.h
    common-lib/mpl/internal/tuple-traits/is-std-tuple.h
    common-lib/mpl/internal/tuple-traits/tuple-element.h
    common-lib/mpl/internal/tuple-traits/tuple-size.h
    common-lib/mpl/internal/tuple-transform/remove-last-tuple-type.h
    common-lib/mpl/internal/tuple-transform/last-tuple-type.h
    common-lib/mpl/internal/tuple-transform/merge-tuples.h
    common-lib/mpl/internal/tuple-transform/sort-tuple.h
    common-lib/mpl/internal/tuple-transform/swap-first-two-tuple-types.h
    common-lib/mpl/internal/type-traits/check-type-qualifiers.h
    common-lib/mpl/internal/type-traits/is-lvalue-static-castable.h
    common-lib/mpl/internal/type-transform/add-const.h
    common-lib/mpl/internal/type-transform/add-lvalue-ref-to-value.h
    common-lib/mpl/internal/type-transform/remove-c-ref.h
    common-lib/mpl/internal/type-transform/remove-const.h
    common-lib/mpl/internal/type-transform/remove-type-qualifiers.h
    common-lib/mpl/mpl.h
    common-lib/mpl/tuple-traits.h
    common-lib/mpl/tuple-transform.h
    common-lib/mpl/type-traits.h
    common-lib/mpl/type-transform.h
    common-lib/synchronization/event.h
    common-lib/synchronization/ordered-mutex-ref.h
    common-lib/synchronization/internal/ordered-lock/compare-ordered-lockable-tuples.h
    common-lib/synchronization/internal/ordered-lock/has-integral-order-static-member.h
    common-lib/synchronization/internal/ordered-lock/is-lockable.h
    common-lib/synchronization/internal/ordered-lock/is-ordered-lockable-types-valid.h
    common-lib/synchronization/internal/ordered-lock/is-there-repeating-order-number.h
    common-lib/synchronization/internal/ordered-lock/order-comparator.h
    common-lib/synchronization/internal/ordered-lock/ordered-lock-all.h
    common-lib/synchronization/internal/ordered-lock/ordered-lock-functions-impl.h
    common-lib/synchronization/internal/ordered-lock/ordered-lock-functions.h
    common-lib/synchronization/internal/ordered-lock/ordered-lock-impl.h
    common-lib/synchronization/internal/ordered-lock/ordered-lock.h
    common-lib/synchronization/ordered-lock.h
    common-lib/synchronization/ordered-mutex.h
    common-lib/synchronization/spinlock.h
    common-lib/synchronization/synchronization.h
    common-lib/synchronization/value-locker.h
    common-lib/thread/future/future.h
    common-lib/thread/future/internal/future-controller.h
    common-lib/thread/future/internal/future-data.h
    common-lib/thread/future/internal/future-impl.h
    common-lib/thread/future/internal/future-store-type-or-self.h
    common-lib/thread/future/internal/future-tuple.h
    common-lib/thread/future/internal/future.h
    common-lib/thread/future/internal/is-future-tuple.h
    common-lib/thread/future/internal/is-future.h
    common-lib/thread/future/internal/promise.h
    common-lib/thread/thread-pool/strand.h
    common-lib/thread/thread-pool/thread-pool-task.h
    common-lib/thread/thread-pool/thread-pool.h
    common-lib/thread/thread.h
    common-lib/timer/multiple-timer.h
    common-lib/timer/periodic-timer.h
    common-lib/utils/buffer-view.h
    common-lib/utils/buffer.h
    common-lib/utils/cbuffer-view.h
    common-lib/utils/do-on-destruct.h
    common-lib/utils/function.h
    common-lib/utils/internal/do-on-destruct/do-on-destruct.h
    common-lib/utils/internal/tuple-utils/extract-first-tuple-element.h
    common-lib/utils/internal/tuple-utils/extract-last-tuple-element.h
    common-lib/utils/internal/tuple-utils/for-each-tuple-element-reverse.h
    common-lib/utils/internal/tuple-utils/forward-tuple-element.h
    common-lib/utils/internal/tuple-utils/for-each-tuple-element.h
    common-lib/utils/internal/tuple-utils/make-tuple-index-sequense.h
    common-lib/utils/internal/tuple-utils/merge-tuples.h
    common-lib/utils/internal/tuple-utils/sort-tuple.h
    common-lib/utils/internal/tuple-utils/sort-tuple2.h
    common-lib/utils/internal/tuple-utils/swap-first-two-tuple-elements.h
    common-lib/utils/internal/tuple-utils/tie-tuple.h
    common-lib/utils/tuple-utils.h
    common-lib/utils/type-qualifiers-cast.h
    common-lib/utils/type-wrapper.h
    common-lib/utils/utils.h
    common-lib/utils/value-proxy.h
)

add_library(common-lib STATIC
    ${common_lib_sources}
    ${common_lib_headers}
)
add_library(vshalygin::common-lib ALIAS common-lib)

target_include_directories(common-lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(common-lib PUBLIC
    sdk::boost
)

configure_msvc_target(common-lib LIBRARY)

set_target_properties(common-lib PROPERTIES
    FOLDER "Libraries"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${common_lib_sources}
    ${common_lib_headers}
)
