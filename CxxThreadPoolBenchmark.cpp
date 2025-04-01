/**
 * @file CxxThreadPoolBenchmark.cpp
 * @brief Benchmark und Anwendungsbeispiel für CxxThreadPool
 */

#include "include/CxxThreadPool.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// Einfache Test-Thread-Klasse für kleine Aufgaben
class SimpleTestThread : public CxxThread {
public:
    SimpleTestThread(int sleepTime = 50, bool* flagToSet = nullptr)
        : m_sleepTime(sleepTime)
        , m_flagToSet(flagToSet)
    {
    }

    int execute() override
    {
        // Simuliere Arbeit
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));

        // Setze das Flag, falls vorhanden
        if (m_flagToSet != nullptr) {
            *m_flagToSet = true;
        }

        return 0;
    }

private:
    int m_sleepTime;
    bool* m_flagToSet;
};

// Thread für Matrix-Multiplikation
class MatrixMultiplicationThread : public CxxThread {
public:
    MatrixMultiplicationThread(
        const std::vector<std::vector<double>>& matA,
        const std::vector<std::vector<double>>& matB,
        int startRow, int endRow)
        : m_matA(matA)
        , m_matB(matB)
        , m_startRow(startRow)
        , m_endRow(endRow)
    {
        // Ergebnisgröße vorbereiten
        if (matA.size() > 0 && matB.size() > 0) {
            m_result.resize(m_endRow - m_startRow);
            for (auto& row : m_result) {
                row.resize(matB[0].size(), 0.0);
            }
        }
    }

    int execute() override
    {
        // Teilmatrix berechnen
        for (int i = 0; i < m_result.size(); ++i) {
            int row = m_startRow + i;
            for (int j = 0; j < m_result[i].size(); ++j) {
                double sum = 0.0;
                for (int k = 0; k < m_matB.size(); ++k) {
                    sum += m_matA[row][k] * m_matB[k][j];
                }
                m_result[i][j] = sum;
            }
        }
        return 0;
    }

    const std::vector<std::vector<double>>& getResult() const
    {
        return m_result;
    }

private:
    const std::vector<std::vector<double>>& m_matA;
    const std::vector<std::vector<double>>& m_matB;
    int m_startRow, m_endRow;
    std::vector<std::vector<double>> m_result;
};

// Thread für das Sortieren von Arrays
class SortingThread : public CxxThread {
public:
    SortingThread(std::vector<int>& data)
        : m_data(data)
    {
    }

    int execute() override
    {
        std::sort(m_data.begin(), m_data.end());
        return 0;
    }

private:
    std::vector<int>& m_data;
};

// Thread für die Bildverarbeitung (Blur-Effekt)
class ImageBlurThread : public CxxThread {
public:
    ImageBlurThread(
        const std::vector<std::vector<int>>& image,
        std::vector<std::vector<int>>& result,
        int startRow, int endRow,
        int kernelSize = 3)
        : m_image(image)
        , m_result(result)
        , m_startRow(startRow)
        , m_endRow(endRow)
        , m_kernelSize(kernelSize)
    {
    }

    int execute() override
    {
        int height = m_image.size();
        int width = (height > 0) ? m_image[0].size() : 0;
        int halfKernel = m_kernelSize / 2;

        for (int i = m_startRow; i < m_endRow; ++i) {
            for (int j = 0; j < width; ++j) {
                int sum = 0;
                int count = 0;

                // Apply blur kernel
                for (int ki = -halfKernel; ki <= halfKernel; ++ki) {
                    for (int kj = -halfKernel; kj <= halfKernel; ++kj) {
                        int ni = i + ki;
                        int nj = j + kj;

                        if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                            sum += m_image[ni][nj];
                            count++;
                        }
                    }
                }

                m_result[i][j] = sum / count;
            }
        }

        return 0;
    }

private:
    const std::vector<std::vector<int>>& m_image;
    std::vector<std::vector<int>>& m_result;
    int m_startRow, m_endRow;
    int m_kernelSize;
};

// Thread für Monte-Carlo-Pi-Berechnung
class MonteCarloPiThread : public CxxThread {
public:
    MonteCarloPiThread(int samples)
        : m_samples(samples)
    {
    }

    int execute() override
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        int inside = 0;
        for (int i = 0; i < m_samples; ++i) {
            double x = dis(gen);
            double y = dis(gen);

            if (x * x + y * y <= 1.0) {
                inside++;
            }
        }

        m_insideCount = inside;
        return 0;
    }

    int getInsideCount() const { return m_insideCount; }
    int getSamples() const { return m_samples; }

private:
    int m_samples;
    int m_insideCount = 0;
};

