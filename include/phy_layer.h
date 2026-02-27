#pragma once
#include "common_types.h"

enum class MCS : uint8_t {
    QPSK_1_3  = 0,
    QPSK_1_2  = 5,
    QAM16_1_2 = 10,
    QAM64_2_3 = 20,
    QAM64_5_6 = 28,
};

struct PhyConfig {
    MCS     mcs             = MCS::QAM16_1_2;
    uint8_t num_prbs        = 25;
    float   channel_snr_db  = 15.0f;
    bool    harq_enabled    = true;
};

class PhyLayer {
public:
    explicit PhyLayer(PhyConfig cfg = {});
    Status receive_transport_block(const Bytes& tb_in, Bytes& tb_out);
    Status transmit_transport_block(const Bytes& tb_in, Bytes& tb_out);
    void set_snr(float snr_db) { cfg_.channel_snr_db = snr_db; }
    float get_snr() const { return cfg_.channel_snr_db; }
    float estimate_throughput_mbps() const;
private:
    PhyConfig cfg_;
    uint32_t  rx_errors_ = 0;
    uint32_t  rx_total_  = 0;
    bool simulate_crc_pass() const;
    Bytes apply_noise(const Bytes& data) const;
};
