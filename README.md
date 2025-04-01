[![CodeFactor](https://www.codefactor.io/repository/github/conradhuebler/cxxthreadpool/badge)](https://www.codefactor.io/repository/github/conradhuebler/cxxthreadpool)
# CxxThreadPool


A modern, header-only Thread Pool library for C++11 and newer. Inspired by the Qt ThreadPool class, CxxThreadPool provides an easy-to-use framework for parallel execution with built-in progress tracking.
## Features

-   Header-only implementation - just include and use 
-   Thread-safe execution of parallel tasks
-   Customizable number of concurrent threads
-   Built-in progress visualization
-   Automatic resource management
-   Support for thread reorganization strategies
-   Simple but powerful API

## Usage

Include the CxxThreadPool.hpp in your project

```cpp
#include "include/CxxThreadPool.hpp"
```
and link against pthread (on Unix-like systems).

```sh
g++ -std=c++11 -o binfile main.cpp -pthread
```
Basic Usage

1. Create your task by subclassing CxxThread:

```cpp
class MyTask : public CxxThread {
public:
    int execute() override {
        // Your task implementation here
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 0; // Return value can be accessed later
    }
};
```
2. Set up and use the thread pool:

```cpp
// Create and configure the pool
CxxThreadPool pool;
pool.setActiveThreadCount(std::thread::hardware_concurrency());

// Add tasks
for (int i = 0; i < 100; i++) {
    pool.addThread(new MyTask());
}

// Execute all tasks and wait for completion
pool.StartAndWait();
```
3.  Access results:

```cpp
// Iterate through completed tasks
for (auto* thread : pool.getFinishedThreads()) {
    // Cast to your specific task type if needed
    auto* task = static_cast<MyTask*>(thread);
    // Process results
    int returnValue = task->getReturnValue();
}
```

## Advanced Usage
### Task Customization

Each task can be customized with additional parameters:

```cpp
class ParameterizedTask : public CxxThread {
public:
    ParameterizedTask(int input) : m_input(input) {}
    
    int execute() override {
        m_result = m_input * m_input;
        return 0;
    }
    
    int getResult() const { return m_result; }
    
private:
    int m_input;
    int m_result = 0;
};
```
### Memory Management

By default, CxxThreadPool takes ownership of the threads and deletes them when the pool is destroyed. To prevent this:
```cpp
MyTask* task = new MyTask();
task->setAutoDelete(false);  // Must manually delete later
pool.addThread(task);

// Later:
delete task;
```
### Pool Reorganization

For better performance with many small tasks:
```cpp
// Dynamically reorganize threads into balanced blocks
pool.DynamicPool(2);

// Or use static reorganization
pool.StaticPool();
```
### Controlling Thread Execution

Break the pool's execution early:
```cpp
class StopConditionThread : public CxxThread {
public:
    int execute() override {
        if (shouldStop()) {
            m_break_pool = true;  // Signals pool to stop
        }
        return 0;
    }
};
```
Disable specific threads:
```cpp
MyTask* task = new MyTask();
task->setEnabled(false);  // This thread won't execute
pool.addThread(task);
```
### Reusing the Pool

Reset the pool to reuse threads:
```cpp
pool.StartAndWait();
// Process results
pool.Reset();  // Move finished threads back to queue
pool.StartAndWait();  // Run them again
```
## Customization

Configure CxxThreadPool's behavior by defining these before including the header:
```cpp
// Enable verbose logging
#define _CxxThreadPool_Verbose

// Disable progress bar
#define _CxxThreadPool_NoBar

// Set progress bar width (default = 100)
#define _CxxThreadPool_BarWidth 80

// Set thread status check interval in milliseconds (default = 100)
#define _CxxThreadPool_TimeOut 50
```
    
You can also control the progress bar type at runtime:
```cpp
pool.setProgressBar(CxxThreadPool::ProgressBarType::Discrete);  // Updates at 10% intervals
pool.setProgressBar(CxxThreadPool::ProgressBarType::Continously);  // Updates continuously
pool.setProgressBar(CxxThreadPool::ProgressBarType::None);  // No progress display
```

Or via environment variable:
```sh
export CxxThreadBar=2  # 0=None, 1=Discrete, 2=Continuous
```

## Performance Tips

-   Set appropriate thread count: Usually std::thread::hardware_concurrency() or slightly higher
-   Balance task granularity: Too many tiny tasks create overhead; too few large tasks limit parallelism
-   Use reorganization for small tasks: DynamicPool() or StaticPool() can improve performance
-   Minimize shared state: Avoid locks and shared data between threads when possible
-   Consider wake-up interval: Adjust with setWakeUpInterval() based on task duration

## Requirements

-   C++11 or newer compiler
-   pthread library (on Unix/Linux)

## License

MIT License - See LICENSE file for details.
## Contributing

Contributions are welcome! Feel free to submit issues or pull requests on GitHub.
