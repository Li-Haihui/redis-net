#include "TimerWorker.h"

#include <algorithm>


RandomUniqueId::RandomUniqueId()
{
    std::uniform_int_distribution<>::param_type pt(1,
            std::numeric_limits<int>::max() - 1);

    distribution_(engine_, pt);
    distribution_.param(pt);
}

RandomUniqueId::~RandomUniqueId()
{
}

int RandomUniqueId::GenerateUniqueRandNumber()
{
    unsigned retries(0);

    while (retries <= max_retries_) {
        int num = distribution_(engine_);

        if (isSameRandNumber(num)) {
            ++retries;

            continue;
        } else {
            rand_list_.push_back(num);

            return num;
        }
    }

    return std::numeric_limits<int>::max();
}

void RandomUniqueId::RemoveRandNumber(int num)
{
    rand_list_.remove_if([&] (int elem) {
        return elem == num;
    });
}

void RandomUniqueId::Clear()
{
    rand_list_.clear();
}

bool RandomUniqueId::isSameRandNumber(int num)
{
    auto result = std::find(rand_list_.cbegin(),
                            rand_list_.cend(),
                            num);

    return result != rand_list_.cend();
}

thread_local InterruptFlag TimerWorker::thread_exit_flag_;

TimerWorker::TimerWorker()
{
}

TimerWorker::~TimerWorker()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TimerWorker::Clear()
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    timer_run_queue_.clear();
    timer_pending_queue_.clear();
    timer_unique_random_.Clear();
}

void TimerWorker::DisableTimer(int timer_id)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    moveTimerToPendingQueue(timer_id);
}

void TimerWorker::EnableTimer(int timer_id, uint64_t interval)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    moveTimerToRunQueue(timer_id, interval);

    notify();
}

void TimerWorker::EnableOneShot(int timer_id, uint64_t interval)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    TimerWrapper* run_timer = getRunTimerById(timer_id);

    if (run_timer) {
        run_timer->EnableOneShot();
        if (interval > 0) run_timer->SetInterval(interval);

        resetTimer(*run_timer, interval);
        timer_run_queue_.sort();

        notify();

        return;
    }

    TimerWrapper* pending_timer = getPendingTimerById(timer_id);

    if (pending_timer) {
        pending_timer->EnableOneShot();

        if (interval > 0) {
            pending_timer->SetInterval(interval);
        }

        moveTimerToRunQueue(timer_id, interval);

        notify();
    }
}

void TimerWorker::EnableRepeat(int timer_id, uint64_t interval)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    TimerWrapper* timer = getPendingTimerById(timer_id);

    if (timer) {
        timer->EnableRepeat();

        if (interval > 0) {
            timer->SetInterval(interval);
        }

        moveTimerToRunQueue(timer->GetTimerId(), interval);
    }

    notify();
}

TimerWrapper* TimerWorker::getRunTimerById(int timer_id)
{
    auto iter = std::find_if(
        timer_run_queue_.begin(),
        timer_run_queue_.end(),
        [&timer_id] (const TimerWrapper & tw) {
            return tw.GetTimerId() == timer_id;
        });

    if (iter != timer_run_queue_.end()) {
        return &(*iter);
    }

    return nullptr;
}

TimerWrapper* TimerWorker::getPendingTimerById(int timer_id)
{
    auto iter = std::find_if(
        timer_pending_queue_.begin(),
        timer_pending_queue_.end(),
        [&timer_id] (const TimerWrapper & tw) {
            return tw.GetTimerId() == timer_id;
        });

    if (iter != timer_pending_queue_.end()) {
        return &(*iter);
    }

    return nullptr;
}

bool TimerWorker::Init()
{
    try {
        std::promise<InterruptFlag*> p;
        auto callable = std::bind(&TimerWorker::runTimer, this);
        thread_ = std::thread([callable, &p] {
            p.set_value(&thread_exit_flag_);
            callable();
        });
        interrupt_flag_ = p.get_future().get();

        return true;
    } catch (...) {
        return false;
    }
}

void TimerWorker::Shutdown()
{
    if (interrupt_flag_) {
        interrupt_flag_->Interrupt();
    }
}

void TimerWorker::interruptPoint()
{
    if (thread_exit_flag_.Interrupted()) {
        throw InterruptException();
    }
}

