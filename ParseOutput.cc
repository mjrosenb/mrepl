#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "Debug.h"
#include "MiddleEnd.h"
#include "ParseOutput.h"

void ParseLines(CharBuf &log, LinesBuf *dest)
{
    // scan through the character buffer, and count the number of '\n's
    char *left = log.string;
    int num_lines = 0;
    while ((left = strchr(left, '\n'))) {
        left++;
        num_lines++;
    }
    dest->numLines = num_lines;
    dest->numChars = log.numChars;
    dest->textLines = (char**)malloc(sizeof(char*)*num_lines);
    left = log.string;
    for (int i = 0; i < num_lines; i++) {
        dest->textLines[i] = left;
        left = strchr(left, '\n');
        *left = '\0';
        left++;
    }
}

void ParseAS(tmpRunLog &tmp, ExecutableInfo &info)
{
    fprintf(stderr, "parsing AS Output!\n");
    // split the temp buffer into a bunch of lines
    // which are easier to scan, etc.
    ParseLines(tmp.out, &info.aslog.out);
    ParseLines(tmp.err, &info.aslog.err);

    // Now, extract useful information from this file.
    for (int i = 0; i < info.aslog.err.numLines; i++) {
        char *curLine = info.aslog.err.textLines[i];
        fprintf(stderr, "Curent line is '%s'\n", curLine);
        // find the string "error"
        char *err = strstr(curLine, "Error:");
        if (err == NULL)
            continue;
        // The format that I seem to have here is filename:line: Error:'
        char *beforeNum = strchr(curLine, ':');
        assert(beforeNum != NULL);
        beforeNum++;
        long lineNum = strtol(beforeNum, NULL, 10);
        Line *asline = info.topLevel->lookupLine(lineNum);
        fprintf(stderr, "found error: %s on line %d\n", err, lineNum);
        asline->setError(err);
    }
}

void ParseLD(tmpRunLog &tmp, char *objfile, Snippet *s,  ExecutableInfo &info)
{
    // on my machine, ld outputs errors in the form:
    // objname: In function `func':
    // (section+offset): error
    // this stage is extra fun because the offsets are given within the object file
    // not within the source file.  THANKFULLY, I already have a mechanism in place
    // for parsing object files!
    extractLineInfo(objfile, s, info);
    ParseLines(tmp.out, &info.ldlog.out);
    ParseLines(tmp.err, &info.ldlog.err);

    for (int i = 0; i < info.ldlog.err.numLines; i++) {
        char *err = info.ldlog.err.textLines[i];
        if (*err != '(') {
            log("'%s' doesn't start with '('", err);
            continue;
        }
        char *close = strchr(err, ')');
        if (close == NULL) {
            log("'%s' doesn't contain with ')'", err);
            continue;
        }
        // gnu AS doesn't allow '+' as an identifier in labels, so we're good.
        // that being said, the global that it'll fall under should be _start.
        char *plus = strchr(err, '+');
        // sanity check: the + should be within the parens.
        if (plus > close) {
            log("'%s' doesn't contain '+' before ')'", err);
            continue;
        }
        char *end;
        long offset = strtol(plus+1, &end, 0);
        if (end != close) {
            log("'%s' the number doesn't end at ')' (it ends with '%s')", err, end);
            continue;
        }
        Line* errline = s->lookupLineByOffset(offset, false);
        assert(errline != NULL);
        errline->setError(end+2);
    }
}
