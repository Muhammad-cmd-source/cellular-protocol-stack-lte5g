#pragma once
#include "common_types.h"
#include <string>
enum class NasRegistrationState { DEREGISTERED, REGISTERING, REGISTERED, DEREGISTERING };
enum class NasSessionState { INACTIVE, ACTIVATING, ACTIVE };
enum class NasMsgType : uint8_t {
    REGISTRATION_REQUEST=0x41, REGISTRATION_ACCEPT=0x42,
    REGISTRATION_COMPLETE=0x43, REGISTRATION_REJECT=0x44,
    DEREGISTRATION_REQUEST=0x45, AUTH_REQUEST=0x56, AUTH_RESPONSE=0x57,
    SECURITY_MODE_CMD=0x5D, SECURITY_MODE_COMPLETE=0x5E,
    PDU_SESSION_ESTAB_REQ=0xC1, PDU_SESSION_ESTAB_ACC=0xC2,
    PDU_SESSION_RELEASE_CMD=0xD4,
};
struct UeIdentity {
    std::string imsi = "310260123456789";
    std::string supi = "imsi-310260123456789";
    uint32_t    tmsi = 0;
    uint8_t     key[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
                           0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
};
struct PduSession {
    uint8_t     pdu_session_id = 1;
    std::string apn            = "internet";
    std::string ip_address     = "10.0.0.1";
    NasSessionState state      = NasSessionState::INACTIVE;
};
class NasLayer {
public:
    explicit NasLayer(UeIdentity ue_id = {});
    Status initiate_registration();
    Status receive_message(const Bytes& nas_pdu, Bytes& response);
    Status request_pdu_session(const std::string& apn = "internet");
    Status initiate_deregistration();
    NasRegistrationState get_reg_state()     const { return reg_state_; }
    NasSessionState      get_session_state() const { return session_.state; }
    std::string          get_ip_address()    const { return session_.ip_address; }
    std::string          get_reg_state_str() const;
private:
    UeIdentity           ue_id_;
    NasRegistrationState reg_state_ = NasRegistrationState::DEREGISTERED;
    PduSession           session_;
    uint8_t              nas_seq_   = 0;
    Bytes build_nas_msg(NasMsgType type, const Bytes& payload = {});
    bool  parse_nas_msg(const Bytes& pdu, NasMsgType& type, Bytes& payload);
    bool  authenticate(const Bytes& rand, const Bytes& autn, Bytes& res);
};
