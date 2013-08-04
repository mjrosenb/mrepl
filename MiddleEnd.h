#ifndef __MIDDLEEND_H
#define __MIDDLEEND_H
#include "Line.h"

struct RunLog {
    int outsize;
    char *out;
    int errsize;
    char *error;
    int exitStatus;
};

class ExecutableInfo {
 public:
    char *exename;
    Snippet *topLevel;
    void * sentinel;
    RunLog aslog;
    RunLog ldlog;
};

void generateMachineCode(Snippet *s, ExecutableInfo &info);

#endif
