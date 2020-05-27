/* COP 3502C Assignment 1
   This program is written by: Charlton Trezevant (PID 4383060)
 */


/*
   The provided leak detector causes compiler warnings and, somewhat ironically,
   appears to contain a memory leak of its own.This is because fclose() is never
   called on the leak detector's output file.

   Valgrind output:
   HEAP SUMMARY:
       in use at exit: 552 bytes in 1 blocks
     total heap usage: 5 allocs, 4 frees, 5,512 bytes allocated
   
   552 bytes in 1 blocks are still reachable in loss record 1 of 1
      at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
      by 0x4EBAE49: __fopen_internal (iofopen.c:65)
      by 0x4EBAE49: fopen@@GLIBC_2.2.5 (iofopen.c:89)
      by 0x109765: report_mem_leak (in /home/net/ch123722/COP3502C/Assignment1/bin/assignment1)
      by 0x4E7F040: __run_exit_handlers (exit.c:108)
      by 0x4E7F139: exit (exit.c:139)
      by 0x4E5DB9D: (below main) (libc-start.c:344)

   My static analyzer also complains about this format string error in the
   leak detector:

   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c: In function ‘report_mem_leak’:
   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c:190:30: warning: format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘void *’ [-Wformat=]
    sprintf(info, "address : %d\n", leak_info->mem_info.address);
                             ~^     ~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             %p

   On another note, the leak detector doesn't work on macOS at all. I'm looking
   into it.
 */

#include <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include  "leak_detector_c.h"

// debugf.h
// (c) Charlton Trezevant - 2018
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_MMGR 1
#define DEBUG_LEVEL_LOGIC 2
#define DEBUG_LEVEL_STATE 3
#define DEBUG_LEVEL_INFO 4
#define DEBUG_LEVEL_NONE 99

#define DEBUG DEBUG_LEVEL_NONE

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

// Input file name (statically specified here, but can optionally be provided
// on the command line as well)
#define INFILE_NAME "in.txt"
#define TEST_NAME_MAXLEN 21

//// Project spec type definitions
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

//// Project spec function prototypes
course *read_courses(FILE *fp, int *num_courses);
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections);
void process_courses(course *courses, int num_courses);
void release_courses(course *courses, int num_courses);

// MMGR Type definitions/function prototypes
// (c) Charlton Trezevant 2020
// Rave reviews:
// 10/10
//    - IGN
// All heap blocks were freed -- no leaks are possible
//    - Valgrind
// it's pronounced "mugger"
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

// Utility
void panic(const char * format, ...);

////////////////////////// Entry //////////////////////////

int main(int argc, char **argv){
        FILE *infile;

        debugf(DEBUG_LEVEL_MMGR, "Memory leak detector init.\n");
        atexit(report_mem_leak); //Ahmed's memory leak detection

        // Initialize MMGR
        g_MEM = mmgr_init();

        // You can override the default input filename if you'd like by
        // passing a command line argument
        if(argc > 1) {
                infile = fopen(argv[1], "r");
                debugf(DEBUG_LEVEL_INFO, "Input file name: %s\n", argv[1]);
        } else {
                infile = fopen(INFILE_NAME, "r");
                debugf(DEBUG_LEVEL_INFO, "Input file name: %s\n", INFILE_NAME);
        }

        // Panic if opening the input file failed
        if (infile == NULL) {
                panic("Failed to open input file\n");
                return 1;
        }

        int num_cases = -1;

        // Attempt to read the number of test cases from the input file, panic
        // if EOF is encountered
        if(!feof(infile)) {
                fscanf(infile, "%d", &num_cases);
                debugf(DEBUG_LEVEL_STATE, "Infile contains %d cases\n", num_cases);
        } else {
                panic("invalid input file format: number of test cases unknown\n");
        }

        course* test_courses;

        // Run test cases from input file, panic if EOF is encountered
        for(int case_n = 0; case_n < num_cases; case_n++) {
                int case_num_courses = 0;

                if(!feof(infile)) {
                        // Fetch number of courses in current case
                        fscanf(infile, "%d", &case_num_courses);
                        debugf(DEBUG_LEVEL_STATE, "Infile contains %d courses for test case %d\n", case_num_courses, case_n);

                        test_courses = read_courses(infile, &case_num_courses);
                        debugf(DEBUG_LEVEL_LOGIC, "read courses for case %d\n", case_n);

                        printf("test case %d\n", case_n+1);
                        
                        process_courses(test_courses, case_num_courses);
                        debugf(DEBUG_LEVEL_STATE, "processed courses for case %d\n", case_n);
                        
                        release_courses(test_courses, case_num_courses);
                        debugf(DEBUG_LEVEL_STATE, "released courses for case %d\n", case_n);
                        
                        debugf(DEBUG_LEVEL_STATE, "Test case %d ended.\n", case_n);

                } else {
                        panic("reached EOF while attempting to run test case %d", case_n);
                }
        }

        debugf(DEBUG_LEVEL_STATE, "Exiting...\n");

        mmgr_cleanup(g_MEM);
        fclose(infile);

        return 0;
}

