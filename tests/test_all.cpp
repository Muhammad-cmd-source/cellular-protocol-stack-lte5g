#include "common_types.h"
#include "phy_layer.h"
#include "mac_layer.h"
#include "rlc_layer.h"
#include "pdcp_layer.h"
#include "rrc_layer.h"
#include "nas_layer.h"
#include <cassert>
#include <iostream>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN(name) do { \
    tests_run++; \
    std::cout << "  TEST: " #name " ... "; \
    try { test_##name(); std::cout << "PASS\n"; tests_passed++; } \
    catch(...) { std::cout << "FAIL\n"; } \
} while(0)

void test_phy_throughput() {
    PhyConfig cfg; cfg.mcs = MCS::QAM64_5_6; cfg.num_prbs = 100;
    PhyLayer phy(cfg);
    assert(phy.estimate_throughput_mbps() > 50.0f);
}
void test_mac_roundtrip() {
    MacLayer mac;
    Bytes sdu = {0x11,0x22,0x33,0x44}, pdu, recovered;
    assert(mac.transmit_sdu(sdu, pdu) == Status::OK);
    assert(mac.receive_pdu(pdu, recovered) == Status::OK);
    assert(recovered == sdu);
}
void test_mac_harq() {
    MacLayer mac;
    Bytes sdu = {0xAA,0xBB}, pdu;
    mac.transmit_sdu(sdu, pdu);
    mac.harq_feedback(0, false);
    for (int i = 0; i < 8; i++) { Bytes tmp; mac.transmit_sdu(sdu, tmp); }
    assert(mac.get_harq_retx() >= 1);
}
void test_rlc_am() {
    RlcLayer tx(RlcMode::AM), rx(RlcMode::AM);
    Bytes sdu = {0x01,0x02,0x03}, pdu, recovered;
    tx.transmit_sdu(sdu, pdu);
    assert(rx.receive_pdu(pdu, recovered) == Status::OK);
    assert(recovered == sdu);
}
void test_rlc_tm() {
    RlcLayer rlc(RlcMode::TM);
    Bytes sdu = {0xDE,0xAD,0xBE,0xEF}, pdu, recovered;
    rlc.transmit_sdu(sdu, pdu);
    assert(pdu == sdu);
    rlc.receive_pdu(pdu, recovered);
    assert(recovered == sdu);
}
void test_pdcp_roundtrip() {
    PdcpLayer tx(PdcpBearerType::SRB), rx(PdcpBearerType::SRB);
    Bytes msg = {0x01,0x02,0x03,0x04,0x05}, pdu, recovered;
    tx.transmit_sdu(msg, pdu);
    rx.receive_pdu(pdu, recovered);
    assert(recovered == msg);
}
void test_pdcp_integrity() {
    PdcpLayer pdcp(PdcpBearerType::SRB);
    Bytes msg = {0x01,0x02,0x03};
    uint32_t mac_i = pdcp.compute_integrity(msg, 0, 0xDEADBEEF);
    assert(pdcp.verify_integrity(msg, 0, 0xDEADBEEF, mac_i) == true);
    assert(pdcp.verify_integrity(msg, 0, 0x12345678, mac_i) == false);
}
void test_rrc_connection() {
    RrcLayer rrc;
    assert(rrc.get_state() == RrcState::IDLE);
    rrc.initiate_connection();
    assert(rrc.get_state() == RrcState::CONNECTED);
    rrc.release_connection();
    assert(rrc.get_state() == RrcState::IDLE);
}
void test_rrc_inactive() {
    RrcLayer rrc;
    rrc.initiate_connection();
    rrc.suspend_connection();
    assert(rrc.get_state() == RrcState::INACTIVE);
    rrc.resume_connection();
    assert(rrc.get_state() == RrcState::CONNECTED);
}
void test_nas_registration() {
    NasLayer nas;
    assert(nas.get_reg_state() == NasRegistrationState::DEREGISTERED);
    nas.initiate_registration();
    assert(nas.get_reg_state() == NasRegistrationState::REGISTERED);
}
void test_nas_pdu_session() {
    NasLayer nas;
    nas.initiate_registration();
    nas.request_pdu_session("internet");
    assert(nas.get_session_state() == NasSessionState::ACTIVE);
}
void test_nas_deregistration() {
    NasLayer nas;
    nas.initiate_registration();
    nas.initiate_deregistration();
    assert(nas.get_reg_state() == NasRegistrationState::DEREGISTERED);
}

int main() {
    std::cout << "╔══════════════════════════╗\n";
    std::cout << "║  Protocol Stack Tests     ║\n";
    std::cout << "╚══════════════════════════╝\n\n";
    std::cout << "[ PHY ]\n";  RUN(phy_throughput);
    std::cout << "[ MAC ]\n";  RUN(mac_roundtrip); RUN(mac_harq);
    std::cout << "[ RLC ]\n";  RUN(rlc_am); RUN(rlc_tm);
    std::cout << "[ PDCP ]\n"; RUN(pdcp_roundtrip); RUN(pdcp_integrity);
    std::cout << "[ RRC ]\n";  RUN(rrc_connection); RUN(rrc_inactive);
    std::cout << "[ NAS ]\n";  RUN(nas_registration); RUN(nas_pdu_session); RUN(nas_deregistration);
    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
