#pragma once
#include <utility>
#include <memory>
#include <type_traits>

namespace vsh::common_lib {
    class default_allocator
    {
    public:
        template<typename T>
        T *allocate(size_t count)
        {
            return std::allocator<T>{}.allocate(count);
        }

        template<typename T>
        void deallocate(T *ptr, size_t count) noexcept
        {
            static_assert(std::is_nothrow_constructible_v<std::allocator<T>>,
                          "std::allocator<T> contructor is not noexcept");
            //static_assert(noexcept(std::declval(std::allocator<T>).deallocate(ptr, count)),
            //              "std::allocator<T>::deallocate is not noexcept");

            std::allocator<T>{}.deallocate(ptr, count);
        }
    };

    template<typename T>
    class iallocated_object
    {
    public:
        virtual ~iallocated_object() = default;

        virtual T *get() noexcept = 0;
        virtual const T *get() const noexcept = 0;
    };

    template<typename T, typename Allocator>
    class allocated_object final
        : public iallocated_object<T>
    {
    public:
        template<typename...Args>
        explicit allocated_object(Args&&...args)
            : ptr_(nullptr)
        {
            ptr_ = Allocator{}.allocate<T>(1);

            try {
                new (ptr_) T(std::forward<Args>(args)...);
            } catch(...) {
                Allocator{}.deallocate(ptr_, 1);
                throw;
            }
        }

        ~allocated_object() override
        {
            //TODO check noexcept
            if(ptr_) {
                ptr_->~T();
                Allocator{}.deallocate(ptr_, 1);
            }
        }

        allocated_object(allocated_object &) = delete;
        allocated_object &operator=(allocated_object &) = delete;

        void *operator new(size_t /*size*/)
        {
            return Allocator{}.allocate<allocated_object>(1);
        }

        void operator delete(void *self)
        {
            Allocator{}.deallocate(static_cast<allocated_object *>(self), 1);
        }

        allocated_object(allocated_object &&other) noexcept
            : ptr_(nullptr)
        {
            static_assert(noexcept(std::swap(ptr_, other.ptr_)),
                          "std::swap is not noexcept");

            std::swap(other.ptr_, ptr_);
        }

        allocated_object &operator=(allocated_object &&other) noexcept
        {
            static_assert(noexcept(std::swap(ptr_, other.ptr_)),
                          "std::swap is not noexcept");

            std::swap(other.ptr_, ptr_);

            return *this;
        }

        T *get() noexcept override
        {
            return ptr_;
        }

        const T *get() const noexcept override
        {
            return ptr_;
        }

    private:
        T *ptr_;
    };

    template<typename T, typename Allocator = default_allocator>
    class unique_ptr;

    template<typename T, typename Allocator, typename...Args>
    unique_ptr<T, Allocator> make_unique(Args&&...args);

    template<typename T, typename Allocator>
    class unique_ptr
    {
        template<typename T, typename Allocator, typename...Args>
        friend unique_ptr<T, Allocator> make_unique(Args&&...args);

    public:
        unique_ptr() noexcept
        {}

        ~unique_ptr() noexcept
        {
            reset();
        }

        unique_ptr(unique_ptr &) = delete;
        unique_ptr &operator=(unique_ptr &) = delete;

        unique_ptr(unique_ptr &&other) noexcept
            : unique_ptr()
        {
            static_assert(noexcept(std::swap(allocated_object_, other.allocated_object_)),
                          "std::swap is not noexcept");

            std::swap(allocated_object_, other.allocated_object_);
        }

        unique_ptr &operator=(unique_ptr &&other) noexcept
        {
            static_assert(noexcept(std::swap(allocated_object_, other.allocated_object_)),
                          "std::swap is not noexcept");

            std::swap(allocated_object_, other.allocated_object_);

            return *this;
        }

        void reset() noexcept
        {
            delete allocated_object_;
            allocated_object_ = nullptr;
        }

        T *get() noexcept
        {
            return const_cast<T *>(static_cast<const unique_ptr *>(this)->get());
        }

        const T *get() const noexcept
        {
            return allocated_object_ ? allocated_object_->get() : nullptr;
        }

        T *operator->() noexcept
        {
            return allocated_object_->get();
        }

        const T *operator->() const
        {
            return allocated_object_->get();
        }

        T &operator*() noexcept
        {
            return *allocated_object_->get();
        }

        const T &operator*() const noexcept
        {
            return *allocated_object_->get();
        }

        operator bool() const noexcept
        {
            return allocated_object_ ? static_cast<bool>(allocated_object_->get()) : false;
        }

    private:
        iallocated_object<T> *allocated_object_;
    };

    template<typename T, typename Allocator, typename...Args>
    unique_ptr<T, Allocator> make_unique(Args&&...args)
    {
        //TODO check sizeof(std::max_align_t) >= sizeof(memory_holder<T, Allocator>);
        //TODO check exceptions
        allocated_object<T, Allocator> alloc_object{ std::forward<Args>(args)... };

        unique_ptr<T, Allocator> ans;
        ans.allocated_object_ = new allocated_object<T, Allocator>(std::move(alloc_object));

        return ans;
    }
}
