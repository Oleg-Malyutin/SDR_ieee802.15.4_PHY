#ifndef IEEE802_15_4_H
#define IEEE802_15_4_H

#include <complex>

#include "device/device_type.h"

#ifndef M_SQRT2
#define M_SQRT2 (std::sqrtf(2.0f))
#endif

#define SAMPLERATE 4000000
#define CHIP_RATE 2000000
#define OVERSAMPLE  (SAMPLERATE / CHIP_RATE)
#define SHIFT_IQ (2 * OVERSAMPLE)
#define LEN_PN_SEQUENCE 32
#define LEN_CHIP  (LEN_PN_SEQUENCE * OVERSAMPLE)                 // type complex
#define MAX_PHY_PAYLOAD 127
#define MAX_BITS_PACKET (MAX_PHY_PAYLOAD * LEN_CHIP)
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#ifndef M_PI_2
#define M_PI_2 (1.57079632679489661923)
#endif
#define INTEGER_CFO (M_PI / LEN_CHIP)
#define LEN_PREAMBLE  (LEN_CHIP * 8)
#define LEN_SFD  (LEN_CHIP * 2)
#define LEN_SHR (LEN_PREAMBLE + LEN_SFD)
#define LEN_MAX_SAMPLES (LEN_SHR + MAX_BITS_PACKET) // type complex

typedef std::complex<float> complex;

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
    static const int channel = 18;
    static const long long int rf_frequency = fq_channel_mhz[channel] * 1000000;
};

//typedef void (*cb_fn)(void* buffer);

class ieee802_15_4_callback
{
public:
    virtual void preamble_correlation_callback(int len_, std::complex<float> *b_)=0;
    virtual void sfd_callback(int len1_, std::complex<float> *b1_, int len2_, std::complex<float> *b2_)=0;
    virtual void constelation_callback(int len_, std::complex<float> *b_)=0;
    virtual void error_callback(enum_device_status status_)=0;

    virtual void plot_test_callback(int len_, float *b_)=0;
};

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

const float h_matched_filter[4] = {0.0f, 1.0f / M_SQRT2, 1.0f, 1.0f / M_SQRT2};

#endif // IEEE802_15_4_H
