#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

//////////////////////////////////////////////

typedef struct MMGR_Entry {
  void *handle;
  size_t size;
} MMGR_Entry;

typedef struct MMGR {
  MMGR_Entry **entries;
  int numEntries;
  int *free;
  int numFree;
  volatile int mutex;
} MMGR;

MMGR *mmgr_init();
void *mmgr_malloc(MMGR *tbl, size_t size);
void mmgr_free(MMGR *tbl, void *handle);
void mmgr_cleanup(MMGR *tbl);
void mmgr_mutex_acquire(MMGR *tbl);
void mmgr_mutex_release(MMGR *tbl);
MMGR *g_MEM;
///////////////////////////////////////////////

void fill_rand(int *nums, int len);
int *arr_copy(int *src, int len);
long timer(int *arr, int len, void sort(int *, int));
void ms__merge(int *p1, int p1_len, int *p2, int p2len, int *dst);
void merge_sort(int *points, int len);
void insertion_sort(int *points, int len);
void selectionSort(int *arr, int n);
void bubbleSort(int *arr, int n);
void swap(int *a, int *b);
int partition(int *vals, int low, int high);
void quickSort(int *numbers, int low, int high);
void quick_sort(int *arr, int len);

#define NUM_TESTS 6
const int TEST_SIZES[NUM_TESTS] = {10000, 20000, 30000, 40000, 50000, 100000
}
;

typedef struct test {
  const char *name;
  void (*func)(int *, int);
} test;

#define NUM_FUNCS 5
const test TEST_FUNCS[NUM_FUNCS] = {
    {"Quick Sort", &quick_sort},
    {"Bubble Sort", &bubbleSort},
    {"Selection Sort", &selectionSort},
    {"Insertion Sort", &insertion_sort},
    {"Merge Sort", &merge_sort},
};

int main() {
  printf("YEEHAW\n");
  int **tests = mmgr_malloc(g_MEM, sizeof(int *) * NUM_TESTS);
  printf("YEEHAW2\n");
  for (int i = 0; i < NUM_TESTS; i++) {
    tests[i] = mmgr_malloc(g_MEM, sizeof(int) * TEST_SIZES[i]);
    printf("YEEHAW3\n");
    fill_rand(tests[i], TEST_SIZES[i]);
  }

  printf("YEEHAW4\n");

  for (int i = 0; i < NUM_TESTS; i++) {
    for (int j = 0; j < NUM_FUNCS; j++) {
      int *tmp = arr_copy(tests[i], TEST_SIZES[i]);
      printf("Sorting %d values takes %lu milliseconds for %s\n", TEST_SIZES[i], timer(tmp, TEST_SIZES[i], TEST_FUNCS[j].func), TEST_FUNCS[j].name);
    }
  }
}

void fill_rand(int *nums, int len) {
  if (nums == NULL)
    return;

  for (int i = 0; i < len; i++)
    nums[i] = rand() % 1000 + 1;
}

int *arr_copy(int *src, int len) {
  int *tmp = mmgr_malloc(g_MEM, sizeof(int) * len);

  for (int i = 0; i < len; i++)
    tmp[i] = src[i];

  return tmp;
}

long timer(int *arr, int len, void sort(int *, int)) {
  clock_t start, end;
  start = clock();
  sort(arr, len);
  end = clock();
  return ((double)end - start) / CLOCKS_PER_SEC * 1000;
}

// Merge sort helper
void ms__merge(int *p1, int p1_len, int *p2, int p2len, int *dst) {
  int i1 = 0, i2 = 0, j = 0;

  // I didn't bother with making multiple temp arrays. Instead,
  // the p1 and p2 partitions are actually split out of the
  // whole input array, and the sorted values are then copied
  // to a fresh destination array.
  while (i1 < p1_len && i2 < p2len) {
    if (p1[i1] < p2[i2]) {
      dst[j++] = p1[i1++];
    } else {
      dst[j++] = p2[i2++];
    }
  }

  // Any remaining elements in the source array are
  // copied to the destination
  while (i1 < p1_len)
    dst[j++] = p1[i1++];

  while (i2 < p2len)
    dst[j++] = p2[i2++];
}

void merge_sort(int *points, int len) {
  if (points == NULL || len < 2)
    return;

  // Divide and conquer!
  // First, we've gotta sort the first and second halves of the input
  merge_sort(points, len / 2);
  merge_sort(points + (len / 2), len - (len / 2));

  // Next we instantiate the destination array and merge the sorted partitions
  int *tmp = mmgr_malloc(g_MEM, len * sizeof(int *));
  ms__merge(points, len / 2, points + (len / 2), len - (len / 2), tmp);

  // Overwrite the contents of the original unsorted array with the sorted
  // contents of the temporary array that we just filled up
  for (int i = 0; i < len; i++)
    points[i] = tmp[i];

  // Free the temporary array
  mmgr_free(g_MEM, tmp);
}

