#include <stdlib.h>

struct CharBuf {
    int numChars;
    int bufSize;
    char *string;
    CharBuf() : numChars(0), bufSize(4096), string((char*)malloc(bufSize)) {}
    void append(char *chr, int size) {
        if (size + numChars > bufSize) {
            while (size + numChars > bufSize) {
                bufSize *= 2;
            }
            string = (char*)realloc(string, bufSize);
        }
        memcpy(string + numChars, chr, size);
        numChars += size;
    }
};

struct tmpRunLog {
    CharBuf out;
    CharBuf err;
    int exitStatus;
};

void ParseAS(tmpRunLog &tmp, ExecutableInfo &info);
