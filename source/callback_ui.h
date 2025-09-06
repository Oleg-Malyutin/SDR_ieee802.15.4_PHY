#ifndef CALLBACK_UI_H
#define CALLBACK_UI_H

#include "ieee802_15_4/mac_sublayer/mac_types.h"

class ui_callback
{
public:
    virtual ~ui_callback(){};
    virtual void channel_scan(int channel_, bool done_)=0;
    virtual void energy_detect(uint8_t channel_, uint8_t energy_level)=0;
    virtual void active_detect(uint8_t channel_,  uint16_t pan_id_, uint64_t address_)=0;
};

#endif // CALLBACK_UI_H
