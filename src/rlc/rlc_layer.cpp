#include "rlc_layer.h"
#include <sstream>
RlcLayer::RlcLayer(RlcMode mode) : mode_(mode) {}
Bytes RlcLayer::build_am_pdu(const RlcAmHeader& hdr, const Bytes& payload) {
    Bytes pdu;
    uint8_t b0 = 0;
    b0 |= (hdr.data_ctrl ? 0x80 : 0x00);
    b0 |= (hdr.poll_bit  ? 0x40 : 0x00);
    b0 |= ((hdr.seg_info & 0x03) << 4);
    b0 |= ((hdr.sn >> 8) & 0x0F);
    pdu.push_back(b0);
    pdu.push_back(hdr.sn & 0xFF);
    pdu.insert(pdu.end(), payload.begin(), payload.end());
    return pdu;
}
bool RlcLayer::parse_am_pdu(const Bytes& pdu, RlcAmHeader& hdr, Bytes& payload) {
    if (pdu.size() < 2) return false;
    hdr.data_ctrl = (pdu[0] & 0x80) != 0;
    hdr.poll_bit  = (pdu[0] & 0x40) != 0;
    hdr.seg_info  = (pdu[0] >> 4) & 0x03;
    hdr.sn        = ((uint16_t)(pdu[0] & 0x0F) << 8) | pdu[1];
    payload       = Bytes(pdu.begin() + 2, pdu.end());
    return true;
}
Bytes RlcLayer::build_status_pdu(uint16_t ack_sn) {
    Bytes pdu;
    pdu.push_back((uint8_t)(ack_sn >> 8) & 0x0F);
    pdu.push_back(ack_sn & 0xFF);
    return pdu;
}
Status RlcLayer::transmit_sdu(const Bytes& pdcp_sdu, Bytes& mac_pdu) {
    if (mode_ == RlcMode::TM) {
        mac_pdu = pdcp_sdu;
        LOG_DEBUG("RLC", "TM TX " + std::to_string(pdcp_sdu.size()) + " bytes");
        return Status::OK;
    }
    RlcAmHeader hdr;
    hdr.data_ctrl = true;
    hdr.sn        = tx_sn_;
    hdr.poll_bit  = (tx_sn_ % 16 == 0);
    hdr.seg_info  = 0x00;
    mac_pdu = build_am_pdu(hdr, pdcp_sdu);
    if (mode_ == RlcMode::AM) {
        RlcTxBuffer buf; buf.sdu = pdcp_sdu; buf.sn = tx_sn_;
        tx_window_.push_back(buf);
        if (tx_window_.size() > RLC_AM_WINDOW_SIZE) tx_window_.pop_front();
    }
    LOG_INFO("RLC", "TX AM-PDU SN=" + std::to_string(tx_sn_) + " size=" + std::to_string(mac_pdu.size()));
    tx_sn_ = next_sn(tx_sn_);
    return Status::OK;
}
Status RlcLayer::receive_pdu(const Bytes& mac_pdu, Bytes& pdcp_sdu) {
    if (mode_ == RlcMode::TM) { pdcp_sdu = mac_pdu; return Status::OK; }
    RlcAmHeader hdr; Bytes payload;
    if (!parse_am_pdu(mac_pdu, hdr, payload)) return Status::ERROR;
    LOG_INFO("RLC", "RX AM-PDU SN=" + std::to_string(hdr.sn));
    if (hdr.sn == rx_sn_) {
        pdcp_sdu = payload;
        rx_sn_   = next_sn(rx_sn_);
        return Status::OK;
    }
    RlcRxBuffer rbuf; rbuf.payload = payload; rbuf.sn = hdr.sn; rbuf.received = true;
    rx_window_[hdr.sn] = rbuf;
    LOG_WARN("RLC", "Out-of-order SN=" + std::to_string(hdr.sn));
    return Status::PENDING;
}
void RlcLayer::process_status_pdu(uint16_t ack_sn, const std::vector<uint16_t>& nack_sns) {
    for (auto& buf : tx_window_) {
        bool before_ack = ((ack_sn - buf.sn) & 0x0FFF) < RLC_AM_WINDOW_SIZE;
        if (before_ack) buf.acked = true;
    }
    for (uint16_t nack : nack_sns) nack_list_.push_back(nack);
    while (!tx_window_.empty() && tx_window_.front().acked) tx_window_.pop_front();
}
Status RlcLayer::retransmit_nacked(Bytes& mac_pdu) {
    if (nack_list_.empty()) return Status::OK;
    uint16_t nack_sn = nack_list_.front(); nack_list_.pop_front();
    for (auto& buf : tx_window_) {
        if (buf.sn == nack_sn) {
            if (buf.retx_count >= RLC_MAX_RETX) return Status::ERROR;
            buf.retx_count++;
            RlcAmHeader hdr; hdr.data_ctrl=true; hdr.sn=buf.sn; hdr.poll_bit=true; hdr.seg_info=0;
            mac_pdu = build_am_pdu(hdr, buf.sdu);
            return Status::OK;
        }
    }
    return Status::ERROR;
}
