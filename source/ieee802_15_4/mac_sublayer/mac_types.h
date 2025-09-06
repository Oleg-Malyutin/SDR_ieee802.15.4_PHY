#ifndef MAC_TYPES_H
#define MAC_TYPES_H

#include <random>

#include "../phy_layer/phy_types.h"

#define macAckWaitDuration             (0x40)
#define macAssociationPermit           (0x41)
#define macAutoRequest                 (0x42)
#define macBattLifeExt                 (0x43)
#define macBattLifeExtPeriods          (0x44)
#define macBeaconPayload               (0x45)
#define macBeaconPayloadLength         (0x46)
#define macBeaconOrder                 (0x47)
#define macBeaconTxTime                (0x48)
#define macBSN                         (0x49)
#define macCoordExtendedAddress        (0x4a)
#define macCoordShortAddress           (0x4b)
#define macDSN                         (0x4c)
#define macGTSPermit                   (0x4d)
#define macMaxCSMABackoffs             (0x4e)
#define macMinBE                       (0x4f)
#define macPANId                       (0x50)
#define macPromiscuousMode             (0x51)
#define macRxOnWhenIdle                (0x52)
#define macShortAddress                (0x53)
#define macSuperframeOrder             (0x54)
#define macTransactionPersistenceTime  (0x55)
#define macDefaultSecurity             (0x72)

#define aAckWaitDuration         1 // 54 symbols = 864us (rounding up to 1ms)
#define aBaseSlotDuration        1 // 60 symbols = 960us (rounding up to 1ms)
#define aNumSuperframeSlots      16
#define aBaseSuperframeDuration  (aBaseSlotDuration * aNumSuperframeSlots)
#define aMaxBeaconOverhead       75
#define amacBeaconOrder          15
#define aMaxBeaconPayloadLength  (MaxPHYPacketSize - aMaxBeaconOverhead)
#define aMaxLostBeacons          4
#define aMinMPDUOverhead         9
#define aMaxMACFrameSize         (MaxPHYPacketSize - aMinMPDUOverhead)
#define aMinCAPLength            7 // 440 symbols = 7040us (rounding up to 7ms)
#define aUnitBackoffPeriod       1 // 20 symbols = 320us (rounding up to 1ms)
#define aMaxFrameTotalWaitTime   20 // 1220 symbols = 19500us (rounding up to 20ms)
#define aResponseWaitTime        (aBaseSlotDuration * 32)
#define aMaxFrameRetries         3
#define aMaxCsmaBackoffs         5
#define aMinBE                   3
#define aSuperframeOrder         15
#define aTransactionPersistenceTime 0x01f4

#define MAC_BROADCAST_ADDR 0xFFFF

typedef struct mac_pib_tag{
    uint16_t ack_wait_duration;
    bool associaton_permit;
    bool auto_request;
    bool batt_life_ext;
    uint8_t *beacon_payload;
    uint8_t beacon_payload_length;
    uint8_t beacon_order;
    uint32_t beacon_tx_time;
    uint8_t bsn;
    uint8_t dsn;
    uint8_t min_be;
    uint8_t max_csma_backoffs;
    uint16_t pan_id;
    uint16_t original_pan_id;
    bool rx_on_when_idle;
    uint16_t coord_short_address;
    uint64_t coord_extendet_address;
    bool promiscuous_mode;
    uint16_t short_address;
    uint8_t superframe_order;
    uint16_t transaction_persistence_time;
    bool default_security;
    void reset(){
        ack_wait_duration = aAckWaitDuration;
        associaton_permit = false;
        auto_request = true;
        batt_life_ext = false;
        beacon_payload = nullptr;
        beacon_payload_length = 0;
        beacon_order = amacBeaconOrder;
        beacon_tx_time = 0;
        bsn = (static_cast<double>(std::rand()) / RAND_MAX) * 0xff;
        dsn = (static_cast<double>(std::rand()) / RAND_MAX) * 0xff;
        min_be = aMinBE;
        max_csma_backoffs = aMaxCsmaBackoffs;
        pan_id = 0xffff;
        rx_on_when_idle = false;
        coord_short_address = 0xffff;
        short_address = 0xffff;
        promiscuous_mode = false;
        superframe_order = aSuperframeOrder;
        transaction_persistence_time = aTransactionPersistenceTime;
        default_security = false;
    };
} mac_pib_t;

