#ifndef TASK_WORKER_INC_THREADPOOL_H_
#define TASK_WORKER_INC_THREADPOOL_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <future>
#include <functional>
#include <utility>
#include <vector>

#include "InterruptFlag.h"
#include "LockQueue.h"
#include "TaskStealQueue.h"
#include "TaskWrapper.h"
#include "ThreadsJoiner.h"

class ThreadPool
{
public:
    /**
     * @param n number of threads to create
     * @note: set default thread number using the value returned by
     * hardware_concurrency() API in c++11 standard library,
     * some platforms may not support this API then this API will
     * return 0, set 2 as the default value for this case
     */
    explicit ThreadPool(unsigned n = std::thread::hardware_concurrency());

    ~ThreadPool();

    /**
     * init thread pool, threads are started here
     * @return true if initialized successfully, false otherwise
     */
    bool Init();

    /**
     * interrupt thread pool
     */
    void Interrupt();

    /**
     * shut down thread pool
     */
    void Shutdown();

    /**
     * submit tasks to thread pool, a task could be function, callable object or
     * member function. If task submission are done in local thread of thread pool,
     * it will be put into local queue of the associated thread, otherwise, it will be
     * put into global thread pool queue
     */
    template<typename FunctionType, typename...Arguments>
    std::future<typename std::result_of<FunctionType(Arguments...)>::type>
    Submit(FunctionType&& f, Arguments&& ...args)
    {
        using result_type =
            typename std::result_of<FunctionType(Arguments...)>::type;

        std::packaged_task<result_type()> task(
                std::bind(std::forward<FunctionType>(f),
                          std::forward<Arguments>(args)...));

        std::future<result_type> res(task.get_future());

        {
            std::lock_guard<std::mutex> lg(size_mutex_);

            if (local_task_queue_) {
                local_task_queue_->Push(std::move(task));
            } else {
                pool_task_queue_.Push(std::move(task));
            }
        }

        notify();

        return res;
    }

private:
    /**
     * interrupt point where the thread pool considers could  be interrupted
     * safely
     */
    void interruptPoint();

    /**
     * waiting for tasks to run, the waiting still could be interrupted if
     * interrupt is triggerred.
     * @param lk mutex to synchronize between tasks submit
     * @param pool global thread pool task queue
     * @param steal local queues of threads in thread pool
     * @param cv waiting on this condition variable
     * and tasks pick up job in each thread
     */
    template<typename Lockable,
             typename PoolQueue,
             typename StealQueue>
    void interruptibleWait(Lockable& lk,
                           const PoolQueue& pool,
                           const StealQueue& steal,
                           std::condition_variable_any& cv)
    {
        if (local_interrupt_flag_) {
            local_interrupt_flag_->Wait(lk, pool, steal, cv);
        }
    }

    /**
     * notify task is coming to threads in thread pool
     */
    void notify();

    /**
     * take tasks from local queue
     * @param task to store tasks retrieved
     */
    bool popTaskFromLocalQueue(TaskWrapper& task);

    /**
     * take tasks from global thread pool queue
     * @param task to store tasks retrieved
     */
    bool popTaskFromPoolQueue(TaskWrapper& task);

    /**
     * steal tasks from other global queue
     * @param task to store tasks retrieved
     */
    bool popTaskFromOtherThreadQueue(TaskWrapper& task);

    /**
     * loop to pick up tasks of each thread in thread pool
     */
    void runPendingTask();

    /**
     * entry point for each thread in thread pool
     * @param index thead ID in thread pool
     */
    void workerThread(unsigned index);

private:
    using StealQueue  =
        std::vector<std::unique_ptr<TaskStealQueue>>;
    using ThreadConds =
        std::vector<std::unique_ptr<std::condition_variable_any>>;
    using ThreadFlags =
        std::vector<std::unique_ptr<InterruptFlag>>;

    /**
     * global thread pool queue
     */
    LockQueue<TaskWrapper>                   pool_task_queue_;

    /**
     * local thread queue
     */
    StealQueue                               steal_queues_;

    /**
     * condition variables of each thread, depending on which
     * we could interrupt threads in thread pool
     */
    ThreadConds                              thread_conds_;

    /**
     * interrupt flags for each thread in thread pool
     */
    ThreadFlags                              thread_flags_;

    /**
     * internal thread objects
     */
    std::vector<std::thread>                 threads_;

    /**
     * helper to wait for threads exit safely
     */
    ThreadsJoiner                            thread_joiner_;

    /**
     * thread numbers
     */
    unsigned                                 thread_nums_ = 0;

    /**
     * mutex to synchronize between task submit and task pick up jobs
     */
    mutable std::mutex                       size_mutex_;

    /**
     * pointing to local thread queue
     */
    static thread_local TaskStealQueue*      local_task_queue_;

    /**
     * pointing to local thread interrupt flag
     */
    static thread_local InterruptFlag*       local_interrupt_flag_;

    /**
     * pointing to local condition variable
     */
    static thread_local std::condition_variable_any*             local_cond_;

    /**
     * thread index
     */
    static thread_local unsigned             thread_index_;
};


#endif
