#pragma once
#include <stdint.h>

namespace Astra 
{

typedef void (*CoroutineEntranceFunc)(void* ptr);

struct Context 
{
    /*---------X86-64 16个寄存器-----------*/

    enum ENMREGISTERID {
        RID_RSP = 0,
        RID_RIP = 1,
        RID_RBX = 2,
        RID_RDI = 3,
        RID_RSI = 4,
        RID_RCX = 5,
        RID_RDX = 6,
        RID_RBP = 7,
        RID_R8 = 8,
        RID_R9 = 9,
        RID_R10 = 10,
        RID_R11 = 11,
        RID_R12 = 12,
        RID_R13 = 13,
        RID_R14 = 14,
        RID_R15 = 15,
        RID_MAX
    };
    /*---------X86-64 16个寄存器-----------*/

    void* m_regs_[RID_MAX];
    void* m_baseaddr;
    uint32_t m_stacksize_;
    void* m_stacktop_;

    void Reset();

    void Make(CoroutineEntranceFunc func, void* para, void* stackptr, uint32_t stacksize);
};

extern "C" 
{
    extern void ContextSwap(Context* Ctx1, Context* Ctx2) asm("ContextSwap");
};

}