enum mac_command_frames{
    Association_request=0,
    Association_response,
    Disassociation_notification,
    Data_request,
    PAN_ID_conflict_notification,
    Orphan_notification,
    Beacon_request,
    Coordinator_realignment,
    GTS_request
};

enum frame_type{
    Frame_type_beacon=0,
    Frame_type_data,
    Frame_type_acknowledgment,
    Frame_type_MAC_command,
    Frame_type_reserved
};

enum adressing_mode{
    Adressing_mode_not_present=0,
    Adressing_mode_reserved,
    Adressing_mode_16bit_short,
    Adressing_mode_64bit_extended
};

struct frame_control_field{
    frame_type type;
    uint8_t security_enabled;
    uint8_t frame_pending;
    uint8_t ask_request;
    uint8_t intra_pan;
    adressing_mode dest_mode;
    uint8_t frame_version;
    adressing_mode source_mode;
};

struct addressing_fields{
    adressing_mode dest_address_mode;
    uint16_t dest_pan_id;
    uint64_t dest_address;
    adressing_mode source_address_mode;
    uint16_t source_pan_id;
    uint64_t source_address;
    uint16_t offset;
};

typedef struct{
    frame_control_field fcf;
    addressing_fields af;
    uint8_t msdu[MaxPHYPacketSize];
    uint8_t len_data;
} mpdu_analysis_t;

typedef struct{
    uint8_t src_addr_mode;
    uint16_t src_pan_id;
    uint64_t src_addr;
    uint8_t dst_addr_mode;
    uint16_t dst_pan_id;
    uint16_t dst_addr;
    uint8_t msdu_length;
    uint8_t *msdu;
    uint8_t msdu_handle;
    uint8_t tx_options;
} mcps_data_request_t;

typedef struct{
    uint8_t src_addr_mode;
    uint16_t src_pan_id;
    uint64_t src_addr;
    uint8_t dst_addr_mode;
    uint16_t dst_pan_id;
    uint16_t dst_addr;
    uint8_t msdu_length;
    uint8_t msdu[aMaxMACFrameSize];
    uint8_t mpdu_link_quality;
    bool security_use;
    uint8_t acl_entry;
} spdu_indication_t;

typedef struct {
    uint8_t src_addr_mode;
    uint64_t src_addr;
    uint8_t dst_addr_mode;
    uint16_t dst_panid;
    uint64_t dst_addr;
    uint8_t tx_options;
    uint8_t msdu_length;
    uint8_t *msdu;
    uint8_t msdu_handle;
} spdu_request_t;

typedef pd_data_request_t mpdu_t;

typedef struct {
    uint8_t msdu_handle;
    uint8_t status;
} mcps_data_confirm_t;

enum scan_type_t{
    energy_detect_scan,
    active_scan,
    passive_scan,
    orphan_scan,
    all_scan
};

typedef struct{
    uint16_t pan_id;
    uint8_t logical_channel;
    uint8_t beacon_order;
    uint8_t superframe_order;
    bool pan_coordinator;
    bool battery_life_extension;
    bool coord_realignment;
    bool security_enable;
} mlme_start_request_t;

typedef struct{
    status_t status;
    uint8_t energy_level;
} energy_detect_t;

typedef struct {
    uint8_t coord_addr_mode;
    uint16_t coord_pan_id;
    uint64_t   coord_address;
    uint8_t logical_channel;
    uint16_t super_frame_spec;
    bool gts_remit;
    uint8_t link_quality;
    uint64_t time_stamp;
    bool security_use;
    uint8_t acl_entry;
    bool security_failure;
} pan_description_t;

typedef struct{
    status_t status;
    scan_type_t scan_type;
    uint32_t unscanned_channels;
    uint32_t result_list_size;
    uint8_t *energy_detect_list;
    pan_description_t *pan_description_list;
}mlme_scan_confirm_t;

typedef status_t mlme_start_confirm_t;

#endif // MAC_TYPES_H
