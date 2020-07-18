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
#define CONFIG_ALPHABET_LEN 26
#define CONFIG_ASCII_BASE 97

// Global debug levels
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_TRACE 1
#define DEBUG_LEVEL_INFO 2
#define DEBUG_TRACE_SEARCH -4
#define DEBUG_TRACE_SUGGEST -5
#define DEBUG_TRACE_INSERT -6

// Functionality-specific debug levels
// These enable debug output only for specific sections of the program
#define DEBUG_TRACE_MMGR -7

// Current debug level set/disable
//#define DEBUG DEBUG_LEVEL_ALL

////////////////////////// Assignment 4 Prototypes and Globals //////////////////////////

typedef struct trie_node {
  struct trie_node **children;
  int freq;
  int sum_freq;
  int cur_max_freq;
  int isEnd;
} trie_node;

// Sentinel values
// Used as error markers in lieu of NULL
trie_node EMPTY_NODE;
char NOT_FOUND;

trie_node *trie_node_create();
void trie_node_destroy(trie_node *node);
trie_node *trie_node_insert(trie_node *root, const char c);

void trie_insert(trie_node *root, const char *str);
trie_node *trie_search(trie_node *root, const char *str);
char *trie_suggest(trie_node *root, const char *str);

char index_to_char(int i);

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

  // Instantiate the root of the trie
  trie_node *root = trie_node_create();

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
    char *found;

    if (!feof(infile)) {
      // Read command type
      fscanf(infile, "%d", &cmd_type);
      debugf(DEBUG_LEVEL_TRACE, "Command type: %d\n", cmd_type);

      switch (cmd_type) {
      case 1: // Insertion
        fscanf(infile, "%s %d", str, &num_insertions);
        debugf(DEBUG_LEVEL_TRACE, "Insert: %s %d times\n", str, num_insertions);
        for (int j = 0; j < num_insertions; j++) {
          trie_insert(root, str);
        }
        break;

      case 2: // Queries
        fscanf(infile, "%s", str);
        found = trie_suggest(root, str);
        debugf(DEBUG_LEVEL_TRACE, "Running suggest on '%s', got '%s'\n", str, found);
        if (found != &NOT_FOUND && strlen(found) > 0)
          write_out("%s\n", found);
        else
          write_out("unknown word\n");
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

// Allocates and initializes a trie node
trie_node *trie_node_create() {
  trie_node *tmp = mmgr_malloc(g_MEM, sizeof(trie_node));
  tmp->children = (trie_node **)mmgr_malloc(g_MEM, sizeof(trie_node *) * CONFIG_ALPHABET_LEN);
  tmp->freq = 0;
  tmp->sum_freq = 0;
  tmp->cur_max_freq = 0;
  tmp->isEnd = 0;
  return tmp;
}

// Destroys a trie node
void trie_node_destroy(trie_node *node) {
  if (node == NULL || node == &EMPTY_NODE)
    return;

  free(node->children);
  free(node);
}

// Converts an index in a trie node's child array into an ASCII character
char index_to_char(int i) {
  return (char)(CONFIG_ASCII_BASE + i);
}

// Inserts a single character into a trie
trie_node *trie_node_insert(trie_node *root, char c) {
  if (root == NULL || root == &EMPTY_NODE || root->children == NULL) {
    debugf(DEBUG_TRACE_INSERT, "node_insert: given bad node for char '%c'\n", c);
    return &EMPTY_NODE;
  }

  int index = (int)c - CONFIG_ASCII_BASE;

  debugf(DEBUG_TRACE_INSERT, "node_insert: calculate index of char '%c' = %d\n", c, index);

  // Return an error sentinel value if an invalid index is calculated based on the
  // provided character
  if (index > CONFIG_ALPHABET_LEN - 1)
    return &EMPTY_NODE;

  // Create child nodes if they don't already exist
  if (root->children[index] == NULL) {
    debugf(DEBUG_TRACE_INSERT, "node_insert: parent node has no child for char '%c', creating\n", c);
    root->children[index] = trie_node_create();
  }

  // Increments individual node frequency
  root->children[index]->freq++;

  return root->children[index];
}

// Inserts an entire string into a trie
void trie_insert(trie_node *root, const char *str) {
  if (root == NULL || root == &EMPTY_NODE) {
    debugf(DEBUG_TRACE_INSERT, "trie_node: given bad node for %s\n", str);
    return;
  }

  int len = strlen(str);
  trie_node *cursor = root;
  trie_node **path = (trie_node **)mmgr_malloc(g_MEM, sizeof(trie_node *) * len);

  debugf(DEBUG_TRACE_INSERT, "trie_insert: calculate len of '%s' = %d\n", str, len);

  for (int i = 0; i < len; i++) {
    debugf(DEBUG_TRACE_INSERT, "trie_insert: create node to contain '%c' at index %d\n", str[i], i);
    cursor = trie_node_insert(cursor, str[i]);
    // Increment frequencies for which this string is a prefix of words in the dictionary
    cursor->sum_freq++;

    path[i] = cursor;

    if (cursor == &EMPTY_NODE || path[i] == NULL)
      break;
  }

  // Mark the last node inserted in the trie as the end of the
  // string
  cursor->isEnd = 1;

  // Calculate the current maximum frequency of any child node for
  // each node in the path that was taken to insert the given string
  int tmp_child_max = 0;
  for (int i = len - 1; i-- > 0;) {
    if (path[i] == &EMPTY_NODE || path[i] == NULL)
      continue;

    for (int j = 0; j < CONFIG_ALPHABET_LEN; j++) {
      if (path[i]->children[j] == NULL || path[i]->children[j] == &EMPTY_NODE)
        continue;

      if (path[i]->children[j]->freq > tmp_child_max)
        tmp_child_max = path[i]->children[j]->freq;
    }

    path[i]->cur_max_freq = tmp_child_max;
    tmp_child_max = 0;
  }
}

// Searches a trie for a given string
trie_node *trie_search(trie_node *root, const char *str) {
  int len = strlen(str);
  trie_node *cursor = root;

  for (int i = 0; i < len; i++) {
    int idx = ((int)str[i] - CONFIG_ASCII_BASE);

    if (cursor->children[idx] == &EMPTY_NODE || cursor->children[idx] == NULL) {
      debugf(DEBUG_TRACE_SEARCH, "search: interrupted finding '%s', intermediate node %d has no child '%c'\n", str, i, str[i]);
      return 0;
    }

    cursor = cursor->children[idx];
  }

  if (cursor != NULL && cursor != &EMPTY_NODE) {
    debugf(DEBUG_TRACE_SEARCH, "search: '%s' was found\n", str);
    return cursor;
  }

  debugf(DEBUG_TRACE_SEARCH, "search: couldn't find '%s'\n", str);
  return &EMPTY_NODE;
}

// Searches a trie for a given prefix and returns the most likely character(s)
// following the end of that prefix
char *trie_suggest(trie_node *root, const char *str) {
  trie_node *pfx_root = trie_search(root, str);

  // If the search was unsuccessful, return an error sentinel value
  if (pfx_root == NULL || pfx_root == &EMPTY_NODE || pfx_root->children == NULL) {
    debugf(DEBUG_TRACE_SUGGEST, "suggest: got bad node from search\n");
    return &NOT_FOUND;
  }

  // Otherwise, prepare an array to store candidate characters
  char *sug = mmgr_malloc(g_MEM, sizeof(char) * CONFIG_ALPHABET_LEN);
  int likely = 0, sug_i = 0;

  // First pass: determine the highest frequency value in the array of the trie
  // node's children. This will provide the threshold against each child will be
  // compared
  for (int i = 0; i < CONFIG_ALPHABET_LEN; i++) {
    if (pfx_root->children[i] == NULL || pfx_root->children[i] == &EMPTY_NODE)
      continue;

    if (pfx_root->children[i]->sum_freq > likely)
      likely = pfx_root->children[i]->sum_freq;
  }

  debugf(DEBUG_TRACE_SUGGEST, "suggest: determined most likely frequency threshold is %d\n", likely);

  // Now that the highest frequency threshold has been determined, the next suggested
  // character(s) will consist of any child nodes with a frequency equal to the likely
  // threshold determined above. These will all be concatenated together into the
  // suggestion string
  for (int i = 0; i < CONFIG_ALPHABET_LEN; i++) {
    if (pfx_root->children[i] == NULL || pfx_root->children[i] == &EMPTY_NODE)
      continue;

    if (pfx_root->children[i]->sum_freq == likely) {
      debugf(DEBUG_TRACE_SUGGEST, "suggest: found likely character '%c' at index %d\n", index_to_char(i), i);
      sug[sug_i] = index_to_char(i);
      sug_i++;
    }
  }

  return sug;
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
