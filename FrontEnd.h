#ifndef __FRONTEND_H
#define __FRONTEND_H
#include <list>
#include "Line.h"
class EditedSnippet : public Snippet
{
public:
    list<Line*>::iterator pos;
    EditedSnippet();
    void handleLine(char *line);
};
void ui_init();
void ui_loop();

#endif
