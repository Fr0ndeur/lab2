# Lab Work #2: Atomic Variables and Lock-Free Operations. Paralel Calculations.

This repository contains the source code and experimental results for Lab Work No3. The objective of this lab is to explore atomic variables, atomic operations, and lock-free algorithms in C++. The project demonstrates three different approaches to finding the sum of the three smallest elements in an array:

- **Sequential Implementation:** A basic version without parallelization.
- **Blocking Implementation:** A parallel version utilizing mutexes for synchronization.
- **Lock-Free Implementation:** A parallel version that employs atomic operations (`CAS`) to eliminate blocking.

Since using three separate atomic integers does not guarantee atomicity across all values, the three minimum numbers are encoded into a single 64-bit integer. This approach ensures that updates to all three values happen atomically using `compare_exchange_weak`.

## Experimental Setup

The program measures execution time (in microseconds) for different input sizes and varying thread counts. Performance is analyzed by comparing:
- Sequential execution time,
- Synchronization overhead in the blocking approach (mutexes), and
- Performance improvement in the lock-free approach using atomic operations.

## Compilation and Execution

A C++ compiler with C++20 or later is required. To compile and run you can use VS 2022.

The source code allows modification of input sizes and thread counts to replicate experiments and visualize performance results.

## Files

- **main.cpp** – Implements sequential, blocking, and lock-free algorithms.
- **README.md** – This documentation file.

