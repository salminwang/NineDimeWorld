#include <sys/mman.h>
#include "coroutine/coroutine_mgr.h"
#include "base/memory_pool.h"
#include "log/log.h"
#include "coroutine/coroutine_define.h"
#include "coroutine/general_coroutine.h"
namespace Astra
{

/*static*/ bool CoroutineMgr::RegisterCoroutine(ICoroutine& coroutine)
{
    GlobalData& global_data = GetGlobalData();
    uint32_t id = coroutine.GetID();
    if (id <= 0)
    {
        return false;
    }

    if (global_data.coroutine_map.find(id) != global_data.coroutine_map.end())
    {
        return false;
    }

    global_data.coroutine_map[id] = &coroutine;
    return true;
}

/*static*/ ICoroutine* CoroutineMgr::GetCoroutine(uint32_t id)
{
    GlobalData& global_data = GetGlobalData();
    auto iter = global_data.coroutine_map.find(id);
    if (iter == global_data.coroutine_map.end())
    {
        return nullptr;
    }

    return iter->second;
}

/*static*/ void CoroutineMgr::SwapCoroutine(void* ptr)
{
    CoroutineMgr* co_mgr = (CoroutineMgr*)ptr;
    while (true)
    {
        CoroutineData* out_data_ptr = co_mgr->m_swapout_data;
        CoroutineData* in_data_ptr = co_mgr->m_swapin_data;
        if (0 != out_data_ptr->m_coroutine_id)
        {
            int copySize = out_data_ptr->SaveRunStack();
            if (copySize > 0)
            {
                BG_DEBUG("SaveRunStack uCopySize:%d", copySize);
            }
        }

        if (0 != in_data_ptr->m_coroutine_id)
        {
            int copySize = in_data_ptr->RestoreRunStack();
            if (copySize > 0)
            {
                BG_DEBUG("RestoreRunStack uCopySize:%d", copySize);
            }
        }

        ContextSwap(&co_mgr->m_swap_co_data.m_ctx, &in_data_ptr->m_ctx);
    }
}

/*static*/ void CoroutineMgr::RunForkCoroutine(void* ptr)
{
    CoroutineData::Para* para = (CoroutineData::Para*)ptr;
    if (nullptr == para || nullptr == para->m_data || nullptr == para->m_para)
    {
        BG_ERROR("Coroutine Framwork error");
        return;
    }

    CoroutineData* co_data = para->m_data;
    FuncBinderInfo func_binder_info = *(FuncBinderInfo*)(para->m_para);
    co_data->m_status = ES_RUNNING;
    try
    {
        func_binder_info.func_binder->Invoke();
    }
    catch (CouroutineBindFailedException)
    {
        BG_ERROR("Unhandled CouroutineBindFailedException for Coroutine(%u:%lu)", co_data->m_coroutine_id,
                 co_data->m_inst_id);
    }

    func_binder_info.func_binder->~IFuncBinder();
    MemoryPool::instance()->Recycle(func_binder_info.func_binder, func_binder_info.size);

    try 
    {
        co_data->Join();
    } 
    catch (CouroutineBindFailedException) 
    {
        BG_ERROR("Unhandled CouroutineBindFailedException for Coroutine(%u:%lu)", co_data->m_coroutine_id,
                 co_data->m_inst_id);
    }

    co_data->Exit();
}

/*static*/ size_t CoroutineMgr::GetStackSize(void* stacktop)
{
    char rsp;
    return (size_t)stacktop - (size_t)&rsp;
}

/*static*/ void CoroutineMgr::RunCoroutine(void* ptr)
{
    CoroutineData::Para& co_para = *(CoroutineData::Para*)ptr;

    //注意对象生命周期
    CoroutineData* co_data = co_para.m_data;
    if (NULL == co_data)
    {
        BG_ERROR("Coroutine Framwork error");
        return;
    }

    ICoroutine* co = GetCoroutine(co_data->m_coroutine_id);
    if (nullptr == co)
    {
        BG_ERROR("GetCoroutine(%u) failed\n", co_data->m_coroutine_id);
    }
    else
    {
        co_data->m_status = ES_RUNNING;
        try
        {
            do
            {
                int ret = co->SetUp(co_para.m_para);
                if (ret)
                {
                    BG_ERROR("Coroutine(%u) SetUp failed:%d", co_data->m_coroutine_id, ret);
                    break;
                }

                ret = co->OnInnerStart(*co_data, co_para.m_para);
                if (ret)
                {
                    BG_ERROR("Coroutine(%u) run failed:%d", co_data->m_coroutine_id, ret);
                    break;
                }
                else
                {
                    break;
                }
            } while (false);
        }
        catch(CouroutineBindFailedException)
        {
            BG_ERROR("Unhandled CouroutineBindFailedException for Coroutine(%u:%lu)", co_data->m_coroutine_id,
                     co_data->m_inst_id);
        }

        try
        {
            co_data->Join();
        }
        catch (CouroutineBindFailedException)
        {
            BG_ERROR("Unhandled CouroutineBindFailedException for Coroutine(%u:%lu)", co_data->m_coroutine_id,
                     co_data->m_inst_id);
        }

        int end_ret = co->OnInnerEnd(*co_data);
        if (end_ret)
        {
            BG_ERROR("Coroutine:%u OnInnerEnd failed:%d", co->GetID(), end_ret);
        }
    }

    co_data->Exit();
    return;
}

/*static*/ CoroutineData*& CoroutineMgr::GetCurCoroutineDataPtr()
{
    return ICoroutine::GetCoroutineDataPtr();
}

/*static*/ CoroutineMgr::GlobalData& CoroutineMgr::GetGlobalData()
{
    static GlobalData* global_data = new GlobalData();
    return *global_data;
}

/*static*/ ICoroutine* CoroutineMgr::GetCurICoroutine()
{
    return GetCoroutine(GetCurCoroutineDataPtr()->m_coroutine_id);
}

CoroutineMgr::CoroutineMgr()
{
    m_main_co_data.Setup(this, 0, 0, 0, 0);
    m_swap_co_data.Setup(this, 0, 0, 0, 0);

    m_cur_data = &m_main_co_data;
    m_caller_data = NULL;
    m_inst_id_loop = 0;
    m_swap_stack = NULL;
    m_swapout_data = NULL;
    m_swapin_data = NULL;
    m_run_stack.m_baseaddr = NULL;
    m_run_stack.m_size = 0;
}

CoroutineMgr::~CoroutineMgr()
{
}

bool CoroutineMgr::Initialize(const CoroutineOption& option) {
    m_option = option;
    BG_DEBUG(
            "coroutine option: \r{\r\ttime_out = %u\r\tswap_stack_size = %u\r\trun_stacksize = %u\r\tprivate_bufferlen = %u\r\tmax_coroutine_num = %u\r}",
            m_option.m_time_out, m_option.m_swap_stack_size, m_option.m_run_stacksize, m_option.m_private_bufferlen,
            m_option.m_max_coroutine_num);

    m_swap_stack = (char*)malloc(m_option.m_swap_stack_size);
    memset(m_swap_stack, 0, m_option.m_swap_stack_size);

    m_run_stack.m_baseaddr = malloc(m_option.m_run_stacksize);
    m_run_stack.m_size = m_option.m_run_stacksize;
    memset(m_run_stack.m_baseaddr, 0, m_option.m_run_stacksize);

    m_swap_co_data.m_ctx.Make(SwapCoroutine, (void*)this, m_swap_stack, m_option.m_swap_stack_size);
    BG_DEBUG("Coroutine Initialize runstack baseAddr:%p size:%u", m_run_stack.m_baseaddr, m_run_stack.m_size);
    mprotect((char*)m_run_stack.m_baseaddr + kPageSize, kPageSize, PROT_NONE);

    if (false == m_coroutine_pool.Initialize(m_option.m_max_coroutine_num)) {
        BG_ERROR("init m_coroutine_pool failed");
        return false;
    }

    if (false == m_userinfo_pool.Initialize(m_option.m_max_coroutine_num)) {
        BG_ERROR("init m_userinfo_pool failed");
        return false;
    }

    new GeneralCoroutine();
    return true;
}

int CoroutineMgr::Start(uint32_t id, void* para) {
    const ICoroutine* pCoroutine = GetCoroutine(id);
    if (nullptr == pCoroutine) {
        BG_ERROR("GetCoroutine(%u) failed", id);
        return -1;
    }

    CoroutineData* co_data = Allocate(id);
    if (nullptr == co_data) {
        BG_ERROR("Allocate Coroutine(%u) failed", id);
        return -2;
    }

    if (nullptr == m_cur_data) {
        BG_ERROR("m_pCurData is null, framework is err");
        return -3;
    }

    co_data->m_start_para = para;

    CoroutineData* cur_data = m_cur_data;
    BackupData back_data;
    BeforeYield(co_data, back_data);
    co_data->OnStart();
    Swap(*cur_data, *co_data);
    AfterYield(co_data, back_data);

    return 0;
}

int CoroutineMgr::CreateCoroutine(const std::function<void()>& callback) {
    const ICoroutine* coroutine = GetCoroutine(kGeneralCoroutineId);
    if (NULL == coroutine) {
        BG_ERROR("GetCoroutine(%u) failed", kGeneralCoroutineId);
        return -1;
    }

    CoroutineData* co_data = Allocate(kGeneralCoroutineId);
    if (NULL == co_data) {
        BG_ERROR("Allocate Coroutine(%u) failed", kGeneralCoroutineId);
        return -2;
    }

    if (NULL == m_cur_data) {
        BG_ERROR("m_pCurData is null, framework is err");
        return -3;
    }

    GCCoroutineStartPara para;
    para.callback = callback;
    co_data->m_start_para = &para;

    CoroutineData* cur_data = m_cur_data;
    BackupData back_data;
    BeforeYield(co_data, back_data);
    co_data->OnStart();
    Swap(*cur_data, *co_data);
    AfterYield(co_data, back_data);

    return 0;
}

void CoroutineMgr::Update() {
    uint32_t now = time(0);

    while (true) {
        uint32_t idx = m_coroutine_pool.GetFirstUsedIdx();
        if (m_coroutine_pool.GetInvalidIdx() == idx) {
            break;
        }

        CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
        if (nullptr == co_data) {
            BG_ERROR("[F40]Get Coroutine data(%u) failed", idx);
            m_coroutine_pool.Recycle(idx);
            assert(false);
            continue;
        }

        if ((co_data->m_last_active_time + m_option.m_time_out) > now) {
            break;
        }

        ICoroutine* co = GetCoroutine(co_data->m_coroutine_id);
        if (nullptr == co) {
            BG_ERROR("GetCoroutine failed for (%u:%lu)", co_data->m_coroutine_id, co_data->m_inst_id);
            co_data->Finalize();
            auto iter = m_co_run_map.find(co_data->m_inst_id);
            if (iter == m_co_run_map.end()) {
                BG_ERROR("m_IDMap->find(%lu) failed", co_data->m_inst_id);
            } else {
                uint32_t co_idx = iter->second;
                m_co_run_map.erase(iter);
                m_userinfo_pool.Recycle(co_data->m_user_data_idx);
                m_coroutine_pool.Recycle(co_idx);
            }
            continue;
        }

        BG_DEBUG("Coroutine(%u, %lu) timeout", co_data->m_coroutine_id, co_data->m_inst_id);
        CoroutineData* cur_data = m_cur_data;

        BackupData back_data;
        BeforeYield(co_data, back_data);
        co_data->OnTimeOut();
        Swap(*cur_data, *co_data);
        AfterYield(co_data, back_data);
    }
}

CoroutineData* CoroutineMgr::GetCoroutineData(uint64_t instance_id) {
    auto it = m_co_run_map.find(instance_id);
    if (it == m_co_run_map.end()) {
        return nullptr;
    }

    uint32_t idx = it->second;
    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    return co_data;
}

int CoroutineMgr::OnMsg(uint64_t instance_id, uint32_t rsp_msg_id, void* rsp_msg) {
    auto iter = m_co_run_map.find(instance_id);
    if (iter == m_co_run_map.end()) {
        BG_ERROR("Coroutine of InstID(%lu) not exists", instance_id);
        return -1;
    }

    uint32_t idx = iter->second;
    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    if (nullptr == co_data) {
        m_coroutine_pool.Recycle(idx);
        m_co_run_map.erase(iter);
        BG_ERROR("Coroutine of InstID(%lu) exists but data not exist", instance_id);
        assert(false);
        return -2;
    }

    if (co_data->m_wait_msgid != rsp_msg_id) {
        BG_ERROR("Coroutine of InstID(%lu) waitmsg:%u not match:%u", instance_id, co_data->m_wait_msgid, rsp_msg_id);
        return -3;
    }

    ICoroutine* co = GetCoroutine(co_data->m_coroutine_id);
    if (NULL == co) {
        BG_ERROR("GetCoroutine failed for (%u:%lu)", co_data->m_coroutine_id, instance_id);
        m_co_run_map.erase(iter);
        return -4;
    }

    CoroutineData* cur_data = m_cur_data;
    BackupData back_data;
    BeforeYield(co_data, back_data);
    co_data->OnMsg(rsp_msg);
    Swap(*cur_data, *co_data);
    AfterYield(co_data, back_data);
    return 0;
}

int CoroutineMgr::OnMsg(uint64_t instance_id) {
    auto iter = m_co_run_map.find(instance_id);
    if (iter == m_co_run_map.end()) {
        BG_ERROR("Coroutine of InstID(%lu) not exists", instance_id);
        return -1;
    }

    uint32_t idx = iter->second;
    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    if (nullptr == co_data) {
        m_coroutine_pool.Recycle(idx);
        m_co_run_map.erase(iter);
        BG_ERROR("Coroutine of InstID(%lu) exists but data not exist", instance_id);
        assert(false);
        return -2;
    }

    ICoroutine* co = GetCoroutine(co_data->m_coroutine_id);
    if (NULL == co) {
        BG_ERROR("GetCoroutine failed for (%u:%lu)", co_data->m_coroutine_id, instance_id);
        m_co_run_map.erase(iter);
        return -4;
    }

    CoroutineData* cur_data = m_cur_data;
    BackupData back_data;
    BeforeYield(co_data, back_data);
    co_data->OnMsg();
    Swap(*cur_data, *co_data);
    AfterYield(co_data, back_data);
    return 0;
}

int CoroutineMgr::Fork(IFuncBinder* func_binder, size_t func_binder_size) {
    uint32_t coroutine_id = m_cur_data->m_coroutine_id;
    const ICoroutine* co = GetCoroutine(coroutine_id);
    if (NULL == co) {
        BG_ERROR("GetCoroutine(%u) failed", coroutine_id);
        return -1;
    }

    CoroutineData* co_data = NULL;
    co_data = Allocate(coroutine_id, true);
    if (NULL == co_data) {
        BG_ERROR("Allocate Coroutine(%u) failed", coroutine_id);
        return -2;
    }

    if (m_cur_data->IsActive()) {
        m_cur_data->m_last_active_time = time(0);
    }

    FuncBinderInfo func_binder_info;
    func_binder_info.func_binder = func_binder;
    func_binder_info.size = func_binder_size;

    co_data->m_start_para = &func_binder_info;
    co_data->m_user_data_idx = m_cur_data->m_user_data_idx;

    m_cur_data->AddChild(co_data);

    CoroutineData* cur_data = m_cur_data;

    BackupData back_data;
    BeforeYield(co_data, back_data);
    co_data->OnFork();
    Swap(*cur_data, *co_data);
    AfterYield(co_data, back_data);
    return 0;
}

CoroutineInfo* CoroutineMgr::GetUserDataBuffer(uint64_t instance_id) {
    auto iter = m_co_run_map.find(instance_id);
    if (iter == m_co_run_map.end()) {
        BG_ERROR("run Coroutine of InstID(%lu) not exists", instance_id);
        return NULL;
    }

    uint32_t idx = iter->second;
    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    if (nullptr == co_data) {
        BG_ERROR("Coroutine data of InstID(%lu) not exists", instance_id);
        return nullptr;
    }

    CoroutineInfo* user_data = m_userinfo_pool.GetDataByIdx(co_data->m_user_data_idx);
    return user_data;
}

bool CoroutineMgr::IsLockExist(const CoroutineLockKey& key) {
    return m_wait_lock_map.find(key) != m_wait_lock_map.end();
}

bool CoroutineMgr::Lock(CoroutineData& co_data, const CoroutineLockKey& key) {
    if (m_wait_lock_map.find(key) != m_wait_lock_map.end()) {
        return false;
    }

    m_wait_lock_map[key] = NULL;
    co_data.AddLockingKey(key);

    return true;
}

void CoroutineMgr::Unlock(CoroutineData& co_data, const CoroutineLockKey& key, bool succeed) {
    auto iter4 = co_data.m_lock_key_set.find(key);
    if (iter4 == co_data.m_lock_key_set.end()) {
        return;
    }

    co_data.m_lock_key_set.erase(iter4);

    auto iter = m_wait_lock_map.find(key);
    if (iter == m_wait_lock_map.end()) {
        return;
    }

    InstanceIDSet* set_ptr = iter->second;
    m_wait_lock_map.erase(iter);
    if (nullptr == set_ptr) {
        return;
    }

    CoroutineInstIDSet& id_set = set_ptr->inst_id_set;
    while (id_set.size() > 0) {
        auto iter2 = id_set.begin();
        uint64_t instance_id = *iter2;
        id_set.erase(iter2);

        auto iter3 = m_co_run_map.find(instance_id);
        if (iter3 == m_co_run_map.end()) {
            BG_ERROR("Coroutine of InstID(%lu) not exists", instance_id);
            continue;
        }

        CoroutineData* co_data = GetCoroutineData(instance_id);
        if (nullptr == co_data) {
            BG_ERROR("CoroutineData:%lu not exist!", instance_id);
            continue;
        }

        if (nullptr == co_data->m_wait_lock_key) {
            BG_ERROR("Coroutine(%u, %lu) not waiting lock", co_data->m_coroutine_id, co_data->m_inst_id);
            continue;
        }

        if (false == (*co_data->m_wait_lock_key == key)) {
            BG_ERROR("Coroutine(%u, %lu) waiting key not match", co_data->m_coroutine_id, co_data->m_inst_id);
            continue;
        }

        CoroutineData* cur_data = m_cur_data;

        BackupData back_data;
        BeforeYield(co_data, back_data);
        co_data->OnWaitLock(succeed);
        Swap(*cur_data, *co_data);
        AfterYield(co_data, back_data);
    }

    set_ptr->Construct();
    POOLED_DELETE(set_ptr);
}

void CoroutineMgr::ResetWaitLock(CoroutineData& co_data) {
    if (NULL == co_data.m_wait_lock_key) {
        return;
    }

    auto iter = m_wait_lock_map.find(*co_data.m_wait_lock_key);
    if (iter != m_wait_lock_map.end()) {
        InstanceIDSet* instance_id_set = iter->second;
        if (NULL != instance_id_set) {
            auto iter2 = instance_id_set->inst_id_set.find(co_data.m_inst_id);
            if (iter2 != instance_id_set->inst_id_set.end()) {
                instance_id_set->inst_id_set.erase(iter2);
            }
        }
    }

    co_data.ResetWaitingKey();
}

bool CoroutineMgr::WaitLock(CoroutineData& co_data, const CoroutineLockKey& key) {
    if (NULL != co_data.m_wait_lock_key) {
        BG_ERROR("Coroutine(%u, %lu) already waiting lock", co_data.m_coroutine_id, co_data.m_inst_id);
        return false;
    }

    auto iter = m_wait_lock_map.find(key);
    if (iter == m_wait_lock_map.end()) {
        return false;
    }

    InstanceIDSet* instance_id_set = iter->second;
    if (NULL == instance_id_set) {
        instance_id_set = POOLED_NEW(InstanceIDSet);
        instance_id_set->Construct();
        m_wait_lock_map[key] = instance_id_set;
    }

    instance_id_set->inst_id_set.insert(co_data.m_inst_id);

    co_data.SetWaitingKey(key);

    return true;
}

uint64_t CoroutineMgr::AllocInstanceID() {
    uint32_t now = time(0);
    uint64_t instance_id = now;
    instance_id = instance_id << 32;
    instance_id += m_inst_id_loop;
    m_inst_id_loop++;
    return instance_id;
}

CoroutineData* CoroutineMgr::Allocate(uint32_t id, bool fork /*= false*/) {
    const ICoroutine* co = GetCoroutine(id);
    if (NULL == co) {
        BG_ERROR("GetCoroutine(%u) failed", id);
        return nullptr;
    }

    uint32_t idx = m_coroutine_pool.AllocateToLastUsed();
    if (idx == m_coroutine_pool.GetInvalidIdx()) {
        BG_ERROR("m_coroutine_pool alloc(%u) failed is full", id);
        return nullptr;
    }

    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    if (nullptr == co_data) {
        BG_ERROR("Get CCoroutine data(%u) failed", idx);
        m_coroutine_pool.Recycle(idx);
        return nullptr;
    }

    co_data->Construct();

    if (!fork) {
        uint32_t user_idx = m_userinfo_pool.AllocateToLastUsed();
        if (user_idx == m_userinfo_pool.GetInvalidIdx()) {
            BG_ERROR("copool:%u userpool:%u alloc(%u) failed is full", m_coroutine_pool.GetUsedNum(),
                     m_userinfo_pool.GetUsedNum(), id);
            m_coroutine_pool.Recycle(idx);
            return nullptr;
        }

        CoroutineInfo* user_data = m_userinfo_pool.GetDataByIdx(user_idx);
        if (nullptr == user_data) {
            BG_ERROR("Get CoroutineInfo data(%u) failed", user_idx);
            m_userinfo_pool.Recycle(user_idx);
            m_coroutine_pool.Recycle(idx);
            return nullptr;
        }
        std::shared_ptr<CoroutineInfo> co_info(new CoroutineInfo());
        user_data->Reset();
        co_data->m_user_data_idx = user_idx;
    }

    uint64_t instance_id = AllocInstanceID();
    co_data->Setup(this, id, instance_id, idx, m_option.m_private_bufferlen);
    m_co_run_map[co_data->m_inst_id] = idx;

    return co_data;
}

void CoroutineMgr::Recycle(uint64_t instance_id) {
    auto iter = m_co_run_map.find(instance_id);
    if (iter == m_co_run_map.end()) {
        return;
    }

    uint32_t idx = iter->second;
    BG_DEBUG("Recycle Coroutine(%lu) idx[%u]", instance_id, idx);
    CoroutineData* co_data = m_coroutine_pool.GetDataByIdx(idx);
    if (co_data && nullptr == co_data->m_parent) {
        m_userinfo_pool.Recycle(co_data->m_user_data_idx);
    }

    m_co_run_map.erase(iter);
    m_coroutine_pool.Recycle(idx);
    BG_DEBUG("Recycle Coroutine(%lu) idx[%u], current co size:%u user size:%u", instance_id, idx,
             m_coroutine_pool.GetUsedNum(), m_userinfo_pool.GetUsedNum());
}

void CoroutineMgr::BeforeYield(CoroutineData* co_data, BackupData& co_back_data) {
    co_back_data.pre_data = m_cur_data;
    co_back_data.pre_caller_data = m_caller_data;

    m_caller_data = m_cur_data;
    m_cur_data = co_data;
    GetCurCoroutineDataPtr() = m_cur_data;
}

void CoroutineMgr::AfterYield(CoroutineData* co_data, BackupData& co_back_data) {
    m_caller_data = co_back_data.pre_caller_data;
    m_cur_data = co_back_data.pre_data;

    GetCurCoroutineDataPtr() = m_cur_data;
    if (nullptr == co_data) {
        BG_ERROR("AfterYield framework wrong");
        return;
    }

    if (0 == co_data->m_coroutine_id) {
        return;
    }

    if (co_data->IsActive()) {
        //更新时间戳
        CoroutineData* cur_data = co_data;
        while (true) {
            if (NULL == cur_data) {
                break;
            }
            if (cur_data->IsActive()) {
                cur_data->m_last_active_time = time(0);
                m_coroutine_pool.MoveUsedIdxToTail(cur_data->m_pool_idx);
            }

            cur_data = cur_data->m_parent;
        }
        return;
    }

    assert(co_data->m_status == ES_DONE);
    co_data->Finalize();
    CoroutineData* parent_data = co_data->m_parent;
    if (nullptr != parent_data) {
        parent_data->DeleteChild(co_data);
    }

    Recycle(co_data->m_inst_id);

    if (nullptr != parent_data && nullptr == parent_data->m_first_child) {
        CoroutineData* cur_data = m_cur_data;

        BackupData back_data;
        BeforeYield(parent_data, back_data);
        Swap(*cur_data, *parent_data);
        AfterYield(parent_data, back_data);
    }
}

void CoroutineMgr::SwapOut() {
    Swap(*m_cur_data, *m_caller_data);
}

void CoroutineMgr::Swap(CoroutineData& swap_out_data, CoroutineData& swap_in_data) {
    m_swapout_data = &swap_out_data;
    m_swapin_data = &swap_in_data;
    if (0 != swap_out_data.m_coroutine_id) {
        swap_out_data.UpdateStackSize();
    }

    CoroutineData::Para co_para;

    if (0 != swap_in_data.m_coroutine_id && NULL == swap_in_data.m_ctx.m_baseaddr) {
        co_para.m_para = swap_in_data.m_start_para;
        if (NULL == swap_in_data.m_parent) {
            swap_in_data.MakeCoroutine(CoroutineMgr::RunCoroutine, swap_out_data, co_para);
        } else {
            swap_in_data.MakeCoroutine(CoroutineMgr::RunForkCoroutine, swap_out_data, co_para);
        }
    }

    ContextSwap(&swap_out_data.m_ctx, &m_swap_co_data.m_ctx);
}

StackInfo& CoroutineMgr::GetRunStackInfo() {
    return m_run_stack;
}
}