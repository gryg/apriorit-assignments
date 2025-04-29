#include "SimpleThreadPool.hpp"
#include <iostream> // For limited status messages (optional)

SimpleThreadPool::SimpleThreadPool(std::size_t threadCount) :
    m_threadCount(threadCount), stop(false)
{
    if (threadCount == 0) {
         std::cerr << "Warning: Creating SimpleThreadPool with 0 threads." << std::endl;
         // Or throw std::invalid_argument("Thread count must be positive.");
         return; // Or proceed, depending on desired behavior
    }
    threads.reserve(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        threads.emplace_back(&SimpleThreadPool::WorkOn, this);
    }
    // std::cout << "SimpleThreadPool created with " << m_threadCount << " threads." << std::endl;
}

SimpleThreadPool::~SimpleThreadPool() {
    // Ensure Destroy is called to cleanup threads properly upon destruction (RAII).
    if (!stop) { // Avoid redundant calls if Destroy was already explicitly called
        Destroy();
    }
    // std::cout << "SimpleThreadPool destroyed." << std::endl;
}

void SimpleThreadPool::Destroy() {
    // Use a simple lock and check mechanism for setting the stop flag.
    bool already_stopping = false;
    {
        std::unique_lock<std::mutex> lock(mut);
        if (stop) {
            already_stopping = true;
        } else {
            stop = true; // Signal threads to stop
        }
    } // Release lock before potentially long operations (notify, join)

    if (already_stopping) {
        // If already stopping, perhaps ensure threads are joined if the previous
        // Destroy call was interrupted, though join logic below handles this.
        // std::cout << "Destroy called on already stopped pool." << std::endl;
    } else {
        // std::cout << "Stopping thread pool..." << std::endl;
        // Notify all waiting threads to wake up and check the stop flag.
        condition.notify_all();
    }


    // Wait for all worker threads to finish execution.
    for (std::thread& worker : threads) {
        if (worker.joinable()) {
            // std::cout << "Joining thread " << worker.get_id() << std::endl;
            worker.join();
        }
    }
    threads.clear(); // Optional: clear the vector after joining
    // std::cout << "All threads joined." << std::endl;
}


void SimpleThreadPool::WorkOn() {
    // std::cout << "Worker thread " << std::this_thread::get_id() << " started." << std::endl;
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mut);

            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            // If stop is signaled and the queue is empty, the thread can exit.
            if (stop && tasks.empty()) {
                // std::cout << "Worker thread " << std::this_thread::get_id() << " stopping." << std::endl;
                return;
            }

            // Check if a task is available (could have been woken by stop, but tasks remain)
            if (!tasks.empty()) {
                task = std::move(tasks.front()); // Move task out of queue
                tasks.pop();
            } else {
                // Spurious wake or woken by stop signal but tasks remain, loop again
                continue;
            }

        } // Release lock before executing the task

        // Execute the task outside the lock to allow other threads to proceed.
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "Thread " << std::this_thread::get_id() << " caught exception: " << e.what() << std::endl;
            // Log and continue is a common strategy for thread pools.
        } catch (...) {
            std::cerr << "Thread " << std::this_thread::get_id() << " caught unknown exception." << std::endl;
        }
    }
}