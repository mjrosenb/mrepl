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
    // how far along has everything gotten?
    // 0 -> AS failed
    // 1 -> LD failed
    // 2 -> we executed
    int stage;
};

void generateMachineCode(Snippet *s, ExecutableInfo &info);

#endif
