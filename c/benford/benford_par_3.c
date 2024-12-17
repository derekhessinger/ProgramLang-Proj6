/**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/1/24
 * 
 * This file performs parallel versions of counting for Bedford's Law
 * Method of counting: Local Counter Array, with Final Update Protected by Single Mutex
 * 
 * gcc -o benford_par_3 my_timing.c benford_par_3.c -lm -lpthread
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
pthread_mutex_t mutex;
int N = 0;
double *data;
int global_digit_counts[10] = {0};

// Structure to pass arguments to threads
typedef struct {
    int start;
    int end;
    int digit_counts[10];
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

  fread( &N, sizeof(int), 1, fp );
  data = (double*)malloc( sizeof(double)*N );
  fread( data, sizeof(double), N, fp );
  fclose( fp );
  // Uncomment this to verify the right data are being read in.
  // For super_short.bin, it should print out
  // data[0] = 97.137926
  // data[1] = 24.639612
  // data[2] = 55.692572
//   int i;
//   for (i = 0; i < N; i++) {
//         printf( "data[%d] = %f\n", i, data[i] );
//   }
  return 1; // success
}

// Return the leading Digit of n.
int leadingDigit( double n ) {
    // This is not a particularly efficient approach.
    if (fabs(n) == 1.0)
        return 1;
    else if (fabs(n) == 0.0)
        return 0;
    else if (fabs(n) < 1.0) {
        // multiply it by 10 until you get a number that is at least 1.
        // Then chop off the fractional part. All that remains is the first digit.
        double tmp = fabs(n);
        while (tmp < 1.0) {
            tmp *= 10.0;
        }
        return (int)floor( tmp );
    }
    else {
        // Divide it by 10 until you get a number smaller than 10.
        // That number will be the first digit of the original number.
        long long unsigned in = (long long unsigned) floor(fabs(n));
        while (in > 9) {
            in /= 10;
        }
        return in;
    }
} // end leadingDigit

// Worker function for threads
void* count_digits(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    // Process assigned portion of data array - no mutex needed here
    for(int i = args->start; i < args->end; i++) {
        int digit = leadingDigit(data[i]);
        args->digit_counts[digit]++;  // Update local counts only
    }
    
    // After counting, lock once to update global counts
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < 10; i++) {
        global_digit_counts[i] += args->digit_counts[i];
    }
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(){

    for (int i = 0; i<5;i++){
    double t1,t2;

    // Load data first
    if(loadData("medium.bin") < 0) {
        printf("Error loading data\n");
        return 1;
    }

    // Start the timer after we have loaded the data.
    t1 = get_time_sec();

    // Initialize single mutex
    pthread_mutex_init(&mutex, NULL);


    // Create exactly 8 threads
    const int NUM_THREADS = 8;
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    
    // Create threads and assign work, handling non-divisible N
    for(int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * (N / NUM_THREADS);
        if (i == NUM_THREADS - 1) {
            // Last thread gets any remaining elements
            args[i].end = N;
        } else {
            args[i].end = (i + 1) * (N / NUM_THREADS);
        }
        memset(args[i].digit_counts, 0, sizeof(args[i].digit_counts));

        pthread_create(&threads[i], NULL, count_digits, &args[i]);
    }
    
    // Wait for all threads to complete
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // End the timer  
    t2 = get_time_sec();

    // Print results from global counts instead of args
    for(int i = 1; i < 10; i++) {
        printf("Digit %d: %d occurrences\n", i, global_digit_counts[i]);
    }

    printf("It took %f seconds for the whole thing to run\n",t2-t1); 
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    free(data);
    }
    return 0;
}