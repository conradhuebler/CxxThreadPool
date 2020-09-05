/*
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

// #define _CxxThreadPool_Verbose
// #define _CxxThreadPool_NoBar

#define _CxxThreadPool_BarWidth 100

#include "include/CxxThreadPool.h"
#include "include/Timer.h"

#include <iostream>


using namespace std;

class Thread : public CxxThread{
public:
    Thread(int rand) : m_rand(rand)
    {

    }
    ~Thread() = default;

    inline int execute()
    {
        //printf("Thread %d started  ...\n", m_rand);
        std::this_thread::sleep_for(std::chrono::milliseconds(m_rand));
        //printf("Thread finished! ... %d ... \n", m_rand);
        return 0;
    }
private:
    int m_rand;
};


int main()
{
    unsigned int seed = time(NULL);
    RunTimer timer(true);
    int max_threads = 1000;
    int active_threads = 32;
    CxxThreadPool *pool = new CxxThreadPool;
    pool->setActiveThreadCount(active_threads);
    //pool->EcoBar(true);
    std::cout << "This a example application to present the CxxThreadPool method!\nThe demo will run with " << max_threads << " jobs and " << active_threads << " active threads!\n";
    std::cout << "Each thread will be initialised with a random number : rand_r(&seed)/1e6 - that equals the msecs to sleep.\n";
    std::cout << "The thread pool will run a couple of times - with the same threads and random numbers to demonstrate the :\nSingle Pool,\nStatic Pool and\nDynamic Pool ability of the CxxThreadPool Class.\n\n";
    for(int i = 0; i < max_threads; ++i)
    {
        Thread *thread = new Thread(rand_r(&seed)/1e6);
        pool->addThread(thread);
    }
    std::cout << "Single Pool: Each thread will be run isolated, so " << max_threads << " will be executed." << std::endl;
    pool->StartAndWait();

    pool->Reset();

    std::cout << "Static Pool: " << max_threads / active_threads << " threads will be executed ( + remaining individual threads )." << std::endl;
    pool->StaticPool();
    pool->StartAndWait();

    pool->Reset();
    std::cout << "Dynamic Pool with divider 1: " << max_threads / active_threads << " threads will be executed ( + remaining individual threads )." << std::endl;

    pool->DynamicPool(1);
    pool->StartAndWait();

    pool->Reset();

    std::cout << "Dynamic Pool with divider 2." << std::endl;
    pool->DynamicPool();
    pool->StartAndWait();

    pool->Reset();

    std::cout << "Dynamic Pool with divider 3." << std::endl;
    pool->DynamicPool(3);
    pool->StartAndWait();

    pool->Reset();

    std::cout << "Dynamic Pool with divider 4." << std::endl;
    pool->DynamicPool(4);
    pool->StartAndWait();

    pool->Reset();

    std::cout << "Dynamic Pool with divider 5." << std::endl;
    pool->DynamicPool(4);
    pool->StartAndWait();
    return 0;
}
