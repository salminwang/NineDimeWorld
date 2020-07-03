#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <set>
#include <memory>
#include "context.h"
#include "coroutine/coroutine_buffer.h"
#include "base/func_bind_helper.h"
#include "coroutine/coroutine_error.h"
#include "log/log.h"
namespace Astra {

class CoroutineMgr;

#define MAX_COROUTINE_INFOSIZE 5120     //单个协程用户数据大小
#define MAX_COROUTINE_STACK_SIZE 10240  //单个协程的,只用来做检查告警

// bind的excetion定义
class CouroutineBindFailedException {
public:
    CouroutineBindFailedException(int ret) {
        m_result = ret;
    }

protected:
    int m_result;
};

struct CoroutineInfo {
    CoroutineInfo() {
        Reset();
    }

    void Reset() {
        m_init = false;
        memset(this, 0, sizeof(*this));
    }
    bool m_init;
    char m_abyInfo_[MAX_COROUTINE_INFOSIZE];
};

enum ENMSTATUS {
    ES_INIT = 0,
    ES_RUNNING = 1,
    ES_WAITING = 2,
    ES_DONE = 3,
};

struct StackInfo {
    void* m_baseaddr;
    uint32_t m_size;
};

//协程锁
struct CoroutineLockKey {
    static const uint32_t KEY_NUM = 5;

    CoroutineLockKey() {
    }

    CoroutineLockKey(const CoroutineLockKey& key) {
        *this = key;
    }

    CoroutineLockKey(uint16_t type, uint64_t key1 = 0, uint64_t key2 = 0, uint64_t key3 = 0, uint64_t key4 = 0,
                     uint64_t key5 = 0) {
        m_type = type;
        m_key[0] = key1;
        m_key[1] = key2;
        m_key[2] = key3;
        m_key[3] = key4;
        m_key[4] = key5;
    }

    inline bool operator==(const CoroutineLockKey& key) const {
        if (m_type != key.m_type) {
            return false;
        }

        for (uint32_t i = 0; i < KEY_NUM; ++i) {
            if (m_key[i] != key.m_key[i]) {
                return false;
            }
        }

        return true;
    }

    inline bool operator<(const CoroutineLockKey& key) const {
        if (m_type != key.m_type) {
            return m_type < key.m_type;
        }

        for (uint32_t i = 0; i < KEY_NUM; ++i) {
            if (m_key[i] != key.m_key[i]) {
                return m_key[i] < key.m_key[i];
            }
        }

        return false;
    }

    uint16_t m_type;
    uint64_t m_key[KEY_NUM];
};

typedef std::shared_ptr<CoroutineInfo> CoroutineInfoPtr;

//协程的数据类
class CoroutineData {
public:
    friend class CoroutineMgr;
    typedef std::set<CoroutineLockKey> LockKeySet;
    struct Para {
        CoroutineData* m_data;
        void* m_para;
    };

    CoroutineData();
    void Construct();
    void Setup(CoroutineMgr* mgr, uint32_t coroutineId, uint64_t instId, uint32_t idx, uint32_t buffLen);
    void OnMsg(void* pMsg);
    void OnMsg();
    void SetStaus(ENMSTATUS eStatus);
    int RestoreRunStack();
    int SaveRunStack();
    void Exit();
    void UpdateStackSize();
    void MakeCoroutine(CoroutineEntranceFunc pFunc, CoroutineData& stCallerData, CoroutineData::Para& stPara);
    void AddChild(CoroutineData* pChild);
    void DeleteChild(CoroutineData* pChild);
    void Join();
    bool IsActive();
    void Wait();
    void OnStart();
    void OnTimeOut();
    void OnFork();
    void Finalize();
    void OnBindFailed(int iRet);
    void SetWaitingKey(const CoroutineLockKey& stKey);
    void ResetWaitingKey();
    void AddLockingKey(const CoroutineLockKey& stKey);
    void OnWaitLock(bool bSucceed);

public:
    uint32_t m_coroutine_id;  //协程的id
    uint64_t m_inst_id;       //协程实例id
    ENMSTATUS m_status;       //协程状态

