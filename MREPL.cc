
#include "FrontEnd.h"
/* for terminfo usage only. curses input/output is never used */


int main()
{
    ui_init();
    ui_loop();
}
