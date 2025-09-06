#ifndef PHY_TYPES_H
#define PHY_TYPES_H

#include <stdint.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>


#define MaxPHYPacketSize (127)

#define TurnaroundTime 12

typedef int pib_attribute_t;

#define phyCurrentChannel       (0x00)
#define phyChannelsSupported    (0x01)
#define phyTransmitPower        (0x02)
#define phyCCAMode              (0x03)

enum rx_mode{
    rx_mode_data,
    rx_mode_rssi,
    rx_mode_off
};

typedef struct{
    int channel;
    bool tx_on;
    bool succes;
    double rssi;
} modulator_status_t;



enum status_t{
    BUSY=0,                 //CCA attempt has detected a busy channel.
    BUSY_RX,                //The transceiver is asked to change its state while receiving.
    BUSY_TX,                //The transceiver is asked to change its state while transmitting.
    FORCE_TRX_OFF,          //The transceiver is to be switched off.
    IDLE,                   //The CCA attempt has detected an idle channel.
    INVALID_PARAMETER,      //SET/GET request was issued with a parameter in the primitive that is out of the valid range.
    RX_ON,                  //The transceiver is in or is to be configured into the receiver enabled state.
    SUCCESS,                //SET/GET, an ED operation, or a transceiver state change was successful.
    TRX_OFF,                //The transceiver is in or is to be configured into the transceiver disabled state.
    TX_ON,                  //The transceiver is in or is to be configured into the transmitter enabled state.
    UNSUPPORTED_ATTRIBUTE   //SET/GET request was issued with the identifier of an attribute that is not supported.
};

enum cca_mode_t{
    energy_threshold,
    carrier_sense,
    carrier_sense_energy_threshold
};

typedef struct{
    uint8_t current_channel;
    uint32_t channels_supported;
    uint32_t transmit_power;
    cca_mode_t cca_mode;
} phy_pib_t;

typedef struct rx_psdu_tag{
    rx_mode mode;
    int channel;
    bool ready;
    double rssi;
    bool is_signal = false;
    int len_symbols;
    uint8_t *symbols;
    std::mutex mutex;
    std::condition_variable condition;
} rx_psdu_t;


typedef struct {
    uint8_t mpdu_length;
    uint8_t mpdu[MaxPHYPacketSize];
    uint8_t ppdu_link_quality;
} pd_data_indication_t;

typedef struct rf_sap_tag{
    int channel = 11;
    bool is_started = false;
    bool ready = false;
    bool stop = false;
    status_t status;
    double rssi = 0;
    std::condition_variable cond_value;
    std::mutex mutex;
    bool ready_confirm = false;
    std::condition_variable cond_confirm;
    std::mutex mutex_confirm;
    std::vector<uint8_t> *symbols;
} rf_sap_t;

typedef struct {
    uint8_t mpdu_length;
    uint8_t mpdu[MaxPHYPacketSize];
} pd_data_request_t;

typedef struct{
    status_t status;
    pib_attribute_t pib_atribute;
}plme_set_confirm_t;

typedef struct{
    status_t status;
    pib_attribute_t pib_atribute;
    void *value;
}plme_get_confirm_t;

struct plme_ed_confirm_t{
    status_t status;
    uint8_t energy_level;
};

#endif // PHY_TYPES_H
