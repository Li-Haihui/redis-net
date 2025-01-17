#include "TaskPool.h"

#include <iostream>

thread_local InterruptFlag TaskPool::this_thread_interrupt_flag_;

TaskPool::TaskPool()
{
}

TaskPool::~TaskPool()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TaskPool::notify()
{
    if (interrupt_flag_) {
        interrupt_flag_->NotifyTask();
    }
}

void TaskPool::pickUpTasks(std::unique_lock<std::mutex>& ul)
{
    ul.lock();

    while (!task_queue_.empty()) {
        interruptPoint();

        TaskWrapper task = std::move(task_queue_.front());
        task_queue_.pop_front();

        ul.unlock();

        task();

        ul.lock();
    }
}

void TaskPool::runPendingTask()
{
    try {
        while (true) {
            interruptPoint();

            std::unique_lock<std::mutex> ul(task_queue_mutex_,
                                            std::defer_lock);

            pickUpTasks(ul);

            interruptibleWait(ul,
                              task_queue_,
                              task_queue_cond_);
        }
    } catch (const std::exception& e) {
        // std::cout << e.what() << std::endl;

        return;
    }
}

bool TaskPool::Init()
{
    try {
        std::promise<InterruptFlag*> p;
        auto callable = std::bind(&TaskPool::runPendingTask, this);
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

void TaskPool::Interrupt()
{
    if (interrupt_flag_) {
        interrupt_flag_->Interrupt();
    }
}

void TaskPool::Shutdown()
{
    Interrupt();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TaskPool::interruptPoint()
{
    if (this_thread_interrupt_flag_.Interrupted()) {
        throw InterruptException();
    }
}

