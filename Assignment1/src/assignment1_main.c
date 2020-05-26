/* COP 3502C Assignment 1
   This program is written by: Charlton Trezevant (PID 4383060)
 */

#include  <stdlib.h>
#include <string.h>

/*
  ~ ~ Did You Know? ~ ~

  The supplied leak detector causes compiler warnings, and ironically appears to
  contain a memory leak.

  This is apparently because fclose() is never called on the leak detector's
  output file.

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

  My static analyzer complains about this:

   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c: In function ‘report_mem_leak’:
   /home/net/ch123722/COP3502C/Assignment1/src/leak_detector_c.c:190:30: warning: format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘void *’ [-Wformat=]
    sprintf(info, "address : %d\n", leak_info->mem_info.address);
                             ~^     ~~~~~~~~~~~~~~~~~~~~~~~~~~~
                             %p
*/

#include  "leak_detector_c.h"
#include <stdio.h>

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

typedef struct FileMeta {
        int numTestCases;
        int* testCaseNumCourses;

} FileMeta;


//// Required Function Prototypes

course *read_courses(FILE *fp, int *num_courses);
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections);
void process_courses(course *courses, int num_courses);
void release_courses(course *courses, int num_courses);

// My function prototypes

FileMeta* read_file_meta(FILE *fp);

// Prototypes for my memory manager
// Rave reviews:
// ==3291== All heap blocks were freed -- no leaks are possible
//                                       - Valgrind, 2020
typedef struct MemMgr_Entry {
        void* handle;
        size_t size;
        // todo: refcounts, passive gc sweeps
        // allow entries to be marked/given affinities for simpler release
} MemMgr_Entry;

typedef struct MemMgr_Table {
        // todo: benchmark current realloc strat vs lili vs pointer hash table
        MemMgr_Entry **entries;
        int numEntries;
        int *free;
        int numFree;
        volatile int mutex;
} MemMgr_Table;

MemMgr_Table *mmgr_init();
void *mmgr_malloc(MemMgr_Table *t, size_t size);
void mmgr_free(MemMgr_Table *t, void* handle);
void mmgr_cleanup(MemMgr_Table *t);
void mmgr_mutex_acquire(MemMgr_Table *t);
void mmgr_mutex_release(MemMgr_Table *t);

// Instance of my memory manager
MemMgr_Table* global_MEM;

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

FileMeta* read_file_meta(FILE *fp){
        FileMeta* meta = (FileMeta*) mmgr_malloc(global_MEM, sizeof(FileMeta));

        return meta;
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
        student **placeholder = (student**) mmgr_malloc(global_MEM, sizeof(student**));

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
                                mmgr_free(global_MEM, courses[i].sections[k]->scores);
                                mmgr_free(global_MEM, courses[i].sections[k]->lname);
                                mmgr_free(global_MEM, courses[i].sections[k]);
                        }

                        mmgr_free(global_MEM, courses[i].sections);
                        mmgr_free(global_MEM, courses[i].course_name);
                        mmgr_free(global_MEM, courses[i].num_students);
                        mmgr_free(global_MEM, courses[i].num_scores);
                }
        }

        mmgr_free(global_MEM, courses);
}

////////////////////////// Memory manager //////////////////////////

MemMgr_Table *mmgr_init(){
        MemMgr_Table *table = malloc(sizeof(MemMgr_Table));

        table->free = NULL;
        table->numFree = 0;

        table->entries = NULL;
        table->numEntries = 0;

        table->mutex = 0;

        debugf("mmgr: initialized\n");

        return table;
}

void *mmgr_malloc(MemMgr_Table *t, size_t size){
        mmgr_mutex_acquire(t);

        MemMgr_Entry* target;

        // todo: if an index is at the very end of the entries array, shrink the entire array
        if(t->numFree > 0) {
                debugf("mmgr: found reusable previously allocated entry %p\n", t->entries[t->free[t->numFree - 1]]);

                int targetIdx = t->free[t->numFree - 1];

                target = (MemMgr_Entry*) realloc(t->entries[targetIdx], sizeof(MemMgr_Entry) + size);
                target->size = size;

                t->entries[targetIdx] = target;

                t->free = (int*) realloc(t->free, (sizeof(int) * (t->numFree - 1)));

                t->numFree--;

                debugf("mmgr: reallocated %lu bytes\n", size);

        } else {
                debugf("mmgr: no recyclable entries available, increasing table size\n");

                t->entries = (MemMgr_Entry**) realloc(t->entries, (sizeof(MemMgr_Entry*) * (t->numEntries + 1)));

                t->entries[t->numEntries] = (MemMgr_Entry*) malloc(sizeof(MemMgr_Entry) + size);

                target = t->entries[t->numEntries];

                target->handle = malloc(size);
                target->size = size;

                t->numEntries++;

                debugf("mmgr: allocated %lu bytes, handle is %p\n", target->size, target->handle);
        }

        mmgr_mutex_release(t);

        return target->handle;
}


void mmgr_free(MemMgr_Table *t, void* handle){
        mmgr_mutex_acquire(t);

        int found = 0;

        debugf("mmgr: num active entries is %d, called to free %p\n", t->numEntries, handle);

        for(int i = t->numEntries; i--> 0; ) {
                if(handle == NULL) {
                        debugf("mmgr: provided NULL handle! no-op\n");
                        break;
                }

                if(t->entries[i]->handle == handle) {
                        debugf("mmgr: found handle %p at index %d\n", handle, i);

                        MemMgr_Entry* target = t->entries[i];

                        t->numFree++;

                        free(target->handle);
                        target->size = 0;

                        t->free = (int*) realloc(t->free, (sizeof(int) * (t->numFree + 1)));
                        t->free[t->numFree] = i;

                        found = 1;

                        debugf("mmgr: freed %p, %d entries remain active\n", handle, t->numEntries);
                        break;
                }
        }

        if(found == 0)
                debugf("mmgr: called to free %p but couldn't find it, no-op\n", handle);

        mmgr_mutex_release(t);
}

void mmgr_cleanup(MemMgr_Table *t){
        mmgr_mutex_acquire(t);

        int deEn = 0, deFr = 0;

        debugf("mmgr: cleaning up\n");

        for(int i = 0; i < t->numEntries; i++) {
                if(t->entries[i] != NULL) {
                        free(t->entries[i]->handle);
                        free(t->entries[i]);
                        deEn++;
                }
        }

        debugf("mmgr: cleanup deallocd %d of %d active, %d of %d free entries\n", deEn, (t->numEntries - t->numFree), deFr, t->numFree);

        free(t->entries);
        free(t->free);
        free(t);
}

void mmgr_mutex_acquire(MemMgr_Table *t){
        while (t->mutex == 1);
        t->mutex = 1;
}

void mmgr_mutex_release(MemMgr_Table *t){
        t->mutex = 0;
}
