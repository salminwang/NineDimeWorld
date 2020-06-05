#include "framework/base_server.h"
#include "log/log.h"
#include "base/linux_helper.h"
#include "log/log_cfg_bridge.h"
#include "log/write_log_thread.h"
#include "log/log_msg_define.h"
#include "cfgloader/log_cfg_mgr.h"

namespace Astra {

bool BaseServer::ParseArgs(int arg_num, const char* args[]) {
    AS_DEBUG("pot");
    return true;
}

bool BaseServer::SetServerName() {
    std::string server_name = BaseServer::GetSingleton().GetSvrName();

    //名字不能有空格等字符
	assert(std::string::npos == server_name.find_first_of(' '));
	assert(std::string::npos == server_name.find_first_of('\t'));
	assert(std::string::npos == server_name.find_first_of('\n'));
	assert(std::string::npos == server_name.find_first_of('\r'));

    m_server_name = server_name;
    AS_DEBUG("server name:%s", m_server_name.c_str());
    return true;
}

bool BaseServer::Setup() {
    //注册预先加载的配置
    RegisterPreCfg();

    //初始化日志
    if (false == InitLocalLog()) {
        return false;
    }
}

void BaseServer::RegisterPreCfg() {

}

bool BaseServer::InitLocalLog() {
    //加载日志配置
    if (!LogCfgMgr::Instance()->LoadCfg()) {
        printf("Init LocalLog failed!\n");
        return false;
    }

    LogCfgBridgeInstance::Instance()->Update(LogCfgMgr::Instance()->GetLogCfg());
    LogCfgBridge::Instance()->Update();
    //日志线程初始化及创建
    Singleton<WriteLogThread>::Instance()->init();
    Singleton<WriteLogThread>::Instance()->CreateLogThread();
    //主线程日志管道创建
    return Singleton<LogMsgQueue, true>::Instance()->CreateMsgQueue(-1, kMaxLogMsgNum);
}

bool BaseServer::InitProcInfo() {
    //初始化运行目录
    const char* bin_dir = LinuxHelper::GetBinDirectory();
    if(false == LinuxHelper::ChangeWorkingDirectory(bin_dir)) {
        AS_ERROR("Can't change working directory to %s\n", bin_dir);
        return false;
    }

    //查看进程是否正在跑
    char lock_file[256] = {0};
    snprintf(lock_file, sizeof(lock_file), "%s.%s.lock", m_server_name.c_str(), m_proc_id.c_str());
    if(false == LinuxHelper::LockFile(lock_file))
    {
        AS_ERROR("Lock file(%s) failed, a program instance is already in process\n", lock_file);
        return false;
    }

    //生产pid文件
    

    return true;
}

int BaseServer::Run(int arg_num, const char* args[]) {
    //解析参数
    if (false == ParseArgs(arg_num, args)) {
        return -1;
    }

    //deamon进程
    if (0) {
        LinuxHelper::Deamonize();
    }
    
    //设置进程名字
    if (false == SetServerName()) {
        return -1;
    }

    //初始化进程环境
    if (false == Setup()) {
        return -1;
    }

    return 0;
}

}