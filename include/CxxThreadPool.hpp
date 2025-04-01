/**
 * @file CxxThreadPool.hpp
 * @brief Modern C++ Thread Pool and Thread Management Library
 * @details Provides helper classes for C++ thread management and thread pools,
 *          inspired by QThreadPool and QRunners
 *
 * @author Conrad Hübler <Conrad.Huebler@gmx.net>
 * @author AI assisted using Claude 3.7 Sonnet
 * @copyright 2020-2025 Conrad Hübler
 * @license MIT License
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

/**
 * @brief Basic thread class that can be executed in the thread pool
 */
class CxxThread {
public:
    CxxThread() = default;
    virtual ~CxxThread() = default;

    /**
     * @brief Starts thread execution
     * @details Times the execution and marks thread as finished when done
     */
    void start()
    {
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThread::start() - Thread " << m_increment_id << " is up and running." << std::endl;
#endif
        m_start = std::chrono::steady_clock::now();
        m_return = execute();
        m_running = false;
        m_finished = true;
        m_end = std::chrono::steady_clock::now();
        m_time = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count();
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThread::start() - Thread " << m_increment_id << " finished after " << m_time << " mseconds." << std::endl;
#endif
    }

    /**
     * @brief Check if thread is currently running
     * @return True if thread is running
     */
    bool isRunning() const
    {
        return m_running;
    }

    /**
     * @brief Check if thread has finished
     * @return True if thread has finished
     */
    bool isFinished() const
    {
        return m_finished;
    }

    /**
     * @brief Pure virtual function that needs to be implemented by derived classes
     * @return Return code
     */
    virtual int execute() = 0;

    /**
     * @brief Resets the finished state
     */
    void reset() { m_finished = false; }

    /**
     * @brief Get autoDelete setting
     * @return True if thread should be auto-deleted after execution
     */
    bool isAutoDelete() const { return m_autodelete; }

    /**
     * @brief Configure auto-deletion behavior
     * @param autodelete Whether thread should be deleted after execution
     */
    void setAutoDelete(bool autodelete) { m_autodelete = autodelete; }

    /**
     * @brief Set the incremental ID for this thread
     * @param id Unique identifier for this thread
     */
    void setIncrementId(int id) { m_increment_id = id; }

    /**
     * @brief Get execution time
     * @return Execution time in milliseconds
     */
    int getExecutionTime() const { return m_time; }

    /**
     * @brief Checks if the thread pool should be interrupted
     * @return True if thread pool should stop
     */
    virtual bool shouldBreakThreadPool() const { return m_break_pool; }

    /**
     * @brief Enable or disable the thread
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Check if thread is enabled
     * @return True if thread is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set thread ID
     * @param id Thread ID
     */
    void setThreadId(int id) { m_thread_id = id; }

    /**
     * @brief Get thread ID
     * @return Thread ID
     */
    int getThreadId() const { return m_thread_id; }

    /**
     * @brief Get execution return value
     * @return Return code from execute()
     */
    int getReturnValue() const { return m_return; }

private:
    std::atomic<bool> m_running{ true };
    std::atomic<bool> m_finished{ false };
    std::atomic<bool> m_enabled{ true };
    bool m_autodelete{ true };
    int m_return{ 0 };
    std::chrono::time_point<std::chrono::steady_clock> m_start, m_end;
    int m_increment_id{ 0 };
    int m_time{ 0 };

protected:
    int m_thread_id{ 0 };
    bool m_break_pool{ false };
};

/**
 * @brief Thread container that executes multiple threads in sequence
 */
class CxxBlockedThread : public CxxThread {
public:
    CxxBlockedThread() = default;
    ~CxxBlockedThread() = default;

    /**
     * @brief Executes all contained threads in sequence
     * @return 0 on success, or error code if a thread breaks the pool
     */
    int execute() override
    {
        for (auto* thread : m_threads) {
            if (thread->isEnabled()) {
                thread->execute();
                if (thread->shouldBreakThreadPool())
                    return 0;
            }
        }
        return 0;
    }

