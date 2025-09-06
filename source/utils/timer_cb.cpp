#include "timer_cb.h"

#include <chrono>

//-----------------------------------------------------------------------------------------
timer_cb::timer_cb()
{

}
//-----------------------------------------------------------------------------------------
timer_cb::~timer_cb()
{
    stop();
}
//-----------------------------------------------------------------------------------------
void timer_cb::start(int ms_, cb_fn cb_, void *ctx_)
{
    ctx = ctx_;
    is_active = true;
    thread_timer = new std::thread(&timer_cb::work, this, ms_, cb_);
}
//-----------------------------------------------------------------------------------------
void timer_cb::stop()
{
    if(thread_timer != nullptr){
        thread_timer->join();
        delete thread_timer;
        thread_timer = nullptr;
    }
}
//-----------------------------------------------------------------------------------------
void timer_cb::work(int ms_, cb_fn cb_)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms_));
    if(cb_ != nullptr) cb_(ctx);
    is_active = false;
}
//-----------------------------------------------------------------------------------------