// Hilfsfunktionen

// Erzeugt eine zufällige Matrix
std::vector<std::vector<double>> generateRandomMatrix(int rows, int cols)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-10.0, 10.0);

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = dis(gen);
        }
    }

    return matrix;
}

// Erzeugt ein zufälliges "Bild" (2D-Array mit Int-Werten)
std::vector<std::vector<int>> generateRandomImage(int width, int height)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::vector<std::vector<int>> image(height, std::vector<int>(width));
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            image[i][j] = dis(gen);
        }
    }

    return image;
}

// Benchmark-Funktion
template <typename Func>
long long runBenchmark(const std::string& name, Func func, int iterations = 1)
{
    std::cout << "Benchmark: " << name << std::endl;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Dauer: " << duration << " ms"
              << (iterations > 1 ? " (Durchschnitt: " + std::to_string(duration / iterations) + " ms)" : "")
              << "\n"
              << std::endl;

    return duration;
}

int main()
{
    std::cout << "CxxThreadPool Benchmark und Anwendungsbeispiele" << std::endl;
    std::cout << "=============================================" << std::endl;

    int maxThreads = std::thread::hardware_concurrency();
    std::cout << "Verfügbare Hardware-Threads: " << maxThreads << "\n"
              << std::endl;

    // Benchmark 1: Matrix-Multiplikation
    std::cout << "Benchmark 1: Matrix-Multiplikation (500x500 * 500x500)" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    const int matrixSize = 500;
    auto matrixA = generateRandomMatrix(matrixSize, matrixSize);
    auto matrixB = generateRandomMatrix(matrixSize, matrixSize);
    std::vector<std::vector<double>> resultMatrix(matrixSize, std::vector<double>(matrixSize, 0.0));

    // Sequentielle Berechnung
    auto seqMatrixMultTime = runBenchmark("Sequentielle Matrix-Multiplikation", [&]() {
        for (int i = 0; i < matrixSize; ++i) {
            for (int j = 0; j < matrixSize; ++j) {
                double sum = 0.0;
                for (int k = 0; k < matrixSize; ++k) {
                    sum += matrixA[i][k] * matrixB[k][j];
                }
                resultMatrix[i][j] = sum;
            }
        }
    });

    // Berechnung mit CxxThreadPool
    auto threadedMatrixMultTime = runBenchmark("Parallele Matrix-Multiplikation mit CxxThreadPool", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        // Teile die Matrix in Zeilen auf
        int rowsPerThread = std::max(1, matrixSize / maxThreads);
        std::vector<MatrixMultiplicationThread*> threads;

        for (int i = 0; i < matrixSize; i += rowsPerThread) {
            int endRow = std::min(i + rowsPerThread, matrixSize);
            auto* thread = new MatrixMultiplicationThread(matrixA, matrixB, i, endRow);
            threads.push_back(thread);
            pool.addThread(thread);
        }

        pool.StartAndWait();

        // Ergebnisse zusammenführen
        for (size_t t = 0; t < threads.size(); ++t) {
            const auto& partialResult = threads[t]->getResult();
            int startRow = t * rowsPerThread;

            for (size_t i = 0; i < partialResult.size(); ++i) {
                for (size_t j = 0; j < partialResult[i].size(); ++j) {
                    resultMatrix[startRow + i][j] = partialResult[i][j];
                }
            }
        }
    });

    std::cout << "Beschleunigung: " << std::fixed << std::setprecision(2)
              << (seqMatrixMultTime / static_cast<double>(threadedMatrixMultTime))
              << "x\n"
              << std::endl;

    // Benchmark 2: Bildverarbeitung (Blur-Filter)
    std::cout << "Benchmark 2: Bildverarbeitung (Blur-Filter auf 2000x2000 Bild)" << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl;

    const int imageWidth = 2000;
    const int imageHeight = 2000;
    const int blurKernelSize = 5;

    auto image = generateRandomImage(imageWidth, imageHeight);
    std::vector<std::vector<int>> blurredImage(imageHeight, std::vector<int>(imageWidth, 0));

    // Sequentielle Berechnung
    auto seqBlurTime = runBenchmark("Sequentieller Blur-Filter", [&]() {
        int halfKernel = blurKernelSize / 2;

        for (int i = 0; i < imageHeight; ++i) {
            for (int j = 0; j < imageWidth; ++j) {
                int sum = 0;
                int count = 0;

                for (int ki = -halfKernel; ki <= halfKernel; ++ki) {
                    for (int kj = -halfKernel; kj <= halfKernel; ++kj) {
                        int ni = i + ki;
                        int nj = j + kj;

                        if (ni >= 0 && ni < imageHeight && nj >= 0 && nj < imageWidth) {
                            sum += image[ni][nj];
                            count++;
                        }
                    }
                }

                blurredImage[i][j] = sum / count;
            }
        }
    });

    // Parallele Berechnung mit CxxThreadPool
    auto threadedBlurTime = runBenchmark("Paralleler Blur-Filter mit CxxThreadPool", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        int rowsPerThread = std::max(1, imageHeight / maxThreads);

        for (int i = 0; i < imageHeight; i += rowsPerThread) {
            int endRow = std::min(i + rowsPerThread, imageHeight);
            auto* thread = new ImageBlurThread(image, blurredImage, i, endRow, blurKernelSize);
            pool.addThread(thread);
        }

        pool.StartAndWait();
    });

    std::cout << "Beschleunigung: " << std::fixed << std::setprecision(2)
              << (seqBlurTime / static_cast<double>(threadedBlurTime))
              << "x\n"
              << std::endl;

    // Benchmark 3: Monte-Carlo-Pi-Berechnung
    std::cout << "Benchmark 3: Monte-Carlo-Pi-Berechnung (100 Millionen Samples)" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    const int totalSamples = 100000000;
    int insideCount = 0;

    // Sequentielle Berechnung
    auto seqPiTime = runBenchmark("Sequentielle Pi-Berechnung", [&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        insideCount = 0;
        for (int i = 0; i < totalSamples; ++i) {
            double x = dis(gen);
            double y = dis(gen);

            if (x * x + y * y <= 1.0) {
                insideCount++;
            }
        }

        double pi = 4.0 * insideCount / totalSamples;
        std::cout << "  Sequentielles Ergebnis: Pi ≈ " << std::setprecision(10) << pi << std::endl;
    });

    // Parallele Berechnung mit CxxThreadPool
    auto threadedPiTime = runBenchmark("Parallele Pi-Berechnung mit CxxThreadPool", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        int samplesPerThread = totalSamples / maxThreads;
        std::vector<MonteCarloPiThread*> threads;

        for (int i = 0; i < maxThreads; ++i) {
            auto* thread = new MonteCarloPiThread(samplesPerThread);
            threads.push_back(thread);
            pool.addThread(thread);
        }

        pool.StartAndWait();

        // Ergebnisse zusammenführen
        int totalInsideCount = 0;
        int actualTotalSamples = 0;

        for (auto* thread : threads) {
            totalInsideCount += thread->getInsideCount();
            actualTotalSamples += thread->getSamples();
        }

        double pi = 4.0 * totalInsideCount / actualTotalSamples;
        std::cout << "  Paralleles Ergebnis: Pi ≈ " << std::setprecision(10) << pi << std::endl;
    });

    std::cout << "Beschleunigung: " << std::fixed << std::setprecision(2)
              << (seqPiTime / static_cast<double>(threadedPiTime))
              << "x\n"
              << std::endl;

    // Vergleich der Pool-Strategien bei vielen kleinen Aufgaben
    std::cout << "Vergleich der Pool-Strategien bei vielen kleinen Aufgaben" << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;

    const int smallTaskCount = 1000;

    auto normalTime = runBenchmark("Normale Pool-Strategie", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        for (int i = 0; i < smallTaskCount; ++i) {
            pool.addThread(new SimpleTestThread(2)); // Sehr kleine Aufgaben
        }

        pool.StartAndWait();
    });

    auto dynamicTime = runBenchmark("Dynamic Pool-Strategie", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        for (int i = 0; i < smallTaskCount; ++i) {
            pool.addThread(new SimpleTestThread(2));
        }

        pool.DynamicPool(4);
        pool.StartAndWait();
    });

    auto staticTime = runBenchmark("Static Pool-Strategie", [&]() {
        CxxThreadPool pool;
        pool.setActiveThreadCount(maxThreads);
        pool.setProgressBar(CxxThreadPool::ProgressBarType::None);

        for (int i = 0; i < smallTaskCount; ++i) {
            pool.addThread(new SimpleTestThread(2));
        }

        pool.StaticPool();
        pool.StartAndWait();
    });

    std::cout << "Vergleich:" << std::endl;
    std::cout << "  Normal:  " << normalTime << " ms (Referenz)" << std::endl;
    std::cout << "  Dynamic: " << dynamicTime << " ms ("
              << std::fixed << std::setprecision(1) << (100.0 * dynamicTime / normalTime) << "%)" << std::endl;
    std::cout << "  Static:  " << staticTime << " ms ("
              << std::fixed << std::setprecision(1) << (100.0 * staticTime / normalTime) << "%)" << std::endl;

    return 0;
}
