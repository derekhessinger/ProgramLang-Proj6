/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/6/24
 * 
 * This file performs a pixel wise operation to a given ppg file, taking in
 * arguments for the file name and the number of threads to use
 * 
 * gcc -o task2 -I. task2.c ppmIO.c my_timing.c -lm
 * ./task2 <file name> <num of threads>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ppmIO.h"
#include "my_timing.h"

#define ITERATIONS 100  // Number of times to process image for accurate timing

// Structure to pass data to threads
typedef struct {
    Pixel* image;
    int start_idx;
    int end_idx;
} ThreadData;

// Thread function to process image chunk
void* process_chunk(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // Process the chunk multiple times
    for(int iter = 0; iter < ITERATIONS; iter++) {
        for(int i = data->start_idx; i < data->end_idx; i++) {
            data->image[i].r = data->image[i].r > 128 ? (220 + data->image[i].r)/2 : (30 + data->image[i].r)/2;
            data->image[i].g = data->image[i].g > 128 ? (220 + data->image[i].g)/2 : (30 + data->image[i].g)/2;
            data->image[i].b = data->image[i].b > 128 ? (220 + data->image[i].b)/2 : (30 + data->image[i].b)/2;
        }
    }
    
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    Pixel *src;
    int rows, cols, colors;
    int num_threads;
    double start_time, end_time;
    
    // Check usage
    if(argc < 3) {
        printf("Usage: %s <image filename> <number of threads (1,2,4)>\n", argv[0]);
        exit(-1);
    }
    
    // Get number of threads
    num_threads = atoi(argv[2]);
    if(num_threads != 1 && num_threads != 2 && num_threads != 4) {
        printf("Number of threads must be 1, 2, or 4\n");
        exit(-1);
    }
    
    // Read image
    src = ppm_read(&rows, &cols, &colors, argv[1]);
    if(!src) {
        printf("Unable to read file %s\n", argv[1]);
        exit(-1);
    }
    
    // Calculate total pixels
    int total_pixels = rows * cols;
    
    // Create thread arrays
    pthread_t threads[4];
    ThreadData thread_data[4];
    
    // Start timing
    start_time = get_time_sec();
    
    // Create threads to process image chunks
    for(int i = 0; i < num_threads; i++) {
        thread_data[i].image = src;
        thread_data[i].start_idx = (total_pixels / num_threads) * i;
        thread_data[i].end_idx = (i == num_threads - 1) ? 
                                total_pixels : 
                                (total_pixels / num_threads) * (i + 1);
        
        int rc = pthread_create(&threads[i], NULL, process_chunk, (void*)&thread_data[i]);
        if(rc) {
            printf("Error creating thread %d\n", i);
            exit(-1);
        }
    }
    
    // Wait for all threads to complete
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // End timing
    end_time = get_time_sec();
    
    // Calculate and print timing results
    double total_time = end_time - start_time;
    printf("Image processing completed:\n");
    printf("Number of threads: %d\n", num_threads);
    printf("Number of iterations: %d\n", ITERATIONS);
    printf("Total time: %.3f seconds\n", total_time);
    printf("Time per iteration: %.3f seconds\n", total_time / ITERATIONS);
    printf("Image dimensions: %d x %d (%d pixels)\n", rows, cols, total_pixels);
    
    // Write output image
    ppm_write(src, rows, cols, colors, "updated_img.ppm");
    
    // Clean up
    free(src);
    
    return 0;
}