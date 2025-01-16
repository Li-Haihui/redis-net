#ifndef TASK_WORKER_INC_THREADSJOINER_H_
#define TASK_WORKER_INC_THREADSJOINER_H_

#include <thread>
#include <vector>

class ThreadsJoiner
{
public:
    explicit ThreadsJoiner(std::vector<std::thread>& threads)
        : threads_(threads)
    {}

    ~ThreadsJoiner()
    {
        for (auto& item : threads_) {
            item.join();
        }
    }

    ThreadsJoiner(const ThreadsJoiner&) = delete;
    ThreadsJoiner& operator=(const ThreadsJoiner&) = delete;

private:
    /**
     * containing reference of threads in thread pool
     */
    std::vector<std::thread>&  threads_;
};



#endif
