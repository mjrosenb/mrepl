#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <pty.h>

#include <ncurses.h>
#include <term.h>


#include <readline/readline.h>

#include "Line.h"
#include "MiddleEnd.h"
#include "BackEnd.h"
#include "FrontEnd.h"
#include "Util.h"
#include "Debug.h"

// With a 1920x1200 monitor, and a small font, I can get about 300 chars across
// be rather conservative, and assume that we'll see terminals that are 1024 wide

#define MAX_TERM_WIDTH 1024
WINDOW *codebuffer, *minibuffer, *regsbuffer;


EditedSnippet *cur_snippet;
ExecutionCtx *cur_ctx;

void redraw_codebuffer(Snippet *snip)
{

    int lineno = 1;
    werase(codebuffer);
    box(codebuffer, 0, 0);
    for (list<Line*>::const_iterator it = snip->code.begin();
         it != snip->code.end();
         it++, lineno++) {
        if (*cur_snippet->pos == *it) {
            wattron(codebuffer, A_REVERSE);
        }
        if ((*it)->getError()) {
            wattron(codebuffer, COLOR_PAIR(1));
        }
        mvwprintw(codebuffer, lineno, 1, "%s", (*it)->render());
        if ((*it)->getError()) {
            wattroff(codebuffer, COLOR_PAIR(1));
        }

        if (*cur_snippet->pos == *it) {
            wattroff(codebuffer, A_REVERSE);
        }
    }
    wrefresh(codebuffer);

}

struct regdata {
    const char *name;
    const int offset;
    regdata(const char *name_, int offset_) : name(name_), offset(offset_) {}
};

#define REG(R) regdata(#R, offsetof(user_regs_struct, R))
regdata regs[] = {
    REG(rax),
    REG(rbx),
    REG(rcx),
    REG(rdx),
    REG(rsi),
    REG(rdi),
    REG(rbp),
    REG(rsp),
    REG(r8),
    REG(r9),
    REG(r10),
    REG(r11),
    REG(r12),
    REG(r13),
    REG(r14),
    REG(r15),
    REG(rip),
};

static const int numRegs = sizeof(regs) / sizeof(regdata);
void redraw_regs(ExecutionCtx *cur_ctx)
{
    werase(regsbuffer);
    box(regsbuffer, 0, 0);
    if (cur_ctx->stage < 2) {
        wrefresh(regsbuffer);
        return;
    }
    char *regStore = reinterpret_cast<char*>(&cur_ctx->cur_pos->regs);
    for (int idx = 0; idx < numRegs; idx++) {
        mvwprintw(regsbuffer, idx+1, 1, "%s: %p",
                  regs[idx].name,
                  *reinterpret_cast<int**>(regStore + regs[idx].offset));
    }

    wrefresh(regsbuffer);
}

void do_exec()
{
    cur_ctx = new ExecutionCtx;
    generateMachineCode(cur_snippet, *cur_ctx);
    redraw_codebuffer(cur_snippet);
    if (cur_ctx->stage == 2)
        gatherTrace(*cur_ctx);
    redraw_regs(cur_ctx);
}

void handle_input(char *c)
{
    // Don't do anything if there is no input (for either definition of 'no input'
    if (c == NULL)
        return;
    // I may want to actually do something here...
    if (*c == '\0')
        return;
    // call the actual handler
    cur_snippet->handleLine(c);

    // run the engine, and display the results.
    do_exec();
    return;
}


int rl_prev(int arg, int key) {
    log("prev was called!\n");

    // if attempting to move up would cause us to move past the first element, turn back
    if (cur_snippet->pos == cur_snippet->code.begin()) {
        return 0;
    }

    // If we're currently looking at the last instruction, don't save its state when moving away
    if (cur_snippet->pos != cur_snippet->code.end()) {
        (*cur_snippet->pos)->saveText(rl_line_buffer);
    }

    // actuall move into the previous position
    cur_snippet->pos--;

    // restore the text of the previous position
    char *tmp = (*cur_snippet->pos)->restoreText();
    if (tmp == NULL) {
        rl_replace_line("", 1);
    } else {
        rl_replace_line(tmp, 1);
        rl_point = strlen(tmp);
    }

    redraw_codebuffer(cur_snippet);
    return 0;
}

int rl_next(int arg, int key) {
    log("next was called!\n");

    // if attempting to move up would cause us to move past the first element, turn back
    if (cur_snippet->pos == cur_snippet->code.end()) {
        return 0;
    }

    // always save when moving away
    (*cur_snippet->pos)->saveText(rl_line_buffer);


    // actuall move into the previous position
    cur_snippet->pos++;

    // If we're now looking at the last instruction, don't restore its state
    if (cur_snippet->pos != cur_snippet->code.end()) {
        // restore the text of the previous position
        char *tmp = (*cur_snippet->pos)->restoreText();
        if (tmp == NULL) {
            rl_replace_line("", 1);
        } else {
            rl_replace_line(tmp, 1);
            rl_point = strlen(tmp);
        }
    } else {
        rl_replace_line("", 1);
    }
    redraw_codebuffer(cur_snippet);
    return 0;
}

