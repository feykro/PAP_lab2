#include <stdio.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#include <x86intrin.h>
#include <stdbool.h>

#include <stdlib.h>
#include "sorting.h"

#define MAX_THREADS 4

void swap(uint64_t *a, uint64_t *b) {
  int t = *a;
  *a = *b;
  *b = t;
}


int partition(uint64_t * T, int first, int last) {
    int pivot = T[last];
    int i = (first - 1);

    for (int j=first; j < last; j++) {
        if (T[j] <= pivot) {
        i++;
        swap(&T[i], &T[j]);
        }
    }

    swap(&T[i+1], &T[last]);
    return (i+1);
}

void quickSort(uint64_t * T, int first, int last) {
    if (first < last) {
        int pivot = partition(T, first, last);
        quickSort(T, first, pivot - 1);
        quickSort(T, pivot + 1, last);
    }
}


void sequential_quick_sort (uint64_t *T, int size){
    quickSort(T, 0, size-1);
    return;
}

void parallel_quick_sort (uint64_t *T, int size){
    //actuellement c'est de la merde mais apparemment il faut
    //écrire une fonction de partionnement et faire deux appels
    //a quicksort avec pragma omp task
    omp_set_num_threads(MAX_THREADS);
    #pragma omp task shared(T)
    quickSort(T, 0, size-1);

    return;
}


int main (int argc, char **argv){
    uint64_t start, end;
    uint64_t av ;
    unsigned int exp ;

    /* the program takes one parameter N which is the size of the array to
       be sorted. The array will have size 2^N */
    if (argc != 2)
    {
        fprintf (stderr, "quick-sort.run N \n") ;
        exit (-1) ;
    }

    uint64_t N = 1 << (atoi(argv[1])) ;
    /* the array to be sorted */
    uint64_t *X = (uint64_t *) malloc (N * sizeof(uint64_t)) ;

    printf("--> Sorting an array of size %llu\n",N);
#ifdef RINIT
    printf("--> The array is initialized randomly\n");
#endif
    

    for (exp = 0 ; exp < NBEXPERIMENTS; exp++){
#ifdef RINIT
        init_array_random (X, N);
#else
        init_array_sequence (X, N);
#endif
        
      
        start = _rdtsc () ;
        sequential_quick_sort (X, N) ;
     
        end = _rdtsc () ;
        experiments [exp] = end - start ;

        /* verifying that X is properly sorted */
#ifdef RINIT
        if (! is_sorted (X, N))
        {
            fprintf(stderr, "ERROR: the sequential sorting of the array failed\n") ;
            print_array (X, N) ;
            exit (-1) ;
	}
#else
        if (! is_sorted_sequence (X, N))
        {
            fprintf(stderr, "ERROR: the sequential sorting of the array failed\n") ;
            print_array (X, N) ;
            exit (-1) ;
	}
#endif
    }

    av = average_time() ;  

    printf ("\n quick sort serial \t\t\t %.2lf Mcycles\n\n", (double)av/1000000) ;

  
    for (exp = 0 ; exp < NBEXPERIMENTS; exp++)
    {
#ifdef RINIT
        init_array_random (X, N);
#else
        init_array_sequence (X, N);
#endif
        
        start = _rdtsc () ;

        parallel_quick_sort (X, N) ;
     
        end = _rdtsc () ;
        experiments [exp] = end - start ;

        /* verifying that X is properly sorted */
#ifdef RINIT
        if (! is_sorted (X, N))
        {
            fprintf(stderr, "ERROR: the parallel sorting of the array failed\n") ;
            exit (-1) ;
	}
#else
        if (! is_sorted_sequence (X, N))
        {
            fprintf(stderr, "ERROR: the parallel sorting of the array failed\n") ;
            exit (-1) ;
	}
#endif
                
        
    }
    
    av = average_time() ;  
    printf ("\n quick sort parallel \t\t %.2lf Mcycles\n\n", (double)av/1000000) ;
  
    /* print_array (X, N) ; */

    /* before terminating, we run one extra test of the algorithm */
    uint64_t *Y = (uint64_t *) malloc (N * sizeof(uint64_t)) ;
    uint64_t *Z = (uint64_t *) malloc (N * sizeof(uint64_t)) ;

#ifdef RINIT
    init_array_random (Y, N);
#else
    init_array_sequence (Y, N);
#endif

    memcpy(Z, Y, N * sizeof(uint64_t));

    sequential_quick_sort (Y, N) ;
    parallel_quick_sort (Z, N) ;

    if (! are_vector_equals (Y, Z, N)) {
        fprintf(stderr, "ERROR: sorting with the sequential and the parallel algorithm does not give the same result\n") ;
        exit (-1) ;
    }


    free(X);
    free(Y);
    free(Z);
    
}