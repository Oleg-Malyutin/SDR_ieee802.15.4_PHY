#ifndef CALLBACK_LAYER_H
#define CALLBACK_LAYER_H

#include "phy_layer/phy_types.h"
#include "mac_sublayer/mac_types.h"

class phy_layer_callback
{
public:
    virtual ~phy_layer_callback(){};

    virtual status_t pd_data_request(pd_data_request_t *pd_data_)=0;

    virtual void callback_demodulator_rssi()=0;
    virtual void callback_demodulator_rx()=0;

    virtual void callback_stop_phy_layer()=0;
};

class plme_sap_callback
{
public:
    virtual status_t  plme_cca_request()=0;
    virtual plme_ed_confirm_t plme_ed_request()=0;
    virtual plme_get_confirm_t plme_get_request(pib_attribute_t attribute_)=0;
    virtual status_t plme_set_trx_state_request(status_t state_)=0;
    virtual plme_set_confirm_t plme_set_request(pib_attribute_t attribute_, void *value_)=0;
};

class mac_sublayer_callback
{
public:
    virtual ~mac_sublayer_callback(){};

    virtual mcps_data_confirm_t mcps_data_request(mcps_data_request_t *mcsps_data_)=0;
//    virtual void mcps_data_indication(spdu_indication_t *mcps_data_)=0;

    virtual void pd_data_indication(pd_data_indication_t *pd_data_)=0;

    virtual void stop_mac_syblayer()=0;

};

class mlme_sap_callback
{
public:
    virtual mlme_start_confirm_t mlme_start_request(mlme_start_request_t start_request_)=0;
    virtual mlme_scan_confirm_t mlme_scan_request(scan_type_t scan_type_,
                                                  uint32_t scan_channels_,
                                                  uint8_t scan_duration_)=0;
    virtual void mlme_set_request(pib_attribute_t attribute_, void *value_)=0;
};

class mpdu_callback
{
public:
    virtual void rx_mpdu(mpdu_analysis_t *mpdu_)=0;
};


class callback_layer
{
public:
    callback_layer();
};

#endif // CALLBACK_LAYER_H
