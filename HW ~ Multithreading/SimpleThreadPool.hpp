#ifndef SIMPLE_THREAD_POOL_HPP
#define SIMPLE_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>
#include <memory>
#include <utility>
#include <cstddef>   

class SimpleThreadPool {
public:
    explicit SimpleThreadPool(std::size_t threadCount);
    ~SimpleThreadPool();

    // Non-copyable and non-movable
    SimpleThreadPool(const SimpleThreadPool&) = delete;
    SimpleThreadPool& operator=(const SimpleThreadPool&) = delete;
    SimpleThreadPool(SimpleThreadPool&&) = delete;
    SimpleThreadPool& operator=(SimpleThreadPool&&) = delete;

    /**
     * @brief Submits a task for execution by a worker thread.
     * @tparam Fnc_T The type of the callable task.
     * @param task The callable task (function, lambda, functor).
     * @return std::future<ReturnType> A future associated with the task's result.
     * @throws std::runtime_error if called after the pool has been stopped.
     */
    template<typename Fnc_T>
    auto Post(Fnc_T task) -> std::future<decltype(task())> {
        using ReturnType = decltype(task());

        // Wrap the task in a packaged_task to manage its future result.
        // Use shared_ptr for safe lifecycle management when captured by lambda.
        auto packagedTask = std::make_shared<std::packaged_task<ReturnType()>>(std::move(task));

        std::future<ReturnType> future = packagedTask->get_future();

        {
            std::unique_lock<std::mutex> lock(mut);

            // Prevent enqueueing tasks after the pool has been signaled to stop.
            if (stop) {
                throw std::runtime_error("Post on stopped SimpleThreadPool");
            }

            // Enqueue a lambda that executes the packaged_task.
            tasks.emplace([packagedTask]() { (*packagedTask)(); });
        } // Mutex lock released here

        // Notify one waiting worker thread that a new task is available.
        condition.notify_one();

        return future;
    }


    void Destroy();

private:
    void WorkOn();

    size_t m_threadCount;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;

    std::mutex mut; // Mutex to protect access to tasks queue and stop flag
    std::condition_variable condition; // Condition variable to signal threads
    bool stop; // Flag to signal threads to stop execution
};

#endif 