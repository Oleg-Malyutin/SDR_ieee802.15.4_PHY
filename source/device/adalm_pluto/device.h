#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

class device
{
public:
    device();

    virtual void rx_data(int16_t *i_buffer_, int16_t *q_buffer_, int len_) = 0;
};

#endif // DEVICE_H
