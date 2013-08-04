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
}
void
Line::saveText(char* newtext)
{
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
Line::dump(FILE *f, const char* name, int &labelNum) const
{
    if (text == NULL)
        return;
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


Snippet::Snippet() : name(""), code()
{
}

void
Snippet::dump(FILE *f) const
{
    int labelNumber = 0;
    for (list<Line*>::const_iterator it = code.begin(); it != code.end(); it++) {
        (*it)->dump(f, name, labelNumber);
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
