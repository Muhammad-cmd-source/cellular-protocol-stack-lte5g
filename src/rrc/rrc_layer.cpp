#include "rrc_layer.h"
#include <sstream>
#include <cstdlib>
#include <ctime>
RrcLayer::RrcLayer(CellConfig cell) : cell_cfg_(cell) { std::srand((unsigned)std::time(nullptr)); }
void RrcLayer::transition(RrcState new_state) {
    state_ = new_state;
    LOG_INFO("RRC", "State -> " + get_state_str());
    if (state_cb_) state_cb_(state_, new_state);
}
std::string RrcLayer::get_state_str() const {
    switch(state_) {
        case RrcState::IDLE:      return "RRC_IDLE";
        case RrcState::CONNECTED: return "RRC_CONNECTED";
        case RrcState::INACTIVE:  return "RRC_INACTIVE";
    }
    return "UNKNOWN";
}
Bytes RrcLayer::build_rrc_msg(RrcMsgType type, const Bytes& payload) {
    Bytes msg;
    msg.push_back((uint8_t)type);
    msg.push_back((payload.size() >> 8) & 0xFF);
    msg.push_back(payload.size() & 0xFF);
    msg.insert(msg.end(), payload.begin(), payload.end());
    return msg;
}
bool RrcLayer::parse_rrc_msg(const Bytes& pdu, RrcMsgType& type, Bytes& payload) {
    if (pdu.size() < 3) return false;
    type = (RrcMsgType)pdu[0];
    uint16_t len = ((uint16_t)pdu[1] << 8) | pdu[2];
    if (pdu.size() < (size_t)(3 + len)) return false;
    payload = Bytes(pdu.begin() + 3, pdu.begin() + 3 + len);
    return true;
}
Status RrcLayer::initiate_connection() {
    if (state_ != RrcState::IDLE && state_ != RrcState::INACTIVE) return Status::INVALID_STATE;
    rnti_ = 0xC000 + (std::rand() % 0x3FFF);
    LOG_INFO("RRC", "Sending RRC_SETUP_REQUEST RNTI=0x" + std::to_string(rnti_));
    Bytes req = build_rrc_msg(RrcMsgType::RRC_SETUP_REQUEST, {(uint8_t)(rnti_>>8),(uint8_t)(rnti_&0xFF),0x01});
    Bytes resp;
    return receive_message(req, resp);
}
Status RrcLayer::receive_message(const Bytes& pdu, Bytes& response) {
    RrcMsgType type; Bytes payload;
    if (!parse_rrc_msg(pdu, type, payload)) return Status::ERROR;
    switch(type) {
        case RrcMsgType::RRC_SETUP_REQUEST:
            transition(RrcState::CONNECTED);
            response = build_rrc_msg(RrcMsgType::RRC_SETUP, {0x01,(uint8_t)cell_cfg_.num_prbs,0x02});
            LOG_INFO("RRC", "-> RRC_SETUP sent");
            break;
        case RrcMsgType::RRC_SETUP:
            response = build_rrc_msg(RrcMsgType::RRC_SETUP_COMPLETE, {0x00});
            LOG_INFO("RRC", "-> RRC_SETUP_COMPLETE sent");
            break;
        case RrcMsgType::RRC_RELEASE:
            transition(RrcState::IDLE);
            LOG_INFO("RRC", "Connection released");
            response = {};
            break;
        case RrcMsgType::SECURITY_MODE_CMD:
            response = build_rrc_msg(RrcMsgType::SECURITY_MODE_COMPLETE, {});
            break;
        default:
            LOG_WARN("RRC", "Unhandled message type");
            break;
    }
    msg_count_++;
    return Status::OK;
}
Status RrcLayer::release_connection() {
    if (state_ == RrcState::IDLE) return Status::INVALID_STATE;
    Bytes msg = build_rrc_msg(RrcMsgType::RRC_RELEASE, {});
    Bytes resp;
    return receive_message(msg, resp);
}
Status RrcLayer::suspend_connection() {
    if (state_ != RrcState::CONNECTED) return Status::INVALID_STATE;
    transition(RrcState::INACTIVE);
    LOG_INFO("RRC", "Connection suspended - context preserved");
    return Status::OK;
}
Status RrcLayer::resume_connection() {
    if (state_ != RrcState::INACTIVE) return Status::INVALID_STATE;
    LOG_INFO("RRC", "Resuming from RRC_INACTIVE");
    Bytes req = build_rrc_msg(RrcMsgType::RRC_SETUP_REQUEST, {});
    Bytes resp;
    return receive_message(req, resp);
}
Status RrcLayer::send_measurement_report(int8_t rsrp, int8_t rsrq) {
    LOG_INFO("RRC", "MeasReport RSRP=" + std::to_string(rsrp) + " RSRQ=" + std::to_string(rsrq));
    return Status::OK;
}
