/*
 * <Helper Class for C++ Threads and Pools.>
 * <Inspired by QThreadPool and QRunners.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

#ifndef _CxxThreadPool_TimeOut
static int wake_up = 100;
#else
static int wake_up = _CxxThreadPool_TimeOut;
#endif

#ifndef _CxxThreadPool_BarWidth
static int bar_width = 100;
#else
static int bar_width = _CxxThreadPool_BarWidth;
#endif

class CxxThread {
public:
    CxxThread() = default;
    virtual ~CxxThread() = default;

    inline void start()
    {
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThread::start() - Thread " << m_increment_id << " is up and running." << std::endl;
#endif
        m_start = std::chrono::system_clock::now();
        m_return = execute();
        m_running = false;
        m_finished = true;
        m_end = std::chrono::system_clock::now();
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThread::start() - Thread  " << m_increment_id << " finished after " << std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count() << " mseconds." << std::endl;
#endif
    }

    inline bool Running()
    {
        return m_running;
    }

    inline bool Finished()
    {
        return m_finished;
    }

    virtual int execute() = 0;

    //inline void wait()
    //{
    //    m_thread.join();
    //}

    inline std::thread::id Id() const { return m_thread.get_id(); }
    inline bool AutoDelete() const { return m_auto_delete; }

    inline std::thread *Thread() { return &m_thread; }
    inline void setIncrementId(int id) { m_increment_id = id; }

private:
    bool m_running = true, m_finished = false;
    bool m_auto_delete = true;
    int m_return = 0;
    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;
    int m_increment_id = 0;

protected:
    std::thread m_thread;
};

class CxxThreadPool
{
public:
    CxxThreadPool()
    {
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - Setting up thread pool for usage" << std::endl;
        std::cout << "CxxThreadPool::CxxThreadPool() - Wake up every " << wake_up << " msecs." << std::endl;
#endif

        /* Set active threads to OMP NUM Threads and set OMP NUM Threads to 1 */
#if defined(_OPENMP)

#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - openMP is enabled, getting OMP_NUM_THREADS" << std::endl;
#endif
        const char* val = std::getenv("OMP_NUM_THREADS");
        if (val == nullptr) { // invalid to assign nullptr to std::string
            m_omp_env_thread = 1;
        } else {
            m_omp_env_thread = std::max(atoi(std::getenv("OMP_NUM_THREADS")), 1);
        }
#ifdef _CxxThreadPool_Verbose
        std::cout << "CxxThreadPool::CxxThreadPool() - OMP_NUM_THREADS was " << m_omp_env_thread << std::endl;
#endif
        omp_set_num_threads(1);
#endif
        setActiveThreadCount(m_omp_env_thread);
    }

    /*! \brief Clean up all threads
     * TODO - consistent pointer handling - autodelete vs manual deletion
     */
    virtual ~CxxThreadPool()
    {
       while(m_pool.size())
       {
           auto thread = m_pool.front();
           delete thread;
           m_pool.pop();
       }

        for(int i = 0; i < m_active.size(); ++i)
            if(m_active[i]->AutoDelete())
                delete m_active[i];

        for(int i = 0; i < m_finished.size(); ++i)
            if(m_finished[i]->AutoDelete())
                delete m_finished[i];

                /* Restore old OMP NUM Thread value */
#if defined(_OPENMP)
        omp_set_num_threads(m_omp_env_thread);
#endif
    }

    /*! \brief Set number of active threads */
    inline void setActiveThreadCount(int thread_count) { m_max_thread_count = thread_count; }

    /*! \brief Add a thread to the pool */
    inline void addThread(CxxThread *thread)
    {
        m_pool.push(thread);
    }

    /*! \brief Start threads and wait until all finished */
    inline void StartAndWait()
    {
        m_max = m_pool.size();
        bool start_next = true;
        while ((m_pool.size() || m_active.size())) {
            if (m_pool.size() > 0) {
                while(m_active.size() < m_max_thread_count)
                {
                    if (!StartNext())
                        break;
                    Status();
                }
            }
            for(int i = 0; i < m_active.size(); ++i)
            {
                if(!m_active[i]->Finished())
                    continue;
                else
                {
                    m_finished.push_back(m_active[i]);
                    m_active.erase(m_active.begin() + i);
                    Status();
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(wake_up));
        }
    }

    // inline void startThreads() { m_allow_start = true; start(); }
    // inline void stopThreads() { m_allow_start = false; }

private:
    inline bool StartNext()
    {
        auto thread = m_pool.front();
        if (thread == NULL)
            return false;
        thread->setIncrementId(m_increment_id);
        m_increment_id++;
        std::thread th = std::thread(&CxxThread::start, thread);
        th.detach();
        m_active.push_back(thread);
        m_pool.pop();
        return m_pool.size();
    }

    inline void Status() const
    {
#ifdef _CxxThreadPool_Verbose
        printf(("\n\nCxxThreadPool::Status() - Running %d, Waiting %d, Finished %d\n\n"), m_active.size(), m_pool.size(), m_finished.size());
#endif

#ifndef _CxxThreadPool_Verbose
        Progress();
#endif
    }

    inline void Progress() const
    {
#ifndef _CxxThreadPool_NoBar
        int finished = m_finished.size();
        int active = m_active.size();
        int cum_active = finished + m_active.size();
        double p_finished = finished / m_max;
        double p_active = cum_active / m_max;
        std::cerr << "[";
        int bar_finished = bar_width * p_finished;
        int bar_active = bar_width * p_active;
        for (int i = 0; i < bar_width; ++i) {
            if (i < bar_finished)
                std::cerr << "=";
            else if (i < bar_active && i > bar_finished)
                std::cerr << "-";
            else if (i > bar_active && i > bar_finished)
                std::cerr << " ";
        }
        std::cerr << "] " << int(p_finished * 100.0) << " % finished jobs |" << int(active / m_max * 100.0) << " % active jobs |" << int(active / double(m_max_thread_count) * 100.0) << " % load \r";
        std::cerr.flush();
#endif
    }

    int m_max_thread_count = 1;
    int m_omp_env_thread = 1;
    int m_increment_id = 0;
    double m_max = 0;
    std::queue<CxxThread *>m_pool;
    std::vector<CxxThread *> m_active, m_finished;
    bool m_allow_start = true;
};
