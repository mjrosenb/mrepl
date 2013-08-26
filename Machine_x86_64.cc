#include "BackEnd.h"
void *
ExecutionState::getIP()
{
    return reinterpret_cast<void*>(regs.rip);
}
