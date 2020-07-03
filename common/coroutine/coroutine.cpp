#include "coroutine.h"
#include <time.h>
#include <stdio.h>
#include "base/memory_pool.h"
#include "coroutine/coroutine_mgr.h"

namespace Astra {

CoroutineData::CoroutineData() {
}

void CoroutineData::Construct() {
    m_coroutine_id = 0;
    m_inst_id = 0;
    m_status = ES_INIT;
    m_parent = NULL;
    m_first_child = NULL;
    m_next_sibling = NULL;
    memset(&m_ctx, 0, sizeof(m_ctx));
    m_start_para = NULL;
    m_private_buffer.Finalize();
    m_start_time = 0;
    m_used_stack_size = 0;
    m_wait_msgid = 0;
    m_timeout = false;
    m_msg = NULL;
    m_last_active_time = 0;
    m_pool_idx = 0;
    m_coroutine_mgr = NULL;
    m_user_data_idx = 0;
    m_bind_ret = 0;
    m_wait_lock_succeed = false;
    m_wait_lock_key = NULL;
    m_lock_key_set.clear();
}

void CoroutineData::Setup(CoroutineMgr* mgr, uint32_t coroutineId, uint64_t instId, uint32_t idx, uint32_t buffLen) {
    m_coroutine_id = coroutineId;
    m_inst_id = instId;
    m_pool_idx = idx;
    m_start_time = time(0);
    m_last_active_time = time(0);
    m_coroutine_mgr = mgr;

    m_private_buffer.SetBuffLen(buffLen);
}

void CoroutineData::SetStaus(ENMSTATUS eStatus) {
    m_status = eStatus;
}

//私有栈 恢复 运行栈
int CoroutineData::RestoreRunStack() {
    if (0 == m_coroutine_id) {
        BG_ERROR("resotre run stack but coroutine id is 0, frame must wrong!!");
        return 0;
    }

    if (false == IsActive() || m_used_stack_size <= 0) {
        return 0;
    }

    m_private_buffer.CopyBufferOut(m_used_stack_size, (char*)m_ctx.m_stacktop_ - m_used_stack_size);
    return (int)m_used_stack_size;
}

//运行栈 保存 私有栈
int CoroutineData::SaveRunStack() {
    if (0 == m_coroutine_id) {
        return 0;
    }

    if (false == IsActive() || m_used_stack_size <= 0) {
        return 0;
    }

    if (m_used_stack_size > MAX_COROUTINE_STACK_SIZE) {
        BG_WARN("Coroutine(%u, %lu) Save Running Privatestack Size(%u) too much", m_coroutine_id, m_inst_id,
                m_used_stack_size);
    }
    // memcpy(m_pStack,(char*)m_stCtx.m_pStackTop - m_u32UsedStackSize,m_u32UsedStackSize);
    m_private_buffer.CopyBufferIn(m_used_stack_size, (char*)m_ctx.m_stacktop_ - m_used_stack_size);
    return (int)m_used_stack_size;
}

void CoroutineData::UpdateStackSize() {
    m_used_stack_size = CoroutineMgr::GetStackSize(m_ctx.m_stacktop_);
}

void CoroutineData::MakeCoroutine(CoroutineEntranceFunc pFunc, CoroutineData& stCallerData,
                                  CoroutineData::Para& stPara) {
    stPara.m_data = this;
    if (0 == stCallerData.m_coroutine_id) {
        // MDEBUG("calldata id is main, make coroutine id:%u instid:%lu",m_u32CoroutineID,m_u64InstID);
        m_ctx.Make(pFunc, (void*)&stPara, m_coroutine_mgr->GetRunStackInfo().m_baseaddr,
                   m_coroutine_mgr->GetRunStackInfo().m_size);
    } else {
        // MDEBUG("call id:%u instid:%lu, make coroutine id:%u instid:%lu", stCallerData.m_u32CoroutineID,
        // stCallerData.m_u64InstID, m_u32CoroutineID, m_u64InstID);
        char* rsp = (char*)stCallerData.m_ctx.m_stacktop_ - stCallerData.m_used_stack_size;
        size_t uFreeStackSize = (size_t)rsp - (size_t)m_coroutine_mgr->GetRunStackInfo().m_baseaddr;
        m_ctx.Make(pFunc, (void*)&stPara, m_coroutine_mgr->GetRunStackInfo().m_baseaddr, uFreeStackSize);
    }
}

void CoroutineData::Exit() {
    BG_DEBUG("Coroutine(%u, %lu) exit\n", m_coroutine_id, m_inst_id);
    m_status = ES_DONE;
    m_coroutine_mgr->SwapOut();
}

void CoroutineData::AddChild(CoroutineData* pChild) {
    if (NULL == pChild || NULL != pChild->m_next_sibling || NULL != pChild->m_parent) {
        BG_ERROR("coroutine id:%u instid:%lu AddChild failed!", m_coroutine_id, m_inst_id);
        return;
    }

    //有序链表
    CoroutineData** ptr = &m_first_child;
    while (*ptr != NULL && *ptr < pChild) {
        ptr = &((*ptr)->m_next_sibling);
    }

    pChild->m_parent = this;
    pChild->m_next_sibling = *ptr;
    *ptr = pChild;
}

void CoroutineData::DeleteChild(CoroutineData* pChild) {
    if (NULL == pChild) {
        BG_ERROR("Coroutine(%u, %lu) delete child but null", m_coroutine_id, m_inst_id);
        return;
    }

    CoroutineData** pPtr = &m_first_child;

    while (*pPtr != NULL && *pPtr < pChild) {
        pPtr = &((*pPtr)->m_next_sibling);
    }

    if (NULL == *pPtr || *pPtr > pChild) {
        return;
    }

    *pPtr = (*pPtr)->m_next_sibling;
}

bool CoroutineData::IsActive() {
    return (ES_WAITING == m_status || ES_RUNNING == m_status);
}

void CoroutineData::Join() {
    while (NULL != m_first_child) {
        BG_DEBUG("Coroutine(%u, %lu) join waiting\n", m_coroutine_id, m_inst_id);
        Wait();
    }
}

void CoroutineData::Wait() {
    BG_DEBUG("Coroutine(%u, %lu) waiting\n", m_coroutine_id, m_inst_id);
    m_status = ES_WAITING;
    m_coroutine_mgr->SwapOut();
}

void CoroutineData::OnStart() {
    BG_DEBUG("Coroutine(%u, %lu) OnStart\n", m_coroutine_id, m_inst_id);
    m_status = ES_RUNNING;
}

void CoroutineData::OnTimeOut() {
    BG_DEBUG("Coroutine(%u, %lu) OnTimeOut\n", m_coroutine_id, m_inst_id);
    m_status = ES_RUNNING;
    m_timeout = true;
}

void CoroutineData::OnFork() {
    BG_DEBUG("Coroutine(%u, %lu) OnFork by Parent(%u, %lu)\n", m_coroutine_id, m_inst_id, m_parent->m_coroutine_id,
             m_parent->m_inst_id);
}
void CoroutineData::OnMsg(void* pMsg) {
    BG_DEBUG("Coroutine(%u, %lu) OnMsg\n", m_coroutine_id, m_inst_id);
    if (ES_WAITING != m_status) {
        return;
    }

    m_msg = pMsg;
    m_status = ES_RUNNING;
}

void CoroutineData::OnMsg() {
    BG_DEBUG("Coroutine(%u, %lu) OnMsg\n", m_coroutine_id, m_inst_id);
    if (ES_WAITING != m_status) {
        return;
    }

    m_status = ES_RUNNING;
}

void CoroutineData::Finalize() {
    BG_DEBUG("Coroutine(%u, %lu) Finalize\n", m_coroutine_id, m_inst_id);
    m_private_buffer.Finalize();

    while (m_lock_key_set.size() > 0) {
        BG_DEBUG("Coroutine(%u, %lu) have LockKey", m_coroutine_id, m_inst_id);
        m_coroutine_mgr->Unlock(*this, *m_lock_key_set.begin(), false);
    }

    m_lock_key_set.clear();

    if (NULL != m_wait_lock_key) {
        m_coroutine_mgr->ResetWaitLock(*this);
        if (NULL != m_wait_lock_key) {
            POOLED_DELETE(m_wait_lock_key);
            m_wait_lock_key = NULL;
        }
    }
}

void CoroutineData::OnBindFailed(int iRet) {
    m_bind_ret = iRet;
    BG_ERROR("CoroutineID:%u InstID:%lu Bind failed:%d ", m_coroutine_id, m_inst_id, iRet);
}

void CoroutineData::SetWaitingKey(const CoroutineLockKey& stKey) {
    if (NULL != m_wait_lock_key) {
        return;
    }

    m_wait_lock_key = POOLED_NEW(CoroutineLockKey, stKey);
}

void CoroutineData::AddLockingKey(const CoroutineLockKey& stKey) {
    m_lock_key_set.insert(stKey);
}

void CoroutineData::ResetWaitingKey() {
    if (NULL == m_wait_lock_key) {
        return;
    }

    POOLED_DELETE(m_wait_lock_key);
    m_wait_lock_key = NULL;
}

void CoroutineData::OnWaitLock(bool bSucceed) {
    BG_DEBUG("Coroutine(%u, %lu) OnWaitLock\n", m_coroutine_id, m_inst_id);
    if (ES_WAITING != m_status) {
        return;
    }

    m_wait_lock_succeed = bSucceed;
    m_status = ES_RUNNING;
}

/*****************************分界线，上面是data*********************************/
/*static*/ CoroutineData*& ICoroutine::GetCoroutineDataPtr() {
    static __thread CoroutineData* pPtr;
    return pPtr;
}

ICoroutine::ICoroutine(uint32_t id) {
    m_u32ID = id;
    CoroutineMgr::RegisterCoroutine(*this);
}

int ICoroutine::_ForkImpl(IFuncBinder* func_binder, size_t func_binder_size) {
    CoroutineData& stData = *CoroutineMgr::GetCurCoroutineDataPtr();
    return stData.m_coroutine_mgr->Fork(func_binder, func_binder_size);
}

CoroutineInfo* ICoroutine::GetUserData(CoroutineMgr* coMgr, uint64_t instId) {
    if (NULL == coMgr) {
        return NULL;
    }
    return coMgr->GetUserDataBuffer(instId);
}

int ICoroutine::OnContinue() {
    CoroutineData& co_data = *GetCoroutineDataPtr();
    int iRet = BindData();
    if (0 != iRet) {
        co_data.OnBindFailed(iRet);
        return iRet;
    }

    return 0;
}

int ICoroutine::OnInnerStart(CoroutineData& co_data, void* para) {
    return OnStart(co_data, para);
}

int ICoroutine::OnInnerEnd(CoroutineData& co_data) {
    return OnEnd(co_data);
}

int ICoroutine::WaitLock(bool& wait, const CoroutineLockKey& lockKey) {
    CoroutineData& stData = *GetCoroutineDataPtr();
    wait = false;

    if (false == stData.m_coroutine_mgr->WaitLock(stData, lockKey)) {
        return COROUTINE_SUCCESS;
    }

    stData.Wait();

    stData.m_coroutine_mgr->ResetWaitLock(stData);

    int iRet = this->OnContinue();
    if (0 != iRet) {
        BG_ERROR("Coroutine(%u,%lu) OnContinue() returns %d\n", stData.m_coroutine_id, stData.m_inst_id, iRet);
    }

    if (0 != stData.m_bind_ret) {
        throw CouroutineBindFailedException(stData.m_bind_ret);
    }

    if (stData.m_timeout) {
        return COROUTINE_WAIT_TIMEOUT_ERR;
    }

    if (false == stData.m_wait_lock_succeed) {
        return COROUTINE_WAIT_LOCK_ERR;
    }

    wait = true;
    return 0;
}

int ICoroutine::WaitTillUnlock(uint16_t type, uint64_t key1, uint64_t key2, uint64_t key3, uint64_t key4,
                               uint64_t key5) {
    return WaitTillUnlock(CoroutineLockKey(type, key1, key2, key3, key4, key5));
}

int ICoroutine::WaitTillUnlock(const CoroutineLockKey& key) {
    while (true) {
        bool bWaited = false;
        int iRet = WaitLock(bWaited, key);
        if (COROUTINE_SUCCESS != iRet) {
            return iRet;
        }
        if (bWaited) {
            continue;
        }
        break;
    }

    return COROUTINE_SUCCESS;
}

int ICoroutine::WaitLock(bool& wait, uint16_t type, uint64_t key1, uint64_t key2, uint64_t key3, uint64_t key4,
                         uint64_t key5) {
    CoroutineLockKey stKey(type, key1, key2, key3, key4, key5);
    return WaitLock(wait, stKey);
}

bool ICoroutine::Lock(const CoroutineLockKey& key) {
    CoroutineData& stData = *GetCoroutineDataPtr();
    return stData.m_coroutine_mgr->Lock(stData, key);
}

bool ICoroutine::WaitAndLock(const CoroutineLockKey& key) {
    CoroutineData& stData = *GetCoroutineDataPtr();
    while (stData.m_coroutine_mgr->IsLockExist(key)) {
        bool bWaited = false;
        int iRet = WaitLock(bWaited, key);
        if (COROUTINE_SUCCESS != iRet) {
            return iRet;
        }
    }

    return Lock(key);
}

bool ICoroutine::Lock(uint16_t type, uint64_t key1, uint64_t key2, uint64_t key3, uint64_t key4, uint64_t key5) {
    CoroutineData& stData = *GetCoroutineDataPtr();
    return stData.m_coroutine_mgr->Lock(stData, CoroutineLockKey(type, key1, key2, key3, key4, key5));
}

void ICoroutine::Unlock(const CoroutineLockKey& key) {
    CoroutineData& stData = *GetCoroutineDataPtr();
    stData.m_coroutine_mgr->Unlock(stData, key, true);
}

int ICoroutine::WaitMsgNil() {
    CoroutineData* data = GetCoroutineDataPtr();
    if (NULL == data || data->m_coroutine_id == 0 || data->m_inst_id == 0) {
        BG_ERROR("WaitMsg error,curdata is wrong!!!\n");
        return COROUTINE_WAIT_PARA_ERR;
    }

    data->Wait();
    int iRet = this->OnContinue();
    if (iRet) {
        BG_ERROR("OnContinue failed for (%u:%lu) ret:%d\n", data->m_coroutine_id, data->m_inst_id, iRet);
    }

    if (0 != data->m_bind_ret) {
        BG_ERROR("Bind failed for (%u:%lu) ret:%d\n", data->m_coroutine_id, data->m_inst_id, data->m_bind_ret);
        throw CouroutineBindFailedException(data->m_bind_ret);
    }

    if (data->m_timeout) {
        BG_ERROR("coroutine(%u-%lu) timeout", data->m_coroutine_id, data->m_inst_id);
        return COROUTINE_WAIT_TIMEOUT_ERR;
    }

    BG_DEBUG("coroutine(%u-%lu) wait res msg", data->m_coroutine_id, data->m_inst_id);
    return iRet;
}

CCoroutineLockAgent::CCoroutineLockAgent(ICoroutine& co, const CoroutineLockKey& key)
        : m_is_lock_(false), m_co_(co), m_lock_key_(key) {
}

CCoroutineLockAgent::CCoroutineLockAgent(ICoroutine& co, uint16_t type, uint64_t key1, uint64_t key2, uint64_t key3,
                                         uint64_t key4, uint64_t key5)
        : m_is_lock_(false), m_co_(co), m_lock_key_(type, key1, key2, key3, key4, key5) {
}

CCoroutineLockAgent::~CCoroutineLockAgent() {
    if (m_is_lock_) {
        m_co_.Unlock(m_lock_key_);
    }
}

bool CCoroutineLockAgent::Lock() {
    if (m_is_lock_) {
        return true;
    }

    m_is_lock_ = m_co_.Lock(m_lock_key_);
    return m_is_lock_;
}

bool CCoroutineLockAgent::WaitAndLock() {
    if (m_is_lock_) {
        return true;
    }

    m_is_lock_ = m_co_.WaitAndLock(m_lock_key_);
    return m_is_lock_;
}
}  // namespace bg