int rl_rstep(int arg, int key) {
    log("I was called!\n");
    if (cur_ctx->cur_pos == cur_ctx->trace.begin())
        return 0;
    cur_ctx->cur_pos--;
    redraw_codebuffer(cur_snippet);
    redraw_regs(cur_ctx);
    return 0;
}

int rl_step(int arg, int key) {
    log("I was called!\n");

    cur_ctx->cur_pos++;
    if (cur_ctx->cur_pos == cur_ctx->trace.end()) {
        cur_ctx->cur_pos--;
        return 0;
    }
    redraw_codebuffer(cur_snippet);
    redraw_regs(cur_ctx);
    return 0;
}

int rl_kill(int arg, int key) {
    log("kill was called!\n");

    // if we're already on the last line, then nop.
    if (cur_snippet->pos == cur_snippet->code.end()) {
        return 0;
    }

    cur_snippet->pos = cur_snippet->code.erase(cur_snippet->pos);
    // If we're now looking at the last instruction, don't restore its state
    if (cur_snippet->pos != cur_snippet->code.end()) {
        // restore the text of the previous position
        char *tmp = (*cur_snippet->pos)->restoreText();
        if (tmp == NULL) {
            rl_replace_line("", 1);
        } else {
            rl_replace_line(tmp, 1);
            rl_point = strlen(tmp);
        }
    } else {
        rl_replace_line("", 1);
    }
    do_exec();
    return 0;
}

void ui_init()
{
    // hax due to debugging foo
    winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);

    // standard ncurses setup:
    initscr();
    noecho();
    start_color();
    // initialize the error coloring
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    raw();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);
    resizeterm(ws.ws_row, ws.ws_col);
    // hook readline so it always goes through us
    rl_callback_handler_install ("", handle_input);
    rl_variable_bind("editing-mode", "emacs");
    Keymap km = rl_get_keymap();
    // up
    rl_set_key("[A",  rl_rstep, km);
    // down
    rl_set_key("[B",  rl_step, km);
    // C-p
    rl_set_key("",  rl_prev, km);
    // C-n
    rl_set_key("",  rl_next, km);

    // C-k
    rl_set_key("",  rl_kill, km);

    codebuffer = newwin(LINES-1, COLS-80, 0, 0);
    regsbuffer = newwin(LINES-1, 80, 0, COLS-80);
    minibuffer = newwin(1, COLS, LINES-1, 0);
    wrefresh(minibuffer);
    box(regsbuffer, 0, 0);
    wrefresh(regsbuffer);

    box(codebuffer, 0, 0);
    wrefresh(codebuffer);

    cur_snippet = new EditedSnippet;
    cur_ctx = new ExecutionCtx;
}


void ui_loop()
{
    char inbuf[MAX_TERM_WIDTH];
    while(1) {
        fd_set rd, wr, ex;
        FD_ZERO(&rd);
        FD_ZERO(&wr);
        FD_ZERO(&ex);
        FD_SET(STDIN_FILENO, &rd);
        int ret = select(1, &rd, &wr, &ex, NULL);
        if (ret == -1)
            continue;
        // is there input waiting on stdin?
        if (!FD_ISSET(STDIN_FILENO, &rd))
            continue;
        rl_callback_read_char();
        strncpy(inbuf, rl_line_buffer, 512);
        werase(minibuffer);
        mvwprintw(minibuffer, 0, 0, "%s", inbuf);
        mvwchgat(minibuffer, 0, rl_point, 1, A_REVERSE, 0, NULL);
        wrefresh(minibuffer);

    }
}

EditedSnippet::EditedSnippet()
    : Snippet(), pos(code.end())
{
}

void
EditedSnippet::handleLine(char *line)
{
    if (pos == code.end()) {
        // Special case: focus is currently on the end of the snippet,
        // we want to insert a new last node, and we want the focus to remain
        //  the last element
        code.push_back(new Line(line));
        return;
    }
    if ((*pos)->render() != NULL && strcmp(line, (*pos)->render()) == 0) {
        // if we haven't edited anything, no changes to save, so
        // insert a new blank line here,  leaving focus on the new line

        (*pos)->setText(line);
        pos++;
        pos = code.insert(pos, new Line(NULL));

    } else {
        // text has changed, probably some update, Don't bother trying to insert a new line.
        (*pos)->setText(line);
        rl_replace_line((*pos)->render(), 1);
        rl_point = strlen((*pos)->render());

    }
}
