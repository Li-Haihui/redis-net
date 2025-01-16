#ifndef TASK_WORKER_INC_TASKWRAPPER_H_
#define TASK_WORKER_INC_TASKWRAPPER_H_

#include <memory>
#include <utility>

class TaskWrapper
{
public:
    template<typename F>
    TaskWrapper(F&& func) : task_type_(
                new TaskType<F>(std::move(func)))
    {}

    void operator()()
    {
        task_type_->Call();
    }

    TaskWrapper() = default;

    TaskWrapper(TaskWrapper&& other) :
        task_type_(std::move(other.task_type_))
    {}

    TaskWrapper& operator=(TaskWrapper&& other)
    {
        task_type_ = std::move(other.task_type_);
        return *this;
    }

    TaskWrapper(const TaskWrapper&) = delete;
    TaskWrapper& operator=(const TaskWrapper&) = delete;

private:
    struct TaskBase
    {
        TaskBase() {}
        virtual ~TaskBase() {}

        virtual void Call() = 0;
    };

    template<typename F>
    struct TaskType : TaskBase
    {
        F f;

        TaskType(F&& func) : f(std::move(func))
        {}

        virtual void Call()
        {
            f();
        }
    };

private:
    std::unique_ptr<TaskBase>     task_type_;
};


#endif