    CoroutineData* m_parent;        //父协程
    CoroutineData* m_first_child;   //第一个子协程
    CoroutineData* m_next_sibling;  //下一个协程

    Context m_ctx;                     //协程的上下文
    void* m_start_para;                //启动参数
    uint32_t m_used_stack_size;        //使用的栈大小
    CoroutineBuffer m_private_buffer;  //私有栈信息
    uint32_t m_start_time;             //启动时间
    // uint64_t m_u64StartUsTime;//启动时间 us级

    uint32_t m_wait_msgid;  //等待的消息ID
    bool m_timeout;         //是否超时
    void* m_msg;            //返回的协议指针

    size_t m_last_active_time;  //上次激活时间
    uint32_t m_user_data_idx;   //存储业务层数据
    uint32_t m_pool_idx;        //池子的索引

    CoroutineMgr* m_coroutine_mgr;

    int m_bind_ret;

    CoroutineLockKey* m_wait_lock_key;
    LockKeySet m_lock_key_set;
    bool m_wait_lock_succeed;
};

#define DECLARE_COROUTINE(CoroutineType) CoroutineType g_st##CoroutineType##Inst;
#define FORK_COROUTINE(...) Fork(this, &__VA_ARGS__)
#define JOIN_COROUTINE() JoinCoroutine()

#define FORK_COROUTINE_BYOTHER(pCoroutine, obj, ...) pCoroutine->Fork(obj, &__VA_ARGS__)
#define JOIN_COROUTINE_BYOTHER(pCoroutine) pCoroutine->JoinCoroutine()

class ICoroutine {
public:
    virtual int OnStart(CoroutineData& co_data, void* para) = 0;
    virtual int OnEnd(CoroutineData& co_data) {
        return 0;
    }

public:
    friend class CoroutineMgr;
    ICoroutine(uint32_t id);
    //获取协程的缓存区
    CoroutineInfo* GetUserData(CoroutineMgr* coMgr, uint64_t instId);
    int OnContinue();
    int OnInnerStart(CoroutineData& co_data, void* para);
    int OnInnerEnd(CoroutineData& co_data);

    template <typename ObjType, typename FuncType, typename... Args>
    int Fork(ObjType&& stObj, FuncType&& pFunc, Args&&... args);

    uint32_t GetID() {
        return m_u32ID;
    }
    //挂起协程，业务层调用该函数后
    template <typename MsgType>
    int WaitMsg(uint32_t u32MsgID, MsgType*& pMsg);

    int WaitMsgNil();

    //获取协程的用户数据
    template <typename _DataType>
    _DataType& GetCoroutineInfo();

    //获取当前运行协程的实例ID
    uint64_t GetInstID() {
        return GetCoroutineDataPtr()->m_inst_id;
    }

    template <typename DATA_TYPE>
    DATA_TYPE& GetCoroutineParaData(void* para) {
        return *(DATA_TYPE*)para;
    }

    void JoinCoroutine() {
        GetCoroutineDataPtr()->Join();
    }

    virtual int SetUp(void* pPrara) {
        return 0;
    }

    virtual int BindData() {
        return 0;
    }

    //判断是否需要加锁,wait返回是否需要等待
    int WaitLock(bool& wait, uint16_t uType, uint64_t key1 = 0, uint64_t key2 = 0, uint64_t key3 = 0, uint64_t key4 = 0,
                 uint64_t key5 = 0);
    //等待加锁,wait返回是否需要等待
    int WaitLock(bool& wait, const CoroutineLockKey& key);

    //等待加锁,如果有所一直等待
    int WaitTillUnlock(uint16_t type, uint64_t key1 = 0, uint64_t key2 = 0, uint64_t key3 = 0, uint64_t key4 = 0,
                       uint64_t key5 = 0);

    //等待加锁,如果有所一直等待
    int WaitTillUnlock(const CoroutineLockKey& key);

    //加锁
    bool Lock(uint16_t type, uint64_t key1 = 0, uint64_t key2 = 0, uint64_t key3 = 0, uint64_t key4 = 0,
              uint64_t key5 = 0);
    //加锁
    bool Lock(const CoroutineLockKey& key);
    //等待和加锁
    bool WaitAndLock(const CoroutineLockKey& key);