// Panic is called when something goes wrong
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

/*
   Spec:
   This function takes a file pointer and reference of an integer to track how may
   courses the file has. Then it reads the data for an entire test case and return
   the allocated memory for all the courses (including sections) for a test case.
   Note that you can call other functions from this function as needed.
 */
course *read_courses(FILE *fp, int *num_courses){
        course *courses = (course*) mmgr_malloc(g_MEM, (sizeof(course) * *num_courses));

        debugf(DEBUG_LEVEL_STATE, "will now parse %d courses\n", *num_courses);
        for(int i = 0; i < *num_courses; i++) {
                debugf(DEBUG_LEVEL_LOGIC, "begin parsing course %d of %d\n", i, *num_courses);

                // Panic on premature EOF
                if(feof(fp))
                        panic("invalid input file format: course %d\n", i+1);

                course* cur_course = (course*) mmgr_malloc(g_MEM, sizeof(course));
                debugf(DEBUG_LEVEL_MMGR, "allocated memory to hold course %d\n", i);

                cur_course->course_name = (char*) mmgr_malloc(g_MEM, (sizeof(char) * TEST_NAME_MAXLEN));
                debugf(DEBUG_LEVEL_MMGR, "course %d name array allocated\n", i);

                // Fetch number of courses in current case
                fscanf(fp, "%s", cur_course->course_name);
                debugf(DEBUG_LEVEL_STATE, "read course name for %d: %s\n", i, cur_course->course_name);
                fscanf(fp, "%d", &cur_course->num_sections);
                debugf(DEBUG_LEVEL_STATE, "course %s has %d sections\n", cur_course->course_name, cur_course->num_sections);

                cur_course->num_scores = mmgr_malloc(g_MEM, (sizeof(int) * cur_course->num_sections));
                debugf(DEBUG_LEVEL_MMGR, "course %s num_scores array allocated\n", cur_course->course_name);

                cur_course->num_students = mmgr_malloc(g_MEM, (sizeof(int) * cur_course->num_sections));
                debugf(DEBUG_LEVEL_MMGR, "course %s num_students array allocated\n", cur_course->course_name);

                cur_course->sections = read_sections(fp, cur_course->num_students, cur_course->num_scores, cur_course->num_sections);

                courses[i] = *cur_course;
                debugf(DEBUG_LEVEL_STATE, "course %d appended to courses array\n", i);
        }

        return courses;
}

