#pragma once
namespace Astra 
{

enum ENM_COROUTINE_ERROR 
{
    COROUTINE_SUCCESS = 0,              //成功
    COROUTINE_AGENT_NULL_POINTER = 1,   //协程参数为null
    COROUTINE_AGENT_DB_RETURN_ERR = 2,  // db操作返回失败
    COROUTINE_AGENT_DB_SYSTEM_ERR = 3,  // db框架错误

    COROUTINE_WAIT_PARA_ERR = 101,       //参数错误
    COROUTINE_WAIT_FRAMEWORK_ERR = 102,  //框架错误
    COROUTINE_WAIT_TIMEOUT_ERR = 103,    //超时
    COROUTINE_WAIT_LOCK_ERR = 104,       //加锁失败

    COROUTINE_SATAR_PARA_ERR = 201,         //协程启动参数错误
    COROUTINE_PLAYER_NOT_ONLINE_ERR = 202,  // Player不在线
};
}
