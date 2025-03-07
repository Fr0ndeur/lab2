#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <random>


constexpr int MAX_VAL = 1000000; 

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

// 2. Paralel Mutex version
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


int main() {
    const int dataSize = 1000;
    const int numThreads = 8;

    // Random integers in range [0, MAX_VAL)
    std::vector<int> data(dataSize);
    for (int i = 0; i < dataSize; i++) {
        data[i] = std::rand() % MAX_VAL;
    }

    // 1. Sequential version
    auto startSerial = std::chrono::steady_clock::now();
    Triple resultSerial = findThreeMinSerial(data);
    auto endSerial = std::chrono::steady_clock::now();
    auto timeSerial = std::chrono::duration_cast<std::chrono::microseconds>(endSerial - startSerial).count();

    // 2. Paralel Mutex version
    auto startBlocking = std::chrono::steady_clock::now();
    Triple resultBlocking = findThreeMinBlocking(data, numThreads);
    auto endBlocking = std::chrono::steady_clock::now();
    auto timeBlocking = std::chrono::duration_cast<std::chrono::microseconds>(endBlocking - startBlocking).count();


    // Results
    std::cout << "Sequential version:" << std::endl;
    printTriple("Sequential", resultSerial);
    std::cout << "Execution Time: " << timeSerial << " microseconds" << std::endl << std::endl;

    std::cout << "Блокуюча версія (mutex):" << std::endl;
    printTriple("Paralel Mutex", resultBlocking);
    std::cout << "Execution Time: " << timeBlocking << " microseconds" << std::endl << std::endl;


    return 0;
}
