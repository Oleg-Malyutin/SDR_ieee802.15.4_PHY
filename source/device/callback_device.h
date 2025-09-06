#ifndef CALLBACK_DEVICE_H
#define CALLBACK_DEVICE_H

#include <complex>
#include "device/device_type.h"

class device_callback
{
public:
    virtual ~device_callback(){};
    virtual void set_channel(int channel_, bool &status_)=0;
    virtual void preamble_correlation_callback(int len_, std::complex<float> *b_)=0;
    virtual void sfd_callback(int len1_, std::complex<float> *b1_, int len2_, std::complex<float> *b2_)=0;
    virtual void constelation_callback(int len_, std::complex<float> *b_)=0;
    virtual void error_callback(enum_device_status status_)=0;

    virtual void plot_test_callback(int len_, float *b_)=0;
};



class callback_device
{
public:
    callback_device();
};

#endif // CALLBACK_DEVICE_H
