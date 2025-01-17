#include "TimerWrapper.h"


void TimerWrapper::EnableOneShot()
{
    repeat_ = false;
}

void TimerWrapper::EnableRepeat()
{
    repeat_ = true;
}

uint64_t TimerWrapper::GetInterval() const
{
    return interval_;
}

TimerWrapper::TimePoint TimerWrapper::GetNextTriggerTime() const
{
    return time_trigger_;
}

bool TimerWrapper::GetRepeat() const
{
    return repeat_;
}

int TimerWrapper::GetTimerId() const
{
    return timer_id_;
}

void TimerWrapper::RunTimer()
{
    if (timer_) {
        timer_();
    }
}

void TimerWrapper::SetConfig(uint64_t interval, bool repeat,
                             int id, const std::function<void()>& timer)
{
    SetInterval(interval);
    SetRepeat(repeat);
    SetTimerId(id);
    SetTimer(timer);
}

void TimerWrapper::SetInterval(uint64_t interval)
{
    interval_ = interval;
}

void TimerWrapper::SetNextTriggerTime(const TimePoint& trigger)
{
    time_trigger_ = trigger;
}

void TimerWrapper::SetRepeat(bool repeat)
{
    repeat_ = repeat;
}

void TimerWrapper::SetTimer(const std::function<void()>& timer)
{
    timer_ = timer;
}

void TimerWrapper::SetTimerId(int id)
{
    timer_id_ = id;
}

