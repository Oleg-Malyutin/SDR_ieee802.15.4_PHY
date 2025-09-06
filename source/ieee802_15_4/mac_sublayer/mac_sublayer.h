#ifndef MAC_SUBLAYER_H
#define MAC_SUBLAYER_H

#include "../callback_layer.h"
#include "callback_ui.h"
#include "callback_higher_layer.h"
#include "../../utils/timer_cb.h"

//struct MCPS_DATA_request{
//        uint8_t SrcAddrMode;
//        uint16_t SrcPANId;
//        uint16_t SrcAddr;
//        uint8_t DstAddrMode;
//        uint16_t DstPANId;
//        uint16_t DstAddr;
//        uint8_t msduLength;
//        uint8_t msdu[aMaxMACFrameSize];
//        uint8_t msduHandle;
//        uint8_t TxOptions;
//};

//struct MCPS_DATA_confirm{
//    enum confirm_status{
//        SUCCESS,
//        TRANSACTION_OVERFLOW,
//        TRANSACTION_EXPIRED,
//        CHANNEL_ACCESS_FAILURE,
//        INVALID_GTS, NO_ACK,
//        UNAVAILABLE_KEY,
//        FRAME_TOO_LONG,
//        FAILED_SECURITY_CHECK,
//        INVALID_PARAMETER
//    };
//    uint8_t msduHandle;
//    confirm_status status;
//};

//struct MCPS_PURGE_request{
//    uint8_t msduHandle;
//};

//struct MCPS_PURGE_confirm{
//    uint8_t msduHandle;
//    enum confirm_status{
//        SUCCESS,
//        INVALID_HANDLE
//    };
//    confirm_status status;
//};





class mac_sublayer : public mac_sublayer_callback, public mlme_sap_callback

{
public:
    mac_sublayer();
    ~mac_sublayer();

    bool is_started = false;
    void start();
    void connect_mpdu(mpdu_callback *cb_){callback_mpdu = cb_;};
    void connect_phy(phy_layer_callback *cb_){callback_phy = cb_;};
    void connect_plme(plme_sap_callback *cb_){callback_plme = cb_;};
    void connect_higher(higher_layer_callbck *cb_){callback_higher = cb_;};
    void connect_ui(ui_callback *cb_){callback_ui = cb_;};

private:
    mac_pib_t *mac_pib;
    mpdu_analysis_t *mpdu;
    mpdu_callback *callback_mpdu;
    enum rf_mode{
        Rx,
        Tx,
        CCA_confirm,
        Idle
    };
    rf_mode event_mode = Rx;
    bool event_ready = false;
    std::condition_variable event;
    std::mutex event_mutex;
    typedef struct {
        bool ready;
        std::condition_variable condition;
        std::mutex mutex;
    } wait_confirm_t;
    wait_confirm_t wait_confirm;
    bool start_on = true;
    void work();
    void stop_mac_syblayer();
    void stop();

    mlme_start_request_t *start_request;
    mlme_start_confirm_t mlme_start_request(mlme_start_request_t start_request_);

    mcps_data_confirm_t mcps_data_request(mcps_data_request_t *mcps_data_);
    mcps_data_confirm_t send_mpdu(mpdu_t mpdu_);

    pd_data_indication_t *pd_data;
    void pd_data_indication(pd_data_indication_t *pd_data_);
    spdu_indication_t *spdu_indication;
    void mac_filtering_mpdu();
    frame_control_field frame_control_parser();
    void mac_acknowledgment(frame_control_field fcf_);
    void mac_beacon(frame_control_field fcf_);
    void mcps_data_rx(frame_control_field fcf_);
    void mac_command(frame_control_field fcf_);
    addressing_fields parse_addressing(frame_control_field &fcf_);

    phy_layer_callback *callback_phy;
    plme_sap_callback *callback_plme;
    pib_attribute_t phy_pib_attribute;
    uint8_t phy_current_channel;
    uint32_t channels_supported;
    uint32_t transmit_power;
    cca_mode_t cca_mode;

    pib_attribute_t mac_pib_attribute;
    void mlme_set_request(pib_attribute_t attribute_, void *value_);
    mlme_scan_confirm_t mlme_scan_request(scan_type_t scan_type_, uint32_t scan_channels_,
                           uint8_t scan_duration_);
    timer_cb *timer_scan;
    static void callback_timer_scan(void *ctx_);
    std::vector<uint8_t> energy_detect_list;
    mlme_scan_confirm_t mlme_energy_detect_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                                 uint16_t scan_duration_ms_, uint8_t scan_current_channel_);

    std::vector<pan_description_t> pan_description_list;
    mlme_scan_confirm_t mlme_active_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                                 uint16_t scan_duration_ms_, uint8_t scan_current_channel_);
    mlme_scan_confirm_t mlme_passive_scan(scan_type_t scan_type_, uint32_t scan_channels_,
                                 uint16_t scan_duration_ms_, uint8_t scan_current_channel_);

    higher_layer_callbck *callback_higher;

    ui_callback *callback_ui;

};

#endif // MAC_SUBLAYER_H
