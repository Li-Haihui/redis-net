#ifndef TASK_WORKER_INC_TASKPOOL_H_
#define TASK_WORKER_INC_TASKPOOL_H_

#include <future>
#include <functional>
#include <queue>
#include <utility>

#include "InterruptFlag.h"
#include "TaskWrapper.h"

class TaskPool
{
public:
    TaskPool();
    ~TaskPool();

    TaskPool(const TaskPool&) = delete;
    TaskPool& operator=(const TaskPool&) = delete;

    /**
     * submit new created tasks to task pool for handle
     */
    template<typename FunctionType, typename...Arguments>
    std::future<typename std::result_of<FunctionType(Arguments...)>::type>
    Submit(bool emergent, FunctionType&& f, Arguments&& ...args)
    {
        using result_type =
            typename std::result_of<FunctionType(Arguments...)>::type;

        std::packaged_task<result_type()> task(
                std::bind(std::forward<FunctionType>(f),
                          std::forward<Arguments>(args)...));

        std::future<result_type> res(task.get_future());

        std::lock_guard<std::mutex> lg(task_queue_mutex_);

        if (emergent) {
            task_queue_.push_front(std::move(task));
        } else {
            task_queue_.push_back(std::move(task));
        }

        notify();

        return res;
    }

    /**
     * init task pool, create a thread to dispatch tasks
     * @return true if thread is created successfully, false otherwise
     */
    bool Init();

    /**
     * interrupt task pool
     */
    void Interrupt();

    /**
     * shut down task pool
     */
    void Shutdown();

private:
    /**
     * interrupt point where it's safe to stop task pool
     */
    void interruptPoint();

    /**
     * waiting for new tasks, waiting could be stopped if
     * interruption has been triggerred
     */
    template<typename Lockable, typename Queue>
    void interruptibleWait(Lockable& lk,
                           const Queue& queue,
                           std::condition_variable_any& cv)
    {
        this_thread_interrupt_flag_.Wait(lk, queue, cv);
    }

    /**
     * notify thread that task is coming
     */
    void notify();

    /**
     * pick up tasks submitted to task pool to run
     */
    void pickUpTasks(std::unique_lock<std::mutex>& ul);

    /**
     * enrty point for internal thread to run tasks
     */
    void runPendingTask();

private:
    /**
     * point to interrupt flag
     */
    InterruptFlag*                     interrupt_flag_ = nullptr;

    /**
     * queue to store tasks
     */
    std::deque<TaskWrapper>            task_queue_;

    /**
     * mutex to protect queue
     */
    std::mutex                          task_queue_mutex_;

    /**
     * internal thread will use this to wait for new coming tasks
     */
    std::condition_variable_any         task_queue_cond_;

    /**
     * internal thread to run tasks dispatch job
     */
    std::thread                         thread_;

    /**
     * thread local interrupt flag
     */
    static thread_local InterruptFlag  this_thread_interrupt_flag_;
};


#endif  // TASK_WORKER_INC_TASKPOOL_H_
