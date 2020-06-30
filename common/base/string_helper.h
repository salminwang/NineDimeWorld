#pragma once
#include <stdint.h>
#include <stddef.h>
namespace Astra
{

    class StringTokenizer
    {
    public:
        void Begin(char spliter, const char *str, uint32_t len);
        bool Next(const char *&str, uint32_t &len);

    protected:
        uint32_t m_len;
        const char *m_str;
        char m_spliter;
        uint32_t m_ptr;
    };

    class StringHelper
    {
    public:
        static int FindLastChar(const char *str, size_t len, char tag);
    };

} // namespace Astra