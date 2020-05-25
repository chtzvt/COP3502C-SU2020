/* COP 3502C Assignment 1
   This program is written by: Charlton Trezevant (PID 4383060)
 */

#include  <stdlib.h>
#include  "leak_detector_c.c"
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

#define FILENAME "in.txt"

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
        student **sections; //stores array of student arrays(2D array). Size is num_sections;
        int *num_students; //stores array of number of students in each section. Size is num_sections; int *num_scores; //stores array of number of assignments in each section. Size is num_sections;
} course;

//// Function Prototypes

course *read_courses(FILE *fp, int *num_courses);
student **read_sections(FILE *fp, int num_students[], int num_scores[], int num_sections);
void process_courses(course *courses, int num_courses);
void release_courses(course *courses, int num_courses);

//// Entry

int main(){
        FILE *infile;
        
        infile = fopen(FILENAME, "r");
        if (infile == NULL)
        {
          printf("Failed to open input file %s\n", FILENAME);
          return 1;
        }


        debugf("Started!\n");

        atexit(report_mem_leak); //memory leak detection
        
        return 0;
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
  course *placeholder = malloc(sizeof(student**));

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
  student **placeholder = malloc(sizeof(student**));

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

   Translation: free()
 */
void release_courses(course *courses, int num_courses){

}