    /**
     * @brief Add a thread to the block
     * @param thread Thread to execute in this block
     */
    void addThread(CxxThread* thread) { m_threads.push_back(thread); }

    /**
     * @brief Get contained threads
     * @return Reference to vector of threads
     */
    std::vector<CxxThread*>& getThreads() { return m_threads; }

private:
    std::vector<CxxThread*> m_threads;
};

/**
 * @brief Thread pool manager that handles execution of multiple threads
 */
class CxxThreadPool {
public:
    /**
     * @brief Progress bar display options
     */
    enum class ProgressBarType {
        None = 0, ///< No progress bar
        Discrete = 1, ///< Display progress at discrete intervals
        Continously = 2 ///< Continuously updated progress bar
    };

    /**
     * @brief Constructor initializes the thread pool
     */
    CxxThreadPool()
    {
        // Check for environment configuration
        const char* val = std::getenv("CxxThreadBar");
        if (val != nullptr) {
            int barType = std::atoi(val);
            if (barType >= 0 && barType <= 2) {
                m_evn_overwrite_bar = true;
                m_progresstype = static_cast<ProgressBarType>(barType);
            }
        }

#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - Setting up thread pool for usage" << std::endl;
        std::cout << "CxxThreadPool::CxxThreadPool() - Wake up every " << m_wake_up << " msecs." << std::endl;
#endif

        // Initialize OpenMP thread settings if available
#if defined(_OPENMP)
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - openMP is enabled, getting OMP_NUM_THREADS" << std::endl;
#endif
        val = std::getenv("OMP_NUM_THREADS");
        if (val != nullptr) {
            m_omp_env_thread = std::max(std::atoi(val), 1);
        } else {
            m_omp_env_thread = 1;
        }

#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - OMP_NUM_THREADS was " << m_omp_env_thread << std::endl;
#endif
        omp_set_num_threads(1);
#endif

        // Set thread count to available processors or OMP setting
        setActiveThreadCount(m_omp_env_thread > 0 ? m_omp_env_thread : std::thread::hardware_concurrency());
    }

    /**
     * @brief Destructor cleans up all threads
     */
    virtual ~CxxThreadPool()
    {
        // Clean up queued threads
        while (!m_pool.empty()) {
            auto thread = m_pool.front();
            if (thread->isAutoDelete()) {
                delete thread;
            }
            m_pool.pop();
        }

        // Clean up active threads
        for (auto* thread : m_active) {
            if (thread->isAutoDelete()) {
                delete thread;
            }
        }

        // Clean up finished threads
        for (auto* thread : m_finished) {
            if (thread->isAutoDelete()) {
                delete thread;
            }
        }

        // Restore original OpenMP thread settings
#if defined(_OPENMP)
        omp_set_num_threads(m_omp_env_thread);
#endif
    }

    /**
     * @brief Set the progress bar type
     * @param type Progress bar display mode
     */
    void setProgressBar(ProgressBarType type)
    {
        if (!m_evn_overwrite_bar) {
            m_progresstype = type;
        }
    }

    /**
     * @brief Set number of active threads
     * @param threadCount Maximum number of concurrent threads
     */
    void setActiveThreadCount(int threadCount)
    {
        m_max_thread_count = std::max(1, threadCount);
    }

    /**
     * @brief Add a thread to the pool
     * @param thread Thread to be added to the execution queue
     */
    void addThread(CxxThread* thread)
    {
        m_pool.push(thread);
        m_threads_map.insert(std::make_pair(m_pool.size() - 1, thread));
    }

    /**
     * @brief Add multiple threads to the pool
     * @param threads Vector of threads to be added
     */
    void addThreads(const std::vector<CxxThread*>& threads)
    {
        for (auto* thread : threads) {
            addThread(thread);
        }
    }

