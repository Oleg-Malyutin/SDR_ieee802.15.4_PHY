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
#include <unordered_map>
#include <functional>
#include <string>

#include "../ieee802_15_4/phy_layer/phy_types.h"
#include "../utils/buffers.hh"

#if defined(_WIN32) || defined(__CYGWIN__)
typedef unsigned int uint;
#endif


typedef struct {
    void *ctx;
    void *buffer;
    int len_buffer;
    bool usb_event_ready;
    std::condition_variable usb_event;
    std::mutex usb_event_mutex;
} rx_usb_transfer;

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
    bool stop = false;
    bool stop_demodulator = false;
    std::condition_variable condition;
    std::mutex mutex;
    int len_buffer = 0;
    std::complex<float> *ptr_buffer = nullptr;
    float rssi;
    enum_device_status status;
    void reset(){
        ready = false;
        stop = false;
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
    virtual bool tx_data(uint &len_buffer_, const float *ptr_buffer_)=0;
};

class sdr_device
{
public:
    virtual ~sdr_device(){};
    virtual void* get_dev_tx()=0;
    virtual bool open_device(std::string &name_, std::string &err_)=0;
    virtual void close_device()=0;
    virtual bool check_connect()=0;
    virtual void advanced_settings_dialog()=0;
    virtual void start(rx_thread_data_t *rx_thread_data_)=0;
    virtual void stop()=0;
    virtual void set_rx_rf_bandwidth(long long int bandwidht_hz_)=0;
    virtual void set_rx_sampling_frequency(long long int sampling_frequency_hz_)=0;
    virtual void get_rx_max_hardwaregain(double &hardwaregain_db_)=0;
    virtual void set_rx_hardwaregain(double hardwaregain_db_)=0;
    virtual void set_rx_frequency(long long int frequency_hz_)=0;
    virtual void set_tx_rf_bandwidth(long long int bandwidht_hz_)=0;
    virtual void set_tx_sampling_frequency(long long int sampling_frequency_hz_)=0;
    virtual void get_tx_max_hardwaregain(double &hardwaregain_db_)=0;
    virtual void set_tx_hardwaregain(double hardwaregain_db_)=0;
    virtual void set_tx_frequency(long long int frequency_hz_)=0;
};

class sdr_factory {
public:
    typedef std::unordered_map<std::string, std::function<sdr_device*()>> registry_map;

    static sdr_device * instantiate(const std::string& name)
    {
        auto it = sdr_factory::registry().find(name);
        return it == sdr_factory::registry().end() ? nullptr : (it->second)();
    }

    static registry_map & registry(){
        static registry_map impl;
        return impl;
    }
};

template<typename T> struct sdr_device_new
{
    sdr_device_new(std::string name)
    {
        sdr_factory::registry()[name] = []() { return new T; };
    }
};


#endif // DEVICE_TYPE_H
