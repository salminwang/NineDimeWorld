#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include "framework/base_server.h"
#include "log/log.h"
#include "base/linux_helper.h"
#include "log/log_cfg_bridge.h"
#include "log/write_log_thread.h"
#include "log/log_msg_define.h"
#include "cfgloader/log_cfg_mgr.h"
#include "base/time_utility.h"

namespace Astra
{

    BaseServer::BaseServer(std::string server_name)
    {
        m_server_name = server_name;
    }

    bool BaseServer::PrepareLocalLog()
    {
        return true;
        //加载日志配置
        if (!LogCfgMgr::Instance()->LoadCfg())
        {
            printf("prepare locallog............ failed\n");
            return false;
        }

        LogCfgBridgeInstance::Instance()->Update(LogCfgMgr::Instance()->GetLogCfg());
        LogCfgBridge::Instance()->Update();
        //日志线程初始化及创建
        Singleton<WriteLogThread>::Instance()->init();
        Singleton<WriteLogThread>::Instance()->CreateLogThread();
        //主线程日志管道创建
        if (false == Singleton<LogMsgQueue, true>::Instance()->CreateMsgQueue(-1, kMaxLogMsgNum))
        {
            printf("prepare locallog............ failed\n");
            return false;
        }

        AS_DEBUG("prepare locallog............ success");
        return true;
    }

    bool BaseServer::PrepareArgs(int arg_num, const char *args[])
    {
        printf("prepare args............ success\n");
        return true;
    }

    bool BaseServer::PrepareServerName()
    {
        //名字不能有空格等字符
        assert(std::string::npos == m_server_name.find_first_of(' '));
        assert(std::string::npos == m_server_name.find_first_of('\t'));
        assert(std::string::npos == m_server_name.find_first_of('\n'));
        assert(std::string::npos == m_server_name.find_first_of('\r'));
        AS_DEBUG("prepare server name............ success");
        return true;
    }

    bool BaseServer::PreparePreLoadCfg()
    {
        AS_DEBUG("prepare pre load cfg............ success");
        return true;
    }

    bool BaseServer::PrepareProcInfo()
    {
        //初始化运行目录
        const char *bin_dir = LinuxHelper::GetBinDirectory();
        if (false == LinuxHelper::ChangeWorkingDirectory(bin_dir))
        {
            AS_ERROR("prepare proc info............ failed");
            return false;
        }

        //查看进程是否正在跑
        char lock_file[256] = {0};
        snprintf(lock_file, sizeof(lock_file), "%s.%s.lock", m_server_name.c_str(), m_proc_id.c_str());
        if (false == LinuxHelper::LockFile(lock_file))
        {
            AS_ERROR("prepare proc info: already in process............ failed");
            return false;
        }

        //生产pid文件
        {
            m_proc_pid_file.clear();
            m_proc_pid_file.append("./%s.pid", m_proc_id.c_str());
            //打开pid文件
            FILE *file = fopen(m_proc_pid_file.c_str(), "w");
            if (nullptr == file)
            {
                AS_ERROR("prepare proc info............ failed");
                return false;
            }

            //写入pid
            if (0 >= fprintf(file, "%d\n", (int)getpid()))
            {
                AS_ERROR("prepare proc info............ failed");
                fclose(file);
                return false;
            }

            fclose(file);
        }

        //创建日志目录

        //注册信号处理函数, 并初始化相关的控制变量
        LinuxHelper::SetSignalHandler(SIGUSR1, BaseServer::SignalHandler);
        LinuxHelper::SetSignalHandler(SIGUSR2, BaseServer::SignalHandler);
        LinuxHelper::SetSignalHandler(SIGABRT, BaseServer::SignalHandler);
        LinuxHelper::SetSignalHandler(SIGSEGV, BaseServer::SignalHandler);
        LinuxHelper::SetSignalHandler(SIGCHLD, BaseServer::SignalHandler);

        AS_DEBUG("prepare proc info............  success");
        return true;
    }