/*
   Spec:
   This function takes the file pointer, references of two arrays, one for number
   of students, and one for number of scores for a course. The function also takes
   an integer that indicates the number of sections the course has. The function
   reads all the data for all the sections of a course, fill-up the num_students
   and num_socres array of the course and returns 2D array of students that contains
   all the data for all the sections of a course. A good idea would be calling
   this function from the read_course function.
 */
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections){
        student **sections = (student**) mmgr_malloc(g_MEM, (sizeof(student*) * num_sections));
        float avg = 0, readScore = 0;

        for(int sect = 0; sect < num_sections; sect++) {

                fscanf(fp, "%d %d", &num_students[sect], &num_scores[sect]);
                debugf(DEBUG_LEVEL_STATE, "expecting %d students and %d scores in section %d\n", num_students[sect], num_scores[sect], sect);

                // Allocate memory to store a section
                sections[sect] = (student*) mmgr_malloc(g_MEM, (sizeof(student) * num_students[sect]));
                debugf(DEBUG_LEVEL_MMGR, "allocated student section %d\n", sect);

                // Parse information for each student in the section
                debugf(DEBUG_LEVEL_LOGIC, "will now parse %d students in section %d\n", num_students[sect], sect);
                for(int stu = 0; stu < num_students[sect]; stu++) {
                  
                        // Allocate memory for student scores and name
                        sections[sect][stu].scores = (float*) mmgr_malloc(g_MEM, (sizeof(float) * num_scores[sect]));
                        debugf(DEBUG_LEVEL_MMGR, "allocated student scores\n");
                        sections[sect][stu].lname = (char*) mmgr_malloc(g_MEM, (sizeof(char) * TEST_NAME_MAXLEN));
                        debugf(DEBUG_LEVEL_MMGR, "allocated student lname\n");

                        // Read the student's ID
                        fscanf(fp, " %d", &sections[sect][stu].id);
                        debugf(DEBUG_LEVEL_STATE, "read student ID: %d\n", sections[sect][stu].id);

                        // Read the student's last name
                        fscanf(fp, " %s", sections[sect][stu].lname);
                        debugf(DEBUG_LEVEL_STATE, "read student name: %s\n", sections[sect][stu].lname);

                        // For all scores in this section of the course, read student scores
                        for(int score = 0; score < num_scores[sect]; score++) {
                                fscanf(fp, " %f", &readScore);
                                sections[sect][stu].scores[score] = readScore;
                                debugf(DEBUG_LEVEL_STATE, "read score %.2f for student %s\n", readScore, sections[sect][stu].lname);
                                avg += readScore;
                                readScore = 0;
                        }

                        // Calculate the student's average score based on the scores that were just read
                        avg /= num_scores[sect];
                        sections[sect][stu].std_avg = avg;
                        avg = 0;

                        debugf(DEBUG_LEVEL_STATE, "completed processing scores for student %d average: %.2f\n", sections[sect][stu].id, sections[sect][stu].std_avg);
                }
        }

        return sections;
}

/*
   Spec:
   This function takes the array of courses produced and filled by all the courses
   of a test case and also takes the size of the array. Then it displays the
   required data in the same format discussed in the sample output. You can write
   and use more functions in this process as you wish.
 */
void process_courses(course *courses, int num_courses){
        // Temp variables used for calculating high scores/averages/etc
        int pass_count = 0;
        float sa_tmp = 0, hs_tmp = 0;
        float* avgs_list;
        student *hiscore;
        
        // Default hiscore placeholder
        student hiscore_default = { .lname = "nobody", .id = 0, .scores = 0, .std_avg = 0 };
        hiscore = &hiscore_default;

        // For all courses in the provided course pointer
        for(int i = 0; i < num_courses; i++) {
                // Allocate memory to store section average scores
                avgs_list = (float*) mmgr_malloc(g_MEM, (sizeof(float) * courses[i].num_sections));

                // Process high scores, section averages, pass rates for each course section
                for (int sect = 0; sect < courses[i].num_sections; sect++) {

                        for(int stu = 0; stu < courses[i].num_students[sect]; stu++) {
                                // Accumulate student average score in section average
                                // https://math.stackexchange.com/questions/95909/why-is-an-average-of-an-average-usually-incorrect
                                sa_tmp += courses[i].sections[sect][stu].std_avg;

                                if(courses[i].sections[sect][stu].std_avg > 70.0) {
                                        debugf(DEBUG_LEVEL_LOGIC, "%s passed %s with a score of %.2f\n", courses[i].sections[sect][stu].lname, courses[i].course_name, courses[i].sections[sect][stu].std_avg);
                                        pass_count++;
                                } else {
                                        debugf(DEBUG_LEVEL_LOGIC, "%s failed %s with a score of %.2f\n", courses[i].sections[sect][stu].lname, courses[i].course_name, courses[i].sections[sect][stu].std_avg);
                                }

                                if(courses[i].sections[sect][stu].std_avg > hs_tmp) {
                                        debugf(DEBUG_LEVEL_LOGIC, "%s achieved high score of %.2f in %s prev: %.2f\n", courses[i].sections[sect][stu].lname, courses[i].sections[sect][stu].std_avg, courses[i].course_name, hs_tmp);
                                        hiscore = &courses[i].sections[sect][stu];
                                        hs_tmp = courses[i].sections[sect][stu].std_avg;
                                }
                        }

                        // Calculate overall section average
                        avgs_list[sect] = sa_tmp / courses[i].num_students[sect];

                        debugf(DEBUG_LEVEL_STATE, "Section %d average: %.2f [%.2f]\n", sect, (sa_tmp / courses[i].num_scores[sect]), avgs_list[sect]);
                        sa_tmp = 0;
                }

                // Print course information
                printf("%s %d ", courses[i].course_name, pass_count);

                // Print section averages
                for(int j = 0; j < courses[i].num_sections; j++) {
                        printf("%.2f ", avgs_list[j]);
                        avgs_list[j] = 0;
                }

                // Print high score
                printf("%d %s %.2f\n", hiscore->id, hiscore->lname, hiscore->std_avg);

                // Free section averages array
                mmgr_free(g_MEM, avgs_list);

                // Reset tmp variables
                pass_count = 0;
                sa_tmp = 0;
                hs_tmp = 0;
                hiscore = NULL;
        }

        printf("\n");
}

