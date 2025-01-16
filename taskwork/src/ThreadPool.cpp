#include "ThreadPool.h"

#include <iostream>

thread_local TaskStealQueue* ThreadPool::local_task_queue_ = nullptr;
thread_local InterruptFlag* ThreadPool::local_interrupt_flag_ = nullptr;
thread_local std::condition_variable_any* ThreadPool::local_cond_ = nullptr;
thread_local unsigned ThreadPool::thread_index_;

ThreadPool::ThreadPool(unsigned n)
    : thread_joiner_(threads_),
      thread_nums_(n)
{
    if (thread_nums_ == 0) {
        thread_nums_ = 2;
    }
}

ThreadPool::~ThreadPool()
{
}

bool ThreadPool::Init()
{
    try {
        for (unsigned i = 0; i < thread_nums_; ++i) {
            steal_queues_.push_back(
                    std::unique_ptr<TaskStealQueue>(
                            new TaskStealQueue));

            thread_flags_.push_back(
                    std::unique_ptr<InterruptFlag>(
                            new InterruptFlag));

            thread_conds_.push_back(
                    std::unique_ptr<std::condition_variable_any>(
                            new std::condition_variable_any));
        }

        for (unsigned i = 0; i < thread_nums_; ++i) {
            threads_.push_back(
                    std::thread(std::bind(&ThreadPool::workerThread,
                                          this,
                                          i)));
        }

        return true;
    } catch (...) {
        // std::cout << "exception caught" << std::endl;
        Interrupt();
    }

    return false;
}

void ThreadPool::Interrupt()
{
    for (decltype(thread_flags_.size()) i = 0; i < thread_flags_.size(); ++i) {
        if (thread_flags_[i]) {
            thread_flags_[i]->Interrupt();
        }
    }
}

void ThreadPool::Shutdown()
{
    Interrupt();
}

void ThreadPool::runPendingTask()
{
    TaskWrapper task;

    if (popTaskFromLocalQueue(task) ||
        popTaskFromPoolQueue(task) ||
        popTaskFromOtherThreadQueue(task)) {
        interruptPoint();
        task();
    } else {
        if (local_cond_) {
            // std::cout << std::this_thread::get_id()
            //                << "waiting..."
            //                << std::endl;
            interruptibleWait(size_mutex_,
                              pool_task_queue_,
                              steal_queues_,
                              *local_cond_);
            // std::cout << std::this_thread::get_id()
            //                << "wake up"
            //                << std::endl;
        } else {
            std::this_thread::yield();
        }
    }
}

void ThreadPool::interruptPoint()
{
    if (local_interrupt_flag_ && local_interrupt_flag_->Interrupted()) {
        throw InterruptException();
    }
}

void ThreadPool::notify()
{
    for (decltype(thread_flags_.size()) i = 0;
         i < thread_flags_.size(); ++i) {
        if (thread_flags_[i] &&
            thread_flags_[i]->NotifyTask()) {
            break;
        }
    }
}

void ThreadPool::workerThread(unsigned index)
{
    thread_index_ = index;
    local_task_queue_ = steal_queues_[thread_index_].get();
    local_interrupt_flag_ = thread_flags_[thread_index_].get();
    local_cond_ = thread_conds_[thread_index_].get();

    try {
        while (true) {
            interruptPoint();
            runPendingTask();
        }
    } catch (const std::exception& e) {
        // std::cout << e.what() << std::endl;

        return;
    }
}

bool ThreadPool::popTaskFromLocalQueue(TaskWrapper& task)
{
    return local_task_queue_ && local_task_queue_->TryPop(task);
}

bool ThreadPool::popTaskFromPoolQueue(TaskWrapper& task)
{
    return pool_task_queue_.TryPop(task);
}

bool ThreadPool::popTaskFromOtherThreadQueue(TaskWrapper& task)
{
    for (decltype(steal_queues_.size()) i = 0; i < steal_queues_.size(); ++i) {
        auto const index = (thread_index_ + i + 1) % steal_queues_.size();

        if (steal_queues_[index] && steal_queues_[index]->TrySteal(task)) {
            return true;
        }
    }

    return false;
}
