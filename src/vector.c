/*
 * Implementation of vector utilities
 * Author: Jaffar Alzeidi
 */

#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

/*
 * This function is to be called right before an attempt to access an index which not 
 * guaranteed to be part of the array's memory space.
 * After this call, access to said index is safe, it is now part of the memory space.
 * Note: 'pos' - 1 must be known in advance to be within the vector's memory space, otherwise
 * this function does not guarantee that the resize will be sufficient.
 */

void *check_vector(void *vector, int *capacity, int pos) {
    if(pos >= *capacity) {
        *capacity *= 2;
        void *temp = realloc(vector, *capacity * sizeof(void *));
	    if(temp == NULL) {
	        free(vector);
	        perror("realloc");
	        exit(1);
	    }
        return temp;
    }
    return vector;
}