/*
   Spec:
   This function takes the array of courses produced and filled by all the
   courses of a test case and also takes the size of the array. Then it free up
   all the memory allocated within it. You can create more function as needed to
   ease the process.

   N.B. anything not explicitly removed by release_courses will be garbage collected
   by the memory manager's cleanup routine.
 */
void release_courses(course *courses, int num_courses){
        debugf(DEBUG_LEVEL_STATE, "releasing courses\n");

        for(int i = 0; i < num_courses; i++) {

                debugf(DEBUG_LEVEL_MMGR, "releasing course %d\n", i);

                for(int j = 0; j < courses[i].num_sections; j++) {

                        debugf(DEBUG_LEVEL_MMGR, "releasing course %d section %d\n", i, j);
                        debugf(DEBUG_LEVEL_MMGR, "%d students to release in course %d section %d\n", courses[i].num_students[j], i, j);

                        for(int k = 0; k < courses[i].num_students[j]; k++) {
                                debugf(DEBUG_LEVEL_MMGR, "LOOP %d\n", k);
                                mmgr_free(g_MEM, courses[i].sections[j][k].scores);
                                debugf(DEBUG_LEVEL_MMGR, "released course %d section %d student %d scores\n", i, j, k);
                                mmgr_free(g_MEM, courses[i].sections[j][k].lname);
                                debugf(DEBUG_LEVEL_MMGR, "released course %d section %d student %d lname\n", i, j, k);
                        }

                }

                mmgr_free(g_MEM, courses[i].sections);
                debugf(DEBUG_LEVEL_MMGR, "released course %d sections array\n", i);

                mmgr_free(g_MEM, courses[i].course_name);
                debugf(DEBUG_LEVEL_MMGR, "released course %d name\n", i);

                mmgr_free(g_MEM, courses[i].num_students);
                debugf(DEBUG_LEVEL_MMGR, "released course %d num_students\n", i);

                mmgr_free(g_MEM, courses[i].num_scores);
                debugf(DEBUG_LEVEL_MMGR, "released course %d num_scores\n", i);
        }
        
        mmgr_free(g_MEM, courses);
        debugf(DEBUG_LEVEL_STATE, "released courses\n");
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

        debugf(DEBUG_LEVEL_MMGR, "mmgr: initialized\n");

        return state_table;
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

// Mutex acquisition/release helpers to atomicize access to the memory manager's
// state table. Not really necessary atm but eventually I might play with threading
void mmgr_mutex_acquire(MMGR *tbl){
        while (tbl->mutex == 1);
        tbl->mutex = 1;
}

void mmgr_mutex_release(MMGR *tbl){
        tbl->mutex = 0;
}
