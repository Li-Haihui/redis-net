#ifndef TASK_WORKER_INC_TIMERWRAPPER_H_
#define TASK_WORKER_INC_TIMERWRAPPER_H_

#include <chrono>
#include <functional>


class TimerWorker;

class TimerWrapper
{
    friend class TimerWorker;
public:
    TimerWrapper():interval_(0),repeat_(0),timer_id_(0){}
    ~TimerWrapper() {}

private:
    using TimePoint = std::chrono::steady_clock::time_point;

public:
    /**
     * run timer in timer worker only once
     */
    void EnableOneShot();

    /**
     * run timer in timer worker periodically
     */
    void EnableRepeat();

    /**
     * get timer interval time
     */
    uint64_t GetInterval() const;

    /**
     * set next trigger time point, timer will run when the time point
     * arrive
     */
    TimePoint GetNextTriggerTime() const;

    /**
     * if timer will run only once or periodically
     */
    bool GetRepeat() const;

    /**
     * get unique timer ID assigned by timer worker
     */
    int GetTimerId() const;

    /**
     * run timer
     */
    void RunTimer();

    /**
     * config timer
     */
    void SetConfig(uint64_t interval, bool repeat, int id,
                   const std::function<void()>& timer);

    /**
     * set timer interval
     */
    void SetInterval(uint64_t interval);

    /**
     * set next trigger time point, timer will run once the time point
     * arrive
     */
    void SetNextTriggerTime(const TimePoint& trigger);

    /**
     * set timer to run periodically
     */
    void SetRepeat(bool repeat);

    /**
     * set function object as timer
     */
    void SetTimer(const std::function<void()>& timer);

    /**
     * set timer ID
     */
    void SetTimerId(int id);

    /**
     * rule to sort timer in the queue, timer with the earliest run
     * time point will be on top of the queue
     */
    bool operator<(const TimerWrapper& timer_wrapper) const
    {
        return time_trigger_ < timer_wrapper.time_trigger_;
    }

private:
    /**
     * interval to run timer
     */
    uint64_t interval_;

    /**
     * whether run timer periodically or not
     */
    bool     repeat_;

    /**
     * unique timer ID in timer worker
     */
    int      timer_id_;

    /**
     * function object as timer entry point
     */
    std::function<void()> timer_;

    /**
     * triggered time point to run timer
     */
    TimePoint   time_trigger_;
};


#endif  // TASK_WORKER_INC_TIMERWRAPPER_H_

