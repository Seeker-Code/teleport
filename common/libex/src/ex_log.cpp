﻿#include <ex/ex_log.h>
#include <ex/ex_path.h>
#include <ex/ex_thread.h>
//#include <ex/ex_thread.h>
//#include <vector>
//#include <deque>
//#include <algorithm>

#ifdef EX_OS_WIN32
#   include <io.h>
#   include <stdio.h>
#   include <direct.h>
#else
//#   include <dirent.h>
//#   include <sys/time.h>
#endif

#define EX_LOG_CONTENT_MAX_LEN 2048

static ExLogger* g_exlog = nullptr;

void EXLOG_USE_LOGGER(ExLogger* logger) {
    g_exlog = logger;
}

void EXLOG_LEVEL(int min_level) {
    if (nullptr != g_exlog)
        g_exlog->min_level = min_level;
}

void EXLOG_DEBUG(bool debug_mode) {
    if (nullptr != g_exlog)
        g_exlog->debug_mode = debug_mode;
}

void EXLOG_CONSOLE(bool output_to_console) {
    if (nullptr != g_exlog)
        g_exlog->to_console = output_to_console;
}

void EXLOG_FILE(const wchar_t* log_file, const wchar_t* log_path /*= nullptr*/, ex_u32 max_filesize /*= EX_LOG_FILE_MAX_SIZE*/, ex_u8 max_filecount /*= EX_LOG_FILE_MAX_COUNT*/) {
    if (nullptr == g_exlog)
        return;

    ex_wstr _path;
    if (nullptr == log_path) {
        ex_exec_file(_path);
        ex_dirname(_path);
        ex_path_join(_path, false, L"log", nullptr);
    }
    else {
        _path = log_path;
    }

    g_exlog->set_log_file(_path, log_file, max_filesize, max_filecount);
}

ExLogger::ExLogger() {
#ifdef EX_OS_WIN32
    console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

    min_level = EX_LOG_LEVEL_INFO;
    debug_mode = false;
    to_console = true;

    m_file = nullptr;
    m_filesize = 0;
}

ExLogger::~ExLogger() {
    if (nullptr != m_file) {
#ifdef EX_OS_WIN32
        CloseHandle(m_file);
#else
        fclose(m_file);
#endif
        m_file = nullptr;
    }
}

