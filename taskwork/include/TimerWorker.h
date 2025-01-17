#ifndef TASK_WORKER_INC_TIMERWORKER_H_
#define TASK_WORKER_INC_TIMERWORKER_H_

#include <condition_variable>
#include <future>
#include <memory>
#include <queue>
#include <random>
#include <thread>
#include <list>
#include <utility>

#include "InterruptFlag.h"
#include "Timer.h"
#include "TimerWrapper.h"

/**
* @brief unique random number generator
* unique random number will be used as unique timer ID
* in a timerworker instance
*/
class RandomUniqueId
{
public:
    RandomUniqueId();
    ~RandomUniqueId();

    int  GenerateUniqueRandNumber();
    void RemoveRandNumber(int num);
    void Clear();

private:
    bool isSameRandNumber(int num);

private:
    std::default_random_engine engine_;
    std::uniform_int_distribution<int> distribution_;
    std::list<int> rand_list_;

    static constexpr unsigned max_retries_ = 100;
};

class TimerWorker
{
private:
    using TimerType = std::shared_ptr<Timer>;
    using TimePoint = std::chrono::steady_clock::time_point;
public:
    TimerWorker();
    ~TimerWorker();

    TimerWorker(const TimerWorker&) = delete;
    TimerWorker& operator=(const TimerWorker&) = delete;

    /**
     * remove all timers in timer pool
     */
    void Clear();

    /**
     * disable timer in the queue, timer will not run again unless
     * being enabled it again
     */
    void DisableTimer(int timer_id);

    /**
     * enable timer in the queue
     */
    void EnableTimer(int timer_id, uint64_t interval = 0);

    /**
     * enable timer running only once
     */
    void EnableOneShot(int timer_id, uint64_t interval = 0);

    /**
     * enable timer running periodically
     */
    void EnableRepeat(int timer_id, uint64_t interval = 0);

    /**
     * init timer pool, create a thread to dispatch timers
     * @return true if thread is created successfully, false otherwise
     */
    bool Init();

    /**
     * shut down timer pool
     */
    void Shutdown();

    /**
     * remove timer from timer pool
     */
    void RemoveTimer(int timer_id);


    /**
     * submit new created timers to timer pool for handle
     */
    template<typename FunctionType, typename...Arguments>
    int Submit(uint64_t interval, bool repeat,
               FunctionType&& f, Arguments&& ...args)
    {
        std::function<void()> timer(
                std::bind(std::forward<FunctionType>(f),
                          std::forward<Arguments>(args)...));

        std::lock_guard<std::mutex> lg(timer_queue_mutex_);
        TimerWrapper timer_wrapper = setConfig(interval, repeat, timer);

        updateNowTime();
        resetTimer(timer_wrapper);

        putTimerIntoRunQueue(timer_wrapper);

        notify();

        return timer_wrapper.GetTimerId();
    }

private:
    /**
    * get timer in the run timer queue by ID
    */
    TimerWrapper* getRunTimerById(int timer_id);

    /**
    * get timer in the pending timer queue by ID
    */
    TimerWrapper* getPendingTimerById(int timer_id);

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
                           TimePoint& till_time,
                           uint64_t size)
    {
        thread_exit_flag_.Wait(cv, lk, till_time, size);
    }

    /**
    * move timer from run timer queue to pending queue
    */
    void moveTimerToPendingQueue(int timer_id);

    /**
    * move timer from pending timer queue to run queue
    */
    void moveTimerToRunQueue(int timer_id, uint64_t interval = 0);

    /**
      * notify thread that timer is coming
      */
    void notify();

    /**
     * pick up timers to run whose trigger time point have arrived
     */
    std::unique_lock<std::mutex> pickUpTimers();

    void putTimerIntoRunQueue(const TimerWrapper& timer);

    /**
     * set next trigger time point for the timer
     */
    void resetTimer(TimerWrapper& timer, uint64_t interval = 0);

    /**
      * reset repeat timer in running queue
      */
    void resetRepeatTimer(int timer_id);

    /**
     * enrty point for internal thread to run timers
     */
    void runTimer();

    /**
     * run pending timers
     */
    void runPendingTimer();

    /**
     * config timer
     */
    TimerWrapper setConfig(uint64_t interval, bool repeat,
                           const std::function<void()>& timer);

    /**
     * update now time point of steady clock
     */
    void updateNowTime();

private:
    /**
     * helper class instance to generate non-repetitive random number
     */
    RandomUniqueId                     timer_unique_random_;

    /**
     * point to interrupt flag
     */
    InterruptFlag*                     interrupt_flag_ = nullptr;

    /**
     * queue to store timers
     */
    std::list<TimerWrapper>            timer_run_queue_;

    /**
     * queue to store pending timers
     */
    std::list<TimerWrapper>            timer_pending_queue_;

    /**
     * what's the time point now based on steady clock
     */
    TimePoint                          now_time_;

    /**
     * mutex to protect queue
     */
    std::mutex                         timer_queue_mutex_;

    /**
     * internal thread will use this to wait for new coming timers
     */
    std::condition_variable_any        timer_run_queue_cond_;

    /**
     * internal thread to run timer dispatch job
     */
    std::thread                        thread_;

    /**
     * thread local interrupt flag
     */
    static thread_local InterruptFlag  thread_exit_flag_;
};


#endif  // TASK_WORKER_INC_TIMERWORKER_H_
