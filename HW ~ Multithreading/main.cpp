#include "SimpleThreadPool.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>   
#include <stdexcept> 
#include <functional> 

// --- Example Usage Functions ---

int Multiply(int a, int b) {
    std::cout << "Task Multiply(" << a << ", " << b << ") started by thread " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    int result = a * b;
    std::cout << "Task Multiply(" << a << ", " << b << ") finished by thread " << std::this_thread::get_id() << " with result " << result << std::endl;
    return result;
}

void PrintMessage(const std::string& msg) {
     std::cout << "Task PrintMessage(\"" << msg << "\") started by thread " << std::this_thread::get_id() << std::endl;
     std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work
     std::cout << "Message from thread " << std::this_thread::get_id() << ": " << msg << std::endl;
}

void ThrowingTask() {
    std::cout << "Task ThrowingTask started by thread " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    throw std::runtime_error("Something went wrong in ThrowingTask!");
}


int main() {
    std::cout << "--- Creating Thread Pool ---" << std::endl;
    SimpleThreadPool pool(4);

    std::vector<std::future<int>> multiplyFutures;
    std::vector<std::future<void>> printFutures;
    std::future<void> throwingFuture;

    std::cout << "\n--- Posting Tasks ---" << std::endl;

    // Submit multiplication tasks
    for (int i = 0; i < 5; ++i) {
        multiplyFutures.push_back(pool.Post([i]() { return Multiply(i, i + 1); }));
    }
    multiplyFutures.push_back(pool.Post([](){ return Multiply(10, 5); }));

    // Submit print tasks
    printFutures.push_back(pool.Post([](){ PrintMessage("Hello from lambda!"); }));
    printFutures.push_back(pool.Post(std::bind(PrintMessage, "Hello from std::bind!")));
    printFutures.push_back(pool.Post([](){ PrintMessage("Another message."); }));

    // Submit the task that throws
    try {
        throwingFuture = pool.Post(ThrowingTask);
        std::cout << "Posted ThrowingTask." << std::endl;
    } catch (const std::runtime_error& e) {
        // This should only happen if the pool was stopped before Post could enqueue
        std::cerr << "Caught exception during Post: " << e.what() << std::endl;
    }

    std::cout << "\n--- Getting Results (Futures) ---" << std::endl;

    // Retrieve multiplication results
    for (size_t i = 0; i < multiplyFutures.size(); ++i) {
        try {
            int result = multiplyFutures[i].get(); // .get() blocks until result is available
            std::cout << "Main: Got result for multiplication task " << i << ": " << result << std::endl;
        } catch (const std::exception& e) {
             std::cerr << "Main: Caught exception getting multiplication result " << i << ": " << e.what() << std::endl;
        }
    }

     // Wait for print tasks to complete
    for (size_t i = 0; i < printFutures.size(); ++i) {
        try {
             printFutures[i].get(); // Wait for void tasks
             std::cout << "Main: Confirmed print task " << i << " completed." << std::endl;
        } catch (const std::exception& e) {
             std::cerr << "Main: Caught exception getting print result " << i << ": " << e.what() << std::endl;
        }
    }

    // Check the future from the throwing task
    if (throwingFuture.valid()) {
        try {
            throwingFuture.get(); // This call will re-throw the exception
            std::cout << "Main: ThrowingTask completed without error (UNEXPECTED)." << std::endl;
        } catch (const std::runtime_error& e) {
            std::cout << "Main: Caught expected exception from ThrowingTask via future: " << e.what() << std::endl;
        } catch (const std::exception& e) {
             std::cerr << "Main: Caught unexpected exception type from ThrowingTask: " << e.what() << std::endl;
        }
    } else {
         std::cout << "Main: ThrowingTask future was invalid (perhaps Post failed)." << std::endl;
    }

    std::cout << "\n--- Main Function Ending (Pool Destruction) ---" << std::endl;

    // pool.Destroy(); // Explicit call is possible, but RAII handles it.

    // Pool goes out of scope here. Its destructor calls Destroy(),
    // which signals threads to stop, waits for active tasks, and joins threads.

    return 0;
}