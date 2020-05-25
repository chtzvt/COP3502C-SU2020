/*This program demonstrates a simple example of file I/O
and storing the data from the file into a structure.
It reads a text file with 5 students. A student has id number and score for 3 courses.
The program reads those scores and output the same information and average
score into another text file. Then it finds and writes the maximum average and
the student who obtained the max average to the new text file
*/
#include <stdio.h>
struct Student
{
    int student_ID;
    int g1, g2, g3;
    float average;

};

struct Student getMaxAverageStudent (struct Student *s)
{
    int i, max_idx = 0;
    float max = -1;
    for (i = 0; i < 5; i++)
    {
        if (s[i].average>max)
        {
            max = s[i].average;
            max_idx = i;

        }
    }

    return s[max_idx];



}

int main () {
    struct Student students[5];
    FILE * infile;
    FILE * outfile;
    int student_ID,i,g1,g2,g3;
    infile  = fopen(  "students.txt",   "r" );
    outfile = fopen( "new.txt" , "w");

    for (i =0; i < 5; i++)
    {
        fscanf( infile, "%d", &students[i].student_ID);
        fscanf( infile," %d %d %d ",  &students[i].g1, &students[i].g2, &students[i].g3);
        students[i].average = (students[i].g1 + students[i].g2+students[i].g3)/3;
        printf("%d %d %d %d %0.2f\n", students[i].student_ID, students[i].g1, students[i].g2,students[i].g3, students[i].average);
        fprintf(outfile,"%d %d %d %d %0.2f\n", students[i].student_ID, students[i].g1, students[i].g2,students[i].g3, students[i].average);
    }
    struct Student maxStudent = getMaxAverageStudent(students);

    printf("\nMaximum Average is %0.2f and the student is %d\n", maxStudent.average, maxStudent.student_ID);

    fprintf(outfile,"\nMaximum Average is %0.2f and the student is %d\n", maxStudent.average, maxStudent.student_ID);




   fclose(infile); // Close both files.
   fclose(outfile);

   printf("\nnew.txt file has been written. Please check the data in new.txt file\n");
   return 0;
}
