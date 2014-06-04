
#include "FrontEnd.h"
/* for terminfo usage only. curses input/output is never used */
FILE* errfd;

int main()
{
    errfd = fopen("err.log", "w");
    if (!errfd)
        errfd = stderr;
    ui_init();
    ui_loop();
}
