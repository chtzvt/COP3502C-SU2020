/*
   COP 3502C Assignment 4
   This program is written by: Charlton Trezevant
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leak_detector_c.h"

////////////////////////// Global Project Configuation //////////////////////////
#define CONFIG_INFILE_NAME "in.txt"
#define CONFIG_OUTFILE_NAME "out.txt"
#define CONFIG_MAX_TIME 10E9
#define CONFIG_MAX_TASK_TIME 10E9
#define CONFIG_MAX_TASK_PHASES 100
#define CONFIG_MAX_TASK_PHASE_CUMUL_TIME 10E6
#define CONFIG_HEAP_INIT_SIZE 10

// Global debug levels
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_TRACE 1
#define DEBUG_LEVEL_INFO 2

// Functionality-specific debug levels
// These enable debug output only for specific sections of the program
#define DEBUG_TRACE_MMGR -7

// Current debug level set/disable
//#define DEBUG DEBUG_LEVEL_ALL

////////////////////////// Assignment 5 Prototypes and Globals //////////////////////////

typedef struct task {
  int id;
  int time_assigned;
  int num_phases;
  int **phase_lengths;
  int time_left;
  int next_phase;
  int priority;
} task;

task EMPTY_TASK;

task *task_create(int id, int time_assigned, int num_phases, int time_left, int next_phase);
void task_destroy(task *t);
void task_run_next_phase(task *task);
int task_compare(task *t1, task *t2); // comparison

typedef struct task_heap {
  task **tasks;
  int capacity;
  int size;
} task_heap;

task *heap_pop_min(task_heap *heap);
void heap_insert(task_heap *heap, task *t);
int heap_min(task *a, int ia, task *b, int ib);
void heap_swap(task_heap *heap, int i1, int i2);
void heap_heapify(task_heap *heap);
void heap_perc_down(task_heap *heap, int i);
void heap_perc_up(task_heap *heap, int i);
task_heap *heap_init_array(task **tasks, int n);
void heap_destroy(task_heap *heap);
task_heap *heap_init();

task *sched_poll(task_heap *heap); // get next from pq
void sched_add(task *t);           // add to pq

////////////////////////// Debug Output //////////////////////////
// (c) Charlton Trezevant - 2018

#ifdef DEBUG
#define debugf(lvl, fmt, ...)                                                   \
  ({                                                                            \
    if (DEBUG == 0 || (lvl > 0 && lvl >= DEBUG) || (lvl < 0 && lvl == DEBUG)) { \
      fprintf(stderr, fmt, ##__VA_ARGS__);                                      \
      fflush(stderr);                                                           \
    }                                                                           \
  })
#else
#define debugf(lvl, fmt, ...) ((void)0)
#endif

////////////////////////// Memory Manager Prototypes //////////////////////////
// (c) Charlton Trezevant - 2020

typedef struct MMGR_Entry {
  void *handle;
  size_t size;
} MMGR_Entry;

typedef struct MMGR {
  MMGR_Entry **entries;
  int numEntries;
  int *free; // available recyclable slots
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

////////////////////////// Utility //////////////////////////
FILE *g_outfp = NULL; // output file pointer (for test harness compatibility)

void panic(const char *format, ...);
void write_out(const char *format, ...);

////////////////////////// Entry //////////////////////////

int main(int argc, char **argv) {
  debugf(DEBUG_LEVEL_TRACE, "Ahmed memory leak detector init.\n");
  atexit(report_mem_leak); //Ahmed's memory leak detection

  // Initialize MMGR
  debugf(DEBUG_LEVEL_TRACE, "MMGR init.\n");
  g_MEM = mmgr_init();

  // You can override the default input filename if you'd like by
  // passing a command line argument
  // This is for my own use in an automated testing harness and will not impact any
  // tests performed by TAs
  FILE *infile;
  if (argc > 1 && strcmp(argv[1], "use_default") != 0) {
    infile = fopen(argv[1], "r");
    debugf(DEBUG_LEVEL_INFO, "Input file name: %s\n", argv[1]);
  } else {
    infile = fopen(CONFIG_INFILE_NAME, "r");
    debugf(DEBUG_LEVEL_INFO, "Input file name: %s\n", CONFIG_INFILE_NAME);
  }

  // Panic if opening the input file failed
  if (infile == NULL) {
    panic("Failed to open input file\n");
    return 1;
  }

  // You can override the default output file name if you'd like to send output to either
  // a different file or to stdout
  // This is for my own use in an automated testing harness and will not impact any
  // tests performed by TAs
  if (argc > 2) {
    if (strcmp(argv[2], "stdout") == 0) {
      debugf(DEBUG_LEVEL_INFO, "Writing output to stdout\n");
    } else {
      g_outfp = fopen(argv[2], "w+");
      debugf(DEBUG_LEVEL_INFO, "Output file name: %s\n", argv[2]);
    }
  } else {
    g_outfp = fopen(CONFIG_OUTFILE_NAME, "w+");
    debugf(DEBUG_LEVEL_INFO, "Output file name: %s\n", CONFIG_OUTFILE_NAME);
  }

  // Read in the number of commands to process
  int n_cmds = 0;
  if (!feof(infile)) {
    fscanf(infile, "%d", &n_cmds);
  } else {
    printf("invalid input file format: number of commands unknown\n");
    return 1;
  }

  // Command processing loop
  for (int i = 0; i < n_cmds; i++) {
    // Input buffer and temporary variables for use
    // in command processing
    char str[50];
    int cmd_type, num_insertions;

    if (!feof(infile)) {
      // Read command type
      fscanf(infile, "%d", &cmd_type);
      debugf(DEBUG_LEVEL_TRACE, "Command type: %d\n", cmd_type);

      switch (cmd_type) {
      case 1: // Insertion
        fscanf(infile, "%s %d", str, &num_insertions);
        debugf(DEBUG_LEVEL_TRACE, "Insert: %s %d times\n", str, num_insertions);
        break;

      case 2: // Queries
        fscanf(infile, "%s", str);

        debugf(DEBUG_LEVEL_TRACE, "Running suggest on '%s', got '%s'\n", str, found);
        break;
      }
    }
  }

  // Clean up allocated memory
  debugf(DEBUG_LEVEL_TRACE, "MMGR Cleanup.\n");
  mmgr_cleanup(g_MEM); // #noleaks

  // Close input and output files
  debugf(DEBUG_LEVEL_TRACE, "Infile close.\n");
  fclose(infile);

  debugf(DEBUG_LEVEL_TRACE, "Outfile close.\n");
  if (g_outfp != NULL)
    fclose(g_outfp);

  debugf(DEBUG_LEVEL_TRACE, "Exiting.\n");
  return 0;
}

task_heap *heap_init() {
  task_heap *heap = mmgr_malloc(g_MEM, sizeof(task_heap));
  heap->capacity = CONFIG_HEAP_INIT_SIZE;
  heap->size = 0;
  heap->tasks = mmgr_malloc(g_MEM, sizeof(task) * (CONFIG_HEAP_INIT_SIZE + 1));
  return heap;
}

void heap_destroy(task_heap *heap) {
  free(heap->tasks);
  free(heap);
}

task_heap *heap_init_array(task **tasks, int n) {
  task_heap *heap;

  heap = mmgr_malloc(g_MEM, sizeof(task_heap));
  heap->tasks = mmgr_malloc(g_MEM, sizeof(task) * (n + 1));
  heap->size = n;

  for (int i = 1; i < n + 1; i++)
    heap->tasks[i] = tasks[i];

  heap_heapify(heap);
  return heap;
}

void heap_perc_up(task_heap *heap, int i) {
  if (i < 1)
    return;

  if (task_compare(heap->tasks[i / 2], heap->tasks[i]) > 0) {
    heap_swap(heap, i, i / 2);
    heap_perc_up(heap, i / 2);
  }
}

void heap_perc_down(task_heap *heap, int i) {
  if ((2 * i + 1) <= heap->size) {
    int min = heap_min(heap->tasks[2 * i], 2 * i, heap->tasks[2 * i + 1], 2 * i + 1);
    if (task_compare(heap->tasks[i], heap->tasks[min]) > 0) {
      heap_swap(heap, i, min);
      heap_perc_down(heap, min);
    }
  }

  else if (heap->size == 2 * i) {
    if (task_compare(heap->tasks[i], heap->tasks[2 * i]) > 0)
      heap_swap(heap, i, 2 * i);
  }
}

void heap_heapify(task_heap *heap) {
  for (int i = heap->size / 2; i > 0; i--)
    heap_perc_down(heap, i);
}

void heap_swap(task_heap *heap, int i1, int i2) {
  task *temp = heap->tasks[i1];
  heap->tasks[i1] = heap->tasks[i2];
  heap->tasks[i2] = temp;
}

int heap_min(task *a, int ia, task *b, int ib) {
  if (task_compare(a, b) < 0)
    return ia;

  return ib;
}

void heap_insert(task_heap *heap, task *t) {
  if (heap->size == heap->capacity) {
    task **tasks_realc = mmgr_malloc(g_MEM, sizeof(task) * (2 * heap->capacity + 1));
    for (int i = 0; i < heap->size; i++) {
      tasks_realc[i] = heap->tasks[i];
    }

    mmgr_free(g_MEM, heap->tasks);
    heap->tasks = tasks_realc;

    heap->capacity *= 2;
  }

  heap->size++;
  heap->tasks[heap->size] = t;
  heap_perc_up(heap, heap->size);
}

task *heap_pop_max(task_heap *heap) {
  if (heap->size <= 0)
    return &EMPTY_TASK;

  task *max = heap->tasks[1];
  heap->tasks[1] = heap->tasks[heap->size];
  heap->size--;

  heap_perc_up(heap, 1);

  return max;
}

////////////////////////// Util //////////////////////////

// Panic is called when something goes wrong and we have to die for the greater good
void panic(const char *fmt, ...) {
  // Vargs to behave like printf (but not for black metal)
  va_list vargs;
  va_start(vargs, fmt);
  debugf(DEBUG_LEVEL_ALL, "panic called\n");
  // Print error message to stderr
  vfprintf(stderr, fmt, vargs);
  fflush(stderr);
  va_end(vargs);
  // Politely clean up any allocated memory and exit
  mmgr_cleanup(g_MEM);
  exit(1);
}

// Write_out prints messages to the global output file
// This shim is more for my own use as I require stdout for compatibility with my
// automated test harness
void write_out(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);

  if (g_outfp == NULL)
    g_outfp = stdout;

  vfprintf(g_outfp, fmt, vargs);
  fflush(g_outfp);

  va_end(vargs);
}

////////////////////////// Memory manager //////////////////////////

// Initializes the memory manager's global state table. This tracks all allocated
// memory, reallocates freed entries, and ensures that all allocated memory is
// completely cleaned up on program termination.
MMGR *mmgr_init() {
  MMGR *state_table = calloc(1, sizeof(MMGR));

  state_table->free = NULL;
  state_table->numFree = 0;

  state_table->entries = NULL;
  state_table->numEntries = 0;

  state_table->mutex = 0;

  debugf(DEBUG_TRACE_MMGR, "mmgr: initialized\n");

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

    debugf(DEBUG_TRACE_MMGR, "mmgr: found reusable previously allocated entry %d\n", tgt_idx);

    // Update table entry with new pointer and size, memset to 0
    tbl->entries[tgt_idx]->size = size;
    tbl->entries[tgt_idx]->handle = malloc(size);
    memset(tbl->entries[tgt_idx]->handle, 0, size);

    // Copy freshly allocated region pointer to handle
    handle = tbl->entries[tgt_idx]->handle;

    // Resize free index array
    tbl->free = (int *)realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));
    tbl->numFree--;

    debugf(DEBUG_TRACE_MMGR, "mmgr: reallocated %lu bytes\n", size);

    // Otherwise the state table will need to be resized to accommodate a new
    // entry
  } else {
    debugf(DEBUG_TRACE_MMGR, "mmgr: no recyclable entries available, increasing table size\n");

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

    debugf(DEBUG_TRACE_MMGR, "mmgr: allocated %lu bytes, handle is %p\n", size, handle);
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
    debugf(DEBUG_TRACE_MMGR, "mmgr: provided NULL handle! no-op\n");
    return;
  }

  mmgr_mutex_acquire(tbl);

  // Whether the provided pointer exists in the MMGR state table
  int found = 0;

  debugf(DEBUG_TRACE_MMGR, "mmgr: num active entries is %d, called to free %p\n", (tbl->numEntries - tbl->numFree), handle);

  // Search backwards through the state table for the provided pointer
  // The idea being that the memory you allocate the last will be the first
  // you want to free (e.g. struct members, then structs)
  // Eventually this will be overhauled with hash maps
  for (int i = tbl->numEntries; i-- > 0;) {
    if (tbl->entries[i]->handle == handle) {
      debugf(DEBUG_TRACE_MMGR, "mmgr: found handle %p at index %d\n", handle, i);

      tbl->numFree++;

      // Free and nullify table entry
      free(tbl->entries[i]->handle);
      tbl->entries[i]->handle = NULL;
      tbl->entries[i]->size = 0;

      // Resize free entry list, ad freed entry
      tbl->free = (int *)realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
      tbl->free[tbl->numFree - 1] = i;

      found = 1;

      debugf(DEBUG_TRACE_MMGR, "mmgr: freed %p at index %d, %d entries remain active\n", handle, i, (tbl->numEntries - tbl->numFree));
    }
  }

  if (found == 0)
    debugf(DEBUG_TRACE_MMGR, "mmgr: called to free %p but couldn't find it, no-op\n", handle);

  mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl) {
  if (tbl == NULL)
    return;

  mmgr_mutex_acquire(tbl);

  int deEn = 0;

  debugf(DEBUG_TRACE_MMGR, "mmgr: cleaning up %d active entries\n", (tbl->numEntries - tbl->numFree));

  // Free all active entries and what they're pointing to
  for (int i = 0; i < tbl->numEntries; i++) {
    if (tbl->entries[i] != NULL) {
      if (tbl->entries[i]->handle != NULL)
        free(tbl->entries[i]->handle);

      free(tbl->entries[i]);
      deEn++;
    }
  }

  debugf(DEBUG_TRACE_MMGR, "mmgr: cleanup deallocd %d entries\n", deEn);

  // Deallocate occupied + free table entries, table itself
  free(tbl->entries);
  free(tbl->free);
  free(tbl);

  tbl = NULL;
}
