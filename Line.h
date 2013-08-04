#ifndef __LINE_H
#define __LINE_H
#include <list>
#include <stdio.h>
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
    void dump(FILE *f, const char *name, int& count) const;
    void dumpTableEntry(FILE *f, const char *name, int& count) const;
    Line(char *);
    void setInst(void**&);
    void *getAddr();
    void setText(char *newtext);
    void saveText(char *newtext);
    char *restoreText() const;
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
public:
    list<Line*> code;
    void dump(FILE *f) const;
    void dumpTable(FILE *f) const;
    void setInst(void*);
    Snippet();
    void assignInsts(void**& insts);
};

#endif
