#ifndef __MIDDLEEND_H
#define __MIDDLEEND_H
#include "Line.h"

struct LinesBuf {
    int numLines;
    int numChars;
    char ** textLines;
};

struct RunLog {
    LinesBuf out;
    LinesBuf err;
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
