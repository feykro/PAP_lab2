//AUTHORS : FOCKE PABLO AND PERIN TOM
#include <stdio.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#include <x86intrin.h>
#include <stdbool.h>

#include <stdlib.h>
#include "sorting.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

#define MIN_PARALLEL_SIZE 1<<10 //has to be 2**k
#define MAX_THREADS 4

void merge (uint64_t *T, const uint64_t size1, const uint64_t size2){
	uint64_t *X = (uint64_t *) malloc ((size1+size2) * sizeof(uint64_t)) ;
	
	uint64_t i = 0 ;
	uint64_t j = size1 ;
	uint64_t k = 0 ;
  
	while ((i < size1) && (j < size1+size2)){
		if (T[i] < T [j]){
			X [k] = T [i] ;
			i = i + 1 ;
		}else{
			X [k] = T [j] ;
			j = j + 1 ;
		}
		k = k + 1 ;
	}

	if (i < size1){
		for (; i < size1; i++, k++){
			X [k] = T [i] ;
		}
	}else{
		for (; j < size1+size2; j++, k++){
			X [k] = T [j] ;
		}
	}
	memcpy (T, X, (size1+size2)*sizeof(uint64_t)) ;
	free (X) ;
	
	return ;
}

int comparInt(const void * a, const void * b){
    return ( *(uint64_t*)a - *(uint64_t*)b );
}

//=======================================================

void sequential_quickSort (uint64_t *T, int size){
    qsort(T, size, sizeof(uint64_t), comparInt);
}

void parallel_quickSort(uint64_t *T, int size){
    
    int nbChunks = MIN(MAX_THREADS,size);
    uint64_t chunkSize = size / nbChunks;

    uint64_t *chunkStartList[nbChunks];
    int chunkSizes[nbChunks];
    for(int i=0; i<nbChunks; i++){
        chunkStartList[i] = &T[i*chunkSize];
        chunkSizes[i] = chunkSize;
    }
    chunkSizes[nbChunks-1] = chunkSize + size%nbChunks;;

    int condition = 1;
    int ind_chunk;
    int nb = 0;
    uint64_t * resultat = (uint64_t *) malloc(size * sizeof(uint64_t));
    omp_set_num_threads(nbChunks);  //better performance with this line

    #pragma omp parallel
    {
        //here we're gonna apply qsort to every chunk
        #pragma omp for 
        for(ind_chunk = 0; ind_chunk < nbChunks; ind_chunk++){
            sequential_quickSort(chunkStartList[ind_chunk], chunkSizes[ind_chunk]);
        }

        int decal = 1;
        while (decal < nbChunks){
            #pragma omp for 
            for(int i = 0; i < nbChunks ; i+=decal*2){
                //merges two adjacent pairs
                if (i+decal < nbChunks){
                    merge(chunkStartList[i],chunkSizes[i],chunkSizes[i+decal]);
                    chunkSizes[i] = chunkSizes[i]+chunkSizes[i+decal] ;
                }
            }

            decal = decal*2;
        }
    }

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
        sequential_quickSort (X, N) ;
     
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

    printf ("\n quick-sort serial \t\t\t %.2lf Mcycles\n\n", (double)av/1000000) ;

  
    for (exp = 0 ; exp < NBEXPERIMENTS; exp++)
    {
#ifdef RINIT
        init_array_random (X, N);
#else
        init_array_sequence (X, N);
#endif
        
        start = _rdtsc () ;

        parallel_quickSort (X, N) ;
     
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
    printf ("\n quick-sort parallel \t\t %.2lf Mcycles\n\n", (double)av/1000000) ;
  
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

    sequential_quickSort (Y, N) ;
    parallel_quickSort (Z, N) ;

    if (! are_vector_equals (Y, Z, N)) {
        fprintf(stderr, "ERROR: sorting with the sequential and the parallel algorithm does not give the same result\n") ;
        exit (-1) ;
    }


    free(X);
    free(Y);
    free(Z);
    
}