    void Unlock(const CoroutineLockKey& key);

protected:
    static CoroutineData*& GetCoroutineDataPtr();
    int _ForkImpl(IFuncBinder* func_binder, size_t func_binder_size);

private:
    uint32_t m_u32ID;  //协程ID
};

template <typename MsgType>
int ICoroutine::WaitMsg(uint32_t u32MsgID, MsgType*& pMsg) {
    CoroutineData* pstData = GetCoroutineDataPtr();
    if (NULL == pstData || pstData->m_coroutine_id == 0 || pstData->m_inst_id == 0) {
        BG_ERROR("WaitMsg error,curdata is wrong!!!\n");
        return COROUTINE_WAIT_PARA_ERR;
    }

    pstData->m_wait_msgid = u32MsgID;
    pstData->Wait();
    int iRet = this->OnContinue();
    if (iRet) {
        BG_ERROR("OnContinue failed for (%u:%lu) ret:%d\n", pstData->m_coroutine_id, pstData->m_inst_id, iRet);
    }

    if (0 != pstData->m_bind_ret) {
        BG_ERROR("Bind failed for (%u:%lu) ret:%d\n", pstData->m_coroutine_id, pstData->m_inst_id, pstData->m_bind_ret);
        throw CouroutineBindFailedException(pstData->m_bind_ret);
    }

    if (pstData->m_timeout) {
        BG_ERROR("coroutine(%u-%lu) timeout", pstData->m_coroutine_id, pstData->m_inst_id);
        return COROUTINE_WAIT_TIMEOUT_ERR;
    }

    BG_DEBUG("coroutine(%u-%lu) wait res msg", pstData->m_coroutine_id, pstData->m_inst_id);
    pMsg = (MsgType*)pstData->m_msg;
    return iRet;
}

//获取协程的用户数据
template <typename _DataType>
_DataType& ICoroutine::GetCoroutineInfo() {
    if (sizeof(_DataType) > MAX_COROUTINE_INFOSIZE) {
        assert(0);
    }

    CoroutineInfo* pstUserData = GetUserData(GetCoroutineDataPtr()->m_coroutine_mgr, GetCoroutineDataPtr()->m_inst_id);
    assert(NULL != pstUserData);
    _DataType* ret = nullptr;
    if (false == pstUserData->m_init) {
        ret = new (pstUserData->m_abyInfo_) _DataType;
        pstUserData->m_init = true;
    } else {
        ret = (_DataType*)(void*)pstUserData->m_abyInfo_;
    }

    return *ret;
}

template <typename ObjType, typename FuncType, typename... Args>
int ICoroutine::Fork(ObjType&& stObj, FuncType&& pFunc, Args&&... args) {
    typedef ObjFuncBinder<typename FuncBindTypeHelper::PtrType<ObjType>::type, FuncType, Args...> FuncBinderType;
    FuncBinderType* pFuncBinder = POOLED_NEW(FuncBinderType, FuncBindTypeHelper::ToPtr(stObj), pFunc, args...);

    // ObjFuncBinder<ObjType,FuncType>* pFuncBinder = new ObjFuncBinder<ObjType,FuncType>(stObj, pFunc, args...);
    int iRet = ForkImpl(pFuncBinder, sizeof(FuncBinderType));
    if (0 != iRet) {
        POOLED_DELETE(pFuncBinder);
        return iRet;
    }

    return 0;
}

class CCoroutineLockAgent {
public:
    CCoroutineLockAgent(ICoroutine& co, const CoroutineLockKey& key);

    CCoroutineLockAgent(ICoroutine& co, uint16_t type, uint64_t key1 = 0, uint64_t key2 = 0, uint64_t key3 = 0,
                        uint64_t key4 = 0, uint64_t key5 = 0);

    ~CCoroutineLockAgent();

    bool Lock();

    bool WaitAndLock();

protected:
    bool m_is_lock_;
    ICoroutine& m_co_;
    CoroutineLockKey m_lock_key_;
};

}  // namespace bg

#endif