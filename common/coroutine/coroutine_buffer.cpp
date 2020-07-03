#include <stdlib.h>
#include <string.h>
#include "coroutine/coroutine_buffer.h"
#include "base/memory_pool.h"

namespace Astra 
{

int CoroutineBuffer::CopyBufferIn(uint32_t data_len, const char* data) 
{
    if (data_len <= 0) 
    {
        return -1;
    }

    if (nullptr == data) 
    {
        return -2;
    }

    // 1. 如果没有使用过，需要创建内存
    if (nullptr == m_buffer_addr) 
    {
        if (data_len > m_buffer_len) 
        {
            m_buffer_len = data_len;
        }

        m_buffer_addr = MemoryPool::instance()->Allocate(m_buffer_len);
        if (nullptr == m_buffer_addr) 
        {
            Finalize();
            return -2;
        }
    } 
    else 
    {
        // 2.如果是已经分配了，说明该协程被多次挂起
        if (data_len > m_buffer_len) 
        {
            //先释放该内存
            MemoryPool::instance()->Recycle(m_buffer_addr, m_buffer_len);
            m_buffer_len = data_len;
            m_buffer_addr = MemoryPool::instance()->Allocate(m_buffer_len);
            if (nullptr == m_buffer_addr) 
            {
                Finalize();
                return -3;
            }
        }
    }

    memcpy(m_buffer_addr, data, data_len);
    m_data_len = data_len;
    return 0;
}

int CoroutineBuffer::CopyBufferOut(uint32_t buffer_size, char* data)
{
    if (NULL == data)
    {
        return -1;
    }

    if (buffer_size < m_data_len)
    {
        return -2;
    }

    if (NULL == m_buffer_addr || 0 == m_data_len)
    {
        return 0;
    }

    memcpy(data, m_buffer_addr, m_data_len);
    return (int)m_data_len;
}

void CoroutineBuffer::Finalize()
{
    if (m_buffer_addr)
    {
        MemoryPool::instance()->Recycle(m_buffer_addr, m_buffer_len);
    }

    m_buffer_addr = NULL;
    m_buffer_len = 0;
}
}  // namespace bg