    //信号处理函数
    /*static*/ void BaseServer::SignalHandler(int signal_value)
    {
        bool use_defult = false;
        switch (signal_value)
        {
        case SIGUSR1:
        {
            AS_DEBUG("Recv reload signal(SIGUSR1)\n");
            BaseServer::GetSingleton().m_need_reload = true;
            break;
        }
        case SIGUSR2:
        {
            AS_DEBUG("Recv quit signal(SIGUSR2)\n");
            BaseServer::GetSingleton().SetNeedQuit(true);
            break;
        }
        case SIGABRT:
        {
            AS_ERROR("Recv assert signal(SIGABRT)\n");
            BaseServer::GetSingleton().HandleExceptionSignal(signal_value);
            Singleton<WriteLogThread>::Instance()->CloseLogThread();
            break;
        }
        case SIGSEGV:
        {
            AS_ERROR("Recv core signal(SIGSEGV)");
            BaseServer::GetSingleton().HandleExceptionSignal(signal_value);
            Singleton<WriteLogThread>::Instance()->CloseLogThread();
            use_defult = true;
            break;
        }
        case SIGCHLD:
        {
            AS_DEBUG("Recv child quit signal(SIGCHLD)\n");
            BaseServer::GetSingleton().SubOnChildQuit();
            break;
        }
        default:
        {
            AS_ERROR("Unhandled signal(%d)\n", signal_value);
            break;
        }
        }

        if (use_defult)
        {
            signal(signal_value, SIG_DFL);
        }
        else
        {
            signal(signal_value, SignalHandler);
        }
    }

    int BaseServer::HandleExceptionSignal(int signal_value)
    {
        SetNeedQuit(true);
        CoreDump(signal_value);
    }

    //输出coredump信息
    void BaseServer::CoreDump(int signal)
    {
        const int BK_SIZE = 64;
        void *buffer[BK_SIZE] = {0};
        char **strings = nullptr;
        size_t i = 0;

        size_t size = backtrace(buffer, BK_SIZE);
        strings = backtrace_symbols(buffer, size);
        if (nullptr == strings)
        {
            AS_ERROR("backtrace_symbols.");
            return;
        }

        char core_buff[10240] = {0};
        size_t offset = 0;
        offset += snprintf(core_buff + offset, sizeof(core_buff) - offset - 1, "----Obtained %zd stack frames-----\n", size);
        for (i = 0; i < size; i++)
        {
            offset += snprintf(core_buff + offset, sizeof(core_buff) - offset - 1, "%s\n", strings[i]);
        }
        AS_ERROR("%s\n", core_buff);
        free(strings);
        strings = nullptr;
    }

    bool BaseServer::BeforeRun()
    {
        //初始化日志
        if (false == PrepareLocalLog())
        {
            return false;
        }

        //设置进程名字
        if (false == PrepareServerName())
        {
            return false;
        }

        //注册预先加载的配置
        if (false == PreparePreLoadCfg())
        {
            return false;
        }

        //初始化进程信息
        if (false == PrepareProcInfo())
        {
            return false;
        }

        return true;
    }

    bool BaseServer::RunServer(int arg_num, const char **args)
    {
        printf("server run............\n");
        //解析参数
        if (false == PrepareArgs(arg_num, args))
        {
            return false;
        }

        //deamon进程
        if (0)
        {
            LinuxHelper::Deamonize();
            printf("fork proc child run............\n");
        }

        //运行前的准备工作
        if (false == BeforeRun())
        {
            return false;
        }

        //子类注册预加载配置
        if (false == SubRegisterPreLoadCfg())
        {
            return false;
        }
        AS_DEBUG("sub register pre load cfg............ success");

        //子类初始化
        if (false == SubInitialize())
        {
            return false;
        }
        AS_DEBUG("sub initialize............ success");

        //启动进程
        Run();
        return true;
    }

    bool BaseServer::Run()
    {
        time_t now = TimeUtility::GetCurrentMS();

        SetNeedQuit(false);

        //是否开启profile
        if (m_open_prifile)
        {
            StartProfiler();
        }

        while (m_need_quit)
        {
            printf("hello game\n");
            sleep(3);
        }

        //是否开启profile
        if (m_open_prifile)
        {
            StopProfiler();
        }

        return true;
    }

} // namespace Astra