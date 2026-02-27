#pragma once
#include "common_types.h"
#include <deque>
#include <map>
enum class RlcMode { TM, UM, AM };
static constexpr uint16_t RLC_AM_WINDOW_SIZE = 512;
static constexpr uint8_t  RLC_MAX_RETX       = 4;
struct RlcAmHeader {
    bool     data_ctrl;
    bool     poll_bit;
    uint8_t  seg_info;
    uint16_t sn;
};
struct RlcTxBuffer {
    Bytes    sdu;
    uint16_t sn;
    uint8_t  retx_count = 0;
    bool     acked      = false;
};
struct RlcRxBuffer {
    Bytes    payload;
    uint16_t sn;
    bool     received = false;
};
class RlcLayer {
public:
    explicit RlcLayer(RlcMode mode = RlcMode::AM);
    Status receive_pdu(const Bytes& mac_pdu, Bytes& pdcp_sdu);
    Status transmit_sdu(const Bytes& pdcp_sdu, Bytes& mac_pdu);
    void process_status_pdu(uint16_t ack_sn, const std::vector<uint16_t>& nack_sns);
    Status retransmit_nacked(Bytes& mac_pdu);
    uint16_t get_tx_sn() const { return tx_sn_; }
    uint16_t get_rx_sn() const { return rx_sn_; }
    RlcMode  get_mode()  const { return mode_; }
private:
    RlcMode  mode_;
    uint16_t tx_sn_ = 0;
    uint16_t rx_sn_ = 0;
    std::deque<RlcTxBuffer>         tx_window_;
    std::map<uint16_t, RlcRxBuffer> rx_window_;
    std::deque<uint16_t>            nack_list_;
    Bytes    build_am_pdu(const RlcAmHeader& hdr, const Bytes& payload);
    bool     parse_am_pdu(const Bytes& pdu, RlcAmHeader& hdr, Bytes& payload);
    Bytes    build_status_pdu(uint16_t ack_sn);
    uint16_t next_sn(uint16_t sn) { return (sn + 1) & 0x0FFF; }
};
