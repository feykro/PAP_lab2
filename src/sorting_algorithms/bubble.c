#include <stdio.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#include <x86intrin.h>

#include "sorting.h"

/*
   bubble sort -- sequential, parallel --
*/

#define MAX_THREADS 12

//quick function to swap 2 elements, works much better than using the array and indexes
void swap(uint64_t * a, uint64_t * b){
    uint64_t temp = *b;
    *b = *a;
    *a = temp;
}

//Sequential bubble sort
void sequential_bubble_sort (uint64_t *T, const uint64_t size)
{
    int condition = 0;
    int i;
    while(condition == 0){
        condition = 1;
        for(i=0; i<size -1; i++){
            if(T[i] > T[i+1]){
                condition = 0;
                swap( &T[i], &T[i+1]);
            }
        }
    }
    return ;
}


//Parallel bubble sort
void parallel_bubble_sort (uint64_t *T, const uint64_t size)
{
    //determining the number of threads
    int nbChunks = MAX_THREADS < size ? MAX_THREADS : size;
    //determining the length of chunks, with potentially a longer last one if we have an odd
    //number of threads/chunks
    uint64_t chunkSize = size / nbChunks;
    uint64_t lastChunkSize = chunkSize + size%nbChunks;

    //creating an array with nbChunks references to the beginning of every chunk
    uint64_t * chunksList[nbChunks];
    for(int i=0; i<nbChunks; i++){
        chunksList[i] = &T[i*chunkSize];
    }

    //Sorting algorithm
    int condition = 1;
    int ind_chunk;
    omp_set_num_threads(nbChunks);  //better performance with this line
    while( condition != 0){
        condition = 0;

        #pragma omp parallel default (none) shared(nbChunks, chunksList, chunkSize, lastChunkSize, condition) private(ind_chunk)
        {
            //here we're gonna apply one bubble to every chunk
            #pragma omp for reduction(+:condition)
            for( ind_chunk = 0; ind_chunk < nbChunks; ind_chunk++){
                uint64_t taille = (ind_chunk == nbChunks-1) ? lastChunkSize : chunkSize;
                int i;
                for(i = 0 ; i < taille -1 ; i++){
                   if(chunksList[ind_chunk][i] > chunksList[ind_chunk][i+1]){
                       condition++;
                       swap(&chunksList[ind_chunk][i] , &chunksList[ind_chunk][i+1]);
                   }
                }
            }

            //here we're gonna do the swapping at the border of 2 chunks
            #pragma omp for
            for(ind_chunk = 0; ind_chunk < nbChunks - 1; ind_chunk++){
                if(chunksList[ind_chunk][chunkSize-1] > chunksList[ind_chunk+1][0]){
                    swap(&chunksList[ind_chunk][chunkSize-1] , &chunksList[ind_chunk+1][0]);
                }
            }
        }
    }
    return;
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
        fprintf (stderr, "bubble.run N \n") ;
        exit (-1) ;
    }

    uint64_t N = 1 << (atoi(argv[1])) ;
    /* the array to be sorted */
    uint64_t *X = (uint64_t *) malloc (N * sizeof(uint64_t)) ;

    printf("--> Sorting an array of size %lu\n",N);
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

        sequential_bubble_sort (X, N) ;

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

    printf ("\n bubble serial \t\t\t %.2lf Mcycles\n\n", (double)av/1000000) ;


    for (exp = 0 ; exp < NBEXPERIMENTS; exp++)
    {
#ifdef RINIT
        init_array_random (X, N);
#else
        init_array_sequence (X, N);
#endif

        start = _rdtsc () ;

        parallel_bubble_sort (X, N) ;

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
    printf ("\n bubble parallel \t\t %.2lf Mcycles\n\n", (double)av/1000000) ;

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

    sequential_bubble_sort (Y, N) ;
    parallel_bubble_sort (Z, N) ;

    if (! are_vector_equals (Y, Z, N)) {
        fprintf(stderr, "ERROR: sorting with the sequential and the parallel algorithm does not give the same result\n") ;
        exit (-1) ;
    }


    free(X);
    free(Y);
    free(Z);

}