void insertion_sort(int *points, int len) {
  int val;
  int j;
  for (int i = 1; i < len; i++) {
    val = points[i];
    j = i;
    while (j > 0 && points[j - 1] > val) {
      points[j] = points[j - 1];
      j--;
    }
    points[j] = val;
  }
}

void selectionSort(int *arr, int n) {
  int i, j, min_idx, temp;
  // One by one move boundary of unsorted subarray
  for (i = 0; i < n - 1; i++) {
    // Find the minimum element in unsorted array
    min_idx = i;
    for (j = i + 1; j < n; j++)
      if (arr[j] < arr[min_idx])
        min_idx = j;
    // Swap the found minimum element with the first element
    temp = arr[i];
    arr[i] = arr[min_idx];
    arr[min_idx] = temp;
  }
}

void bubbleSort(int *arr, int n) {
  int i, j, temp;
  for (i = 0; i < n - 1; i++) {
    for (j = 0; j < n - i - 1; j++) {

      if (arr[j] > arr[j + 1]) { //then swap
        temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

// Swaps the values pointed to by a and b.
void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

// Pre-condition: low and high are valid indexes into values
// Post-condition: Returns the partition index such that all the values
//                 stored in vals from index low to until that index are
//                 less or equal to the value stored there and all the values
//                 after that index until index high are greater than that
//
int partition(int *vals, int low, int high) {
  // Pick a random partition element and swap it into index low.
  int i = low + rand() % (high - low + 1);
  swap(&vals[low], &vals[i]);

  int lowpos = low; //here is our pivot located.

  low++; //our starting point is after the pivot.

  // Run the partition so long as the low and high counters don't cross.
  while (low <= high) {
    // Move the low pointer until we find a value too large for this side.
    while (low <= high && vals[low] <= vals[lowpos])
      low++;

    // Move the high pointer until we find a value too small for this side.
    while (high >= low && vals[high] > vals[lowpos])
      high--;

    // Now that we've identified two values on the wrong side, swap them.
    if (low < high)
      swap(&vals[low], &vals[high]);
  }

  // Swap the pivot element element into its correct location.
  swap(&vals[lowpos], &vals[high]);

  return high; //return the partition point
}

// Pre-condition: s and f are value indexes into numbers.
// Post-condition: The values in numbers will be sorted in between indexes s
//                 and f.
void quickSort(int *numbers, int low, int high) {

  // Only have to sort if we are sorting more than one number
  if (low < high) {
    int split = partition(numbers, low, high);
    quickSort(numbers, low, split - 1);
    quickSort(numbers, split + 1, high);
  }
}

void quick_sort(int *arr, int len) {
  quickSort(arr, 0, len);
}

///////////////////// Memory Manager /////////////////////

#ifndef DEBUG_LEVEL_MMGR
#define DEBUG_LEVEL_MMGR 0
#endif

#ifndef DEBUG
#define debugf(lvl, fmt, ...) ((void)0)
#endif

////////////////////////// Memory manager //////////////////////////

// This is the initial version of my C memory manager.
// At the moment it's very rudimentary.

// Initializes the memory manager's global state table. This tracks all allocated
// memory, reallocates freed entries, and ensures that all allocated memory is
// completely cleaned up on program termination.

// In the future, MMGR will spawn a worker thread to perform automated garbage
// collection through mark and sweep or simple refcounting. It'll also be
// refactored to utilize hashmaps once I figure out how to hash pointers.
MMGR *mmgr_init() {
  MMGR *state_table = calloc(1, sizeof(MMGR));

  state_table->free = NULL;
  state_table->numFree = 0;

  state_table->entries = NULL;
  state_table->numEntries = 0;

  state_table->mutex = 0;

  debugf(DEBUG_LEVEL_MMGR, "mmgr: initialized\n");

  return state_table;
}

// Mutex acquisition/release helpers to atomicize access to the memory manager's
// state table. Not really necessary atm but eventually I might play with threading
void mmgr_mutex_acquire(MMGR *tbl) {
  while (tbl->mutex == 1)
    ;
  tbl->mutex = 1;
}

void mmgr_mutex_release(MMGR *tbl) {
  tbl->mutex = 0;
}

// Allocate memory and maintain a reference in the memory manager's state table
// If any previously allocated entries have since been freed, these will be
// resized and reallocated to serve the request
void *mmgr_malloc(MMGR *tbl, size_t size) {
  if (tbl == NULL)
    return NULL;

  // State table access is atomic. You'll see this around a lot.
  mmgr_mutex_acquire(tbl);

  void *handle = NULL;

  // If freed entries are available, we can recycle them
  if (tbl->numFree > 0) {
    // Fetch index of last reallocatable table entry
    int tgt_idx = tbl->free[tbl->numFree - 1];

    debugf(DEBUG_LEVEL_MMGR, "mmgr: found reusable previously allocated entry %d\n", tgt_idx);

    // Update table entry with new pointer and size, memset to 0
    tbl->entries[tgt_idx]->size = size;
    tbl->entries[tgt_idx]->handle = malloc(size);
    memset(tbl->entries[tgt_idx]->handle, 0, size);

    // Copy freshly allocated region pointer to handle
    handle = tbl->entries[tgt_idx]->handle;

    // Resize free index array
    tbl->free = (int *)realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));
    tbl->numFree--;

    debugf(DEBUG_LEVEL_MMGR, "mmgr: reallocated %lu bytes\n", size);

    // Otherwise the state table will need to be resized to accommodate a new
    // entry
  } else {
    debugf(DEBUG_LEVEL_MMGR, "mmgr: no recyclable entries available, increasing table size\n");

    // Resize occupied entry table to accommodate an additional entry
    tbl->entries = (MMGR_Entry **)realloc(tbl->entries, (sizeof(MMGR_Entry *) * (tbl->numEntries + 1)));

    // Allocate a new entry to include in the state table
    tbl->entries[tbl->numEntries] = (MMGR_Entry *)calloc(1, sizeof(MMGR_Entry) + sizeof(void *));

    tbl->entries[tbl->numEntries]->handle = malloc(size);
    memset(tbl->entries[tbl->numEntries]->handle, 0, size);

    // Copy freshly allocated region pointer to handle
    handle = tbl->entries[tbl->numEntries]->handle;
    tbl->entries[tbl->numEntries]->size = size;

    tbl->numEntries++;

    debugf(DEBUG_LEVEL_MMGR, "mmgr: allocated %lu bytes, handle is %p\n", size, handle);
  }

  mmgr_mutex_release(tbl);

  return handle;
}

// Frees the provided pointer and checks out the active entry in the global
// state table so that it can be reallocated
void mmgr_free(MMGR *tbl, void *handle) {
  if (tbl == NULL)
    return;

  if (handle == NULL) {
    debugf(DEBUG_LEVEL_MMGR, "mmgr: provided NULL handle! no-op\n");
    return;
  }

  mmgr_mutex_acquire(tbl);

  // Whether the provided pointer exists in the MMGR state table
  int found = 0;

  debugf(DEBUG_LEVEL_MMGR, "mmgr: num active entries is %d, called to free %p\n", (tbl->numEntries - tbl->numFree), handle);

  // Search backwards through the state table for the provided pointer
  // The idea being that the memory you allocate the last will be the first
  // you want to free (e.g. struct members, then structs)
  // Eventually this will be overhauled with hash maps
  for (int i = tbl->numEntries; i-- > 0;) {
    if (tbl->entries[i]->handle == handle) {
      debugf(DEBUG_LEVEL_MMGR, "mmgr: found handle %p at index %d\n", handle, i);

      tbl->numFree++;

      // Free and nullify table entry
      free(tbl->entries[i]->handle);
      tbl->entries[i]->handle = NULL;
      tbl->entries[i]->size = 0;

      // Resize free entry list, ad freed entry
      tbl->free = (int *)realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
      tbl->free[tbl->numFree - 1] = i;

      found = 1;

      debugf(DEBUG_LEVEL_MMGR, "mmgr: freed %p at index %d, %d entries remain active\n", handle, i, (tbl->numEntries - tbl->numFree));
    }
  }

  if (found == 0)
    debugf(DEBUG_LEVEL_MMGR, "mmgr: called to free %p but couldn't find it, no-op\n", handle);

  mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl) {
  if (tbl == NULL)
    return;

  mmgr_mutex_acquire(tbl);

  int deEn = 0;

  debugf(DEBUG_LEVEL_MMGR, "mmgr: cleaning up %d active entries\n", (tbl->numEntries - tbl->numFree));

  // Free all active entries and what they're pointing to
  for (int i = 0; i < tbl->numEntries; i++) {
    if (tbl->entries[i] != NULL) {
      if (tbl->entries[i]->handle != NULL)
        free(tbl->entries[i]->handle);

      free(tbl->entries[i]);
      deEn++;
    }
  }

  debugf(DEBUG_LEVEL_MMGR, "mmgr: cleanup deallocd %d entries\n", deEn);

  // Deallocate occupied + free table entries, table itself
  free(tbl->entries);
  free(tbl->free);
  free(tbl);

  tbl = NULL;
}
