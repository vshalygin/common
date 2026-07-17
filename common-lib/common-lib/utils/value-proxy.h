#pragma once
#include <common-lib/mpl/type-transform.h>
#include <algorithm>
#include <memory>

namespace vshalygin::cl {
    struct value_proxy_owned_t {
    } value_proxy_owned;

    struct value_proxy_external_t {
    } value_proxy_external;

    template<typename T>
    class value_proxy
    {
        static_assert(std::is_reference_v<T>,
                      "value type is not allowed to parameterize value_proxy");

        using type_t = std::remove_reference_t<T>;
        using ref_t = T;

        inline static constexpr size_t alignment = std::max(alignof(std::add_pointer_t<ref_t>),
                                                            alignof(std::shared_ptr<type_t>));
        inline static constexpr size_t size = std::max(sizeof(std::add_pointer_t<ref_t>),
                                                       sizeof(std::shared_ptr<type_t>));
        static_assert(alignment <= size);

        template<typename U>
        void own(U &&v)
        {
            std::construct_at(reinterpret_cast<std::shared_ptr<type_t> *>(m_buffer),
                              std::make_shared<type_t>(std::forward<U>(v)));

            destroy = [](void *obj) {
                std::destroy_at(reinterpret_cast<std::shared_ptr<type_t> *>(obj));
            };
            interpret = [](void *obj) -> ref_t {
                return std::forward<ref_t>(**reinterpret_cast<std::shared_ptr<type_t> *>(obj));
            };
            copy = [](void *from, void *to) {
                std::shared_ptr<type_t> *target = reinterpret_cast<std::shared_ptr<type_t> *>(to);
                std::shared_ptr<type_t> *src = reinterpret_cast<std::shared_ptr<type_t> *>(from);
                std::construct_at(target, *src);
            };
            move = [](void *from, void *to) {
                std::shared_ptr<type_t> *target = reinterpret_cast<std::shared_ptr<type_t> *>(to);
                std::shared_ptr<type_t> *src = reinterpret_cast<std::shared_ptr<type_t> *>(from);
                std::construct_at(target, std::move(*src));
                std::destroy_at(src);
            };
        }

    public:
        value_proxy() = default;

        template<typename Tag, std::enable_if_t<std::is_same_v<Tag, value_proxy_owned_t>, int> = 0>
        explicit value_proxy(const type_t &v, Tag)
        {
            own(v);
        }

        template<typename Tag, std::enable_if_t<std::is_same_v<Tag, value_proxy_owned_t>, int> = 0>
        explicit value_proxy(type_t &&v, Tag)
        {
            own(std::move(v));
        }

        template<typename Tag, std::enable_if_t<std::is_same_v<Tag, value_proxy_external_t>, int> = 0>
        explicit value_proxy(ref_t ref, Tag) noexcept
        {
            *reinterpret_cast<type_t **>(m_buffer) = &ref;
            interpret = [](void *obj) -> ref_t {
                return std::forward<ref_t>(**reinterpret_cast<type_t **>(obj));
            };
            copy = [](void *from, void *to) {
                *reinterpret_cast<type_t **>(to) = *reinterpret_cast<type_t **>(from);
            };
            move = [](void *from, void *to) {
                *reinterpret_cast<type_t **>(to) = *reinterpret_cast<type_t **>(from);
                *reinterpret_cast<type_t **>(from) = nullptr;
            };
        }

        value_proxy(const value_proxy &other) noexcept
        {
            if(!other.is_valid()) {
                return;
            }

            other.copy(static_cast<void *>(other.m_buffer), static_cast<void *>(m_buffer));
            copy = other.copy;
            move = other.move;
            destroy = other.destroy;
            interpret = other.interpret;
        }

        value_proxy &operator=(const value_proxy &other) noexcept
        {
            if(this == &other) {
                return *this;
            }

            if(destroy) {
                destroy(static_cast<void *>(m_buffer));
            }

            if(other.copy) {
                other.copy(static_cast<void *>(other.m_buffer), static_cast<void *>(m_buffer));
                copy = other.copy;
                move = other.move;
                destroy = other.destroy;
                interpret = other.interpret;
            } else {
                copy = nullptr;
                move = nullptr;
                destroy = nullptr;
                interpret = nullptr;
            }

            return *this;
        }

        value_proxy(value_proxy &&other) noexcept
        {
            if(!other.is_valid()) {
                return;
            }

            other.move(static_cast<void *>(other.m_buffer), static_cast<void *>(m_buffer));
            copy = other.copy;
            move = other.move;
            destroy = other.destroy;
            interpret = other.interpret;

            other.copy = nullptr;
            other.move = nullptr;
            other.destroy = nullptr;
            other.interpret = nullptr;
        }

        value_proxy &operator=(value_proxy &&other) noexcept
        {
            if(this == &other) {
                return *this;
            }

            if(destroy) {
                destroy(static_cast<void *>(m_buffer));
            }

            if(other.move) {
                other.move(static_cast<void *>(other.m_buffer), static_cast<void *>(m_buffer));
                copy = other.copy;
                move = other.move;
                destroy = other.destroy;
                interpret = other.interpret;
                other.copy = nullptr;
                other.move = nullptr;
                other.destroy = nullptr;
                other.interpret = nullptr;
            } else {
                copy = nullptr;
                move = nullptr;
                destroy = nullptr;
                interpret = nullptr;
            }

            return *this;
        }

        ~value_proxy()
        {
            if(destroy) {
                destroy(static_cast<void *>(m_buffer));
            }
        }

        ref_t to_underlying() noexcept
        {
            return interpret(static_cast<void *>(m_buffer));
        }

        add_const_t<ref_t> to_underlying() const noexcept
        {
            return interpret(static_cast<void *>(m_buffer));
        }

        operator ref_t() noexcept
        {
            return to_underlying();
        }

        operator add_const_t<ref_t>() const noexcept
        {
            return to_underlying();
        }

        bool is_valid() const noexcept
        {
            return interpret != nullptr;
        }

    private:
        void (*destroy)(void *) = nullptr;
        ref_t (*interpret)(void *) = nullptr;
        void (*copy)(void *from, void *to) = nullptr;
        void (*move)(void *from, void *to) = nullptr;

    private:
        mutable alignas(alignment) std::byte m_buffer[size];
    };
}
