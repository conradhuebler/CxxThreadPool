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

#pragma once

#include <chrono>
#include <ctime>

class RunTimer {
public:
    RunTimer(bool print = false)
        : m_print(print)
    {
        m_start = std::chrono::system_clock::now();
        if (m_print) {
            std::time_t end_time = std::chrono::system_clock::to_time_t(m_start);
            std::cout << "Started at " << std::ctime(&end_time) << std::endl;
        }
    }

    ~RunTimer()
    {
        if (m_print) {
            std::cout << "Finished after " << Elapsed() << " seconds!" << std::endl;
            std::time_t end_time = std::chrono::system_clock::to_time_t(m_end);
            std::cout << "Finished at " << std::ctime(&end_time) << std::endl;
        }
    }

    inline int Elapsed()
    {
        m_end = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start).count();
    }

private:
    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;
    bool m_print;
};
