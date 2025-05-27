#ifndef QUEUE_H
#define QUEUE_H

#include <cassert>
#include <cstddef>
#include <type_traits>

template <typename T, size_t Capacity>
class queue
{
private:
    static_assert(Capacity > 0, "Queue capacity must be greater than 0");

    T data[Capacity + 1]; // Extra slot to distinguish full from empty
    size_t head = 0;      // Points to front element
    size_t tail = 0;      // Points to next insertion position

    size_t next_index(size_t index) const
    {
        return (index + 1) % (Capacity + 1);
    }

public:
    using value_type = T;
    using size_type = size_t;
    using reference = T &;
    using const_reference = const T &;

    // Default constructor
    queue() = default;

    // Copy constructor
    queue(const queue &other) : head(other.head), tail(other.tail)
    {
        for (size_t i = 0; i < (Capacity + 1); ++i)
        {
            if (i != head || !empty())
            {
                data[i] = other.data[i];
            }
        }
    }

    // Destructor
    ~queue() = default;

    // Element access
    reference front()
    {
        assert(!empty() && "queue::front(): queue is empty");
        return data[head];
    }

    const_reference front() const
    {
        assert(!empty() && "queue::front(): queue is empty");
        return data[head];
    }

    reference back()
    {
        assert(!empty() && "queue::back(): queue is empty");
        size_t back_index = (tail == 0) ? Capacity : tail - 1;
        return data[back_index];
    }

    const_reference back() const
    {
        assert(!empty() && "queue::back(): queue is empty");
        size_t back_index = (tail == 0) ? Capacity : tail - 1;
        return data[back_index];
    }

    // Capacity
    bool empty() const noexcept
    {
        return head == tail;
    }

    size_type size() const noexcept
    {
        if (tail >= head)
        {
            return tail - head;
        }
        else
        {
            return (Capacity + 1) - head + tail;
        }
    }

    bool full() const noexcept
    {
        return next_index(tail) == head;
    }

    // Modifiers
    void push(const T &value)
    {
        assert(!full() && "queue::push(): queue is full");
        data[tail] = value;
        tail = next_index(tail);
    }

    void push(T &&value)
    {
        assert(!full() && "queue::push(): queue is full");
        data[tail] = value; // std::move(value);
        tail = next_index(tail);
    }

    void pop()
    {
        assert(!empty() && "queue::pop(): queue is empty");
        head = next_index(head);
    }

    void clear() noexcept
    {
        head = tail = 0;
    }
};

#endif
