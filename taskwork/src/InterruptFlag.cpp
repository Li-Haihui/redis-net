#include "InterruptFlag.h"


InterruptFlag::InterruptFlag()
    : interrupt_flag_(false)
{
}

InterruptFlag::~InterruptFlag()
{
}

void InterruptFlag::Interrupt()
{
    interrupt_flag_.store(true, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lg(cond_mutex_);

    if (interrupt_cond_) {
        interrupt_cond_->notify_one();
    }
}

bool InterruptFlag::Interrupted() const
{
    return interrupt_flag_.load(std::memory_order_relaxed);
}

void InterruptFlag::interruptPoint()
{
    if (interrupt_flag_.load()) {
        throw InterruptException();
    }
}

bool InterruptFlag::NotifyTask()
{
    bool ret = false;

    std::lock_guard<std::mutex> lg(cond_mutex_);

    if (interrupt_cond_) {
        interrupt_cond_->notify_one();
        ret = true;
    }

    return ret;
}

