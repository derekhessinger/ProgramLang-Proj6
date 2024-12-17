/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/1/24
 * 
 * This file performs parallel versions of counting for Bedford's Law
 * Method of counting: Global Counter Array of Arrays, Grouped by Thread, no Mutex
 * 
 * gcc -o benford_par_5 my_timing.c benford_par_5.c -lm -lpthread
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

#define NUM_THREADS 8

int N = 0;
double *data;
int global_digit_counts[NUM_THREADS * 10] = {0};

// Structure to pass arguments to threads
typedef struct {
    int thread_id;
    int start;
    int end;
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
    int base_index = args->thread_id * 10;
    
    for(int i = args->start; i < args->end; i++) {
        int digit = leadingDigit(data[i]);
        global_digit_counts[base_index + digit]++;
    }
    
    return NULL;
}

int main() {
    for (int i = 0; i<5; i++){
    double t1, t2;
    if(loadData("medium.bin") < 0) {
        printf("Error loading data\n");
        return 1;
    }

    // Start the timer after we have loaded the data.
    t1 = get_time_sec();

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    
    int chunk_size = N / NUM_THREADS;
    for(int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].start = i * chunk_size;
        args[i].end = (i == NUM_THREADS - 1) ? N : (i + 1) * chunk_size;
        
        if(pthread_create(&threads[i], NULL, count_digits, &args[i]) != 0) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
    }
    
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Sum up counts across all threads for each digit
    int final_counts[10] = {0};
    for(int digit = 0; digit < 10; digit++) {
        for(int thread = 0; thread < NUM_THREADS; thread++) {
            final_counts[digit] += global_digit_counts[thread * 10 + digit];
        }
        if(digit > 0) {  // Skip printing digit 0 counts
            printf("Digit %d: %d occurrences\n", digit, final_counts[digit]);
        }
    }

    // End the timer  
    t2 = get_time_sec();

    printf("It took %f seconds for the whole thing to run\n",t2-t1);
    
    free(data);
    }
    return 0;
}