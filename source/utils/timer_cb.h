#ifndef TIMER_CB_H
#define TIMER_CB_H

#include <thread>
#include <mutex>
#include <condition_variable>

typedef void(*cb_fn)(void *ctx_);

class timer_cb
{
public:
    timer_cb();
    ~timer_cb();

    bool is_active = false;
    void start(int ms_, cb_fn cb_=nullptr, void *ctx_=nullptr);
    void stop();

private:
    void *ctx;
    std::thread *thread_timer = nullptr;
    void work(int ms_, cb_fn cb_);

};

#endif // TIMER_CB_H
