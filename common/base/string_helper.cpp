#include "base/string_helper.h"
#include <iconv.h>
#include <stdarg.h>
#include <string.h>
#include <string>

namespace Astra {

void StringTokenizer::Begin(char spliter, const char* str, uint32_t len) {
    m_spliter = spliter;
    m_str = str;
    m_len = len;
    m_ptr = 0;
}

bool StringTokenizer::Next(const char*& str, uint32_t& len) {
    if (nullptr == m_str || m_ptr >= m_len) {
        return false;
    }

    uint32_t begin_idx = m_ptr;

    while (m_str[m_ptr] != m_spliter && m_ptr < m_len) {
        ++m_ptr;
    }

    len = (m_ptr - begin_idx);
    ++m_ptr;
    str = m_str + begin_idx;

    return true;
}

int StringHelper::FindLastChar(const char* str, size_t len, char tag) {
    if(NULL == str || len == 0) {
        return -1;
    }

    int idx = (int)len - 1;
    while(idx >= 0 && str[idx] != tag) {
        --idx;
    }

    return idx;
}   
}