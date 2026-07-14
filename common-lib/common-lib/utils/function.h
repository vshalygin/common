#pragma once
#include <common-lib/mpl/type-transform.h>
#include <stdexcept>

namespace vshalygin::cl {
    template<typename Signature>
    class function;

    template<typename R, typename...Args>
    class function<R(Args...)>
    {
        using this_type = function<R(Args...)>;

        inline static constexpr size_t sbo_size = 32;
        inline static constexpr size_t sbo_alignment = alignof(std::max_align_t);

        static_assert(sbo_alignment <= sbo_size);

    public:
        function() = default;

        template<
            typename Func,
            std::enable_if_t<!std::is_same_v<remove_type_qualifiers_t<Func>, this_type>, int> = 0>
        function(Func &&func)
        {
            using F = std::decay_t<Func>;

            if constexpr(sizeof(F) <= sbo_size &&
                         alignof(F) <= sbo_alignment &&
                         std::is_nothrow_move_constructible_v<F>)
            {
                std::construct_at(reinterpret_cast<F *>(m_buffer),
                                  std::forward<Func>(func));

                call = [](const void *obj, Args...args) {
                    return std::invoke(*const_cast<F *>(reinterpret_cast<const F *>(obj)),
                                       std::forward<Args>(args)...);
                };
                destroy = [](void *obj) {
                    std::destroy_at(reinterpret_cast<F *>(obj));
                };
                move = [](void *from, void *to) {
                    F *src = reinterpret_cast<F *>(from);
                    std::construct_at(reinterpret_cast<F *>(to), std::move(*src));
                    std::destroy_at(src);
                };

            } else {
                *reinterpret_cast<F **>(m_buffer) = new F(std::forward<Func>(func));
                
                call = [](const void *obj, Args...args) {
                    return std::invoke(**reinterpret_cast<F * const *>(obj),
                                       std::forward<Args>(args)...);
                };
                destroy = [](void *obj) {
                    delete *reinterpret_cast<F **>(obj);
                };
                move = [](void *from, void *to) {
                    *reinterpret_cast<F **>(to) = *reinterpret_cast<F **>(from);
                    *reinterpret_cast<F **>(from) = nullptr;
                };
            }
        }

        ~function() noexcept
        {
            if(destroy) {
                destroy(static_cast<void *>(m_buffer));
            }
        }

        function(const function &) = delete;
        function &operator=(const function &) = delete;

        function(function &&other) noexcept
        {
            if(other.call) {
                other.move(other.m_buffer, m_buffer);

                call = other.call;
                destroy = other.destroy;
                move = other.move;

                other.call = nullptr;
                other.destroy = nullptr;
                other.move = nullptr;
            }
        }

        function &operator=(function &&other) noexcept
        {
            if(this != &other) {
                if(destroy)
                    destroy(m_buffer);

                if(other.move) {
                    other.move(other.m_buffer, m_buffer);
                }
                
                call = other.call;
                destroy = other.destroy;
                move = other.move;

                other.call = nullptr;
                other.destroy = nullptr;
                other.move = nullptr;
            }

            return *this;
        }

        R operator()(Args...args) const
        {
            if(!call) {
                throw std::bad_function_call{};
            }

            return call(static_cast<const void *>(m_buffer), std::forward<Args>(args)...);
        }

        operator bool() const noexcept
        {
            return call != nullptr;
        }

    private:
        R (*call)(const void *, Args...) = nullptr;
        void (*move)(void *from, void *to) = nullptr;
        void (*destroy)(void *) = nullptr;

    private:
        alignas(sbo_alignment) std::byte m_buffer[sbo_size];
    };
}