    /**
     * @brief Start execution of all threads and wait until completion
     */
    void StartAndWait()
    {
        m_start = std::chrono::steady_clock::now();

        // Choose execution strategy
        if (m_max_thread_count == 1) {
            ParallelLoop();
        } else {
            ParallelLoop();
        }

        // Reorganize blocked threads if needed
        if (m_reorganised) {
            std::vector<CxxThread*> finished;
            for (auto* thread : m_finished) {
                auto* blockedThread = static_cast<CxxBlockedThread*>(thread);
                auto& nestedThreads = blockedThread->getThreads();
                finished.insert(finished.end(), nestedThreads.begin(), nestedThreads.end());
                delete blockedThread;
            }
            m_finished = std::move(finished);
        }

        m_end = std::chrono::steady_clock::now();

#ifdef _CxxThreadPool_Verbose
        std::cout << std::endl;
        std::cout << "CxxThreadPool::StartandWait() - Threads finished after "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count()
                  << " mseconds." << std::endl;
#endif
    }

    /**
     * @brief Dynamically reorganize threads into blocks for better performance
     * @param divide Division factor for block size calculation
     */
    void DynamicPool(int divide = 2)
    {
        m_reorganised = false;

        // Check if reorganization is beneficial
        if (m_pool.size() / 2 / m_max_thread_count == 0)
            return;

        m_reorganised = true;
        std::vector<CxxThread*> threads;

        while (!m_pool.empty()) {
            int block_size = m_pool.size() / divide;
            int thread_count = block_size / m_max_thread_count;

            if (thread_count > 0) {
                for (int j = 0; j < m_max_thread_count; ++j) {
                    auto* blockThread = new CxxBlockedThread;
                    for (int i = 0; i < thread_count; ++i) {
                        if (m_pool.empty()) {
                            addThreads(threads);
                            return;
                        }
                        blockThread->addThread(m_pool.front());
                        m_pool.pop();
                    }
                    threads.push_back(blockThread);
                }
            } else {
                auto* blockThread = new CxxBlockedThread;
                blockThread->addThread(m_pool.front());
                m_pool.pop();
                threads.push_back(blockThread);
            }
        }
        addThreads(threads);
    }

    /**
     * @brief Reorganize threads into static blocks
     */
    void StaticPool()
    {
        m_reorganised = false;

        // Check if reorganization is beneficial
        if (m_pool.size() / 2 / m_max_thread_count == 0)
            return;

        m_reorganised = true;
        std::vector<CxxThread*> threads;

        while (!m_pool.empty()) {
            int block_size = m_pool.size();
            int thread_count = block_size / m_max_thread_count;

            if (thread_count > 0) {
                for (int j = 0; j < m_max_thread_count; ++j) {
                    auto* blockThread = new CxxBlockedThread;
                    for (int i = 0; i < thread_count; ++i) {
                        if (m_pool.empty()) {
                            addThreads(threads);
                            return;
                        }
                        blockThread->addThread(m_pool.front());
                        m_pool.pop();
                    }
                    threads.push_back(blockThread);
                }
            } else {
                auto* blockThread = new CxxBlockedThread;
                blockThread->addThread(m_pool.front());
                m_pool.pop();
                threads.push_back(blockThread);
            }
        }
        addThreads(threads);
    }

    /**
     * @brief Get list of finished threads
     * @return Vector of finished threads
     */
    std::vector<CxxThread*>& getFinishedThreads() { return m_finished; }

    /**
     * @brief Get list of active threads
     * @return Vector of active threads
     */
    std::vector<CxxThread*>& getActiveThreads() { return m_active; }

    /**
     * @brief Get queue of threads waiting for execution
     * @return Queue of waiting threads
     */
    std::queue<CxxThread*>& getThreadQueue() { return m_pool; }

    /**
     * @brief Get ordered list of threads
     * @return Map of threads indexed by insertion order
     */
    std::map<int, CxxThread*>& getOrderedThreadList() { return m_threads_map; }

    /**
     * @brief Reset the pool by moving finished threads back to queue
     */
    void Reset()
    {
        for (auto* thread : m_finished) {
            thread->reset();
            m_pool.push(thread);
        }
        m_finished.clear();
    }

