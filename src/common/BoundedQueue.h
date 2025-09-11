// Copyright 2025 Xenon Emulator Project. All rights reserved.

#pragma once

#include "PolyfillThread.h"

namespace Base
{

namespace detail
{
constexpr size_t DefaultCapacity = 0x1000;
}  // namespace detail

template <typename T, size_t Capacity = detail::DefaultCapacity>
class SPSCQueue
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two.");

   public:
    template <typename... Args>
    bool TryEmplace(Args&&... args)
    {
        return Emplace<PushMode::Try>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args)
    {
        Emplace<PushMode::Wait>(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) { return Pop<PopMode::Try>(t); }

    bool PopWait(T& t) { return Pop<PopMode::Wait>(t); }

    bool PopWait(T& t, std::stop_token stopToken) { return Pop<PopMode::WaitWithStopToken>(t, stopToken); }

    T PopWait()
    {
        T t;
        Pop<PopMode::Wait>(t);
        return t;
    }

    T PopWait(std::stop_token stopToken)
    {
        T t;
        Pop<PopMode::WaitWithStopToken>(t, stopToken);
        return t;
    }

   private:
    enum class PushMode
    {
        Try,
        Wait,
        Count
    };

    enum class PopMode
    {
        Try,
        Wait,
        WaitWithStopToken,
        Count
    };

    template <PushMode Mode, typename... Args>
    bool Emplace(Args&&... args)
    {
        const size_t write_index = m_write_index.load(std::memory_order::relaxed);

        if constexpr (Mode == PushMode::Try)
        {
            // Check if we have free slots to write to.
            if ((write_index - m_read_index.load(std::memory_order::acquire)) == Capacity)
            {
                return false;
            }
        }
        else if constexpr (Mode == PushMode::Wait)
        {
            // Wait until we have free slots to write to.
            std::unique_lock lock{producer_cv_mutex};
            producer_cv.wait(lock, [this, write_index]
                             { return (write_index - m_read_index.load(std::memory_order::acquire)) < Capacity; });
        }
        else
        {
            static_assert(Mode < PushMode::Count, "Invalid PushMode.");
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Emplace into the queue.
        new (std::addressof(m_data[pos])) T(std::forward<Args>(args)...);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{consumer_cv_mutex};
        consumer_cv.notify_one();

        return true;
    }

    template <PopMode Mode>
    bool Pop(T& t, [[maybe_unused]] std::stop_token stop_token = {})
    {
        const size_t read_index = m_read_index.load(std::memory_order::relaxed);

        if constexpr (Mode == PopMode::Try)
        {
            // Check if the queue is empty.
            if (read_index == m_write_index.load(std::memory_order::acquire))
            {
                return false;
            }
        }
        else if constexpr (Mode == PopMode::Wait)
        {
            // Wait until the queue is not empty.
            std::unique_lock lock{consumer_cv_mutex};
            consumer_cv.wait(
                lock, [this, read_index] { return read_index != m_write_index.load(std::memory_order::acquire); });
        }
        else if constexpr (Mode == PopMode::WaitWithStopToken)
        {
            // Wait until the queue is not empty.
            std::unique_lock lock{consumer_cv_mutex};
            Base::CondvarWait(consumer_cv, lock, stop_token, [this, read_index]
                              { return read_index != m_write_index.load(std::memory_order::acquire); });
            if (stop_token.stop_requested())
            {
                return false;
            }
        }
        else
        {
            static_assert(Mode < PopMode::Count, "Invalid PopMode.");
        }

        // Determine the position to read from.
        const size_t pos = read_index % Capacity;

        // Pop the data off the queue, moving it.
        t = std::move(m_data[pos]);

        // Increment the read index.
        ++m_read_index;

        // Notify the producer that we have popped off the queue.
        std::scoped_lock lock{producer_cv_mutex};
        producer_cv.notify_one();

        return true;
    }

    alignas(128) std::atomic_size_t m_read_index{0};
    alignas(128) std::atomic_size_t m_write_index{0};

    std::array<T, Capacity> m_data;

    std::condition_variable_any producer_cv;
    std::mutex producer_cv_mutex;
    std::condition_variable_any consumer_cv;
    std::mutex consumer_cv_mutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPSCQueue
{
   public:
    template <typename... Args>
    bool TryEmplace(Args&&... args)
    {
        std::scoped_lock lock{writeMutex};
        return spscQueue.TryEmplace(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args)
    {
        std::scoped_lock lock{writeMutex};
        spscQueue.EmplaceWait(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) { return spscQueue.TryPop(t); }

    bool PopWait(T& t) { return spscQueue.PopWait(t); }

    bool PopWait(T& t, std::stop_token stop_token) { return spscQueue.PopWait(t, stop_token); }

    T PopWait() { return spscQueue.PopWait(); }

    T PopWait(std::stop_token stop_token) { return spscQueue.PopWait(stop_token); }

   private:
    SPSCQueue<T, Capacity> spscQueue;
    std::mutex writeMutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPMCQueue
{
   public:
    template <typename... Args>
    bool TryEmplace(Args&&... args)
    {
        std::scoped_lock lock{writeMutex};
        return spscQueue.TryEmplace(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void EmplaceWait(Args&&... args)
    {
        std::scoped_lock lock{writeMutex};
        spscQueue.EmplaceWait(std::forward<Args>(args)...);
    }

    bool TryPop(T& t)
    {
        std::scoped_lock lock{readMutex};
        return spscQueue.TryPop(t);
    }

    bool PopWait(T& t)
    {
        std::scoped_lock lock{readMutex};
        return spscQueue.PopWait(t);
    }

    bool PopWait(T& t, std::stop_token stopToken)
    {
        std::scoped_lock lock{readMutex};
        return spscQueue.PopWait(t, stopToken);
    }

    T PopWait()
    {
        std::scoped_lock lock{readMutex};
        return spscQueue.PopWait();
    }

    T PopWait(std::stop_token stop_token)
    {
        std::scoped_lock lock{readMutex};
        return spscQueue.PopWait(stop_token);
    }

   private:
    SPSCQueue<T, Capacity> spscQueue;
    std::mutex writeMutex;
    std::mutex readMutex;
};

}  // namespace Base