void TimerWorker::moveTimerToRunQueue(int timer_id,
                                      uint64_t interval)
{
    auto iter = std::find_if(
        timer_pending_queue_.begin(),
        timer_pending_queue_.end(),
        [&timer_id] (const TimerWrapper & tw) {
            return tw.GetTimerId() == timer_id;
        });

    if (iter != timer_pending_queue_.end()) {
        resetTimer(*iter, interval);

        timer_run_queue_.push_back(*iter);
        timer_run_queue_.sort();

        timer_pending_queue_.erase(iter);
    }
}

void TimerWorker::moveTimerToPendingQueue(int timer_id)
{
    auto iter = std::find_if(
        timer_run_queue_.begin(),
        timer_run_queue_.end(),
        [&timer_id] (const TimerWrapper & tw) {
            return tw.GetTimerId() == timer_id;
        });

    if (iter != timer_run_queue_.end()) {
        timer_pending_queue_.push_back(*iter);

        timer_run_queue_.erase(iter);
        timer_run_queue_.sort();
    }
}


void TimerWorker::notify()
{
    if (interrupt_flag_) {
        interrupt_flag_->NotifyTask();
    }
}

std::unique_lock<std::mutex> TimerWorker::pickUpTimers()
{
    std::unique_lock<std::mutex> ul(timer_queue_mutex_);

    while (!timer_run_queue_.empty()) {
        interruptPoint();

        TimerWrapper timer_wrapper = timer_run_queue_.front();

        updateNowTime();

        if (timer_wrapper.GetNextTriggerTime() > now_time_) break;

        interruptPoint();

        ul.unlock();

        timer_wrapper.RunTimer();

        ul.lock();

        if (!timer_wrapper.GetRepeat()) {
            moveTimerToPendingQueue(timer_wrapper.GetTimerId());
        } else {
            resetRepeatTimer(timer_wrapper.GetTimerId());
        }
    }

    return ul;
}

void TimerWorker::putTimerIntoRunQueue(const TimerWrapper& timer)
{
    timer_run_queue_.push_back(timer);
    timer_run_queue_.sort();
}

void TimerWorker::resetTimer(TimerWrapper& timer, uint64_t interval)
{
    updateNowTime();

    if (interval == 0) {
        timer.SetNextTriggerTime(now_time_ +
                         std::chrono::milliseconds(timer.GetInterval()));
    } else {
        timer.SetInterval(interval);

        timer.SetNextTriggerTime(now_time_ +
                         std::chrono::milliseconds(interval));
    }
}

void TimerWorker::resetRepeatTimer(int timer_id)
{
    auto iter = std::find_if(
        timer_run_queue_.begin(),
        timer_run_queue_.end(),
        [&timer_id] (const TimerWrapper& tw) {
            return tw.GetTimerId() == timer_id;
        });

    if (iter != timer_run_queue_.end()) {
        resetTimer(*iter);
        timer_run_queue_.sort();
    }
}

void TimerWorker::runPendingTimer()
{
    while (true) {
        interruptPoint();
        std::unique_lock<std::mutex> ul(pickUpTimers());

        TimePoint till_time;
        auto size = timer_run_queue_.size();

        if (size > 0) {
            const TimerWrapper& top_timer = timer_run_queue_.front();
            till_time = top_timer.GetNextTriggerTime();
        }

        interruptibleWait(timer_run_queue_cond_,
                          ul,
                          till_time,
                          size);
    }
}

void TimerWorker::runTimer()
{
    try {
        runPendingTimer();
    } catch (...) {
        return;
    }
}

void TimerWorker::RemoveTimer(int timer_id)
{
    std::lock_guard<std::mutex> lg(timer_queue_mutex_);

    timer_run_queue_.remove_if([&timer_id] (const TimerWrapper & tw) {
        return tw.GetTimerId() == timer_id;
    });

    timer_pending_queue_.remove_if([&timer_id] (const TimerWrapper & tw) {
        return tw.GetTimerId() == timer_id;
    });

    timer_unique_random_.RemoveRandNumber(timer_id);
}

TimerWrapper TimerWorker::setConfig(uint64_t interval, bool repeat,
                                    const std::function<void()>& timer)
{
    int timer_id = timer_unique_random_.GenerateUniqueRandNumber();

    TimerWrapper timer_wrapper;
    timer_wrapper.SetConfig(interval, repeat, timer_id, timer);

    return timer_wrapper;
}

void TimerWorker::updateNowTime()
{
    now_time_ = std::chrono::steady_clock::now();
}

