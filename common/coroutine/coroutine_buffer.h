#pragma once
#include <stdint.h>
namespace Astra 
{

class CoroutineBuffer 
{
public:
    CoroutineBuffer() 
    {
        m_buffer_addr = nullptr;
        m_buffer_len = 0;
        m_data_len = 0;
    }

    void SetBuffLen(uint32_t len) 
    {
        m_buffer_len = len;
    }

    uint32_t GetUsedLen() 
    {
        return m_data_len;
    }
    
    uint32_t GetBufferLen() 
    {
        return m_buffer_len;
    }

    int CopyBufferIn(uint32_t data_len, const char* data);
    int CopyBufferOut(uint32_t buffer_size, char* data);
    void Finalize();

private:
    uint32_t m_buffer_len;
    uint32_t m_data_len;
    void* m_buffer_addr;
};

}