void ExLogger::log_a(int level, const char* fmt, va_list valist) {
    if (nullptr == fmt)
        return;

    if (0 == strlen(fmt))
        return;

    const char* _level = "";
    if (level == EX_LOG_LEVEL_ERROR)
        _level = " [E]";
    else if (level == EX_LOG_LEVEL_WARN)
        _level = " [W]";
    else if (level == EX_LOG_LEVEL_INFO)
        _level = " [I]";

    char prefix[100] = {0};
#ifdef EX_OS_WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    // sprintf_s(prefix, 100, "[%04d-%02d-%02d %02d:%02d:%02d %llu%s] ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
    // sprintf_s(prefix, 100, "[%02d-%02d %02d:%02d:%02d %llu%s] ", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
    sprintf_s(prefix, 100, "[%04d%02d%02d %02d:%02d:%02d %llu%s] ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
#else
    time_t timep;
    struct tm* p;
    time(&timep);
    p = localtime(&timep);
    if (p == nullptr)
        return;
    // sprintf(prefix, "[%04d-%02d-%02d %02d:%02d:%02d %llu%s] ", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
    // sprintf(prefix, "[%02d-%02d %02d:%02d:%02d %llu%s] ", p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
    sprintf(prefix, "[%04d%02d%02d %02d:%02d:%02d %llu]%s ", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
#endif

    size_t offset = strlen(prefix);

    char szTmp[4096] = {0};
    memcpy(szTmp, prefix, offset);

#ifdef EX_OS_WIN32
    vsnprintf_s(szTmp+offset, 4096-offset, 4095-offset, fmt, valist);
    if(to_console)
    {
        if (nullptr != console_handle)
        {
            printf_s("%s", szTmp);
            fflush(stdout);
        }
        else
        {
            if(debug_mode)
                OutputDebugStringA(szTmp);
        }
    }
#else
    vsnprintf(szTmp + offset, 4095 - offset, fmt, valist);
    if (to_console) {
        // On linux, the stdout only output the first time output format (char or wchar_t).
        // e.g.: first time you use printf(), then after that, every wprintf() not work, and vice versa.
        // so we always use wprintf() to fix that.

        ex_astr tmp(szTmp);
        ex_wstr _tmp;
        ex_astr2wstr(tmp, _tmp);
        wprintf(L"%ls", _tmp.c_str());
        fflush(stdout);
    }
#endif

    write_a(szTmp);
}

void ExLogger::log_w(int level, const wchar_t* fmt, va_list valist) {
    if (nullptr == fmt || 0 == wcslen(fmt))
        return;

    const char* _level = "";
    if (level == EX_LOG_LEVEL_ERROR)
        _level = " [E]";
    else if (level == EX_LOG_LEVEL_WARN)
        _level = " [W]";
    else if (level == EX_LOG_LEVEL_INFO)
        _level = " [I]";

    char prefix[100] = {0};
#ifdef EX_OS_WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    // sprintf_s(prefix, 100, "[%04d-%02d-%02d %02d:%02d:%02d %llu%s] ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
    // sprintf_s(prefix, 100, "[%02d-%02d %02d:%02d:%02d %llu%s] ", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
    sprintf_s(prefix, 100, "[%04d%02d-%02d %02d:%02d:%02d %llu]%s ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ex_get_thread_id(), _level);
#else
    time_t timep;
    struct tm* p;
    time(&timep);
    p = localtime(&timep);
    if (p == nullptr)
        return;
    // sprintf(prefix, "[%04d-%02d-%02d %02d:%02d:%02d %llu%s] ", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
    // sprintf(prefix, "[%02d-%02d %02d:%02d:%02d %llu%s] ", p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
    sprintf(prefix, "[%04d%02d%02d %02d:%02d:%02d %llu]%s ", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, ex_get_thread_id(), _level);
#endif

    ex_wstr w_prefix;
    ex_astr2wstr(prefix, w_prefix);
    size_t offset = wcslen(w_prefix.c_str());


    wchar_t szTmp[4096] = {0};
    memcpy(szTmp, w_prefix.c_str(), offset * sizeof(wchar_t));

#ifdef EX_OS_WIN32
    _vsnwprintf_s(szTmp+offset, 4096-offset, 4095-offset, fmt, valist);
    if(to_console)
    {
        if (nullptr != console_handle)
        {
            wprintf_s(_T("%s"), szTmp);
            fflush(stdout);
        }
        else
        {
            if(debug_mode)
                OutputDebugStringW(szTmp);
        }
    }
#else
    vswprintf(szTmp + offset, 4095 - offset, fmt, valist);
    if (to_console) {
        wprintf(L"%ls", szTmp);
        fflush(stdout);
    }
#endif

    write_w(szTmp);
}

#define EX_PRINTF_XA(fn, level) \
void fn(const char* fmt, ...) \
{ \
    if(nullptr == g_exlog) \
        return; \
    if (g_exlog->min_level > level) \
        return; \
    ExThreadSmartLock locker(g_exlog->lock); \
    va_list valist; \
    va_start(valist, fmt); \
    g_exlog->log_a(level, fmt, valist); \
    va_end(valist); \
}

#define EX_PRINTF_XW(fn, level) \
void fn(const wchar_t* fmt, ...) \
{ \
    if(nullptr == g_exlog) \
        return; \
    if (g_exlog->min_level > level) \
        return; \
    ExThreadSmartLock locker(g_exlog->lock); \
    va_list valist; \
    va_start(valist, fmt); \
    g_exlog->log_w(level, fmt, valist); \
    va_end(valist); \
}

EX_PRINTF_XA(ex_printf_d, EX_LOG_LEVEL_DEBUG)

EX_PRINTF_XA(ex_printf_v, EX_LOG_LEVEL_VERBOSE)

EX_PRINTF_XA(ex_printf_i, EX_LOG_LEVEL_INFO)

EX_PRINTF_XA(ex_printf_w, EX_LOG_LEVEL_WARN)

EX_PRINTF_XA(ex_printf_e, EX_LOG_LEVEL_ERROR)

EX_PRINTF_XW(ex_printf_d, EX_LOG_LEVEL_DEBUG)

EX_PRINTF_XW(ex_printf_v, EX_LOG_LEVEL_VERBOSE)

EX_PRINTF_XW(ex_printf_i, EX_LOG_LEVEL_INFO)

EX_PRINTF_XW(ex_printf_w, EX_LOG_LEVEL_WARN)

EX_PRINTF_XW(ex_printf_e, EX_LOG_LEVEL_ERROR)


#ifdef EX_OS_WIN32
void ex_printf_e_lasterror(const char* fmt, ...)
{
    ExThreadSmartLock locker(g_exlog->lock);

    va_list valist;
    va_start(valist, fmt);
    g_exlog->log_a(EX_LOG_LEVEL_ERROR, fmt, valist);
    va_end(valist);

    //=========================================

    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf, 0, nullptr);

    ex_printf_e(" - WinErr(%d): %s\n", dw, (LPSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

void ex_printf_e_lasterror(const wchar_t* fmt, ...)
{
    ExThreadSmartLock locker(g_exlog->lock);

    va_list valist;
    va_start(valist, fmt);
    g_exlog->log_w(EX_LOG_LEVEL_ERROR, fmt, valist);
    va_end(valist);

    //=========================================

    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf, 0, nullptr);

    ex_printf_e(" - WinErr(%d): %s\n", dw, (LPSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}
#endif

void ex_printf_bin(const ex_u8* bin_data, size_t bin_size, const char* fmt, ...) {
    if (nullptr == g_exlog)
        return;
    if (!g_exlog->debug_mode)
        return;

    ExThreadSmartLock locker(g_exlog->lock);

    if (nullptr == fmt)
        return;
    if (0 == strlen(fmt))
        return;

    char szTmp[128] = {0};

    va_list valist;
    va_start(valist, fmt);

#ifdef EX_OS_WIN32
    vsnprintf_s(szTmp, 128, 127, fmt, valist);
#else
    vsnprintf(szTmp, 127, fmt, valist);
#endif
    va_end(valist);

    ex_printf_d("%s (%d/0x%02x Bytes)\n", szTmp, bin_size, bin_size);

    const ex_u8* line = bin_data;
    size_t offset = 0;
    while (offset < bin_size) {
        size_t _offset = 0;
        memset(szTmp, 0, 128);

        snprintf(szTmp + _offset, 128 - _offset, "%06x  ", (int) offset);
        _offset += 8;

        size_t this_line = bin_size - offset;
        if (this_line > 16)
            this_line = 16;

        size_t i = 0;
        for (; i < this_line; i++) {
            snprintf(szTmp + _offset, 128 - _offset, "%02x ", line[i]);
            _offset += 3;
        }

        snprintf(szTmp + _offset, 128 - _offset, "  ");
        _offset += 2;

        for (; i < 16; i++) {
            snprintf(szTmp + _offset, 128 - _offset, "   ");
            _offset += 3;
        }

        for (i = 0; i < this_line; i++) {
            snprintf(szTmp + _offset, 128 - _offset, "%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
            _offset += 1;
        }

        snprintf(szTmp + _offset, 128 - _offset, "\n");

        ex_printf_d("%s", szTmp);

        offset += this_line;
        line += this_line;
    }

    fflush(stdout);
}

bool ExLogger::set_log_file(const ex_wstr& log_path, const ex_wstr& log_name, ex_u32 max_filesize, ex_u8 max_count) {
    m_max_filesize = max_filesize;
    m_max_count = max_count;

    m_filename = log_name;

    m_path = log_path;
    ex_abspath(m_path);

    ex_mkdirs(m_path);

    m_fullname = m_path;
    ex_path_join(m_fullname, false, log_name.c_str(), nullptr);

    return _open_file();
}

bool ExLogger::_open_file() {
    if (m_file) {
#ifdef EX_OS_WIN32
        CloseHandle(m_file);
#else
        fclose(m_file);
#endif
        m_file = nullptr;
    }

#ifdef EX_OS_WIN32
    // 注意：这里必须使用 CreateFile() 来打开日志文件，使用FILE指针无法传递给动态库进行操作。
    m_file = CreateFileW(m_fullname.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (INVALID_HANDLE_VALUE == m_file)
    {
        m_file = nullptr;
        return false;
    }

    SetFilePointer(m_file, 0, nullptr, FILE_END);
    m_filesize = GetFileSize(m_file, nullptr);
#else
    ex_astr _fullname;
    ex_wstr2astr(m_fullname, _fullname);
    m_file = fopen(_fullname.c_str(), "a");

    if (nullptr == m_file) {
        return false;
    }

    fseek(m_file, 0, SEEK_END);
    m_filesize = (ex_u32) ftell(m_file);
#endif

    return _rotate_file();
}

bool ExLogger::_rotate_file() {
    if (m_filesize < m_max_filesize)
        return true;

    if (m_file) {
#ifdef EX_OS_WIN32
        CloseHandle(m_file);
#else
        fclose(m_file);
#endif
        m_file = nullptr;
    }

    // make a name for backup file.
    wchar_t _tmpname[64] = {0};
#ifdef EX_OS_WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    swprintf_s(_tmpname, 64, L"%s.%04d%02d%02d%02d%02d%02d.bak", m_filename.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
    time_t timep;
    time(&timep);
    struct tm* p = localtime(&timep);
    if (p == nullptr)
        return false;

    ex_wcsformat(_tmpname, 64, L"%ls.%04d%02d%02d%02d%02d%02d.bak", m_filename.c_str(), p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
#endif

    ex_wstr _new_fullname(m_path);
    ex_path_join(_new_fullname, false, _tmpname, nullptr);

#ifdef EX_OS_WIN32
    if (!MoveFileW(m_fullname.c_str(), _new_fullname.c_str()))
    {
        EXLOGE_WIN("can not rename log file, remove old one and try again.");
        DeleteFileW(_new_fullname.c_str());
        if (!MoveFileW(m_fullname.c_str(), _new_fullname.c_str()))
            return false;
    }
#else
    ex_astr _a_fullname;
    ex_astr _a_new_fullname;
    ex_wstr2astr(m_fullname, _a_fullname);
    ex_wstr2astr(_new_fullname, _a_new_fullname);

    if (rename(_a_fullname.c_str(), _a_new_fullname.c_str()) != 0) {
        remove(_a_new_fullname.c_str());
        if (0 != (rename(_a_fullname.c_str(), _a_new_fullname.c_str())))
            return false;
    }
#endif

    return _open_file();
}

bool ExLogger::write_a(const char* buf) {
    if (nullptr == m_file)
        return false;

    size_t len = strlen(buf);

    if (len > EX_LOG_CONTENT_MAX_LEN)
        return false;

#ifdef EX_OS_WIN32
    DWORD dwWritten = 0;
    WriteFile(m_file, buf, len, &dwWritten, nullptr);
    m_filesize += len;
    FlushFileBuffers(m_file);
#else
    fwrite(buf, len, 1, m_file);
    m_filesize += len;
    fflush(m_file);
#endif


    return _rotate_file();
}

bool ExLogger::write_w(const wchar_t* buf) {
    ex_astr _buf;
    ex_wstr2astr(buf, _buf, EX_CODEPAGE_UTF8);
    return write_a(_buf.c_str());
}
