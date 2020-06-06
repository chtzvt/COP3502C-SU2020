#include <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "leak_detector_c.h"

////////////////////////// Global Project Configuation //////////////////////////

#define CONFIG_INFILE_NAME "file_in.txt"
#define CONFIG_OUTFILE_NAME "out.txt"
#define CONFIG_NUM_LANES 12

// Maximum values and limits
#define CONFIG_MAX_TEST_CASES 25
#define CONFIG_MAX_CUSTOMERS 500000
#define CONFIG_MAX_CUST_ITEMS 100
#define CONFIG_MAX_NAME_LEN 10
#define CONFIG_MAX_TIME 1000000000

// Global debug levels
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_TRACE 1
#define DEBUG_LEVEL_INFO 2
#define DEBUG_LEVEL_NONE 1000

// Functionality-specific debug levels
#define DEBUG_TRACE_CUSTOMER -1
#define DEBUG_TRACE_LANE -2
#define DEBUG_TRACE_MMGR -3

#define DEBUG DEBUG_LEVEL_ALL

////////////////////////// Assignment 2 Prototypes //////////////////////////

typedef struct Customer {
        char *name;
        int num_items;
        int line_number;
        int time_enter;
} Customer;

Customer* customer_create(char *name, int num_items, int line_number, int time_enter);
void customer_destroy(Customer *c);

typedef struct Node {
        Customer *cust;
        struct Node *next;
} Node;

Node* node_create(Customer *c, Node *next);
void node_destroy(Node *n);

// Stores our queue.
typedef struct Lane {
        Node *front;
        Node *back;
} Lane;

Lane* lane_create();
void lane_destroy(Lane *l);
void lane_enqueue(Lane *l, Customer *c);
Customer* lane_dequeue(Lane *l);
Customer* lane_peek(Lane *l);
int lane_empty(Lane *l);

Lane* global_lanes[CONFIG_NUM_LANES];
void global_lanes_create();
void global_lanes_destroy();

////////////////////////// Debug Output //////////////////////////
// (c) Charlton Trezevant - 2018

#ifdef DEBUG
#define debugf(lvl, fmt, ...) \
        ({ \
                if (DEBUG == 0 || (lvl > 0 && lvl >= DEBUG) || (lvl < 0 && lvl == DEBUG)) { \
                        fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); \
                } \
        })
#else
  #define debugf(lvl, fmt, ...) ((void)0)
#endif

////////////////////////// Memory Manager Prototypes //////////////////////////
// (c) Charlton Trezevant - 2020

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

MMGR *mmgr_init();
void *mmgr_malloc(MMGR *tbl, size_t size);
void mmgr_free(MMGR *tbl, void* handle);
void mmgr_cleanup(MMGR *tbl);
void mmgr_mutex_acquire(MMGR *tbl);
void mmgr_mutex_release(MMGR *tbl);

// Global MMGR instance
MMGR* g_MEM;

////////////////////////// Utility //////////////////////////
int stdout_enabled = 1;
FILE *g_outfp;

void panic(const char * format, ...);
void write_out(const char * format, ...);

////////////////////////// Entry //////////////////////////

