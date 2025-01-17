#ifndef TASK_WORKER_INC_TIMER_H_
#define TASK_WORKER_INC_TIMER_H_

#include <atomic>
#include <chrono>
#include <memory>

class Timer
{
    friend class PriorityCompare;

private:
    using TimePoint = std::chrono::steady_clock::time_point;

public:
    /**
     * @param interval interval time to run timer in milliseconds
     * @param repeat whether run timer periodically
     */
    explicit Timer(uint64_t interval, bool repeat = true);

    virtual ~Timer();

    /**
     * run timer in timer pool only once then it will be discarded
     */
    void EnableOneShot();

    /**
     * run timer in timer pool periodically
     */
    void EnableRepeat();

    /**
     * get timert interval
     * @return interval
     */
    uint64_t GetInterval() const;

    /**
     * get next timer trigger time point then the timer will run again
     */
    TimePoint GetNextTriggerTime() const;

    /**
     * whether timer will run periodically or not, if not, timer will be
     * discarded by timer pool, the only way to run timer again is
     * to submit this timer to timer pool again
     */
    bool Repeat() const;

    /**
     * task entry point of the timer, you must re-written it if you
     * want to implement your own timer
     */
    virtual void Run() = 0;

    /**
     * set next trigger time point, timer will run once the set time point
     * arrive
     */
    void SetNextTriggerTime(const TimePoint& trigger);

private:
    /**
     * interval to run timer
     */
    uint64_t               interval_;

    /**
     * triggered time point to run timer
     */
    TimePoint              time_trigger_;

    /**
     * whether run timer periodically or not
     */
    std::atomic<bool>      is_repeat_;
};

/***********************************************************
 * We use priority queue in C++ standard library to store
 * timers. The timer with the eariest trigerred time point
 * will be popped to handle at top priority. This is a helper
 * class for queue to do comarison.
 ***********************************************************/

class PriorityCompare
{
private:
    using TimerType = std::shared_ptr<Timer>;

public:
    bool operator()(const TimerType& timer1, const TimerType& timer2)
    {
        return timer2->time_trigger_ < timer1->time_trigger_;
    }
};

#endif  // TASK_WORKER_INC_TIMER_H_
