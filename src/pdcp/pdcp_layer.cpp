#include "pdcp_layer.h"
#include <sstream>
PdcpLayer::PdcpLayer(PdcpBearerType type) : type_(type) {}
Bytes PdcpLayer::build_pdcp_pdu(const PdcpHeader& hdr, const Bytes& payload) {
    Bytes pdu;
    uint8_t b0 = (hdr.data_ctrl ? 0x80 : 0x00) | ((hdr.sn >> 8) & 0x0F);
    pdu.push_back(b0);
    pdu.push_back(hdr.sn & 0xFF);
    pdu.insert(pdu.end(), payload.begin(), payload.end());
    return pdu;
}
bool PdcpLayer::parse_pdcp_pdu(const Bytes& pdu, PdcpHeader& hdr, Bytes& payload) {
    if (pdu.size() < 2) return false;
    hdr.data_ctrl = (pdu[0] & 0x80) != 0;
    hdr.sn        = ((uint16_t)(pdu[0] & 0x0F) << 8) | pdu[1];
    payload       = Bytes(pdu.begin() + 2, pdu.end());
    return true;
}
Bytes PdcpLayer::compress_ip_header(const Bytes& ip_packet) {
    if (ip_packet.size() < 20) return ip_packet;
    if (!rohc_.established) {
        rohc_.established = true;
        Bytes out; out.push_back(0xFD);
        out.push_back((rohc_.last_sn >> 8) & 0xFF);
        out.push_back(rohc_.last_sn & 0xFF);
        out.insert(out.end(), ip_packet.begin(), ip_packet.end());
        LOG_DEBUG("PDCP", "ROHC: IR packet sent");
        return out;
    }
    rohc_.last_sn++;
    Bytes out; out.push_back(0x00); out.push_back(rohc_.last_sn & 0xFF);
    uint8_t crc = 0;
    for (size_t i = 0; i < std::min((size_t)12, ip_packet.size()); i++) crc ^= ip_packet[i];
    out.push_back(crc);
    size_t hdr_len = (ip_packet[0] & 0x0F) * 4;
    out.insert(out.end(), ip_packet.begin() + hdr_len, ip_packet.end());
    LOG_DEBUG("PDCP", "ROHC: compressed " + std::to_string(ip_packet.size()) + " -> " + std::to_string(out.size()) + " bytes");
    return out;
}
Bytes PdcpLayer::decompress_ip_header(const Bytes& compressed, bool full_hdr) {
    if (compressed.empty()) return {};
    if (full_hdr || compressed[0] == 0xFD) {
        if (compressed.size() < 3) return compressed;
        return Bytes(compressed.begin() + 3, compressed.end());
    }
    if (compressed.size() < 3) return {};
    return Bytes(compressed.begin() + 3, compressed.end());
}
uint32_t PdcpLayer::compute_integrity(const Bytes& msg, uint32_t count, uint32_t key) {
    uint32_t mac_i = count ^ key;
    for (size_t i = 0; i < msg.size(); i++) mac_i = (mac_i << 1) ^ (mac_i >> 31) ^ msg[i];
    return mac_i;
}
bool PdcpLayer::verify_integrity(const Bytes& msg, uint32_t count, uint32_t key, uint32_t expected_mac) {
    return compute_integrity(msg, count, key) == expected_mac;
}
Status PdcpLayer::transmit_sdu(const Bytes& sdu_in, Bytes& rlc_pdu) {
    Bytes payload = (type_ == PdcpBearerType::DRB) ? compress_ip_header(sdu_in) : sdu_in;
    PdcpHeader hdr; hdr.data_ctrl = true; hdr.sn = tx_sn_;
    rlc_pdu = build_pdcp_pdu(hdr, payload);
    LOG_INFO("PDCP", "TX PDCP-PDU SN=" + std::to_string(tx_sn_) + " size=" + std::to_string(rlc_pdu.size()));
    tx_sn_ = next_sn(tx_sn_);
    return Status::OK;
}
Status PdcpLayer::receive_pdu(const Bytes& rlc_pdu, Bytes& sdu_out) {
    PdcpHeader hdr; Bytes payload;
    if (!parse_pdcp_pdu(rlc_pdu, hdr, payload)) return Status::ERROR;
    LOG_INFO("PDCP", "RX PDCP-PDU SN=" + std::to_string(hdr.sn));
    sdu_out = (type_ == PdcpBearerType::DRB) ? decompress_ip_header(payload, payload[0]==0xFD) : payload;
    rx_sn_ = next_sn(rx_sn_);
    return Status::OK;
}
