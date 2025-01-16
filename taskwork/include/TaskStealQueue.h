#ifndef TASK_WORKER_INC_TASKSTEALQUEUE_H_
#define TASK_WORKER_INC_TASKSTEALQUEUE_H_

#include <memory>
#include <mutex>
#include <deque>

#include "TaskWrapper.h"


class TaskStealQueue
{
public:
    TaskStealQueue();

    ~TaskStealQueue();

    TaskStealQueue(const TaskStealQueue&) = delete;
    TaskStealQueue& operator=(const TaskStealQueue&) = delete;

    /**
     * push task into the queue
     * @param [in] data the task to push
     */
    void Push(TaskWrapper data);

    /**
     * the queue is empty or not
     * @return true if queue is empty, false otherwise
     */
    bool Empty() const;

    /**
     * try popping task out of queue, it will return immediately
     * if no task in the queue
     * param res reference to store the retrieved task
     * @return true if a task is retrieved successfully, false otherwise
     * @note: the task will be popped out from the front of the queue
     */
    bool TryPop(TaskWrapper& res);

    /**
     * try stealing tasks from other queue, it will return immediately
     * if no task in the queue
     * param res reference to store the retrieved task
     * @return true if a task is retrieved successfully, false otherwise
     * @note: the task will be popped out from the back of the queue
     */
    bool TrySteal(TaskWrapper& res);

private:
    /**
     * internal queue to store tasks
     */
    std::deque<TaskWrapper>     internal_queue_;

    /**
     * mutex to protect the queue
     */
    mutable std::mutex           queue_mutex_;
};


#endif  // TASK_WORKER_INC_TASKSTEALQUEUE_H_

