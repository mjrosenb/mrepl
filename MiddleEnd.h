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
void extractLineInfo(const char *exefile, Snippet *snip, ExecutableInfo &info);

// hardcode us to 64 bit elf for now, this is easy enough
// to make conditional later
#define Elf_Shdr Elf64_Shdr
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Sym  Elf64_Sym

#endif
