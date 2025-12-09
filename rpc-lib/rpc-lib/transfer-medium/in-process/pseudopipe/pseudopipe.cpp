#include "pseudopipe.h"

namespace vsh::rpc {
    namespace {
        void read_from_message_queue(auto &guarded_callback_queue, auto &guarded_message_queue)
        {
            cl::buffer buf;
            std::function<void(cl::buffer &&)> callback;

            {
                auto [guard, callbacks_queue] = guarded_callback_queue.get();
                if(callbacks_queue.empty()) {
                    return;
                }

                auto [guard2, message_queue] = guarded_message_queue.get();
                if(message_queue.empty()) {
                    return;
                }

                buf = std::move(message_queue.front());
                message_queue.pop();

                callback = std::move(callbacks_queue.front());
                callbacks_queue.pop();
            }

            try {
                callback(std::move(buf));
            } catch(...) {
                //TODO log
            }
        }
    }

    std::shared_ptr<pseudopipe> pseudopipe::create(std::shared_ptr<cl::ithread_pool> thread_pool)
    {
        return std::make_shared<pseudopipe>(std::move(thread_pool), creator());
    }

    pseudopipe::pseudopipe(std::shared_ptr<cl::ithread_pool> thread_pool, creator)
        : m_thread_pool(std::move(thread_pool))
        , m_strand_from_client_to_server(m_thread_pool->create_strand())
        , m_strand_from_server_to_client(m_thread_pool->create_strand())
    {}

    void pseudopipe::write_to_client_async(cl::buffer &&buffer)
    {
        post_write_to_client_message_queue(std::move(buffer));
    }

    void pseudopipe::write_to_server_async(cl::buffer &&buffer)
    {
        post_write_to_server_message_queue(std::move(buffer));
    }

    void pseudopipe::read_from_client_async(std::function<void(cl::buffer &&)> &&callback)
    {
        {
            auto [guard, read_from_client_callbacks_queue] = m_read_from_client_callbacks_queue.get();
            read_from_client_callbacks_queue.push(std::move(callback));
        }

        post_read_from_client_message_queue();
    }

    void pseudopipe::read_from_server_async(std::function<void(cl::buffer &&)> &&callback)
    {
        {
            auto [guard, read_from_server_callbacks_queue] = m_read_from_server_callbacks_queue.get();
            read_from_server_callbacks_queue.push(std::move(callback));
        }

        post_read_from_server_message_queue();
    }

    void pseudopipe::write_to_client_message_queue(cl::buffer &&buffer)
    {
        {
            auto [guard2, client_message_queue] = m_client_message_queue.get();
            client_message_queue.push(std::move(buffer));
        }
        
        post_read_from_client_message_queue();
    }

    void pseudopipe::write_to_server_message_queue(cl::buffer &&buffer)
    {
        {
            auto [guard2, server_message_queue] = m_server_message_queue.get();
            server_message_queue.push(std::move(buffer));
        }

        post_read_from_server_message_queue();
    }

    void pseudopipe::read_from_client_message_queue()
    {
        read_from_message_queue(m_read_from_client_callbacks_queue, m_client_message_queue);
    }

    void pseudopipe::read_from_server_message_queue()
    {
        read_from_message_queue(m_read_from_server_callbacks_queue, m_server_message_queue);
    }

    void pseudopipe::post_write_to_client_message_queue(cl::buffer &&buffer)
    {
        auto task = [self = weak_from_this(), buffer = std::make_shared<cl::buffer>(std::move(buffer))]() mutable {
            if(auto s = self.lock()) {
                s->write_to_client_message_queue(std::move(*buffer));
            }
        };
        m_strand_from_server_to_client->post(std::move(task));
    }

    void pseudopipe::post_write_to_server_message_queue(cl::buffer &&buffer)
    {
        auto task = [self = weak_from_this(), buffer = std::make_shared<cl::buffer>(std::move(buffer))]() mutable {
            if(auto s = self.lock()) {
                s->write_to_server_message_queue(std::move(*buffer));
            }
        };
        m_strand_from_client_to_server->post(std::move(task));
    }

    void pseudopipe::post_read_from_client_message_queue()
    {
        auto task = [self = weak_from_this()]() {
            if(auto s = self.lock()) {
                s->read_from_client_message_queue();
            }
        };

        m_thread_pool->post(std::move(task));
    }

    void pseudopipe::post_read_from_server_message_queue()
    {
        auto task = [self = weak_from_this()]() {
            if(auto s = self.lock()) {
                s->read_from_server_message_queue();
            }
        };

        m_thread_pool->post(std::move(task));
    }
}
