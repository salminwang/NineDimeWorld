#include "coroutine/context.h"
#include <string.h>

namespace Astra 
{

void Context::Reset() 
{
    m_baseaddr = NULL;
    m_stacktop_ = NULL;
    m_stacksize_ = 0;
}

void Context::Make(CoroutineEntranceFunc func, void* para, void* stackptr, uint32_t stacksize) 
{
    m_baseaddr = stackptr;

    char* top = (char*)stackptr + stacksize - 1;
    top = (char*)((size_t)top & -16LL);
    m_stacktop_ = top;
    m_stacksize_ = (size_t)top - (size_t)stackptr;

    memset(m_regs_, 0, sizeof(m_regs_));
    m_regs_[RID_RSP] = (char*)m_stacktop_ - 8;
    m_regs_[RID_RIP] = (void*)func;
    m_regs_[RID_RDI] = (void*)para;
}

}  // namespace bg