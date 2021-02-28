#pragma once

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

template <typename T>
class AudioRingBuffer {
public:
    // size must be a power of 2
    AudioRingBuffer(unsigned size)
    {
        assert((size & (size - 1)) == 0);
        size_ = size;
        mask_ = size - 1;
        head_ = 0;
        buffer_ = new T[size_];
    }

    ~AudioRingBuffer()
    {
        delete[] buffer_;
    }

    unsigned size()
    {
        return size_;
    }

    unsigned head()
    {
        return head_;
    }

    unsigned offset(unsigned index)
    {
        return head_ - index;
    }

    T* peek()
    {
        return buffer_ + (head_ & mask_);
    }

    T* peek(unsigned index)
    {
        return buffer_ + (index & mask_);
    }

    void advance()
    {
        std::lock_guard<std::mutex> guad(mutex_);
        head_++;
        condition_.notify_all();
    }

    void wait(unsigned index)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (head_ == index) {
            condition_.wait(lock);
        }
    }

    bool wait_for(unsigned index, unsigned timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (head_ == index) {
            auto result = condition_.wait_for(lock, std::chrono::milliseconds(timeout));
            if (std::cv_status::timeout == result) {
                return false;
            }
        }

        return true;
    }

    class Reader {
    public:
        Reader(AudioRingBuffer<T>* buffer, unsigned expiry)
            : buffer_(buffer)
        {
            index_ = buffer_->head();
            expiry_ = expiry;
        }

        void clear()
        {
            index_ = buffer_->head();
        }

        void keep(unsigned remain)
        {
            index_ = buffer_->head() - remain;
        }

        unsigned available()
        {
            return buffer_->offset(index_);
        }

        T* get()
        {
            unsigned offset = buffer_->offset(index_);
            if (offset) {
                if (offset > expiry_) {
                    index_ += offset - expiry_;
                }

                T* ptr = buffer_->peek(index_);
                index_++;

                return ptr;
            } else {
                buffer_->wait(index_);

                T* ptr = buffer_->peek(index_);
                index_++;

                return ptr;
            }
        }

        int read(T** ptr, unsigned timeout)
        {
            unsigned offset = buffer_->offset(index_);
            int skip = 0;
            if (0 == offset) {
                if (!buffer_->wait_for(index_, timeout)) {
                    return -1;
                }
            } else if (expiry_ < offset) {
                skip = offset - expiry_;
                index_ += skip;
            }

            *ptr = buffer_->peek(index_);
            index_++;

            return skip;
        }

    private:
        AudioRingBuffer<T>* buffer_;
        unsigned expiry_;
        unsigned index_;
    };

    Reader create_reader(unsigned expiry = 5)
    {
        return Reader(this, expiry);
    }

private:
    T* buffer_;
    unsigned head_;
    unsigned size_;
    unsigned mask_;
    std::mutex mutex_;
    std::condition_variable condition_;
};