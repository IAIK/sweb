#include "types.h"
#include "assert.h"

/**
 * A very simple binary (memory-only) swap operation for elements of arbitrary size (in bytes)
 * Chosen this way to be a) simple, b) understandable, not to be the fastest or smartest possible.
 * @param a pointer to first element
 * @param a pointer to the second element
 * @param the size fo the elements in bytes
 */
static inline void swap_elements(char* a, char* b, size_t size)
{
  if(a == b) return;

  for(size_t i = 0; i < size; i++)
  {
    char* a_c = (char*) a + i;
    char* b_c = (char*) b + i;

    char tmp = *a_c;
    *a_c = *b_c;
    *b_c = tmp;
  }
}

/**
 * A simple quicksort partitioning function.
 * @param low A pointer to the beginning of the first element
 * @param high A pointer to the beginning of the last element
 * @param size size of each element
 * @param compar function pointer to a comparison function
 * @return pointer to the final position of the partitioning element
 */
static char* partition(char* low, char* high, size_t size, int (*compar)(const void *, const void*))
{
  char* swapto_ptr = low - size;
  char* pivot_ptr   = high;

  for(char* curr_element = low; curr_element < high; curr_element += size)
  {
    if(compar(curr_element, pivot_ptr) <= 0)
    {
      swapto_ptr += size;
      swap_elements(curr_element, swapto_ptr, size);
    }
  }

  swapto_ptr += size;
  swap_elements(swapto_ptr, high, size);
  return swapto_ptr;
}

/**
 * A simple quicksort main function
 * It partitions the array, then calls itself on the partitions
 * @param low A pointer to the beginning of the first element
 * @param high A pointer to the beginning of the last element
 * @param size size of each element
 * @param compar function pointer to a comparison function
 */
static void quick_sort(char* low, char* high, size_t size, int (*compar)(const void *, const void*))
{
  if(low >= high) return;

  char* part_ptr = partition(low, high, size, compar);

  quick_sort(low, part_ptr - size, size, compar);
  quick_sort(part_ptr + size, high, size, compar);
}

void qsort(void* base, size_t nitems, size_t size, int (*compar)(const void *, const void*))
{
  assert(base);
  assert(size);
  assert(compar);

  quick_sort((char*) base, ((char*)base) + (nitems - 1) * size, size, compar);
  return;
}
