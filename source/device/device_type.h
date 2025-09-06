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
#include <complex>

#include "../ieee802_15_4/phy_layer/phy_types.h"
#include "../utils/buffers.hh"

enum enum_device_status{
    Open,
    Connect,
    Disconnect,
    Unknow
};

typedef struct rx_thread_data {
    rx_mode set_mode;
    rx_mode mode;
    int channel;
    bool ready = false;
    bool stop_rx = false;
    bool stop_demodulator = false;
    std::condition_variable condition_value;
    std::mutex mutex;
    int len_buffer = 0;
    std::complex<float> *ptr_buffer = nullptr;
    float rssi;
    enum_device_status status;
    void reset(){
        ready = false;
        stop_rx = false;
        mode = rx_mode_data;
        stop_demodulator = false;
        if(mutex.try_lock()) mutex.unlock();
    }
    ~rx_thread_data(){ptr_buffer = nullptr;}
} rx_thread_data_t;

class rf_tx_callback
{
public:
    virtual ~rf_tx_callback(){};
    virtual bool tx_data(int &len_buffer_, const float *ptr_buffer_)=0;
};


#endif // DEVICE_TYPE_H
