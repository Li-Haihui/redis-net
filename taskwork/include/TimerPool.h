#ifndef TASK_WORKER_INC_TIMERPOOL_H_
#define TASK_WORKER_INC_TIMERPOOL_H_

#include <condition_variable>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "InterruptFlag.h"
#include "Timer.h"

/**********************************************************
 * priority queue to store timers
 *********************************************************/
class TimerPriorityQueue :
    public std::priority_queue<std::shared_ptr<Timer>,
    std::vector<std::shared_ptr<Timer>>,
                                     PriorityCompare>
{
private:
    using TimerType = std::shared_ptr<Timer>;

public:
    /**
     * remove timer from queue
     */
    void Remove(const TimerType& timer);

    /**
     * remove all timers in queue
     */
    void Clear();

    /**
     * check if timer is in the queue
     */
    bool IsTimerInQueue(const TimerType& timer);
};

class TimerPool
{
private:
    using TimerType = std::shared_ptr<Timer>;
    using TimePoint = std::chrono::steady_clock::time_point;
    using QueueType = TimerPriorityQueue;
public:
    TimerPool();
    ~TimerPool();

    TimerPool(const TimerPool&) = delete;
    TimerPool& operator=(const TimerPool&) = delete;

    /**
     * run timer just once
     */
    void EnableOneShot(const TimerType& timer);

    /**
     * run timer periodically
     */
    void EnableRepeat(const TimerType& timer);

    /**
     * init timer pool, create a thread to dispatch timers
     * @return true if thread is created successfully, false otherwise
     */
    bool Init();

    /**
     * interrupt timer pool
     */
    void Interrupt();

    /**
     * remove timer from timer pool
     */
    void Remove(const TimerType& timer);

    /**
     * remove all timers in timer pool
     */
    void Clear();

    /**
     * submit new created timers to timer pool for handle
     */
    void Submit(const TimerType& timer);

private:
    /**
    * interrupt point where it's safe to stop timer pool
    */
    void interruptPoint();

    /**
     * waiting for new timers, waiting could be stopped if
     * interruption has been triggerred
     */
    template<typename Lockable>
    void interruptibleWait(std::condition_variable_any& cv,
                           Lockable& lk,
                           TimePoint& tillTime,
                           uint64_t size)
    {
        this_thread_interrupt_flag_.Wait(cv, lk, tillTime, size);
    }

    /**
     * pick up timers to run whose trigger time point have arrived
     */
    std::unique_lock<std::mutex> pickUpTimers();

    /**
     * set next trigger time point for the timer
     */
    void resetTimer(const TimerType& timer);

    /**
     * enrty point for internal thread to run timers
     */
    void runPendingTimer();

    /**
      * notify thread that timer is coming
      */
    void notify();

    /**
     * update now time point of steady clock
     */
    void updateNowTime();

private:
    /**
     * point to interrupt flag
     */
    InterruptFlag*                     interrupt_flag_ = nullptr;

    /**
     * priority queue to store timers
     */
    QueueType                           timer_queue_;

    /**
     * what's the time point now based on steady clock
     */
    TimePoint                           now_time_;

    /**
     * mutex to protect queue
     */
    std::mutex                          timer_queue_mutex_;

    /**
     * internal thread will use this to wait for new coming timers
     */
    std::condition_variable_any         timer_queue_cond_;

    /**
     * internal thread to run timer dispatch job
     */
    std::thread                         thread_;

    /**
     * thread local interrupt flag
     */
    static thread_local InterruptFlag  this_thread_interrupt_flag_;
};

#endif  // TASK_WORKER_INC_TIMERPOOL_H_
