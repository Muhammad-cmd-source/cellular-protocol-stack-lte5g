#pragma once
#include "common_types.h"
#include <map>
enum class PdcpBearerType { SRB, DRB };
struct RohcContext {
    uint32_t src_ip   = 0;
    uint32_t dst_ip   = 0;
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint32_t last_sn  = 0;
    bool     established = false;
};
struct PdcpHeader {
    bool     data_ctrl;
    uint16_t sn;
};
class PdcpLayer {
public:
    explicit PdcpLayer(PdcpBearerType type = PdcpBearerType::DRB);
    Status receive_pdu(const Bytes& rlc_pdu, Bytes& sdu_out);
    Status transmit_sdu(const Bytes& sdu_in, Bytes& rlc_pdu);
    uint32_t compute_integrity(const Bytes& msg, uint32_t count, uint32_t key);
    bool     verify_integrity(const Bytes& msg, uint32_t count, uint32_t key, uint32_t expected_mac);
    uint16_t get_tx_sn() const { return tx_sn_; }
private:
    PdcpBearerType type_;
    uint16_t       tx_sn_ = 0;
    uint16_t       rx_sn_ = 0;
    RohcContext    rohc_;
    Bytes compress_ip_header(const Bytes& ip_packet);
    Bytes decompress_ip_header(const Bytes& compressed, bool full_header);
    Bytes    build_pdcp_pdu(const PdcpHeader& hdr, const Bytes& payload);
    bool     parse_pdcp_pdu(const Bytes& pdu, PdcpHeader& hdr, Bytes& payload);
    uint16_t next_sn(uint16_t sn) { return (sn + 1) & 0x0FFF; }
};
