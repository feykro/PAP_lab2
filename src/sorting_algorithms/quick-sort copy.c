#include <stdio.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#include <x86intrin.h>
#include <stdbool.h>

#include <stdlib.h>
#include "sorting.h"

#define MIN_PARALLEL_SIZE 1<<8 //has to be 2**k
#define PARALLEL_V 3 //1 or 2 
#define MAX_THREADS 12


int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

void swap(uint64_t *a, uint64_t *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

/**
 * Du coup on s'en sert pas vu qu'on a implémenté notre version
 */

int comparInt(const void * a, const void * b){
    return ( *(uint64_t*)a - *(uint64_t*)b );
    /*
    uint64_t inta = *((uint64_t *) a);
    uint64_t intb = *((uint64_t *) b);
    if(a == b){
        return 0;
    }else{
        return a < b ? -1 : 1;
    }*/
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

void sequential_quickSort(uint64_t * T, int first, int last) {
    if (first < last) {
        int pivot = partition(T, first, last);
        sequential_quickSort(T, first, pivot - 1);
        sequential_quickSort(T, pivot + 1, last);
    }
}

void parallel_quickSort_V1(uint64_t * T, int first, int last) {
    if (first < last) {
        int pivot = partition(T, first, last);

        #pragma omp task
        parallel_quickSort_V1(T, first, pivot - 1);

        #pragma omp task
        parallel_quickSort_V1(T, pivot + 1, last);
    }
}

void parallel_quickSort_V2(uint64_t * T, int first, int last) {
    if (last-first > MIN_PARALLEL_SIZE) { //if size > MIN_PARALLEL_SIZE
        int pivot = partition(T, first, last);

        #pragma omp task
        parallel_quickSort_V2(T, first, pivot - 1);

        #pragma omp task
        parallel_quickSort_V2(T, pivot + 1, last);

    }else{
        sequential_quickSort(T,first,last);
    }
}

//===========================================

int omegaPartition(uint64_t indices[], uint64_t entiers[], int first, int last) {
    int pivot = entiers[last];
    int i = (first - 1);

    for (int j=first; j < last; j++) {
        if (entiers[j] <= pivot) {
        i++;
        swap(&entiers[i], &entiers[j]);
        swap(&indices[i], &indices[j]);
        }
    }

    swap(&entiers[i+1], &entiers[last]);
    swap(&indices[i+1], &indices[last]);
    return (i+1);
}

void omegaQuickSort(uint64_t indices[], uint64_t entiers[], int first, int last) {
    if (first < last) {
        int pivot = omegaPartition(indices, entiers, first, last);
        omegaQuickSort(indices, entiers, first, pivot - 1);
        omegaQuickSort(indices, entiers, pivot + 1, last);
    }
}

uint64_t * quickSortChunks(uint64_t * chunksList[], int nbChunks){
    uint64_t * indice_list = (uint64_t *) malloc(sizeof(uint64_t)* nbChunks);
    uint64_t first_elem_list[nbChunks];
    for(int i=0; i<nbChunks; i++){
        indice_list[i] = i;
        first_elem_list[i] = chunksList[i][0];
    }
    omegaQuickSort(indice_list, first_elem_list, 0, nbChunks-1);
    return indice_list;
}

void parallel_quickSort_V3 (uint64_t *T, int size){
    int nbChunks = MAX_THREADS < size ? MAX_THREADS : size;
    uint64_t chunkSize = size / nbChunks;
    uint64_t lastChunkSize = chunkSize + size%nbChunks;

    uint64_t * chunksList[nbChunks];
    for(int i=0; i<nbChunks; i++){
        chunksList[i] = &T[i*chunkSize];
    }

    int condition = 1;
    int ind_chunk;
    int nb = 0;
    uint64_t * resultat = (uint64_t *) malloc(size * sizeof(uint64_t));
    omp_set_num_threads(nbChunks);  //better performance with this line

    #pragma omp parallel default (none) shared(nbChunks, chunksList, chunkSize, lastChunkSize, nb, resultat) private(ind_chunk, size)
    {
        //here we're gonna apply one bubble to every chunk
        #pragma omp for 
        for(ind_chunk = 0; ind_chunk < nbChunks; ind_chunk++){
            int taille = ind_chunk == nbChunks - 1 ? lastChunkSize : chunkSize;
            sequential_quickSort(chunksList[ind_chunk], 0, taille -1);
        }

        
        
        //so at this point the chunks are all sorted. What we're gonna do is make an array of the
        //first elements of each array and quicksort it along side an array of index to keep track 
        //of the change. Then using the index, we assemble the chunks in the right order and we win
        uint64_t * indices = quickSortChunks(chunksList, nbChunks);

        //Maintenant on reconstruit l'array résultat

        #pragma omp for 
        for(ind_chunk = nbChunks -1; ind_chunk > -1; ind_chunk--){
            int indice = indices[ind_chunk];
            int taille = ind_chunk == nbChunks - 1 ? lastChunkSize : chunkSize;
            uint64_t * emplacement = resultat;

            if(indice <= indices[nbChunks - 1]){
                emplacement += indice * chunkSize;
            }else{
                emplacement += (indice-1) * chunkSize + lastChunkSize;
            }
            memcpy(emplacement , chunksList[ind_chunk], taille * sizeof(uint64_t));
        }
        
    }

    memcpy(T, resultat, size * sizeof(uint64_t));

    return;
}
//===============================================

void sequential_quick_sort (uint64_t *T, int size){
    sequential_quickSort(T, 0, size-1);
    return;
}

void parallel_quick_sort (uint64_t *T, const uint64_t size)
{
    if (PARALLEL_V == 1 || PARALLEL_V == 2){
        #pragma omp parallel 
        {
            if (PARALLEL_V == 1){
                #pragma omp single
                parallel_quickSort_V1(T, 0, size-1);
            }else{
                #pragma omp single
                parallel_quickSort_V2(T, 0, size-1) ;
            }
        }
    }else{
        parallel_quickSort_V3(T, size) ;
    }
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

    printf ("\n quick-sort serial \t\t\t %.2lf Mcycles\n\n", (double)av/1000000) ;

  
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