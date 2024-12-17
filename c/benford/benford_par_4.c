/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/1/24
 * 
 * This file performs parallel versions of counting for Bedford's Law
 * Method of counting: Local Counter Array, with Final Update Protected by Array of Mutexes
 * 
 * gcc -o benford_par_4 my_timing.c benford_par_4.c -lm -lpthread
 * ./benford_par
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include "my_timing.h"

// allocate mutex array for counting
pthread_mutex_t mutex[10];
int N = 0;
double *data;
int global_digit_counts[10] = {0};

// Structure to pass arguments to threads
typedef struct {
    int start;
    int end;
    int local_counts[10];  // Local counter array for each thread
} ThreadArgs;

// Load data from a binary file that has an int and then
// a sequence of doubles. The value of the int should indicate
// the number of doubles in the sequence.
// Load the data into global variables N and data.
int loadData(char *filename) {
    FILE *fp;

    if(filename != NULL && strlen(filename))
        fp = fopen(filename, "r");
    else
        return -1;

    if (!fp)
        return -1;

    fread(&N, sizeof(int), 1, fp);
    data = (double*)malloc(sizeof(double) * N);
    fread(data, sizeof(double), N, fp);
    fclose(fp);
    return 1; // success
}

// Return the leading Digit of n.
int leadingDigit(double n) {
    if (fabs(n) == 1.0)
        return 1;
    else if (fabs(n) == 0.0)
        return 0;
    else if (fabs(n) < 1.0) {
        double tmp = fabs(n);
        while (tmp < 1.0) {
            tmp *= 10.0;
        }
        return (int)floor(tmp);
    }
    else {
        long long unsigned in = (long long unsigned)floor(fabs(n));
        while (in > 9) {
            in /= 10;
        }
        return in;
    }
}

// Worker function for threads
void* count_digits(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    // Initialize local counts to 0
    memset(args->local_counts, 0, sizeof(args->local_counts));

    // Process assigned portion of data array using local counts
    for(int i = args->start; i < args->end; i++) {
        int digit = leadingDigit(data[i]);
        args->local_counts[digit]++;  // Update local count only
    }

    // After processing all assigned data, update global counts with mutex protection
    for(int digit = 0; digit < 10; digit++) {
        if(args->local_counts[digit] > 0) {  // Only lock if we have something to add
            pthread_mutex_lock(&mutex[digit]);
            global_digit_counts[digit] += args->local_counts[digit];
            pthread_mutex_unlock(&mutex[digit]);
        }
    }
    
    return NULL;
}

int main() {

    for (int i = 0; i<5; i++){
    double t1, t2;
    // Load data first
    if(loadData("medium.bin") < 0) {
        printf("Error loading data\n");
        return 1;
    }

    // Start the timer after we have loaded the data.
    t1 = get_time_sec();

    // Initialize each mutex
    for(int i = 0; i < 10; i++) {
        pthread_mutex_init(&mutex[i], NULL);
    }

    // Create exactly 8 threads
    const int NUM_THREADS = 8;
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    
    // Create threads and assign work
    int chunk_size = N / NUM_THREADS;
    for(int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? N : (i + 1) * chunk_size;
        
        if(pthread_create(&threads[i], NULL, count_digits, &args[i]) != 0) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // End the timer  
    t2 = get_time_sec();
    
    // Print results
    for(int i = 1; i < 10; i++) {
        printf("Digit %d: %d occurrences\n", i, global_digit_counts[i]);
    }
    
    printf("It took %f seconds for the whole thing to run\n",t2-t1);
    
    // Cleanup
    for(int i = 0; i < 10; i++) {
        pthread_mutex_destroy(&mutex[i]);
    }
    free(data);
    }
    return 0;
}