    /**
     * @brief Clear all threads from the pool
     */
    void clear()
    {
        while (!m_pool.empty()) {
            auto* thread = m_pool.front();
            if (thread->isAutoDelete()) {
                delete thread;
            }
            m_pool.pop();
        }

        for (auto* thread : m_active) {
            if (thread->isAutoDelete()) {
                delete thread;
            }
        }
        m_active.clear();

        for (auto* thread : m_finished) {
            if (thread->isAutoDelete()) {
                delete thread;
            }
        }
        m_finished.clear();
    }

    /**
     * @brief Get the wake-up interval
     * @return Wake-up interval in milliseconds
     */
    int getWakeUpInterval() const { return m_wake_up; }

    /**
     * @brief Set the wake-up interval
     * @param wakeup Wake-up interval in milliseconds
     */
    void setWakeUpInterval(int wakeup) { m_wake_up = wakeup; }

    /**
     * @brief Set progress bar width
     * @param width Width in characters
     */
    void setProgressBarWidth(int width) { m_bar_width = width; }

private:
    /**
     * @brief Start next thread from the queue
     * @return True if more threads are available
     */
    bool startNextThread()
    {
        if (m_pool.empty())
            return false;

        auto* thread = m_pool.front();
        if (!thread)
            return false;

        if (!thread->isEnabled()) {
            m_finished.push_back(thread);
            m_pool.pop();
            return !m_pool.empty();
        }

        thread->setIncrementId(m_increment_id++);

        auto* threadHandle = new std::thread(&CxxThread::start, thread);
        m_running_threads.push_back(std::make_pair(threadHandle, thread));
        m_active.push_back(thread);
        m_pool.pop();

        return !m_pool.empty();
    }

    /**
     * @brief Execute and manage threads in parallel
     */
    void ParallelLoop()
    {
        m_max = m_pool.size();
        bool continueExecution = true;

        while (((!m_pool.empty() || !m_active.empty()) && continueExecution)) {
            // Start new threads if slots are available
            if (!m_pool.empty()) {
                while (m_active.size() < m_max_thread_count) {
                    if (!startNextThread())
                        break;
                    updateStatus();
                }
            }

            // Check for completed threads
            for (size_t i = 0; i < m_active.size(); /* no increment */) {
                if (!m_active[i]->isFinished()) {
                    ++i;
                    continue;
                }

                // Handle finished thread
                for (size_t j = 0; j < m_running_threads.size(); ++j) {
                    if (m_running_threads[j].second == m_active[i] && m_running_threads[j].first->joinable()) {
                        m_running_threads[j].first->join();
                        delete m_running_threads[j].first;
                        m_running_threads.erase(m_running_threads.begin() + j);
                        break;
                    }
                }

                m_finished.push_back(m_active[i]);

                // Adjust wake-up interval based on thread execution time
                int executionTime = m_active[i]->getExecutionTime();
                m_wake_up = std::min(m_wake_up, executionTime);

                // Check if thread pool should be interrupted
                if (m_active[i]->shouldBreakThreadPool()) {
                    continueExecution = false;
                }

                // Remove from active list
                m_active.erase(m_active.begin() + i);
                updateStatus();
                // Note: no increment since we removed the element
            }

            // Sleep before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(m_wake_up));
        }
    }

    /**
     * @brief Update and display status information
     */
    void updateStatus() const
    {
#ifdef _CxxThreadPool_Verbose
        std::printf(("\n\nCxxThreadPool::Status() - Running %zu, Waiting %zu, Finished %zu\n\n"),
            m_active.size(), m_pool.size(), m_finished.size());
#endif
#ifndef _CxxThreadPool_Verbose
        updateProgressDisplay();
#endif
    }