int main(int argc, char **argv){
        debugf(DEBUG_LEVEL_TRACE, "Ahmed memory leak detector init.\n");
        atexit(report_mem_leak); //Ahmed's memory leak detection

        // Initialize MMGR
        debugf(DEBUG_LEVEL_TRACE, "MMGR init.\n");
        g_MEM = mmgr_init();

        // You can override the default input filename if you'd like by
        // passing a command line argument
        FILE *infile;
        if(argc > 1 && strcmp(argv[1], "use_default") != 0) {
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
        if(argc > 2) {
                if(strcmp(argv[2], "stdout") == 0) {
                        g_outfp = NULL;
                        stdout_enabled = 1;
                        debugf(DEBUG_LEVEL_INFO, "Writing output to stdout\n");
                } else {
                        g_outfp = fopen(argv[2], "a");
                        debugf(DEBUG_LEVEL_INFO, "Output file name: %s\n", argv[2]);
                }
        } else {
                g_outfp = fopen(CONFIG_OUTFILE_NAME, "a");
                debugf(DEBUG_LEVEL_INFO, "Output file name: %s\n", CONFIG_OUTFILE_NAME);
        }

        // Attempt to read the number of test cases from the input file, panic
        // if EOF is encountered
        int num_cases = -1;
        if(!feof(infile)) {
                fscanf(infile, "%d", &num_cases);
                debugf(DEBUG_LEVEL_INFO, "Infile contains %d cases\n", num_cases);
        } else {
                panic("invalid input file format: number of test cases unknown\n");
        }

        // Run test cases from input file, panic if EOF is encountered
        for(int case_n = 0; case_n < num_cases; case_n++) {
                int case_num_customers = 0;

                if(!feof(infile)) {
                        // Fetch number of courses in current case
                        fscanf(infile, "%d", &case_num_customers);
                        debugf(DEBUG_LEVEL_INFO, "Infile contains %d customers for test case %d\n", case_num_customers, case_n);
                } else {
                        panic("reached EOF while attempting to run test case %d", case_n);
                }
        }

        debugf(DEBUG_LEVEL_TRACE, "MMGR Cleanup.\n");
        mmgr_cleanup(g_MEM);

        debugf(DEBUG_LEVEL_TRACE, "Infile close.\n");
        if(infile != NULL)
                fclose(infile);

        debugf(DEBUG_LEVEL_TRACE, "Outfile close.\n");
        if(g_outfp != NULL)
                fclose(g_outfp);

        debugf(DEBUG_LEVEL_TRACE, "Exiting.\n");
        return 0;
}

////////////////////////// Customer/Lane methods //////////////////////////

Customer* customer_create(char *name, int num_items, int line_number, int time_enter){
        if(num_items > CONFIG_MAX_CUST_ITEMS || time_enter > CONFIG_MAX_TIME || line_number > (CONFIG_NUM_LANES - 1)) {
                debugf(DEBUG_TRACE_CUSTOMER, "customer_create Refusing to instantiate malformed customer.\n");
                return NULL;
        }

        Customer *cust = mmgr_malloc(g_MEM, sizeof(Customer));
        debugf(DEBUG_TRACE_CUSTOMER, "customer_create allocated customer struct\n");

        cust->name = mmgr_malloc(g_MEM, sizeof(char*) * CONFIG_MAX_NAME_LEN);
        strcpy(cust->name, name);

        cust->num_items = num_items;
        cust->line_number = line_number;
        cust->time_enter = time_enter;
        debugf(DEBUG_TRACE_CUSTOMER, "customer_create populated customer struct\n");

        return cust;
}

void customer_destroy(Customer *c){
        if(c == NULL)
                return;

        debugf(DEBUG_TRACE_CUSTOMER, "Destroying customer %s\n", c->name);
        mmgr_free(g_MEM, c->name);
        mmgr_free(g_MEM, c);

        return;
}

Node* node_create(Customer *c, Node *next){
        debugf(DEBUG_TRACE_LANE, "node_create creating lane queue node\n");
        Node *node = mmgr_malloc(g_MEM, sizeof(Node));
        node->cust = c;
        node->next = next;

        return node;
}

void node_destroy(Node *n){
        if(n == NULL) {
                debugf(DEBUG_TRACE_LANE, "node_destroy called to destroy NULL node! Returning\n");
                return;
        }
        mmgr_free(g_MEM, n);
        debugf(DEBUG_TRACE_LANE, "node_destroy destroyed a node\n");
}

Lane* lane_create(){
        debugf(DEBUG_TRACE_LANE, "lane_create initializing new lane\n");
        Lane *lane = mmgr_malloc(g_MEM, sizeof(Lane));
        lane->front = NULL;
        lane->back = NULL;

        return lane;
}

void lane_destroy(Lane *l){
        if(l == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_destroy called to destroy NULL lane! Returning\n");
                return;
        }

        Node *cursor, *tmp;
        cursor = l->front;

        debugf(DEBUG_TRACE_LANE, "lane_destroy cleaning up...\n");
        while(cursor != NULL) {
                customer_destroy(cursor->cust);

                tmp = cursor;
                customer_destroy(tmp->cust);
                node_destroy(tmp);

                cursor = cursor->next;
        }

}

void lane_enqueue(Lane *l, Customer *c){
        if(l == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_enqueue NULL lane provided! returning...\n");
                return;
        }

        Node *node = node_create(c, l->back);

        if(l->front == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_dequeue queue is empty, front pointer updated\n");
                l->front = node;
        }
}

Customer* lane_dequeue(Lane *l){
        debugf(DEBUG_TRACE_LANE, "lane_dequeue called\n");
        if(l == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_dequeue NULL lane provided! returning...\n");
                return NULL;
        }

        Node *n;
        Customer *cust;

        n = l->front;
        l->front = n->next;

        if(l->front == l->back) {
                debugf(DEBUG_TRACE_LANE, "lane_deque dequeued last customer, now empty\n");
                l->back = NULL;
        }

        cust = n->cust;
        node_destroy(n);
        debugf(DEBUG_TRACE_LANE, "lane_dequeue destroyed node containing dequeued customer\n");

        return cust;
}

Customer* lane_peek(Lane *l){
        if(l == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_peek called on NULL lane! returning...\n");
                return NULL;
        }

        return l->front->cust;
}

int lane_empty(Lane *l){
        if(l == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_empty called on NULL lane!\n");
                return -1;
        }

        if(l->front == NULL && l->back == NULL) {
                debugf(DEBUG_TRACE_LANE, "lane_empty called on non-empty lane\n");
                return 1;
        }

        debugf(DEBUG_TRACE_LANE, "lane_empty called on empty lane\n");
        return 0;
}

void global_lanes_create(){
        for(int i = 0; i < CONFIG_NUM_LANES; i++) {
                debugf(DEBUG_TRACE_LANE, "Creating lane %d\n", i);
                global_lanes[i] = lane_create();
        }
}

void global_lanes_destroy(){
        for(int i = 0; i < CONFIG_NUM_LANES; i++) {
                debugf(DEBUG_TRACE_LANE, "Destroying lane %d\n", i);
                lane_destroy(global_lanes[i]);
        }
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
// This shim is more for my own use as I require stdout for compatibility with my
// automated test harness
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

        debugf(DEBUG_TRACE_MMGR, "mmgr: initialized\n");

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

                debugf(DEBUG_TRACE_MMGR, "mmgr: found reusable previously allocated entry %d\n", tgt_idx);

                // Update table entry with new pointer and size, memset to 0
                tbl->entries[tgt_idx]->size = size;
                tbl->entries[tgt_idx]->handle = malloc(size);
                memset(tbl->entries[tgt_idx]->handle, 0, size);

                // Copy freshly allocated region pointer to handle
                handle = tbl->entries[tgt_idx]->handle;

                // Resize free index array
                tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));
                tbl->numFree--;

                debugf(DEBUG_TRACE_MMGR, "mmgr: reallocated %lu bytes\n", size);

                // Otherwise the state table will need to be resized to accommodate a new
                // entry
        } else {
                debugf(DEBUG_TRACE_MMGR, "mmgr: no recyclable entries available, increasing table size\n");

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

                debugf(DEBUG_TRACE_MMGR, "mmgr: allocated %lu bytes, handle is %p\n", size, handle);
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
        for(int i = tbl->numEntries; i--> 0; ) {
                if(tbl->entries[i]->handle == handle) {
                        debugf(DEBUG_TRACE_MMGR, "mmgr: found handle %p at index %d\n", handle, i);

                        tbl->numFree++;

                        // Free and nullify table entry
                        free(tbl->entries[i]->handle);
                        tbl->entries[i]->handle = NULL;
                        tbl->entries[i]->size = 0;

                        // Resize free entry list, ad freed entry
                        tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
                        tbl->free[tbl->numFree - 1] = i;

                        found = 1;

                        debugf(DEBUG_TRACE_MMGR, "mmgr: freed %p at index %d, %d entries remain active\n", handle, i, (tbl->numEntries - tbl->numFree));

                }
        }

        if(found == 0)
                debugf(DEBUG_TRACE_MMGR, "mmgr: called to free %p but couldn't find it, no-op\n", handle);

        mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl){
        if(tbl == NULL)
                return;

        mmgr_mutex_acquire(tbl);

        int deEn = 0;

        debugf(DEBUG_TRACE_MMGR, "mmgr: cleaning up %d active entries\n", (tbl->numEntries - tbl->numFree));

        // Free all active entries and what they're pointing to
        for(int i = 0; i < tbl->numEntries; i++) {
                if(tbl->entries[i] != NULL) {
                        if(tbl->entries[i]->handle != NULL)
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
