#include <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "leak_detector_c.h"

// Configuration
#define CONFIG_INFILE_NAME "in.txt"
#define CONFIG_OUTFILE_NAME "out.txt"
#define CONFIG_NUM_LANES 12
#define CONFIG_MAX_TEST_CASES 25
#define CONFIG_MAX_CUSTOMERS 500000
#define CONFIG_MAX_CUST_ITEMS 100
#define CONFIG_MAX_NAME_LEN 10
#define CONFIG_MAX_TIME 1000000000

// debugf.h
// (c) Charlton Trezevant - 2018
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_MMGR 1
#define DEBUG_LEVEL_TRACE 2
#define DEBUG_LEVEL_INFO 3
#define DEBUG_LEVEL_NONE 1000

//#define DEBUG DEBUG_LEVEL_ALL
#ifdef DEBUG
#define debugf(lvl, fmt, ...) \
        ({ \
                if (DEBUG == 0 || (lvl) >= DEBUG) { \
                        fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); \
                } \
        })
#else
  #define debugf(lvl, fmt, ...) ((void)0)
#endif

typedef struct MMGR_Entry MMGR_Entry;
typedef struct MMGR MMGR;

MMGR *mmgr_init();
void *mmgr_malloc(MMGR *tbl, size_t size);
void mmgr_free(MMGR *tbl, void* handle);
void mmgr_cleanup(MMGR *tbl);
void mmgr_mutex_acquire(MMGR *tbl);
void mmgr_mutex_release(MMGR *tbl);

// Global MMGR instance
MMGR* g_MEM;

// Utility
int stdout_enabled = 1;
FILE *g_outfp;

void panic(const char * format, ...);
void write_out(const char * format, ...);

////////////////////////// Entry //////////////////////////

int main(void){
  atexit(report_mem_leak); //Ahmed's memory leak detection
  printf("Assignment 2!\n");
}

////////////////////////// Util //////////////////////////

// Panic is called when something goes wrong and we have to die for the greater good
void panic(const char * fmt, ...){
        // Vargs to behave like printf (but not to make black metal)
        va_list vargs;
        va_start(vargs, fmt);
        debugf(DEBUG_LEVEL_ALL, "panic called\n");
        // Print error message to stderr
        vfprintf(stderr, fmt, vargs);
        fflush(stderr);
        va_end(vargs);
        // Clean up any allocated memory and exit
        mmgr_cleanup(g_MEM);
        exit(1);
}

// Write_out prints messages to the global output file
void write_out(const char * fmt, ...){
        va_list vargs;
        va_start(vargs, fmt);
        // Print message to outfile
        if(g_outfp != NULL) {
                vfprintf(g_outfp, fmt, vargs);
                fflush(g_outfp);
        }
        // if stdout is enabled (in the case of testing), print msg to stdout as well
        if(stdout_enabled == 1) {
                vfprintf(stdout, fmt, vargs);
                fflush(stdout);
        }
        va_end(vargs);
}

////////////////////////// Memory manager //////////////////////////

// This is the initial version of my C memory manager.
// At the moment it's very rudimentary.

typedef struct MMGR_Entry {
        void* handle;
        size_t size;
        // todo: gc via refcounts or mark+sweep
        // allow entries to be marked/given affinities for batched release
} MMGR_Entry;

typedef struct MMGR {
        // todo: benchmark current realloc strat vs lili vs pointer hash table
        MMGR_Entry **entries;
        int numEntries;
        int *free; // available recyclable slots
        int numFree;
        volatile int mutex;
} MMGR;

// Initializes the memory manager's global state table. This tracks all allocated
// memory, reallocates freed entries, and ensures that all allocated memory is
// completely cleaned up on program termination.

// In the future, MMGR will spawn a worker thread to perform automated garbage
// collection through mark and sweep or simple refcounting. It'll also be
// refactored to utilize hashmaps once I figure out how to hash pointers.
MMGR *mmgr_init(){
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
void mmgr_mutex_acquire(MMGR *tbl){
        while (tbl->mutex == 1);
        tbl->mutex = 1;
}

void mmgr_mutex_release(MMGR *tbl){
        tbl->mutex = 0;
}

// Allocate memory and maintain a reference in the memory manager's state table
// If any previously allocated entries have since been freed, these will be
// resized and reallocated to serve the request
void *mmgr_malloc(MMGR *tbl, size_t size){
        if(tbl == NULL)
                return NULL;

        // State table access is atomic. You'll see this around a lot.
        mmgr_mutex_acquire(tbl);

        void* handle = NULL;

        // If freed entries are available, we can recycle them
        if(tbl->numFree > 0) {
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
                tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));
                tbl->numFree--;

                debugf(DEBUG_LEVEL_MMGR, "mmgr: reallocated %lu bytes\n", size);

                // Otherwise the state table will need to be resized to accommodate a new
                // entry
        } else {
                debugf(DEBUG_LEVEL_MMGR, "mmgr: no recyclable entries available, increasing table size\n");

                // Resize occupied entry table to accommodate an additional entry
                tbl->entries = (MMGR_Entry**) realloc(tbl->entries, (sizeof(MMGR_Entry*) * (tbl->numEntries + 1)));

                // Allocate a new entry to include in the state table
                tbl->entries[tbl->numEntries] = (MMGR_Entry*) calloc(1, sizeof(MMGR_Entry) + sizeof(void*));

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
void mmgr_free(MMGR *tbl, void* handle){
        if(tbl == NULL)
                return;

        if(handle == NULL) {
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
        for(int i = tbl->numEntries; i--> 0; ) {
                if(tbl->entries[i]->handle == handle) {
                        debugf(DEBUG_LEVEL_MMGR, "mmgr: found handle %p at index %d\n", handle, i);

                        tbl->numFree++;

                        // Free and nullify table entry
                        free(tbl->entries[i]->handle);
                        tbl->entries[i]->handle = NULL;
                        tbl->entries[i]->size = 0;

                        // Resize free entry list, ad freed entry
                        tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
                        tbl->free[tbl->numFree - 1] = i;

                        found = 1;

                        debugf(DEBUG_LEVEL_MMGR, "mmgr: freed %p at index %d, %d entries remain active\n", handle, i, (tbl->numEntries - tbl->numFree));

                }
        }

        if(found == 0)
                debugf(DEBUG_LEVEL_MMGR, "mmgr: called to free %p but couldn't find it, no-op\n", handle);

        mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl){
        if(tbl == NULL)
                return;

        mmgr_mutex_acquire(tbl);

        int deEn = 0;

        debugf(DEBUG_LEVEL_MMGR, "mmgr: cleaning up %d active entries\n", (tbl->numEntries - tbl->numFree));

        // Free all active entries and what they're pointing to
        for(int i = 0; i < tbl->numEntries; i++) {
                if(tbl->entries[i] != NULL) {
                        if(tbl->entries[i]->handle != NULL)
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
