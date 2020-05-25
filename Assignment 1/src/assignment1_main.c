/* COP 3502C Assignment 1
   This program is written by: Charlton Trezevant (PID 4383060)
 */

#include <stdio.h>
#include  <stdlib.h>
#include  "leak_detector_c.h"

#define DEBUG
#ifdef DEBUG
  #define debugf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr)
#else
  #define debugf(fmt, ...) ((void)0)
#endif

int main(){
        atexit(report_mem_leak); //memory leak detection
}
