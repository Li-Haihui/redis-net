#include "TimerPool.h"

#include <algorithm>
#include <functional>


void TimerPriorityQueue::Remove(const TimerType& timer)
{
    auto it = std::find(c.begin(), c.end(), timer);

    if (it != c.end()) {
        c.erase(it);
        std::make_heap(c.begin(), c.end(), comp);
    }
}

void TimerPriorityQueue::Clear()
{
    c.clear();
}

bool TimerPriorityQueue::IsTimerInQueue(const TimerType& timer)
{
    auto it = std::find(c.begin(), c.end(), timer);

    return it != c.end();
}

thread_local InterruptFlag TimerPool::this_thread_interrupt_flag_;

TimerPool::TimerPool()
{
}

TimerPool::~TimerPool()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TimerPool::EnableOneShot(const TimerType& timer)
{
    timer->EnableOneShot();

    Submit(timer);
}

void TimerPool::EnableRepeat(const TimerType& timer)
{
    timer->EnableRepeat();

    Submit(timer);
}

bool TimerPool::Init()
{
    try {
        std::promise<InterruptFlag*> p;
        auto callable = std::bind(&TimerPool::runPendingTimer, this);
        thread_ = std::thread([callable, &p] {
            p.set_value(&this_thread_interrupt_flag_);
            callable();
        });
        interrupt_flag_ = p.get_future().get();

        return true;
    } catch (...) {
        return false;
    }
}

void TimerPool::Interrupt()
{
    if (interrupt_flag_) {
        interrupt_flag_->Interrupt();
    }
}

void TimerPool::interruptPoint()
{
    if (this_thread_interrupt_flag_.Interrupted()) {
        throw InterruptException();
    }
}

void TimerPool::notify()
{
    if (interrupt_flag_) {
        interrupt_flag_->NotifyTask();
    }
}

std::unique_lock<std::mutex> TimerPool::pickUpTimers()
{
    std::unique_lock<std::mutex> ul(timer_queue_mutex_);

    while (!timer_queue_.empty()) {
        interruptPoint();

        const TimerType& timer = timer_queue_.top();

        updateNowTime();

        if (timer->GetNextTriggerTime() <= now_time_) {
            TimerType popped_timer = timer_queue_.top();
            timer_queue_.pop();

            interruptPoint();
            ul.unlock();

            popped_timer->Run();

            ul.lock();

            if (popped_timer->Repeat()) {
                resetTimer(popped_timer);
                timer_queue_.push(popped_timer);
            }
        } else {
            break;
        }
    }

    return ul;
}

void TimerPool::Remove(const TimerType& timer)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    timer_queue_.Remove(timer);
}

void TimerPool::Clear()
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    timer_queue_.Clear();
}

void TimerPool::resetTimer(const TimerType& timer)
{
    auto interval = timer->GetInterval();
    timer->SetNextTriggerTime(now_time_ +
                              std::chrono::milliseconds(interval));
}

void TimerPool::runPendingTimer()
{
    try {
        while (true) {
            interruptPoint();
            std::unique_lock<std::mutex> ul(pickUpTimers());

            TimePoint till_time;
            auto size = timer_queue_.size();

            if (size > 0) {
                const TimerType& top_timer = timer_queue_.top();
                till_time = top_timer->GetNextTriggerTime();
            }

            interruptibleWait(timer_queue_cond_,
                              ul,
                              till_time,
                              size);
        }
    } catch (const std::exception& e) {
        return;
    }
}

void TimerPool::Submit(const TimerType& timer)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    bool isTimerInQueue = timer_queue_.IsTimerInQueue(timer);

    updateNowTime();
    resetTimer(timer);

    if (!isTimerInQueue) {
        timer_queue_.push(timer);
    }

    notify();
}

void TimerPool::updateNowTime()
{
    now_time_ = std::chrono::steady_clock::now();
}