    /**
     * @brief Update progress display based on current settings
     */
    void updateProgressDisplay() const
    {
        switch (m_progresstype) {
        case ProgressBarType::Discrete:
            updateDiscreteProgress();
            break;

        case ProgressBarType::Continously:
            updateContinuousProgress();
            break;

        case ProgressBarType::None:
        default:
            break;
        }
    }

    /**
     * @brief Update discrete progress display
     */
    void updateDiscreteProgress() const
    {
        size_t finished = m_finished.size();
        double progressPercentage = static_cast<double>(finished) / m_max;

        if (progressPercentage < 1e-5 && m_active.size() < m_max_thread_count && m_pool.size() > m_max_thread_count)
            return;

        if (progressPercentage * 10 >= m_small_progress) {
            size_t active = m_active.size();
            size_t totalActive = finished + active;
            double activePercentage = static_cast<double>(totalActive) / m_max;

            std::cerr << "[";
            int barFinished = static_cast<int>(m_bar_width * progressPercentage);
            int barActive = static_cast<int>(m_bar_width * activePercentage);

            for (int i = 1; i < m_bar_width; ++i) {
                if (i <= barFinished)
                    std::cerr << "=";
                else if (i <= barActive && i > barFinished)
                    std::cerr << "-";
                else if (i >= barActive && i > barFinished)
                    std::cerr << " ";
            }

            int progressPct = static_cast<int>(progressPercentage * 100.0);
            if (progressPct == 0)
                std::cerr << "]   " << progressPct << " % finished jobs" << std::endl;
            else if (progressPct == 100)
                std::cerr << "] " << progressPct << " % finished jobs ("
                          << std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::steady_clock::now() - m_last)
                                 .count()
                          << " secs)" << std::endl;
            else
                std::cerr << "]  " << progressPct << " % finished jobs ("
                          << std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::steady_clock::now() - m_last)
                                 .count()
                          << " secs)" << std::endl;

            m_last = std::chrono::steady_clock::now();
            m_small_progress += 1;
        }
    }

    /**
     * @brief Update continuous progress display
     */
    void updateContinuousProgress() const
    {
        size_t finished = m_finished.size();
        double progressPercentage = static_cast<double>(finished) / m_max;
        size_t active = m_active.size();
        size_t totalActive = finished + active;
        double activePercentage = static_cast<double>(totalActive) / m_max;

        std::cerr << "[";
        int barFinished = static_cast<int>(m_bar_width * progressPercentage);
        int barActive = static_cast<int>(m_bar_width * activePercentage);

        for (int i = 1; i < m_bar_width; ++i) {
            if (i <= barFinished)
                std::cerr << "=";
            else if (i <= barActive && i > barFinished)
                std::cerr << "-";
            else if (i >= barActive && i > barFinished)
                std::cerr << " ";
        }

        int progressPct = static_cast<int>(progressPercentage * 100.0);
        int activePct = static_cast<int>(static_cast<double>(active) / m_max * 100.0);
        int loadPct = static_cast<int>(static_cast<double>(active) / m_max_thread_count * 100.0);

        std::cerr << "] " << progressPct << " % finished jobs |"
                  << activePct << " % active jobs |"
                  << loadPct << " % load \r";
        std::cerr << std::flush;
    }

    // Configuration
    ProgressBarType m_progresstype = ProgressBarType::Continously;
    int m_max_thread_count = 1;
    int m_omp_env_thread = 1;
    std::atomic<int> m_increment_id{ 0 };
    int m_wake_up = 100;
    int m_bar_width = 100;

    // Thread tracking
    double m_max = 0;
    std::queue<CxxThread*> m_pool;
    std::vector<CxxThread*> m_active, m_finished;
    std::map<int, CxxThread*> m_threads_map;
    std::vector<std::pair<std::thread*, CxxThread*>> m_running_threads;

    // State flags
    bool m_reorganised = false;
    bool m_evn_overwrite_bar = false;

    // Timing
    std::chrono::time_point<std::chrono::steady_clock> m_start, m_end;
    mutable std::chrono::time_point<std::chrono::steady_clock> m_last;
    mutable int m_small_progress = 0;
};
