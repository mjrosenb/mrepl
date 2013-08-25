#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "Line.h"
#include "Debug.h"
Line::Line(char * text_) : text(text_), editedText(NULL)
{
}
void
Line::setInst(void**& newinst)
{
    if (text == NULL)
        return;
    inst = *newinst;
    newinst++;
}
void
Line::setText(char* newtext)
{
    text = newtext;

    // Since the new line has been finalized, any temp lines
    // are now obsolete.
    free(editedText);
    editedText = NULL;
    // editedText = strdup(newtext);
}

void
Line::setError(char* newtext)
{
    // Since the new line has been finalized, any temp lines
    // are now obsolete.
    // don't bother freeing error, it is guaranteed to point into an allocated structure.
    fprintf(stderr, "Attaching error to line \"%s\"\n", text);
    error = newtext;
}

char *
Line::getError()
{
    return error;
}

void
Line::saveText(char* newtext)
{
    // I can't think of any case that we'd ever want to save a blank line
    if (*newtext == '\0')
        return;
    // If there is already edited text, and it is the same as what we have here
    // don't do anything
    if (editedText != NULL && strcmp(newtext, editedText) == 0)
        return;

    // If there isn't any edited text, and we are the same as the current line
    // then there is no reason to do anything.
    if (editedText == NULL && strcmp(newtext, text) == 0)
        return;
    if (editedText != NULL)
        free (editedText);
    editedText = strdup(newtext);
}
char *
Line::restoreText() const
{
    if (editedText != NULL)
        return editedText;
    return text;
}

char *
Line::render() const
{
    return text;
}

void
Line::dump(FILE *f, const char* name, int &labelNum, int &lineNumber)
{
    if (text == NULL)
        return;
    lineNumber += 2;
    lineno = lineNumber;
    fprintf(f, "_line%s%d:\n", name, labelNum);
    fprintf(f, "\t%s\n", text);
    labelNum++;
}

void
Line::dumpTableEntry(FILE *f, const char* name, int &labelNum) const
{
    if (text == NULL)
        return;
    fprintf(f, ".quad _line%s%d\n", name, labelNum);
    labelNum++;
}

int
Line::getLineNo()
{
    return lineno;
}

Snippet::Snippet() : name(""), code()
{
}

void
Snippet::dump(FILE *f, int &lineno) const
{
    int labelNumber = 0;
    for (list<Line*>::const_iterator it = code.begin(); it != code.end(); it++) {
        (*it)->dump(f, name, labelNumber, lineno);
    }
}

void
Snippet::dumpTable(FILE *f) const
{
    int labelNumber = 0;
    for (list<Line*>::const_iterator it = code.begin(); it != code.end(); it++) {
        (*it)->dumpTableEntry(f, name, labelNumber);
    }
}


void
Snippet::assignInsts(void**& insts)
{
    for (list<Line*>::const_iterator it = code.begin(); it != code.end(); it++) {
        (*it)->setInst(insts);
    }

}

Line *
Snippet::lookupLine(int num)
{
    for (list<Line*>::iterator it = code.begin(); it != code.end(); it++) {
        fprintf(stderr, "LOOKUP-- %d: '%s'\n", (*it)->getLineNo(), (*it)->render());
        if ((*it)->getLineNo() == num)
            return *it;
    }
    return NULL;
}

void
Snippet::clearErrors()
{
    for (list<Line*>::iterator it = code.begin(); it != code.end(); it++) {
        (*it)->setError(NULL);
    }

}
