/* COP 3502C Assignment 1
   This program is written by: Charlton Trezevant (PID 4383060)
 */


/*
   The provided leak detector causes compiler warnings and, somewhat ironically,
   appears to contain a memory leak of its own.

   This is apparently because fclose() is never called on the leak detector's
   output file.

   Valgrind output:
   ==2024== HEAP SUMMARY:
   ==2024==     in use at exit: 552 bytes in 1 blocks
   ==2024==   total heap usage: 5 allocs, 4 frees, 5,512 bytes allocated
   ==2024==
   ==2024== 552 bytes in 1 blocks are still reachable in loss record 1 of 1
   ==2024==    at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
   ==2024==    by 0x4EBAE49: __fopen_internal (iofopen.c:65)
   ==2024==    by 0x4EBAE49: fopen@@GLIBC_2.2.5 (iofopen.c:89)
   ==2024==    by 0x109765: report_mem_leak (in /home/net/ch123722/COP3502C/Assignment1/bin/assignment1)
   ==2024==    by 0x4E7F040: __run_exit_handlers (exit.c:108)
   ==2024==    by 0x4E7F139: exit (exit.c:139)
   ==2024==    by 0x4E5DB9D: (below main) (libc-start.c:344)

   My static analyzer also complains about this format string error:

   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c: In function ‘report_mem_leak’:
   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c:190:30: warning: format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘void *’ [-Wformat=]
    sprintf(info, "address : %d\n", leak_info->mem_info.address);
                             ~^     ~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             %p

   On another note, the leak detector doesn't run on macOS at all. I'm looking
   into it.
 */

#include <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include  "leak_detector_c.h"

// debugf.h
// (c) Charlton Trezevant - 2018
#define DEBUG
#ifdef DEBUG
  #define debugf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr)
#else
  #define debugf(fmt, ...) ((void)0)
#endif

#define INFILE_NAME "in.txt"

//// Type definitions

typedef struct student {
        int id;
        char *lname; //stores last name of student
        float *scores; //stores scores of the student. Size is taken from num_scores array.
        float std_avg; //average score of the student (to be calculated)
} student;

typedef struct course {
        char *course_name; //stores course name
        int num_sections; //number of sections
        student **sections;//stores array of student arrays(2D array). Size is num_sections;
        int *num_students;//stores array of number of students in each section. Size is num_sections;
        int *num_scores; //stores array of number of assignments in each section. Size is num_sections;
} course;


course *read_courses(FILE *fp, int *num_courses);
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections);
void process_courses(course *courses, int num_courses);
void release_courses(course *courses, int num_courses);

// My function prototypes
// MMGR Type definitions/function prototypes
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

// Utility
void panic(const char * format, ...);

// Global memory manager instance
MMGR* g_MEM;

//// Entry

int main(){
        FILE *infile;

        debugf("Memory leak detector called.\n");
        atexit(report_mem_leak); //memory leak detection

        global_MEM = mmgr_init();

        infile = fopen(INFILE_NAME, "r");
        if (infile == NULL)
        {
                printf("Failed to open input file %s\n", INFILE_NAME);
                return 1;
        }






        mmgr_cleanup(global_MEM);
        fclose(infile);

        return 0;
}

}


/*
   This function takes a file pointer and reference of an integer to track how may
   courses the file has. Then it reads the data for an entire test case and return
   the allocated memory for all the courses (including sections) for a test case.
   Note that you can call other functions from this function as needed.

   Translation: On the surface, it sounds like this is just handling memory allocation
   for courses and nothing more.
 */
course *read_courses(FILE *fp, int *num_courses){
        course *placeholder = (course*) mmgr_malloc(global_MEM, sizeof(student**));

        return placeholder;

/*
   Course format
   %s
   %d
   %d %d

   cs1 //name of course 1
   2 //number of sections for course 1 (cs1)
   3 4 //no. of students and assignments for sec1 of course 1 (cs1)
   101 john 70 60.5 95.2 50.6 //id lname scores
   102 tyler 80 60.5 95.2 66.6
   103 nusair 70 60.5 85.2 50.6
   2 3 //no. of students and assignments for sec2 of course 1 (cs1) 105 edward 90.5 60.5 98.2
   104 alan 40 60.5 95.2
   math2 //name of course 2
   3 //number of sections for course 2 (math2)
   2 2 //no. of students and assignments for sec1 of course 2 (math2)
   101 john 95.2 53.6
   103 nusair 86.2 56.6
   2 3 //no. of students and assignments for sec2 of course 2 (math2)
   105 edward 90.5 60.5 98.2
   104 alan 40 60.5 95.2
   3 2 //no. of students and assignments for sec3 of course 2 (math2))
   110 kyle 90.5 98.2
   108 bob 45 85.2
   109 smith 75.5 65.9
 */

}

/*
   This function takes the file pointer, references of two arrays, one for number
   of students, and one for number of scores for a course. The function also takes
   an integer that indicates the number of sections the course has. The function
   reads all the data for all the sections of a course, fill-up the num_students
   and num_socres array of the course and returns 2D array of students that contains
   all the data for all the sections of a course. A good idea would be calling
   this function from the read_course function.

   Translation: Primarily sounds like this is going to be the file parser.
 */
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections){
        student **placeholder = (student**) mmgr_malloc(g_MEM, sizeof(student**));

        return placeholder;
}

