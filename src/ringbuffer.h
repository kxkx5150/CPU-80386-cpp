#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T> class RingBuffer {
  public:
    explicit RingBuffer(size_t capacity) : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0)
    {
    }

    void push(const T &item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] { return size_ < capacity_; });

        buffer_[head_] = item;
        head_          = (head_ + 1) % capacity_;
        ++size_;

        lock.unlock();
        not_empty_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return size_ > 0; });

        T item = buffer_[tail_];
        tail_  = (tail_ + 1) % capacity_;
        --size_;

        lock.unlock();
        not_full_.notify_one();
        return item;
    }

    bool isempty()
    {
        return head_ == tail_;
    }

  private:
    std::vector<T>          buffer_;
    size_t                  capacity_;
    size_t                  head_;
    size_t                  tail_;
    size_t                  size_;
    std::mutex              mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};

#ifdef RINGBUFFER_USAGE

#include <iostream>
#include <thread>
#include <chrono>

// ln src/ringbuffer.h ringbuffer.cpp
// g++ -DRINGBUFFER_DUSAGE ringbuffer.h -o ringbuffer

void producer(RingBuffer<int> &buffer)
{
    int value = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        buffer.push(value);
        std::cout << "[producer] pushed " << value++ << "\n";
    }
}

int main()
{
    RingBuffer<int> buffer(5);

    std::thread worker(producer, std::ref(buffer));

    std::cout << "press CR to read value from buffer...\n";
    while (true) {
        std::cin.get();    // wait for CR
        int value = buffer.pop();
        std::cout << "[main] popped " << value << "\n";
    }

    worker.join();    // not reached
}

#endif    // RINGBUFFER_USAGE

#endif    // _RINGBUFFER_H
