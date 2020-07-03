#pragma once
#include <stdlib.h>
#include <map>
#include <string.h>
#include <set>
#include <list>
#include <stdint.h>
#include "coroutine.h"
#include "base/func_bind_helper.h"
#include "base/resource_pool.h"
#include <functional>
namespace Astra {

const int kPageSize = 4 * 1024;              //页面大小
const uint32_t kSwapStackSize = 512 * 1024;  // swap协程的私有栈大小512K

typedef std::map<uint64_t, uint32_t> CoRunMap;      //协程map
typedef std::map<uint32_t, uint32_t> IDRunningMap;  //协程运行中map
typedef ResourcePool<CoroutineData> CoroutinePool;  //协程数据池子
typedef ResourcePool<CoroutineInfo> UserInfoPool;   //用户数据池子

struct CoroutineOption {
    uint32_t m_time_out = 5;               //超时时间 默认5s
    uint32_t m_swap_stack_size = 524288;   // swap私有栈大小 默认512K
    uint32_t m_run_stacksize = 52428800;   //协程栈的大小 默认50M
    uint32_t m_private_bufferlen = 5120;   //协程私有栈默认大小 默认5k
    uint32_t m_max_coroutine_num = 10000;  //最大支持协程数量 默认1w
};

class CoroutineMgr {
public:
    typedef std::set<uint64_t> CoroutineInstIDSet;
    struct InstanceIDSet {
        InstanceIDSet() {
            Construct();
        }
        void Construct() {
            inst_id_set.clear();
        }
        CoroutineInstIDSet inst_id_set;
    };
    typedef std::map<CoroutineLockKey, InstanceIDSet*> WaitLockMap;

    //全局数据结构
    struct GlobalData {
        GlobalData() {
            coroutine_map.clear();
        }

        std::map<uint32_t, ICoroutine*> coroutine_map;
    };

    struct BackupData {
        CoroutineData* pre_data;
        CoroutineData* pre_caller_data;
    };

    struct FuncBinderInfo {
        IFuncBinder* func_binder;
        size_t size;
    };

public:
    //注册协程
    static bool RegisterCoroutine(ICoroutine& coroutine);
    //获取协程
    static ICoroutine* GetCoroutine(uint32_t id);
    //切换协程
    static void SwapCoroutine(void* ptr);
    //执行fork的协程
    static void RunForkCoroutine(void* ptr);
    //执行协程
    static void RunCoroutine(void* ptr);
    //获取stack的大小
    static size_t GetStackSize(void* stacktop);
    //获取全局协程
    static GlobalData& GetGlobalData();
    //获取当前允许的协程指针
    static CoroutineData*& GetCurCoroutineDataPtr();
    //获取当前允许的协程指针
    static ICoroutine* GetCurICoroutine();

public:
    //构造函数
    CoroutineMgr();
    //析构函数
    ~CoroutineMgr();
    //设置必要的协程相关的参数
    bool Initialize(const CoroutineOption& option);
    //每帧更新
    void Update();
    //启动一个协程
    int Start(uint32_t id, void* para);
    //回调函数式启动一个协程
    int CreateCoroutine(const std::function<void()>& callback);
    //唤醒协程
    int OnMsg(uint64_t instance_id, uint32_t rsp_msg_id, void* rsp_msg);
    //唤醒协程
    int OnMsg(uint64_t instance_id);
    //获取协程数据
    CoroutineData* GetCoroutineData(uint64_t instance_id);
    // fork协程
    int Fork(IFuncBinder* func_binder, size_t func_binder_size);
    //获取用户数据缓存
    CoroutineInfo* GetUserDataBuffer(uint64_t instance_id);
    //加锁
    bool Lock(CoroutineData& co_data, const CoroutineLockKey& key);
    //解锁
    void Unlock(CoroutineData& co_data, const CoroutineLockKey& key, bool succeed);
    //是否锁存在
    bool IsLockExist(const CoroutineLockKey& key);
    //重置
    void ResetWaitLock(CoroutineData& co_data);
    //等待加锁
    bool WaitLock(CoroutineData& co_data, const CoroutineLockKey& key);
    //获取运行栈信息
    StackInfo& GetRunStackInfo();
    //换出协程
    void SwapOut();

protected:
    //切换协程
    void Swap(CoroutineData& swap_out_data, CoroutineData& swap_in_data);
    //切换前操作
    void BeforeYield(CoroutineData* co_data, BackupData& co_back_data);
    //切换后
    void AfterYield(CoroutineData* co_data, BackupData& co_back_data);
    //分配协程实例id
    uint64_t AllocInstanceID();
    //分配协程数据
    CoroutineData* Allocate(uint32_t id, bool fork = false);
    //回收协程
    void Recycle(uint64_t instance_id);

private:
    CoroutineOption m_option;      //协程的参数选项
    CoroutineData m_main_co_data;  //保存主进程的环境

    char* m_swap_stack;            // swap协程的私有栈地址
    CoroutineData m_swap_co_data;  // swap协程的数据

    CoroutineData* m_swapout_data;  //需要保存的协程
    CoroutineData* m_swapin_data;   //需要执行的协程

    StackInfo m_run_stack;  // stack full的运行栈

    CoroutineData* m_cur_data;     //当前运行的协程数据
    CoroutineData* m_caller_data;  //当前运行的协程的调用方

    CoRunMap m_co_run_map;  //实例ID映射协程

    uint32_t m_inst_id_loop;         //实例ID分配
    CoroutinePool m_coroutine_pool;  //协程池子
    UserInfoPool m_userinfo_pool;    //用户数据池子

    WaitLockMap m_wait_lock_map;  //锁信息
};
}