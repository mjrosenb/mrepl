#ifndef __DEBUG_H
#define __DEBUG_H
#if 0
#define log(...)
#else
#define log(FMT, ...) do { fprintf(stderr, "%s:%d: " FMT, __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#endif
#endif
