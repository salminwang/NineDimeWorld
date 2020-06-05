#pragma once

#include <string>
#include "base/singleton.h"

namespace Astra {
    
class BaseServer : public ISingleton<BaseServer> {
public:
    //信号处理函数
    static void SignalHandler(int signal_value);

public:
    //step2.进程拉起的入口
    int Run(int arg_num, const char* args[]);

public:
    // 获取进程名，如gamesvr
	virtual const char* GetSvrName() = 0;

protected:
    bool ParseArgs(int arg_num, const char* args[]);
    bool SetServerName();
    bool Setup();
    void RegisterPreCfg();
    bool InitLocalLog();
    bool InitProcInfo();
private:
    std::string m_server_name;
    std::string m_proc_id;
};

}