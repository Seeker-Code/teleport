﻿#include <ex/ex_thread.h>
#include <ex/ex_log.h>

//=========================================================
//
//=========================================================


#ifdef EX_OS_WIN32
unsigned int WINAPI ExThreadBase::_thread_func(LPVOID pParam)
#else

void *ExThreadBase::_thread_func(void *pParam)
#endif
{
    auto _this = (ExThreadBase *) pParam;

    _this->m_is_running = true;
    _this->_thread_loop();
    _this->m_is_running = false;
    _this->m_handle = EX_THREAD_NULL;

    _this->_on_stopped();
    EXLOGV("[thread] - `%s` exit.\n", _this->m_thread_name.c_str());
    
#ifdef  EX_OS_WIN32
    return 0;
#else
    return nullptr;
#endif
}

ExThreadBase::ExThreadBase(const char *thread_name) :
        m_handle(EX_THREAD_NULL),
        m_is_running(false),
        m_need_stop(false) {
    m_thread_name = thread_name;
}

ExThreadBase::~ExThreadBase() {
    if(m_is_running) {
        EXLOGE("[thread] `%s` not stop before destroy.\n", m_thread_name.c_str());
    }
}

bool ExThreadBase::start() {
    m_need_stop = false;
    EXLOGV("[thread] + `%s` starting.\n", m_thread_name.c_str());
#ifdef WIN32
    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, _thread_func, (void*)this, 0, NULL);

    if (NULL == h)
    {
        return false;
    }
    m_handle = h;
#else
    pthread_t tid = EX_THREAD_NULL;
    int ret = pthread_create(&tid, nullptr, _thread_func, (void *) this);
    if (ret != 0) {
        return false;
    }
    m_handle = tid;

#endif

    return true;
}

bool ExThreadBase::stop() {
    if (m_handle == EX_THREAD_NULL) {
        EXLOGW("[thread] `%s` already stopped before stop() call.\n", m_thread_name.c_str());
        return true;
    }

    EXLOGV("[thread] - try to stop `%s.\n", m_thread_name.c_str());
    m_need_stop = true;
    _on_stop();

    //EXLOGV("[thread] - wait `%s` exit, thread-handle=0x%08x.\n", m_thread_name.c_str(), m_handle);

#ifdef EX_OS_WIN32
    if (m_handle) {
        if (WaitForSingleObject(m_handle, INFINITE) != WAIT_OBJECT_0) {
            return false;
        }
    }
#else
    if(m_handle != EX_THREAD_NULL) {
        if (pthread_join(m_handle, nullptr) != 0) {
            return false;
        }
    }
#endif

    return true;
}

bool ExThreadBase::terminate() {
#ifdef EX_OS_WIN32
    return (TerminateThread(m_handle, 1) == TRUE);
#else
    return (pthread_cancel(m_handle) == 0);
#endif
}

//=========================================================
//
//=========================================================

ExThreadManager::ExThreadManager() {}

ExThreadManager::~ExThreadManager() {
    if (!m_threads.empty()) {
        EXLOGE("[thread] when destroy thread manager, there are %d thread not exit.\n", m_threads.size());
        stop_all();
    }
}

void ExThreadManager::stop_all() {
    ExThreadSmartLock locker(m_lock);

    for (auto & t : m_threads) {
        t->stop();
    }
    m_threads.clear();
}

void ExThreadManager::add(ExThreadBase *tb) {
    ExThreadSmartLock locker(m_lock);

    for (auto & t : m_threads) {
        if (t == tb) {
            EXLOGE("[thread] when add thread to manager, it already exist.\n");
            return;
        }
    }

    m_threads.push_back(tb);
}

void ExThreadManager::remove(ExThreadBase *tb) {
    ExThreadSmartLock locker(m_lock);

    for (auto it = m_threads.begin(); it != m_threads.end(); ++it) {
        if ((*it) == tb) {
            m_threads.erase(it);
            return;
        }
    }
    EXLOGE("[thread] thread not hold by thread-manager while remove it.\n");
}

//=========================================================
//
//=========================================================

ExThreadLock::ExThreadLock() {
#ifdef EX_OS_WIN32
    InitializeCriticalSection(&m_locker);
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_locker, &attr);
    pthread_mutexattr_destroy(&attr);
#endif
}

ExThreadLock::~ExThreadLock() {
#ifdef EX_OS_WIN32
    DeleteCriticalSection(&m_locker);
#else
    pthread_mutex_destroy(&m_locker);
#endif
}

void ExThreadLock::lock() {
#ifdef EX_OS_WIN32
    EnterCriticalSection(&m_locker);
#else
    pthread_mutex_lock(&m_locker);
#endif
}

void ExThreadLock::unlock() {
#ifdef EX_OS_WIN32
    LeaveCriticalSection(&m_locker);
#else
    pthread_mutex_unlock(&m_locker);
#endif
}

//=========================================================
//
//=========================================================

int ex_atomic_add(volatile int *pt, int t) {
#ifdef EX_OS_WIN32
    return (int)InterlockedExchangeAdd((long*)pt, (long)t);
#else
    return __sync_add_and_fetch(pt, t);
#endif
}

int ex_atomic_inc(volatile int *pt) {
#ifdef EX_OS_WIN32
    return (int)InterlockedIncrement((long*)pt);
#else
    return __sync_add_and_fetch(pt, 1);
#endif
}

int ex_atomic_dec(volatile int *pt) {
#ifdef EX_OS_WIN32
    return (int)InterlockedDecrement((long*)pt);
#else
    return __sync_add_and_fetch(pt, -1);
#endif
}

uint64_t ex_get_thread_id()
{
#if defined(EX_OS_WIN32)
    return (uint64_t)GetCurrentThreadId();
#elif defined(EX_OS_LINUX)
    return pthread_self();
#elif defined(EX_OS_MACOS)
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    return tid;
#else
#   error "unsupport platform."
#endif
}
