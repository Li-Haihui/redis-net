#ifndef TASK_WORKER_INC_INTERRUPTFLAG_H_
#define TASK_WORKER_INC_INTERRUPTFLAG_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "LockQueue.h"
#include "TaskStealQueue.h"


class InterruptException : public std::exception
{
public:
    virtual const char* what() const throw()
    {
        return "thread interrupted";
    }
};

class InterruptFlag
{
private:
    using TimePoint = std::chrono::steady_clock::time_point;
    using StealQueue =
        std::vector<std::unique_ptr<TaskStealQueue>>;
    using PoolQueue =
        LockQueue<TaskWrapper>;

    template<typename Lockable>
    class CustomizedLock
    {
    public:
        CustomizedLock(InterruptFlag* flag,
                       std::condition_variable_any& cva,
                       Lockable& clk, bool lock = false)
            : intr_flag_(flag), lk_(clk), lock_flag_(lock)
        {
            intr_flag_->cond_mutex_.lock();
            intr_flag_->interrupt_cond_ = &cva;

            if (lock_flag_) {
                lk_.lock();
            }
        }

        ~CustomizedLock()
        {
            intr_flag_->interrupt_cond_ = nullptr;
            intr_flag_->cond_mutex_.unlock();

            if (lock_flag_) {
                lk_.unlock();
            }
        }

        void lock()
        {
            std::lock(intr_flag_->cond_mutex_, lk_);
        }

        void unlock()
        {
            lk_.unlock();
            intr_flag_->cond_mutex_.unlock();
        }

        InterruptFlag* intr_flag_ = nullptr;
        Lockable& lk_;
        bool lock_flag_;
    };

public:
    InterruptFlag();
    ~InterruptFlag();

    /**
     * interrupt thread
     * @note the interrupting thread will only stop at interrupt point
     * where it considers could be stopped safely
     */
    void Interrupt();

    /**
     * whether interruption has been triggerred
     * @return the interruppted flag
     */
    bool Interrupted() const;

    /**
     * notify threads task is coming
     */
    bool NotifyTask();

    template<typename Lockable>
    void Wait(std::condition_variable_any& cv,
              Lockable& lk,
              TimePoint& till_time,
              uint64_t size)
    {
        CustomizedLock<Lockable> clk(this, cv, lk);
        interruptPoint();

        if (size > 0) {
            cv.wait_until(clk, till_time);
        } else {
            cv.wait(clk);
        }

        interruptPoint();
    }

    template<typename Lockable>
    void Wait(std::condition_variable_any& cv,
              Lockable& lk)
    {
        CustomizedLock<Lockable> clk(this, cv, lk);
        interruptPoint();
        cv.wait(clk);
        interruptPoint();
    }

    template<typename Lockable, typename Queue>
    void Wait(Lockable& lk,
              const Queue& queue,
              std::condition_variable_any& cv)
    {
        CustomizedLock<Lockable> clk(this, cv, lk);
        interruptPoint();
        cv.wait(clk, [&] { return isTaskInQueues(queue)
                                  || interrupt_flag_.load();
                         });
        interruptPoint();
    }

    template<typename Lockable,
             typename PoolQueue,
             typename StealQueue>
    void Wait(Lockable& lk,
              const PoolQueue& pool,
              const StealQueue& steal,
              std::condition_variable_any& cv)
    {
        CustomizedLock<Lockable> clk(this, cv, lk, true);
        interruptPoint();
        cv.wait(clk, [&] { return isTaskInQueues(pool, steal)
                                  || interrupt_flag_.load();
                         });
        interruptPoint();
    }

private:
    /**
     * interrupt point where thread could stop safely
     */
    void interruptPoint();

    /**
     * whether global queue or local queues have tasks or not
     */
    template<typename PoolQueue, typename StealQueue>
    bool isTaskInQueues(const PoolQueue& pool,
                        const StealQueue& steal) const
    {
        if (!pool.Empty()) {
            return true;
        }

        for (const auto& item : steal) {
            if (item && !item->Empty()) {
                return true;
            }
        }

        return false;
    }

    /**
     * whether task queue have tasks or not (for TaskPool)
     */
    template<typename Queue>
    bool isTaskInQueues(const Queue& queue) const
    {
        return !queue.empty();
    }

private:
    /**
     * flag to indicate whether the thread should be interrupted or not
     */
    std::atomic<bool>             interrupt_flag_;

    /**
     * pointing to the condition variable by which a thread may be
     * waiting for the coming tasks to handle
     */
    std::condition_variable_any*  interrupt_cond_ = nullptr;

    /**
     * mutex for set the condition variable pointer mentioned above
     */
    std::mutex                    cond_mutex_;
};


#endif  // TASK_WORKER_INC_INTERRUPTFLAG_H_
