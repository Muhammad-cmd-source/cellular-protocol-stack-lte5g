#include "mac_layer.h"
#include <sstream>
MacLayer::MacLayer() {
    for (int i = 0; i < MAX_HARQ_PROCESSES; i++) harq_procs_[i].id = (uint8_t)i;
}
Bytes MacLayer::build_mac_pdu(LogicalChannel lc, const Bytes& payload) {
    Bytes pdu;
    pdu.push_back((uint8_t)lc);
    uint16_t len = (uint16_t)payload.size();
    pdu.push_back((len >> 8) & 0xFF);
    pdu.push_back(len & 0xFF);
    pdu.insert(pdu.end(), payload.begin(), payload.end());
    return pdu;
}
bool MacLayer::parse_mac_pdu(const Bytes& pdu, LogicalChannel& lc, Bytes& payload) {
    if (pdu.size() < 3) return false;
    lc = (LogicalChannel)pdu[0];
    uint16_t len = ((uint16_t)pdu[1] << 8) | pdu[2];
    if (pdu.size() < (size_t)(3 + len)) return false;
    payload = Bytes(pdu.begin() + 3, pdu.begin() + 3 + len);
    return true;
}
uint8_t MacLayer::get_next_harq_process() {
    for (int i = 0; i < MAX_HARQ_PROCESSES; i++) {
        uint8_t id = next_harq_id_;
        next_harq_id_ = (next_harq_id_ + 1) % MAX_HARQ_PROCESSES;
        if (harq_procs_[id].state == HarqState::IDLE) return id;
    }
    return 0;
}
Status MacLayer::transmit_sdu(const Bytes& rlc_sdu, Bytes& phy_pdu) {
    uint8_t proc_id = get_next_harq_process();
    HarqProcess& proc = harq_procs_[proc_id];
    Bytes mac_pdu = build_mac_pdu(LogicalChannel::DTCH, rlc_sdu);
    if (proc.state == HarqState::NACKED && proc.buffer.size() > 0) {
        mac_pdu = proc.buffer;
        proc.retx_count++;
        harq_retx_count_++;
        LOG_INFO("MAC", "HARQ RETX proc=" + std::to_string(proc_id));
        if (proc.retx_count >= MAX_HARQ_RETX) {
            proc = HarqProcess(); proc.id = proc_id;
            return Status::ERROR;
        }
    } else {
        proc.buffer = mac_pdu;
        proc.state  = HarqState::WAITING_ACK;
        proc.retx_count = 0;
    }
    phy_pdu = mac_pdu;
    tx_pdus_++;
    LOG_INFO("MAC", "TX MAC-PDU proc=" + std::to_string(proc_id) + " size=" + std::to_string(mac_pdu.size()));
    return Status::OK;
}
Status MacLayer::receive_pdu(const Bytes& phy_pdu, Bytes& rlc_sdu) {
    LogicalChannel lc;
    if (!parse_mac_pdu(phy_pdu, lc, rlc_sdu)) return Status::ERROR;
    rx_pdus_++;
    LOG_INFO("MAC", "RX MAC-PDU payload=" + std::to_string(rlc_sdu.size()) + " bytes");
    return Status::OK;
}
void MacLayer::harq_feedback(uint8_t process_id, bool ack) {
    if (process_id >= MAX_HARQ_PROCESSES) return;
    HarqProcess& proc = harq_procs_[process_id];
    LOG_INFO("MAC", "HARQ feedback proc=" + std::to_string(process_id) + (ack ? " ACK" : " NACK"));
    if (ack) { proc.state = HarqState::IDLE; proc.ack_received = true; proc.buffer.clear(); }
    else      { proc.state = HarqState::NACKED; }
}
