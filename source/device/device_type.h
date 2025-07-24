/*
 *  Copyright 2025 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef DEVICE_TYPE_H
#define DEVICE_TYPE_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#define RX_PLUTO_LEN_BUFFER (16384 / 2)
#define TX_PLUTO_LEN_BUFFER (16384 / 2)
#define PLUTO_TX_GAIN_MIN -80
#define PLUTO_TX_GAIN_MAX 0
#define PLUTO_SAMPLES_SCALE  32767.0f

#define LMS_FIFO_SIZE 1024 * 8

enum enum_device_status{
    Open,
    Connect,
    Disconnect,
    Unknow
};

typedef struct rx_thread_data {
    int channel;
    bool ready = false;
    bool stop = false;
    std::condition_variable cond_value;
    std::mutex mutex;
    int len_buffer = 0;
    int16_t *ptr_buffer = nullptr;
    void *ctx = nullptr;
    enum_device_status status;
    void reset(){
        ready = false;
        stop = false;
        if(mutex.try_lock()) mutex.unlock();
    }
    ~rx_thread_data(){ptr_buffer = nullptr;}
} rx_thread_data_t;

typedef struct tx_thread_data {
    bool ready = false;
    bool stop = false;
    std::condition_variable cond_value;
    std::mutex mutex;
    int len_buffer = 0;
    float *ptr_buffer = nullptr;
    void *ctx = nullptr;
    enum_device_status status;
    void reset(){
        ready = false;
        stop = false;
        if(mutex.try_lock()) mutex.unlock();
    }
    ~tx_thread_data(){ptr_buffer = nullptr;}
} tx_thread_data_t;

typedef struct tx_thread_send_data {
    bool ready = false;
    bool stop = false;
    std::condition_variable cond_value;
    std::mutex mutex;
    std::vector<uint8_t> mdpu;
    void *ctx = nullptr;
    enum_device_status status;
    void reset(){
        ready = false;
        stop = false;
        if(mutex.try_lock()) mutex.unlock();
        mdpu.clear();
    }
    ~tx_thread_send_data(){reset();}
} tx_thread_send_data_t;

#endif // DEVICE_TYPE_H
