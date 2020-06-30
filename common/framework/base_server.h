#pragma once

#include <string>
#include "base/singleton.h"

namespace Astra
{

    class BaseServer : public ISingleton<BaseServer>
    {
    public:
        //信号处理函数
        static void SignalHandler(int signal_value);

    public:
        BaseServer(std::string server_name);
        //step2.进程拉起的入口
        bool RunServer(int arg_num, const char *args[]);

    public:
        //进程退出标志
        void SetNeedQuit(bool need_quit) { m_need_quit = need_quit; }
        bool IsNeedQuit() { return m_need_quit; }

        //处理异常退出信号
        int HandleExceptionSignal(int signal);

    public:
        //子进程退出通知
        virtual bool SubOnChildQuit() {}
        //由子类注册预加载配置
        virtual bool SubRegisterPreLoadCfg() {}
        //子类初始化
        virtual bool SubInitialize() {}

    protected:
        bool BeforeRun();
        //初始化本地日志
        bool PrepareLocalLog();
        //解析参数
        bool PrepareArgs(int arg_num, const char **args);
        //设置进程名称
        bool PrepareServerName();
        //注册预加载配置
        bool PreparePreLoadCfg();
        //初始化进程信息
        bool PrepareProcInfo();

        //运行
        bool Run();

    protected:
        //输出coredump信息
        void CoreDump(int signal);
        void StartProfiler() {}
        void StopProfiler() {}

    private:
        //进程名称
        std::string m_server_name;
        //进程id
        std::string m_proc_id;
        //进程的pid文件
        std::string m_proc_pid_file;

        //是否需要重载
        bool m_need_reload = false;
        //是否需要退出
        bool m_need_quit = false;
        //是否开启profile
        bool m_open_prifile = false;
    };

} // namespace Astra