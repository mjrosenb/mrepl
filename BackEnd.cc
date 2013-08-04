#include <sys/ptrace.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <assert.h>

#include "BackEnd.h"
#include "Debug.h"
pid_t initTrace(char *name) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("executing child\n");
        exit(1);
    }
    if (pid == 0) {
        // we are the child, hook ourselves up fortracing, then execthe test executable
        log( "CHILD (%s)!\n", name);
        ptrace(PTRACE_TRACEME, pid, NULL, NULL);
        char * const args[] = {name, NULL};
        execv(name, args);
        perror ("exec-ing the child");
        exit(20);
    }
    return pid;
}

void gatherTrace(ExecutionCtx &cx) {
    pid_t child = initTrace(cx.exename);
    int status;
    // The child just started running, so it'll stop at the exec point.
    // Now, we want to continue the process until we get to the beginning of the process
    waitpid(child, &status, 0);
    int exited =  WIFEXITED(status);
    int s = WEXITSTATUS(status);
        //    assert( exited && (WST(status) == SIGTRAP));
    ptrace(PTRACE_CONT, child, 0, 0);

    // Now we should be executing the REPL code.
    int steps = 20;
    while (true) {
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status)) {
            ExecutionState curState;
            ptrace(PTRACE_GETREGS, child, NULL, &curState.regs);
            
            ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
            cx.trace.push_back(curState);
            if (((void*)curState.regs.rip) == cx.sentinel) {
                cx.cur_pos--;
                return;
            }
            if (--steps == 0)
                return;

        } else {
            log("Why on earth did we stop?\n");
            assert(false);
        }
    }
}

ExecutionCtx::ExecutionCtx()
    : ExecutableInfo(), trace(), cur_pos(trace.end())
{
}
