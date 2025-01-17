#include "Timer.h"


Timer::Timer(uint64_t interval, bool repeat)
    : interval_(interval)
{
    is_repeat_.store(repeat, std::memory_order_release);
}

Timer::~Timer()
{
}

void Timer::EnableOneShot()
{
    is_repeat_.store(false, std::memory_order_release);
}

void Timer::EnableRepeat()
{
    is_repeat_.store(true, std::memory_order_release);
}

uint64_t Timer::GetInterval() const
{
    return interval_;
}

Timer::TimePoint Timer::GetNextTriggerTime() const
{
    return time_trigger_;
}

bool Timer::Repeat() const
{
    return is_repeat_.load(std::memory_order_acquire);
}

void Timer::SetNextTriggerTime(const TimePoint& trigger)
{
    time_trigger_ = trigger;
}

