/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/6/24
 * 
 * This file performs a pixel wise operation to a given ppg file, taking in
 * arguments for the file name and the number of threads to use
 * 
 * g++ -o task2 task2.c++ ppmIO.o -pthread -std=c++17
 * ./task2 <file name> <num of threads>
 * 
 */

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>

#ifdef __cplusplus
extern "C" {
#endif

#include "ppmIO.h"

#ifdef __cplusplus
}
#endif

#include "ppmIO.h"

constexpr int ITERATIONS = 100;  // Number of times to process image for accurate timing

// Structure to pass data to threads
struct ThreadData {
    Pixel* image;
    int start_idx;
    int end_idx;
};

double get_time_sec() {
    using namespace std::chrono;
    auto current_time = high_resolution_clock::now();
    auto dur = current_time.time_since_epoch();
    return duration_cast<duration<double>>(dur).count();
}

// Thread function to process image chunk
void process_chunk(ThreadData* data) {
    // Process the chunk multiple times
    for(int iter = 0; iter < ITERATIONS; iter++) {
        for(int i = data->start_idx; i < data->end_idx; i++) {
            data->image[i].r = data->image[i].r > 128 ? (220 + data->image[i].r)/2 : (30 + data->image[i].r)/2;
            data->image[i].g = data->image[i].g > 128 ? (220 + data->image[i].g)/2 : (30 + data->image[i].g)/2;
            data->image[i].b = data->image[i].b > 128 ? (220 + data->image[i].b)/2 : (30 + data->image[i].b)/2;
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <image filename> <number of threads (1,2,4)>\n";
        return -1;
    }
    
    // Get number of threads
    int num_threads = std::stoi(argv[2]);
    if(num_threads != 1 && num_threads != 2 && num_threads != 4) {
        std::cerr << "Number of threads must be 1, 2, or 4\n";
        return -1;
    }
    
    // Read image
    int rows, cols, colors;
    Pixel* src = ppm_read(&rows, &cols, &colors, argv[1]);
    if(!src) {
        std::cerr << "Unable to read file " << argv[1] << "\n";
        return -1;
    }
    
    // Calculate total pixels
    int total_pixels = rows * cols;
    
    // Create vectors for threads and thread data
    std::vector<std::thread> threads;
    std::vector<ThreadData> thread_data(num_threads);
    
    // Start timing
    double start_time = get_time_sec();
    
    // Create threads to process image chunks
    for(int i = 0; i < num_threads; i++) {
        thread_data[i].image = src;
        thread_data[i].start_idx = (total_pixels / num_threads) * i;
        thread_data[i].end_idx = (i == num_threads - 1) ? 
                                total_pixels : 
                                (total_pixels / num_threads) * (i + 1);
        
        try {
            threads.emplace_back(process_chunk, &thread_data[i]);
        } catch (const std::system_error& e) {
            std::cerr << "Error creating thread " << i << ": " << e.what() << "\n";
            return -1;
        }
    }
    
    // Wait for all threads to complete
    for(auto& thread : threads) {
        thread.join();
    }
    
    // End timing
    double end_time = get_time_sec();
    
    // Calculate and print timing results
    double total_time = end_time - start_time;
    std::cout << "Image processing completed:\n"
              << "Number of threads: " << num_threads << "\n"
              << "Number of iterations: " << ITERATIONS << "\n"
              << "Total time: " << std::fixed << std::setprecision(3) << total_time << " seconds\n"
              << "Time per iteration: " << total_time / ITERATIONS << " seconds\n"
              << "Image dimensions: " << rows << " x " << cols 
              << " (" << total_pixels << " pixels)\n";
    
    // Write output image
    ppm_write(src, rows, cols, colors, const_cast<char*>("updated_img.ppm"));
    
    // Clean up
    delete[] src;
    
    return 0;
}