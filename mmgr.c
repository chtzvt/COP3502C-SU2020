////////////////////////// Memory manager //////////////////////////

// This is the initial version of my C memory manager.
// At the moment it's very rudimentary.

#include  <stdlib.h>
#include  <stdio.h>

#define DEBUG_LEVEL_MMGR 0
#define DEBUG 0
#ifdef DEBUG
#define debugf(lvl, fmt, ...)                           \
        ({                                              \
                if (DEBUG == 0 || (lvl) >= DEBUG) {     \
                        fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); \
                }                                       \
        })
#else
  #define debugf(lvl, fmt, ...) ((void)0)
#endif

// MMGR v0.01
// (c) Charlton Trezevant 2020
// Rave reviews:
// 10/10
//    - IGN
// ==3291== All heap blocks were freed -- no leaks are possible
//    - Valgrind
// ~ it's pronounced "mumger" ~
typedef struct MMGR_Entry {
        void* handle;
        size_t size;
        // todo: overhaul for gc via refcounts or mark+sweep
        // consider implementing marking/affinities for batched release
} MMGR_Entry;

typedef struct MMGR {
        // todo: benchmark current realloc strat vs lili vs pointer hash table
        MMGR_Entry **entries;
        int numEntries;
        int *free; // available recyclable slots
        int numFree;
        volatile int mutex;
} MMGR;

MMGR *mmgr_init();
void *mmgr_malloc(MMGR *tbl, size_t size);
void mmgr_free(MMGR *tbl, void* handle);
void mmgr_cleanup(MMGR *tbl);
void mmgr_mutex_acquire(MMGR *tbl);
void mmgr_mutex_release(MMGR *tbl);

// Initializes the memory manager's global state table. This tracks all allocated
// memory, reallocates freed entries, and ensures that all allocated memory is
// completely cleaned up on program termination.
// In the future, I'll spawn a worker thread to performs automated garbage
// collection through mark and sweep or simple refcounting.
MMGR *mmgr_init(){
        MMGR *state_table = malloc(sizeof(MMGR));

        state_table->free = NULL;
        state_table->numFree = 0;

        state_table->entries = NULL;
        state_table->numEntries = 0;

        state_table->mutex = 0;

        debugf(DEBUG_LEVEL_MMGR, "mmgr: initialized\n");

        return state_table;
}

// Allocate memory and maintain a reference in the memory manager's state table
// If any previously allocated entries have since been freed, these will be
// resized and reallocated to serve the request
void *mmgr_malloc(MMGR *tbl, size_t size){
        mmgr_mutex_acquire(tbl);

        MMGR_Entry* target;

        // If freed entries are available, we can recycle them
        if(tbl->numFree > 0) {
                int tgt_idx = tbl->free[tbl->numFree - 1];

                debugf(DEBUG_LEVEL_MMGR, "mmgr: found reusable previously allocated entry %p\n", tbl->entries[tgt_idx]);

                target = (MMGR_Entry*) realloc(tbl->entries[tgt_idx], sizeof(MMGR_Entry) + size);
                target->size = size;

                tbl->entries[tgt_idx] = target;

                tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));

                tbl->numFree--;

                debugf(DEBUG_LEVEL_MMGR, "mmgr: reallocated %lu bytes\n", size);

                // Otherwise the state table will need to be resized to accommodate a new
                // entry
        } else {
                debugf(DEBUG_LEVEL_MMGR, "mmgr: no recyclable entries available, increasing table size\n");

                tbl->entries = (MMGR_Entry**) realloc(tbl->entries, (sizeof(MMGR_Entry*) * (tbl->numEntries + 1)));

                tbl->entries[tbl->numEntries] = (MMGR_Entry*) malloc(sizeof(MMGR_Entry) + size);

                target = tbl->entries[tbl->numEntries];

                target->handle = malloc(size);
                target->size = size;

                tbl->numEntries++;

                debugf(DEBUG_LEVEL_MMGR, "mmgr: allocated %lu bytes, handle is %p\n", target->size, target->handle);
        }

        mmgr_mutex_release(tbl);

        return target->handle;
}

// Frees the provided pointer and checks out the active entry in the g_MEM
// state table so that it can be reallocated
void mmgr_free(MMGR *tbl, void* handle){
        mmgr_mutex_acquire(tbl);

        int found = 0;

        debugf(DEBUG_LEVEL_MMGR, "mmgr: num active entries is %d, called to free %p\n", (tbl->numEntries - tbl->numFree), handle);

        for(int i = tbl->numEntries; i--> 0; ) {
                if(handle == NULL) {
                        debugf(DEBUG_LEVEL_MMGR, "mmgr: provided NULL handle! no-op\n");
                        break;
                }

                if(tbl->entries[i]->handle == handle) {
                        debugf(DEBUG_LEVEL_MMGR, "mmgr: found handle %p at index %d\n", handle, i);

                        MMGR_Entry* target = tbl->entries[i];

                        tbl->numFree++;

                        free(target->handle);

                        target->size = 0;

                        tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
                        tbl->free[tbl->numFree] = i;

                        found = 1;

                        debugf(DEBUG_LEVEL_MMGR, "mmgr: freed %p, %d entries remain active\n", handle, (tbl->numEntries - tbl->numFree));
                        break;
                }
        }

        if(found == 0)
                debugf(DEBUG_LEVEL_MMGR, "mmgr: called to free %p but couldn't find it, no-op\n", handle);

        mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl){
        mmgr_mutex_acquire(tbl);

        int deEn = 0;

        debugf(DEBUG_LEVEL_MMGR, "mmgr: cleaning up\n");

        // Free all active entries and what they're pointing to
        for(int i = 0; i < tbl->numEntries; i++) {
                if(tbl->entries[i] != NULL) {
                        free(tbl->entries[i]->handle);
                        free(tbl->entries[i]);
                        deEn++;
                }
        }

        debugf(DEBUG_LEVEL_MMGR, "mmgr: cleanup deallocd %d of %d active entries\n", deEn, (tbl->numEntries - tbl->numFree));

        free(tbl->entries);
        free(tbl->free);
        free(tbl);
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
