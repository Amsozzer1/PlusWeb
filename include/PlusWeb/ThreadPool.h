#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>
#include <future>
#include <memory>
#include <stdexcept>

class ThreadPool {
public:
    // Constructor: creates thread pool with specified number of threads
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    
    // Destructor: stops all threads and waits for them to finish
    ~ThreadPool();
    
    // Delete copy constructor and assignment operator (non-copyable)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // Submit a task to the thread pool - FULL DEFINITION IN HEADER
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        
        // Create packaged task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        // Add task to queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            
            // Don't allow enqueueing after stopping
            if (stop) {
                throw std::runtime_error("ThreadPool is stopping - cannot enqueue new tasks");
            }
            
            tasks.emplace([task]() { (*task)(); });
        }
        
        // Notify one thread
        condition.notify_one();
        
        return result;
    }
    
    // Get number of active threads
    size_t getThreadCount() const { return threads.size(); }
    
    // Get number of pending tasks
    size_t getPendingTasks() const;
    
    // Check if thread pool is stopping
    bool isStopping() const { return stop; }

private:
    // Worker threads
    std::vector<std::thread> threads;
    
    // Task queue
    std::queue<std::function<void()>> tasks;
    
    // Synchronization primitives
    mutable std::mutex queueMutex;  // mutable allows locking in const methods
    std::condition_variable condition;
    
    // Stop flag
    std::atomic<bool> stop{false};
    
    // Worker function that each thread runs
    void workerThread();
};