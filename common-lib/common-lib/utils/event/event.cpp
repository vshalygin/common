#include "event.h"
#include <condition_variable>
#include <mutex>

namespace vsh::common_lib {
    class event::impl final
    {
    public:
        impl() = default;

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void set()
        {
            {
                std::lock_guard guard(mtx_);
                is_set_ = true;
            }

            cv_.notify_all();
        }

        bool is_set() const
        {
            std::lock_guard guard(mtx_);
            return is_set_;
        }

        void clear()
        {
            std::lock_guard guard(mtx_);
            is_set_ = false;
        }

        void wait()
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this]() { return is_set_; });
        }

        bool wait_for(const std::chrono::microseconds &mcs)
        {
            std::unique_lock lock(mtx_);
            return cv_.wait_for(lock, mcs, [this]() { return is_set_; });
        }

    private:
        mutable std::mutex mtx_;
        bool is_set_ = false;

        std::condition_variable cv_;
    };

    event::event()
        : impl_(std::make_unique<impl>())
    {}

    event::~event() = default;

    event::event(event &&) = default;
    event &event::operator=(event &&) = default;

    void event::set()
    {
        impl_->set();
    }

    bool event::is_set() const
    {
        return impl_->is_set();
    }

    void event::clear()
    {
        impl_->clear();
    }

    void event::wait()
    {
        impl_->wait();
    }

    bool event::wait_for(const std::chrono::microseconds &mcs)
    {
        return impl_->wait_for(mcs);
    }
}