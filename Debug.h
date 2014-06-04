#ifndef __DEBUG_H
#define __DEBUG_H
#include <stdio.h>
#if 0
#define log(...)
#else
extern FILE* errfd;
#define log(FMT, ...) do { fprintf(errfd, "%s:%d: " FMT, __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#endif
#endif
