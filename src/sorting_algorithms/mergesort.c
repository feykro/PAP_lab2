//AUTHORS : FOCKE PABLO AND PERIN TOM
#include <stdio.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#include <x86intrin.h>

#include "sorting.h"

#define MIN_PARALLEL_SIZE 1<<8 //has to be 2**k
#define PARALLEL_V 2 //1 or 2
#define MAX_THREADS 4
/* 
   Merge two sorted chunks of array T!
   The two chunks are of size size
   First chunck starts at T[0], second chunck starts at T[size]
*/
void merge (uint64_t *T, const uint64_t size){
	uint64_t *X = (uint64_t *) malloc (2 * size * sizeof(uint64_t)) ;
	
	uint64_t i = 0 ;
	uint64_t j = size ;
	uint64_t k = 0 ;
  
	while ((i < size) && (j < 2*size)){
		if (T[i] < T [j]){
			X [k] = T [i] ;
			i = i + 1 ;
		}else{
			X [k] = T [j] ;
			j = j + 1 ;
		}
		k = k + 1 ;
	}

	if (i < size){
		for (; i < size; i++, k++){
			X [k] = T [i] ;
		}
	}else{
		for (; j < 2*size; j++, k++){
			X [k] = T [j] ;
		}
	}
	memcpy (T, X, 2*size*sizeof(uint64_t)) ;
	free (X) ;
	
	return ;
}



/* 
   merge sort -- sequential, parallel -- 
*/

void sequential_merge_sort (uint64_t *T, const uint64_t size)
{
    if (size >= 2){
      sequential_merge_sort(T,size/2);
      sequential_merge_sort(T+(size/2),size/2);
    }
    merge(T,size/2);
}

void parallel_merge_sort_V1 (uint64_t *T, const uint64_t size)
{
    if (size >= 2){

      #pragma omp task
      parallel_merge_sort_V1(T,size/2);

      #pragma omp task
      parallel_merge_sort_V1(T+(size/2),size/2);

      #pragma omp taskwait
      
    }
    merge(T,size/2);
  
}

void parallel_merge_sort_V2 (uint64_t *T, const uint64_t size)
{
    if (size >= MIN_PARALLEL_SIZE){

      #pragma omp task
      parallel_merge_sort_V2(T,size/2);

      #pragma omp task
      parallel_merge_sort_V2(T+(size/2),size/2);

      #pragma omp taskwait
      merge(T,size/2);
      
    }else{
      sequential_merge_sort(T,size);
    }
  
}

void parallel_merge_sort (uint64_t *T, const uint64_t size)
{
  #pragma omp parallel 
  {
    if (PARALLEL_V == 1){
      #pragma omp single
      parallel_merge_sort_V1(T, size) ;
    }else{
      #pragma omp single
      parallel_merge_sort_V2(T, size) ;
    }
  }
}



int main (int argc, char **argv)
{
    uint64_t start, end;
    uint64_t av ;
    unsigned int exp ;

    /* the program takes one parameter N which is the size of the array to
       be sorted. The array will have size 2^N */
    if (argc != 2)
    {
        fprintf (stderr, "merge.run N \n") ;
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
        
        sequential_merge_sort (X, N) ;
     
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

    printf ("\n mergesort serial \t\t\t %.2lf Mcycles\n\n", (double)av/1000000) ;

  
    for (exp = 0 ; exp < NBEXPERIMENTS; exp++)
    {
#ifdef RINIT
        init_array_random (X, N);
#else
        init_array_sequence (X, N);
#endif
        
        start = _rdtsc () ;

        parallel_merge_sort (X, N) ;
     
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
    printf ("\n mergesort parallel \t\t %.2lf Mcycles\n\n", (double)av/1000000) ;
  
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

    sequential_merge_sort (Y, N) ;
    parallel_merge_sort (Z, N) ;

    if (! are_vector_equals (Y, Z, N)) {
        fprintf(stderr, "ERROR: sorting with the sequential and the parallel algorithm does not give the same result\n") ;
        exit (-1) ;
    }


    free(X);
    free(Y);
    free(Z);
    
}