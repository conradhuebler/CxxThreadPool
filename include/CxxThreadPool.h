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

#include <omp.h>

class CxxThread {
public:
    CxxThread() = default;
    virtual ~CxxThread() = default;

    inline void start()
    {
        m_return = execute();
        m_running = false;
        m_finished = true;
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
private:
    bool m_running = true, m_finished = false;
    bool m_auto_delete = true;
    int m_return = 0;

protected:
    std::thread m_thread;
};

class CxxThreadPool
{
public:
    CxxThreadPool()
    {
        /* Set active threads to OMP NUM Threads and set OMP NUM Threads to 1 */
#if defined(_OPENMP)
        m_omp_env_thread = std::max(atoi(std::getenv("OMP_NUM_THREADS")), 1);
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
        while(m_pool.size() || m_active.size())
        {
            if(m_pool.size())
            {
                while(m_active.size() < m_max_thread_count)
                {
                   StartNext();
#ifdef _VERBOSE
                   printf(("Running %d, Waiting %d, Finished %d\n"), m_active.size(), m_pool.size(), m_finished.size());
#endif
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
                    continue;
                }
            }
        }
    }

    // inline void startThreads() { m_allow_start = true; start(); }
    // inline void stopThreads() { m_allow_start = false; }

private:
    inline bool StartNext()
    {
        auto thread = m_pool.front();
        std::thread th = std::thread(&CxxThread::start, thread);
        th.detach();
        m_active.push_back(thread);
        m_pool.pop();
        return true;
    }

    int m_max_thread_count = 1;
    int m_omp_env_thread = 1;
    std::queue<CxxThread *>m_pool;
    std::vector<CxxThread *> m_active, m_finished;
    bool m_allow_start = true;
};
