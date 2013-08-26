#ifndef __LINE_H
#define __LINE_H
#include <list>
#include <stdio.h>
#include <elf.h>

// hardcode us to 64 bit elf for now, this is easy enough
// to make conditional later
#define Elf_Shdr Elf64_Shdr
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Sym  Elf64_Sym

using namespace std;
// The repl accepts a single line at a time, stores them in an internal structure
class Line {
    // The line of text that was entered
    char *text;
    char *editedText;
    char *error;
    int lineno;
    // the beginning of the instruction sequence that was generated for this line
    void *inst;
public:
    // render to the screen
    char *render() const;
    // format to be dumped into the assembly file
    void dump(FILE *f, const char *name, int& count, int &lineno);
    void dumpTableEntry(FILE *f, const char *name, int& count) const;
    Line(char *);
    void setInst(Elf_Sym *& sym);
    void *getAddr();
    void setText(char *newtext);
    void saveText(char *newtext);
    char *restoreText() const;
    void setError(char *err);
    char *getError();
    int getLineNo();
};

class MetaLine : public Line {
};
class  AssignLine : public MetaLine {
};

// A Snippet is a series of lines to be compiled/executed
// sort of like a function. Not entirely sure why there would be
// more than one of these.
class Snippet {
    const char *name;
    void *base;
public:
    list<Line*> code;
    void dump(FILE *f, int &lineno) const;
    void dumpTable(FILE *f) const;
    void setInst(void*);
    Snippet();
    void assignInsts(Elf_Sym*& syms);
    Line *lookupLine(int num);
    Line *lookupLineByOffset(long offset, bool exact);
    Line *lookupLineByAddr(void *offset, bool exact);
    void clearErrors();
};

#endif
