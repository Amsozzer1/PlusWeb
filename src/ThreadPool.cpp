#include "../include/PlusWeb/ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) {
    // Ensure we have at least 1 thread, max 16 for safety
    numThreads = std::max(size_t(1), std::min(numThreads, size_t(16)));
    
    // Create worker threads
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back(&ThreadPool::workerThread, this);
    }
    
    std::cout << "ThreadPool initialized with " << numThreads << " threads" << std::endl;
}

ThreadPool::~ThreadPool() {
    // Signal all threads to stop
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    // Wake up all threads
    condition.notify_all();
    
    // Wait for all threads to finish
    for (std::thread& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "ThreadPool shutdown complete" << std::endl;
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        // Wait for a task or stop signal
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            // Wait until there's a task or we need to stop
            condition.wait(lock, [this] { 
                return stop || !tasks.empty(); 
            });
            
            // If stopping and no more tasks, exit
            if (stop && tasks.empty()) {
                return;
            }
            
            // Get next task
            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
            }
        }
        
        // Execute task outside of lock
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "ThreadPool task exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ThreadPool task unknown exception" << std::endl;
            }
        }
    }
}

size_t ThreadPool::getPendingTasks() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}