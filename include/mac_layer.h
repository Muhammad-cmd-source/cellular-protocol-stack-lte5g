#pragma once
#include "common_types.h"
#include "phy_layer.h"
#include <queue>
#include <array>
static constexpr int MAX_HARQ_PROCESSES = 8;
static constexpr int MAX_HARQ_RETX      = 4;
enum class HarqState { IDLE, WAITING_ACK, NACKED };
struct HarqProcess {
    uint8_t id;
    HarqState state = HarqState::IDLE;
    uint8_t retx_count = 0;
    Bytes buffer;
    bool ack_received = false;
};
enum class LogicalChannel : uint8_t { CCCH=0, DCCH=1, DTCH=2 };
class MacLayer {
public:
    MacLayer();
    Status receive_pdu(const Bytes& phy_pdu, Bytes& rlc_sdu);
    Status transmit_sdu(const Bytes& rlc_sdu, Bytes& phy_pdu);
    void harq_feedback(uint8_t process_id, bool ack);
    uint8_t get_next_harq_process();
    uint32_t get_tx_pdus()   const { return tx_pdus_; }
    uint32_t get_rx_pdus()   const { return rx_pdus_; }
    uint32_t get_harq_retx() const { return harq_retx_count_; }
private:
    std::array<HarqProcess, MAX_HARQ_PROCESSES> harq_procs_;
    uint8_t  next_harq_id_    = 0;
    uint32_t tx_pdus_         = 0;
    uint32_t rx_pdus_         = 0;
    uint32_t harq_retx_count_ = 0;
    Bytes build_mac_pdu(LogicalChannel lc, const Bytes& payload);
    bool  parse_mac_pdu(const Bytes& pdu, LogicalChannel& lc, Bytes& payload);
};
