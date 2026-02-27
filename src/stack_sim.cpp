#include "common_types.h"
#include "phy_layer.h"
#include "mac_layer.h"
#include "rlc_layer.h"
#include "pdcp_layer.h"
#include "rrc_layer.h"
#include "nas_layer.h"
#include <iostream>
#include <cassert>

Bytes make_ip_packet(const std::string& payload_str) {
    Bytes pkt;
    pkt.push_back(0x45); pkt.push_back(0x00);
    uint16_t total_len = 20 + (uint16_t)payload_str.size();
    pkt.push_back((total_len >> 8) & 0xFF);
    pkt.push_back(total_len & 0xFF);
    pkt.push_back(0x00); pkt.push_back(0x01);
    pkt.push_back(0x40); pkt.push_back(0x00);
    pkt.push_back(0x40); pkt.push_back(0x11);
    pkt.push_back(0x00); pkt.push_back(0x00);
    pkt.push_back(0x0A); pkt.push_back(0x2D); pkt.push_back(0x00); pkt.push_back(0x01);
    pkt.push_back(0x08); pkt.push_back(0x08); pkt.push_back(0x08); pkt.push_back(0x08);
    for (char c : payload_str) pkt.push_back((uint8_t)c);
    return pkt;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║   Cellular Protocol Stack Simulation (LTE/5G NR) ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "━━━━━━━━━━ PHASE 1: NAS REGISTRATION ━━━━━━━━━━\n";
    UeIdentity ue_id;
    ue_id.imsi = "310260987654321";
    NasLayer nas(ue_id);
    nas.initiate_registration();
    std::cout << "NAS State: " << nas.get_reg_state_str() << "\n\n";

    std::cout << "━━━━━━━━━━ PHASE 2: PDU SESSION ━━━━━━━━━━\n";
    nas.request_pdu_session("internet");
    std::cout << "IP Address: " << nas.get_ip_address() << "\n\n";

    std::cout << "━━━━━━━━━━ PHASE 3: RRC CONNECTION ━━━━━━━━━━\n";
    RrcLayer rrc;
    rrc.initiate_connection();
    std::cout << "RRC State: " << rrc.get_state_str() << "\n\n";

    std::cout << "━━━━━━━━━━ PHASE 4: USER PLANE ━━━━━━━━━━\n";
    PhyConfig phy_cfg;
    phy_cfg.mcs            = MCS::QAM64_2_3;
    phy_cfg.channel_snr_db = 20.0f;
    phy_cfg.num_prbs       = 52;
    PhyLayer  phy(phy_cfg);
    MacLayer  mac;
    RlcLayer  rlc(RlcMode::AM);
    PdcpLayer pdcp(PdcpBearerType::DRB);
    std::cout << "Estimated throughput: " << phy.estimate_throughput_mbps() << " Mbps\n\n";

    std::cout << "━━━━━━━━━━ PHASE 5: DATA TRANSFER ━━━━━━━━━━\n";
    const char* messages[] = {
        "Hello 5G network!",
        "HTTP GET /index.html",
        "DNS query google.com"
    };
    for (auto msg : messages) {
        Bytes ip_pkt = make_ip_packet(msg);
        Bytes pdcp_pdu, rlc_pdu, mac_pdu, tb;
        pdcp.transmit_sdu(ip_pkt, pdcp_pdu);
        rlc.transmit_sdu(pdcp_pdu, rlc_pdu);
        mac.transmit_sdu(rlc_pdu, mac_pdu);
        phy.transmit_transport_block(mac_pdu, tb);
        mac.harq_feedback(0, true);
        std::cout << "Sent: " << msg << "\n";
    }

    std::cout << "\n━━━━━━━━━━ PHASE 6: RRC SUSPEND/RESUME ━━━━━━━━━━\n";
    rrc.suspend_connection();
    std::cout << "RRC State: " << rrc.get_state_str() << "\n";
    rrc.resume_connection();
    std::cout << "RRC State: " << rrc.get_state_str() << "\n";

    std::cout << "\n━━━━━━━━━━ STATS ━━━━━━━━━━\n";
    std::cout << "MAC TX PDUs:   " << mac.get_tx_pdus()   << "\n";
    std::cout << "MAC HARQ Retx: " << mac.get_harq_retx() << "\n";
    std::cout << "RLC TX SN:     " << rlc.get_tx_sn()     << "\n";
    std::cout << "PDCP TX SN:    " << pdcp.get_tx_sn()    << "\n";

    std::cout << "\n━━━━━━━━━━ PHASE 7: TEARDOWN ━━━━━━━━━━\n";
    rrc.release_connection();
    nas.initiate_deregistration();
    std::cout << "\nSimulation complete!\n";
    return 0;
}
