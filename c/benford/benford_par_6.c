/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/1/24
 * 
 * This file performs parallel versions of counting for Bedford's Law
 * Method of counting: Global Counter Array of Arrays, Grouped by Digit, no Mutex
 * 
* gcc -o benford_par_6 my_timing.c benford_par_6.c -lm -lpthread
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
#define COUNTS_PER_DIGIT NUM_THREADS

int N = 0;
double *data;
int digit_thread_counts[10 * COUNTS_PER_DIGIT] = {0};

typedef struct {
    int thread_id;
    int start;
    int end;
} ThreadArgs;

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
    return 1;
}

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

void* count_digits(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    for(int i = args->start; i < args->end; i++) {
        int digit = leadingDigit(data[i]);
        digit_thread_counts[digit * COUNTS_PER_DIGIT + args->thread_id]++;
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
    
    for(int digit = 1; digit < 10; digit++) {
        int sum = 0;
        for(int thread = 0; thread < NUM_THREADS; thread++) {
            sum += digit_thread_counts[digit * COUNTS_PER_DIGIT + thread];
        }
        printf("Digit %d: %d occurrences\n", digit, sum);
    }
    
    // End the timer  
    t2 = get_time_sec();

    printf("It took %f seconds for the whole thing to run\n",t2-t1);

    
    free(data);
    }
    return 0;
}