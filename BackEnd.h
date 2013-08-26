#ifndef __BACKEND_H
#define __BACKEND_H
#include <list>
#include <sys/user.h>
#include "MiddleEnd.h"
class ExecutionState {
public:
    user_regs_struct regs;
    Line *line;
    void *getIP();

};

class ExecutionCtx : public ExecutableInfo {
public:
    list<ExecutionState> trace;
    list<ExecutionState>::iterator cur_pos;
    ExecutionCtx();
};

void gatherTrace(ExecutionCtx &ctx);

#endif
