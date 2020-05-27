#include <stdio.h>
#include "leak_detector_c.h"

// debugf.h
// (c) Charlton Trezevant - 2018
#define DEBUG_LEVEL_ALL 0
#define DEBUG_LEVEL_MMGR 1
#define DEBUG_LEVEL_LOGIC 2
#define DEBUG_LEVEL_STATE 3
#define DEBUG_LEVEL_INFO 4
#define DEBUG_LEVEL_NONE 99

//#define DEBUG DEBUG_LEVEL_NONE

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

int main(void){
  atexit(report_mem_leak); //Ahmed's memory leak detection
  printf("Assignment 2!\n");
}