/*
   This function takes the array of courses produced and filled by all the courses
   of a test case and also takes the size of the array. Then it displays the
   required data in the same format discussed in the sample output. You can write
   and use more functions in this process as you wish.

   Translation: calculates course statistics and prints the apropriate output
 */
void process_courses(course *courses, int num_courses){

/*
   Output specification:

   course_name pass_count list_of_averages_section(separated by space up to two decimal places) id lname avg_score (up to two decimal places)

   test case 1
   cs1 2 70.41 74.15 105 edward 83.07 math2 5 72.90 74.15 76.72 110 kyle 94.35 physics3 2 71.12 105 edward 79.35
 */

}

/*
   This function takes the array of courses produced and filled by all the
   courses of a test case and also takes the size of the array. Then it free up
   all the memory allocated within it. You can create more function as needed to
   ease the process.
 */
void release_courses(course *courses, int num_courses){
        for(int i = 0; i < num_courses; i++) {
                for(int j = 0; j < courses[i].num_sections; j++) {
                        int curSect_numStudents = courses[i].num_students[j];

                        for(int k = 0; k < curSect_numStudents; k++) {
                                mmgr_free(g_MEM, courses[i].sections[k]->scores);
                                mmgr_free(g_MEM, courses[i].sections[k]->lname);
                                mmgr_free(g_MEM, courses[i].sections[k]);
                        }

                        mmgr_free(g_MEM, courses[i].sections);
                        mmgr_free(g_MEM, courses[i].course_name);
                        mmgr_free(g_MEM, courses[i].num_students);
                        mmgr_free(g_MEM, courses[i].num_scores);
                }
        }

        mmgr_free(g_MEM, courses);
}

////////////////////////// Memory manager //////////////////////////

// This is the initial version of my C memory manager.
// At the moment it's very rudimentary.

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

        debugf("mmgr: initialized\n");

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

                debugf("mmgr: found reusable previously allocated entry %p\n", tbl->entries[tgt_idx]);

                target = (MMGR_Entry*) realloc(tbl->entries[tgt_idx], sizeof(MMGR_Entry) + size);
                target->size = size;

                tbl->entries[tgt_idx] = target;

                tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree - 1)));

                tbl->numFree--;

                debugf("mmgr: reallocated %lu bytes\n", size);

                // Otherwise the state table will need to be resized to accommodate a new
                // entry
        } else {
                debugf("mmgr: no recyclable entries available, increasing table size\n");

                tbl->entries = (MMGR_Entry**) realloc(tbl->entries, (sizeof(MMGR_Entry*) * (tbl->numEntries + 1)));

                tbl->entries[tbl->numEntries] = (MMGR_Entry*) malloc(sizeof(MMGR_Entry) + size);

                target = tbl->entries[tbl->numEntries];

                target->handle = malloc(size);
                target->size = size;

                tbl->numEntries++;

                debugf("mmgr: allocated %lu bytes, handle is %p\n", target->size, target->handle);
        }

        mmgr_mutex_release(tbl);

        return target->handle;
}

// Frees the provided pointer and checks out the active entry in the g_MEM
// state table so that it can be reallocated
void mmgr_free(MMGR *tbl, void* handle){
        mmgr_mutex_acquire(tbl);

        int found = 0;

        debugf("mmgr: num active entries is %d, called to free %p\n", tbl->numEntries, handle);

        for(int i = tbl->numEntries; i--> 0; ) {
                if(handle == NULL) {
                        debugf("mmgr: provided NULL handle! no-op\n");
                        break;
                }

                if(tbl->entries[i]->handle == handle) {
                        debugf("mmgr: found handle %p at index %d\n", handle, i);

                        MMGR_Entry* target = tbl->entries[i];

                        tbl->numFree++;

                        free(target->handle);
                        target->size = 0;

                        tbl->free = (int*) realloc(tbl->free, (sizeof(int) * (tbl->numFree + 1)));
                        tbl->free[tbl->numFree] = i;

                        found = 1;

                        debugf("mmgr: freed %p, %d entries remain active\n", handle, tbl->numEntries);
                        break;
                }
        }

        if(found == 0)
                debugf("mmgr: called to free %p but couldn't find it, no-op\n", handle);

        mmgr_mutex_release(tbl);
}

// Cleans up any remaining allocated memory, then frees the state table
void mmgr_cleanup(MMGR *tbl){
        mmgr_mutex_acquire(tbl);

        int deEn = 0, deFr = 0;

        debugf("mmgr: cleaning up\n");

        // Free all active entries and what they're pointing to
        for(int i = 0; i < tbl->numEntries; i++) {
                if(tbl->entries[i] != NULL) {
                        free(tbl->entries[i]->handle);
                        free(tbl->entries[i]);
                        deEn++;
                }
        }

        debugf("mmgr: cleanup deallocd %d of %d active, %d of %d free entries\n", deEn, (tbl->numEntries - tbl->numFree), deFr, tbl->numFree);

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
