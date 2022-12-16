/*
 * Utility to resize array of pointers
 * Author: Jaffar Alzeidi
 */

/* 
 * @param vector: pointer to the start of an allocated memory space
 * @param capacity: maximum elements that @param vector can hold currently
 * @param pos: current index in the vector, where we want to place a new element
 * WARNING: If vector + pos - 1 is not a valid allocated region of vector, there is no guarantee that
 * the resize results in a vector large enough to access vector[pos]
 */
void *check_vector(void *vector, int *capacity, int pos);
