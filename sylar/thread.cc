#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

// 线程静态局部变量，只对当前线程有效，可以防止多线程的一些问题
static thread_local Thread* t_thread = nullptr; 
static thread_local std::string t_thread_name = "UNKNOW";

// 系统日志统一打到 system 
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 获取当前的线程指针
Thread* Thread::GetThis() {
    return t_thread;
}

// 获取当前的线程名称
const std::string& Thread::GetName() {
    return t_thread_name;
}

// 设置当前线程名称
void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}


/**
 * @brief 构造函数
 * @param[in] cb 线程执行函数
 * @param[in] name 线程名称
 */
Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    // 创建一个线程，
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    // 如果创建错误，则打印日志
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}

// 析构函数
Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

// 等待线程执行完成
void Thread::join() {
    if(m_thread) {  // 如果有值，说明线程还没结束，可以join
        int rt = pthread_join(m_thread, nullptr);
        // 如果加入错误，则打印日志
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

// 线程执行函数
void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    // 给线程命名
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}

}
