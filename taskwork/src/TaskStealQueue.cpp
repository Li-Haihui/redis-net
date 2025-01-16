#include "TaskStealQueue.h"


TaskStealQueue::TaskStealQueue()
{
}

TaskStealQueue::~TaskStealQueue()
{
}

void TaskStealQueue::Push(TaskWrapper data)
{
    std::lock_guard<std::mutex> lg(queue_mutex_);

    internal_queue_.push_front(std::move(data));
}

bool TaskStealQueue::Empty() const
{
    std::lock_guard<std::mutex> lg(queue_mutex_);

    return internal_queue_.empty();
}

bool TaskStealQueue::TryPop(TaskWrapper& res)
{
    std::lock_guard<std::mutex> lg(queue_mutex_);

    if (internal_queue_.empty()) {
        return false;
    }

    res = std::move(internal_queue_.front());
    internal_queue_.pop_front();

    return true;
}

bool TaskStealQueue::TrySteal(TaskWrapper& res)
{
    std::lock_guard<std::mutex> lg(queue_mutex_);

    if (internal_queue_.empty()) {
        return false;
    }

    res = std::move(internal_queue_.back());
    internal_queue_.pop_back();

    return true;
}

