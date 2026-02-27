#include "phy_layer.h"
#include <cmath>
#include <cstdlib>
#include <sstream>
PhyLayer::PhyLayer(PhyConfig cfg) : cfg_(cfg) {}
bool PhyLayer::simulate_crc_pass() const {
    float snr = cfg_.channel_snr_db;
    float ber = 0.5f / (1.0f + std::exp((snr - 10.0f) * 0.5f));
    float rand_val = (float)std::rand() / RAND_MAX;
    return rand_val > ber * 100;
}
Bytes PhyLayer::apply_noise(const Bytes& data) const {
    Bytes out = data;
    if (cfg_.channel_snr_db < 5.0f)
        for (size_t i = 0; i < out.size(); i += 10) out[i] ^= 0x01;
    return out;
}
Status PhyLayer::receive_transport_block(const Bytes& tb_in, Bytes& tb_out) {
    rx_total_++;
    tb_out = apply_noise(tb_in);
    if (!simulate_crc_pass()) {
        rx_errors_++;
        std::ostringstream ss;
        ss << "CRC FAIL SNR=" << cfg_.channel_snr_db << " dB";
        LOG_WARN("PHY", ss.str());
        return Status::RETRY;
    }
    LOG_DEBUG("PHY", "RX TB ok " + std::to_string(tb_in.size()) + " bytes");
    return Status::OK;
}
Status PhyLayer::transmit_transport_block(const Bytes& tb_in, Bytes& tb_out) {
    tb_out = tb_in;
    LOG_DEBUG("PHY", "TX TB " + std::to_string(tb_in.size()) + " bytes");
    return Status::OK;
}
float PhyLayer::estimate_throughput_mbps() const {
    float bits_per_sym = 2.0f;
    switch (cfg_.mcs) {
        case MCS::QAM16_1_2: bits_per_sym = 4.0f * 0.5f; break;
        case MCS::QAM64_2_3: bits_per_sym = 6.0f * 0.67f; break;
        case MCS::QAM64_5_6: bits_per_sym = 6.0f * 0.83f; break;
        default: bits_per_sym = 2.0f * 0.33f; break;
    }
    float sym_per_sec = cfg_.num_prbs * 12.0f * 14.0f * 1000.0f;
    return (bits_per_sym * sym_per_sec) / 1e6f;
}
