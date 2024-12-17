/*
Derek Hessinger and Sumira Naroola
add description

to run: g++ -std=c++11 -o matrix matrix.cpp -lpthread
./matrix

*/


#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

// matrix type
using Matrix = std::vector<std::vector<int>>;

// function to multiply a chunk of rows
void multiply_chunk(const Matrix &A, const Matrix &B, Matrix &C, int start_row, int end_row, std::mutex &mutex) {
    int n = A.size();
    int m = B[0].size();
    int p = B.size();

    for (int i = start_row; i < end_row; ++i) {
        for (int j = 0; j < m; ++j) {
            int sum = 0;
            for (int k = 0; k < p; ++k) {
                sum += A[i][k] * B[k][j];
            }
            std::lock_guard<std::mutex> lock(mutex);  // critical section
            C[i][j] = sum;
        }
    }
}

int main() {
    int n = 4; // dimensions for simplicity (n x n)
    Matrix A = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}};
    Matrix B = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    Matrix C(n, std::vector<int>(n, 0));

    int num_threads = 2;  // using 2 threads for this example
    std::vector<std::thread> threads;
    std::mutex mutex;

    int chunk_size = n / num_threads;

    // create threads
    for (int i = 0; i < num_threads; ++i) {
        int start_row = i * chunk_size;
        int end_row = (i == num_threads - 1) ? n : start_row + chunk_size;
        threads.emplace_back(multiply_chunk, std::cref(A), std::cref(B), std::ref(C), start_row, end_row, std::ref(mutex));
    }

    // join threads
    for (auto &t : threads) {
        t.join();
    }

    // print result
    std::cout << "Resultant Matrix:\n";
    for (const auto &row : C) {
        for (int elem : row) {
            std::cout << elem << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
