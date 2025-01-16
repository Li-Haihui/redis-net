#ifndef TASK_WORKER_INC_LOCKQUEUE_H_
#define TASK_WORKER_INC_LOCKQUEUE_H_

#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
#include <utility>

template<typename T>
class LockQueue
{
public:
    LockQueue()
    {}

    ~LockQueue()
    {}

    /**
     * push element into queue
     */
    void Push(T data)
    {
        std::lock_guard<std::mutex> lk(mutex_);

        data_queue_.push(std::move(data));
        data_cond_.notify_one();
    }

    /**
     * wait until a data is popped out of the queue
     * @param value reference to store the return element
     */
    void WaitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        data_cond_.wait(lk, [this] {return !data_queue_.empty();});

        value = std::move(data_queue_.front());
        data_queue_.pop();
    }

    /**
     * wait until a data is popped out of the queue
     * @return the smart pointer of the return element
     */
    std::shared_ptr<T> WaitAndPop()
    {
        std::unique_lock<std::mutex> lk(mutex_);

        data_cond_.wait(lk, [this] {return !data_queue_.empty();});
        std::shared_ptr<T> res(
                std::make_shared<T>(std::move(data_queue_.front())));
        data_queue_.pop();

        return res;
    }

    /**
     * try pop an element from the queue, if queue is empty, it will
     * return immediately instead of waiting
     * @param value reference to store the return element
     * @return whether an element has retrieved successfully or not
     */
    bool TryPop(T& value)
    {
        std::lock_guard<std::mutex> lk(mutex_);

        if (data_queue_.empty()) {
            return false;
        }

        value = std::move(data_queue_.front());
        data_queue_.pop();

        return true;
    }

    /**
     * try pop an element from the queue, if queue is empty, it will
     * return immediately instead of waiting
     * @return smart pointer of the popped element if queue is
     * empty, NULL otherwise
     */
    std::shared_ptr<T> TryPop()
    {
        std::lock_guard<std::mutex> lk(mutex_);

        if (data_queue_.empty()) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> res(
                std::make_shared<T>(std::move(data_queue_.front())));
        data_queue_.pop();

        return res;
    }

    /**
     * the queue is empty or not
     * @return true if queue is empty, false otherwise
     */
    bool Empty() const
    {
        std::lock_guard<std::mutex> lk(mutex_);

        return data_queue_.empty();
    }

private:
    /**
     * mutex to protect queue
     */
    mutable std::mutex        mutex_;

    /**
     * internal queue to store data
     */
    std::queue<T>             data_queue_;

    /**
     * condition variable to wake up waiting thread
     */
    std::condition_variable   data_cond_;
};


#endif
