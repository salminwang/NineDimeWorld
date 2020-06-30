#pragma once
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

namespace Astra
{

    template <class T, const bool IS_THREAD_SINGLETION = false>
    class Singleton
    {
    public:
        //返回单件句柄
        static inline T *Instance()
        {
            T **pPtr = NULL;

            if (IS_THREAD_SINGLETION)
            {
                static __thread T *ptr = 0;
                pPtr = &ptr;
            }
            else
            {
                //局部静态变量
                static T *ptr = 0;
                pPtr = &ptr;
            }

            if (*pPtr == NULL)
            {
                if (((*pPtr) = new T()) == NULL)
                {
                    return NULL;
                }
            }

            return (T *)(*pPtr);
        }
    };

    template <class T, const bool IS_THREAD_SINGLETION = false>
    class ISingleton
    {
    private:
        static T *&GetSingletonPtrData()
        {
            T **pPtr = NULL;
            if (IS_THREAD_SINGLETION)
            {
                static __thread T *g_ptr = NULL;
                pPtr = &g_ptr;
            }
            else
            {
                static T *g_ptr = NULL;
                pPtr = &g_ptr;
            }
            return *pPtr;
        }

    public:
        static T &GetSingleton()
        {
            return *GetSingletonPtrData();
        }
        static T *GetSingletonPtr()
        {
            return GetSingletonPtrData();
        }

    protected:
        ISingleton()
        {
            assert(GetSingletonPtrData() == NULL);
            GetSingletonPtrData() = (T *)this;
        }

        ~ISingleton()
        {
            GetSingletonPtrData() = NULL;
        }
    };

#define DECLARE_SINGLETION(ClassName) ClassName g_st##ClassName;

} // namespace Astra