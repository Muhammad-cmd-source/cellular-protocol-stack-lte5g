#pragma once
#include "common_types.h"
#include <functional>
#include <string>
enum class RrcState { IDLE, CONNECTED, INACTIVE };
enum class RrcMsgType : uint8_t {
    RRC_SETUP_REQUEST=0x01, RRC_SETUP=0x02, RRC_SETUP_COMPLETE=0x03,
    RRC_RECONFIG=0x04, RRC_RECONFIG_COMPLETE=0x05, RRC_RELEASE=0x06,
    MEASUREMENT_REPORT=0x10, UE_CAPABILITY_INFO=0x20,
    SECURITY_MODE_CMD=0x30, SECURITY_MODE_COMPLETE=0x31,
};
struct CellConfig {
    uint32_t cell_id       = 0x0001;
    uint16_t plmn_mcc      = 310;
    uint16_t plmn_mnc      = 260;
    uint16_t tracking_area = 0x1234;
    uint32_t dl_arfcn      = 525000;
    uint8_t  num_prbs      = 106;
    int8_t   rsrp_dbm      = -85;
};
using RrcStateChangeCb = std::function<void(RrcState, RrcState)>;
class RrcLayer {
public:
    explicit RrcLayer(CellConfig cell = {});
    Status initiate_connection();
    Status receive_message(const Bytes& pdu, Bytes& response);
    Status release_connection();
    Status suspend_connection();
    Status resume_connection();
    Status send_measurement_report(int8_t rsrp, int8_t rsrq);
    RrcState    get_state()     const { return state_; }
    std::string get_state_str() const;
    void set_state_change_cb(RrcStateChangeCb cb) { state_cb_ = cb; }
private:
    RrcState         state_ = RrcState::IDLE;
    CellConfig       cell_cfg_;
    RrcStateChangeCb state_cb_;
    uint32_t         rnti_      = 0;
    uint32_t         msg_count_ = 0;
    void transition(RrcState new_state);
    Bytes build_rrc_msg(RrcMsgType type, const Bytes& payload = {});
    bool  parse_rrc_msg(const Bytes& pdu, RrcMsgType& type, Bytes& payload);
};
