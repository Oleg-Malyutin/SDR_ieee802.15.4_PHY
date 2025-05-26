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
#ifndef IEEE802_15_4_H
#define IEEE802_15_4_H

#include <cmath>
#include <complex>
#include <ctime>

#include "device/device_type.h"
#include "mac_sublayer.h"
#include "utils/buffers.hh"

#include <QDebug>

#define SAMPLERATE 4000000
#define CHIP_RATE 2000000
#define OVERSAMPLE  (SAMPLERATE / CHIP_RATE)
#define LEN_CHIP  (32 * OVERSAMPLE)
#define MAX_BITS_PACKET (MAX_PHY_PAYLOAD * LEN_CHIP)
#define INTEGER_CFO (M_PI / LEN_CHIP)
#define LEN_PREAMBLE  (LEN_CHIP * 8)
#define LEN_BUFF_SFD  (LEN_CHIP * 2)

typedef std::complex<float> complex;
typedef fifo_buffer<complex, LEN_PREAMBLE> fifo_buffer_256;

struct ieee802_15_4_info{
    static const int number_of_channels = 27;
    constexpr static const float fq_channel_mhz[number_of_channels] = {
        868.3f,
        906.0f, 908.0f, 910.0f, 912.0f, 914.0f, 916.0f, 918.0f, 920.0f, 922.0f, 924.0f,
        2405.0f, 2410.0f, 2415.0f, 2420.0f, 2425.0f, 2430.0f, 2435.0f, 2440.0f,
        2445.0f, 2450.0f, 2455.0f, 2460.0f, 2465.0f, 2470.0f, 2475.0f, 2480.0f
    };
    enum type_mod{
        BPSK,
        ASK,
        O_QPSK
    };
    static const long long int samplerate = SAMPLERATE;
    static const long long int rf_bandwidth = 2000000;
    static const int channel = 25;
    static const long long int rf_frequency = fq_channel_mhz[channel] * 1000000;
};

//typedef void (*cb_fn)(void* buffer);

class ieee802_15_4_callback
{
public:
    virtual void plot_callback(int len1_, std::complex<float> *b1_, int len2_, std::complex<float> *b2_,
                             int len3_, std::complex<float> *b3_, int len4_, std::complex<float> *b4_)=0;
};

class ieee802_15_4
{

public:
    explicit ieee802_15_4();
    ~ieee802_15_4();

    bool is_started = false;
    void connect_callback(ieee802_15_4_callback *cb_){callback = cb_;};
    void start(rx_thread_data_t *rx_thread_data_);
    mac_sublayer *mac_layer;

private:
    void init_local();

    // matched filter
    std::vector<complex> iq_buffer;
    std::vector<complex> iq_data;
    static constexpr float h_matched_filter[4] = {0.0f, 1.0f / M_SQRT2, 1.0f, 1.0f / M_SQRT2};
    complex buff_matched_filter[3];
    void half_sine_matched_filter(int len_, int16_t *i_buffer_, int16_t *q_buffer_);

    // preamble correlation
    float max_level = 0.0f;
    float level_thr = 0.0f;
    moving_average<float, LEN_CHIP / 2> avg_level;
    complex sum_double_correlation = {0.0f, 0.0f};
    complex max_double_correlation = {0.0f, 0.0f};
    int idx_preamble = 0;
    int idx_margin = 0;
    constexpr static int len_segment = 6 ; //6 should be <= 10
    constexpr static int num_segment = 4;  //3 should be <= (LEN_CHIP / len_segment - 1)
    constexpr static int len_local = len_segment * num_segment;
    constexpr static float k_tr = 2e5 * len_local * len_local;  //2e5 selected from experience
    fifo_buffer<complex, len_local> delay_seg_for_double_correlator;
    float local_zz_real[len_local];
    float local_zz_imag[len_local];
    float seg_real[num_segment];
    float seg_imag[num_segment];
    inline complex double_correlator3(const complex *chip_);
    inline complex double_correlator2(const complex* chip_);
    inline complex double_correlator(const complex* chip_);

    // frequency estimation
    fifo_buffer<complex, LEN_CHIP> delay_chip;
    complex cross_correlation = {0.0f, 0.0f};
    complex sum_cross_correlation = {0.0f, 0.0f};
    float coarse_fq_offset = 0.0f;
    float fine_fq_offset = 0.0f;
    float fq_offset = 0.0f;
    float phase_nco = 0.0f;
    float nco = 0.0f;
    complex nco_derotate = {1.0f, 0.0f};

    // sfd correlation
    float angle_offset = 0.0f;
    int coarse_timming_correction = 0;
    int idx_cor_z = 0;
    float cor_z[4];
    complex *conj_local_sfd;
    complex sfd_correlation[4] = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};

    // interpolator
    float x2, x3, x1 = 0.0f;
    complex delay_data_1 = {0.0f, 0.0f}, delay_data_2 = {0.0f, 0.0f}, delay_data_3 = {0.0f, 0.0f};
    float mu = 0.0f;
    float i_delay_1 = 0.0f;
    float i_delay_2 = 0.0f;
    inline complex interpolator(const complex &data_, const float &mu_);

    // symbol detect
    int idx_chip = 0;
    int idx_symbol = 0;
    float i_correlation[16];
    float q_correlation[16];
    float imag_sum[16];
    float temp_sum_detect = 0;
    float *local_real;
    float *local_imag;
    float pre_angle_local_correlation = 0.0f;
    complex old_local_correlation = {0.0f, 0.0f};
    float sum_max = 0.0f;
    uint8_t symbol_detected = 0;

    mac_sublayer_data_t *mac_data;
    std::thread *mac_layer_thread;
    std::vector<uint8_t> v1_mpdu;
    std::vector<uint8_t> v2_mpdu;
    bool swap_mpdu;

    enum rx_state{
        detect_signal,
        detect_preamble,
        frequency_offset_estimation,
        frame_delimiter_phase_synchronisation,
        frame_delimiter_timing_synchronisation,
        frame_lenght,
        payload
    };
    rx_state state = detect_signal;
    rx_state state_type = frame_lenght;
    int idx_i = 0;
    int len_psdu = 0;
    void detection(int &in_len_, int16_t *iq_data_);
    void reset_detection();

    complex plot_data;
    fifo_buffer_256 preamble_correlation_fifo;
    std::complex<float> *preamble_correlation_buffer;
    int idx_sfd_corr_buf;
    std::complex<float> *sfd_correlation_buffer;
    int idx_sfd_sync_buf;
    std::complex<float> *sfd_synchronize_buffer;
    int idx_const_buf;
    std::complex<float> *constelation_buffer;

    // thread
    ieee802_15_4_callback *callback;
    void work(rx_thread_data_t *rx_thread_data_);
    void rx_data(int len_, int16_t *i_buffer_);
    void stop();

    // todo
    void tx_ptr_data(int16_t* ptr_, int len_);

    constexpr static int chip_table[16][32] = {
        {1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1},
        {1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1},
        {-1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1},
        {-1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1},
        {-1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1},
        {-1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1},
        {1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1},
        {1, -1, -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1},
        {1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1},
        {1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1},
        {-1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1},
        {-1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1},
        {-1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1, 1, -1, -1, 1},
        {1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1, 1, 1, -1, -1},
        {1, 1, -1, -1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, -1}
    };

};

#endif // IEEE802_15_4_H
