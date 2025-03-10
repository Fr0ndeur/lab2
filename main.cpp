#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <random>
#include <algorithm>

constexpr int BITS_PER_NUMBER = 21;
constexpr int64_t MASK = (1LL << BITS_PER_NUMBER) - 1;
constexpr int MAX_VAL = MASK;

int64_t encodeTriple(int a, int b, int c) {
    return (int64_t(a) << (2 * BITS_PER_NUMBER)) | (int64_t(b) << BITS_PER_NUMBER) | c;
}

void decodeTriple(int64_t encoded, int& a, int& b, int& c) {
    a = encoded >> (2 * BITS_PER_NUMBER);
    b = (encoded >> BITS_PER_NUMBER) & MASK;
    c = encoded & MASK;
}

int64_t combineEncodedTriple(int64_t encoded1, int64_t encoded2) {
    int a1, b1, c1, a2, b2, c2;
    decodeTriple(encoded1, a1, b1, c1);
    decodeTriple(encoded2, a2, b2, c2);
    std::vector<int> combined = { a1, b1, c1, a2, b2, c2 };
    std::sort(combined.begin(), combined.end());
    return encodeTriple(combined[0], combined[1], combined[2]);
}

struct Triple {
    int m1;
    int m2;
    int m3;
};

void printTriple(const std::string& label, const Triple& tr) {
    std::cout << label << " minimal elements: "
        << tr.m1 << ", " << tr.m2 << ", " << tr.m3
        << " | sum = " << (tr.m1 + tr.m2 + tr.m3) << std::endl;
}

// 1. Sequential version
Triple findThreeMinSerial(const std::vector<int>& arr) {
    Triple result{ MAX_VAL, MAX_VAL, MAX_VAL };
    for (int x : arr) {
        if (x < result.m1) {
            result.m3 = result.m2;
            result.m2 = result.m1;
            result.m1 = x;
        }
        else if (x < result.m2) {
            result.m3 = result.m2;
            result.m2 = x;
        }
        else if (x < result.m3) {
            result.m3 = x;
        }
    }
    return result;
}

// 2. Paralel Mutex Version
Triple findThreeMinBlocking(const std::vector<int>& arr, int numThreads) {
    Triple global{ MAX_VAL, MAX_VAL, MAX_VAL };
    std::mutex mtx;

    auto worker = [&](int start, int end) {
        Triple local{ MAX_VAL, MAX_VAL, MAX_VAL };
        for (int i = start; i < end; i++) {
            int x = arr[i];
            if (x < local.m1) {
                local.m3 = local.m2;
                local.m2 = local.m1;
                local.m1 = x;
            }
            else if (x < local.m2) {
                local.m3 = local.m2;
                local.m2 = x;
            }
            else if (x < local.m3) {
                local.m3 = x;
            }
        }
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<int> combined = { global.m1, global.m2, global.m3, local.m1, local.m2, local.m3 };
        std::sort(combined.begin(), combined.end());
        global.m1 = combined[0];
        global.m2 = combined[1];
        global.m3 = combined[2];
        };

    std::vector<std::thread> threads;
    int n = static_cast<int>(arr.size());
    int chunk = n / numThreads;
    int start = 0;
    for (int i = 0; i < numThreads; i++) {
        int end = (i == numThreads - 1) ? n : start + chunk;
        threads.emplace_back(worker, start, end);
        start = end;
    }
    for (auto& t : threads) {
        t.join();
    }
    return global;
}


// 3. Paralel Atomic version
Triple findThreeMinNonBlocking(const std::vector<int>& arr, int numThreads) {
    std::atomic<int64_t> atomicGlobal;
    atomicGlobal.store(encodeTriple(MAX_VAL, MAX_VAL, MAX_VAL));

    auto worker = [&](int start, int end) {
        int local1 = MAX_VAL, local2 = MAX_VAL, local3 = MAX_VAL;
        for (int i = start; i < end; i++) {
            int x = arr[i];
            if (x < local1) {
                local3 = local2;
                local2 = local1;
                local1 = x;
            }
            else if (x < local2) {
                local3 = local2;
                local2 = x;
            }
            else if (x < local3) {
                local3 = x;
            }
        }
        int64_t localEncoded = encodeTriple(local1, local2, local3);

        int64_t oldVal = atomicGlobal.load();
        while (true) {
            int64_t newVal = combineEncodedTriple(oldVal, localEncoded);
            if (atomicGlobal.compare_exchange_weak(oldVal, newVal)) {
                break;
            }
        }
        };

    std::vector<std::thread> threads;
    int n = static_cast<int>(arr.size());
    int chunk = n / numThreads;
    int start = 0;
    for (int i = 0; i < numThreads; i++) {
        int end = (i == numThreads - 1) ? n : start + chunk;
        threads.emplace_back(worker, start, end);
        start = end;
    }
    for (auto& t : threads) {
        t.join();
    }

    int64_t finalEncoded = atomicGlobal.load();
    int a, b, c;
    decodeTriple(finalEncoded, a, b, c);
    return Triple{ a, b, c };
}

int main() {
    const int dataSize = 200000000;
    const int numThreads = 32;

    // Filling array with normal distribution in range [0, MAX_VAL)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(MAX_VAL / 2.0, MAX_VAL / 6.0);

    std::vector<int> data(dataSize);
    for (int i = 0; i < dataSize; i++) {
        int val = static_cast<int>(dist(gen));
        data[i] = std::clamp(val, 0, MAX_VAL);
    }

    // 1. Sequential version
    auto startSerial = std::chrono::steady_clock::now();
    Triple resultSerial = findThreeMinSerial(data);
    auto endSerial = std::chrono::steady_clock::now();
    auto timeSerial = std::chrono::duration_cast<std::chrono::microseconds>(endSerial - startSerial).count();

    // 2. Paralel Mutex Version
    auto startBlocking = std::chrono::steady_clock::now();
    Triple resultBlocking = findThreeMinBlocking(data, numThreads);
    auto endBlocking = std::chrono::steady_clock::now();
    auto timeBlocking = std::chrono::duration_cast<std::chrono::microseconds>(endBlocking - startBlocking).count();

    // 3. Paralel Atomic version
    auto startNonBlocking = std::chrono::steady_clock::now();
    Triple resultNonBlocking = findThreeMinNonBlocking(data, numThreads);
    auto endNonBlocking = std::chrono::steady_clock::now();
    auto timeNonBlocking = std::chrono::duration_cast<std::chrono::microseconds>(endNonBlocking - startNonBlocking).count();

    std::cout << "Sequential version:" << std::endl;
    printTriple("Sequential", resultSerial);
    std::cout << "Execution time: " << timeSerial << " microseconds" << std::endl << std::endl;


    std::cout << "Paralel Mutex Version:" << std::endl;
    printTriple("Lock", resultBlocking);
    std::cout << "Execution time: " << timeBlocking << " microseconds" << std::endl << std::endl;

    std::cout << "Paralel Atomic version:" << std::endl;
    printTriple("Lock-free", resultNonBlocking);
    std::cout << "Execution time: " << timeNonBlocking << " microseconds" << std::endl;

    return 0;
}
