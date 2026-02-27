#include "nas_layer.h"
#include <sstream>
#include <cstdlib>
NasLayer::NasLayer(UeIdentity ue_id) : ue_id_(ue_id) {}
std::string NasLayer::get_reg_state_str() const {
    switch(reg_state_) {
        case NasRegistrationState::DEREGISTERED:  return "5GMM-DEREGISTERED";
        case NasRegistrationState::REGISTERING:   return "5GMM-REGISTERING";
        case NasRegistrationState::REGISTERED:    return "5GMM-REGISTERED";
        case NasRegistrationState::DEREGISTERING: return "5GMM-DEREGISTERING";
    }
    return "UNKNOWN";
}
Bytes NasLayer::build_nas_msg(NasMsgType type, const Bytes& payload) {
    Bytes msg;
    msg.push_back(0x7E);
    msg.push_back(0x00);
    msg.push_back((uint8_t)type);
    msg.push_back(nas_seq_++);
    msg.insert(msg.end(), payload.begin(), payload.end());
    return msg;
}
bool NasLayer::parse_nas_msg(const Bytes& pdu, NasMsgType& type, Bytes& payload) {
    if (pdu.size() < 4) return false;
    type    = (NasMsgType)pdu[2];
    payload = Bytes(pdu.begin() + 4, pdu.end());
    return true;
}
bool NasLayer::authenticate(const Bytes& rand, const Bytes& autn, Bytes& res) {
    (void)autn;
    res.resize(8);
    for (int i = 0; i < 8; i++) res[i] = rand[i % rand.size()] ^ ue_id_.key[i];
    LOG_INFO("NAS", "AKA: computed RES");
    return true;
}
Status NasLayer::initiate_registration() {
    if (reg_state_ == NasRegistrationState::REGISTERED) return Status::INVALID_STATE;
    reg_state_ = NasRegistrationState::REGISTERING;
    LOG_INFO("NAS", "State -> " + get_reg_state_str());
    Bytes payload; payload.push_back(0x01);
    for (char c : ue_id_.imsi) payload.push_back((uint8_t)c);
    payload.push_back(0x00);
    Bytes req = build_nas_msg(NasMsgType::REGISTRATION_REQUEST, payload);
    LOG_INFO("NAS", "Sending RegistrationRequest IMSI=" + ue_id_.imsi);
    Bytes resp;
    return receive_message(req, resp);
}
Status NasLayer::receive_message(const Bytes& nas_pdu, Bytes& response) {
    NasMsgType type; Bytes payload;
    if (!parse_nas_msg(nas_pdu, type, payload)) return Status::ERROR;
    switch(type) {
        case NasMsgType::REGISTRATION_REQUEST: {
            Bytes rand_bytes(16, 0xAB), autn_bytes(16, 0x12), auth_req;
            auth_req.insert(auth_req.end(), rand_bytes.begin(), rand_bytes.end());
            auth_req.insert(auth_req.end(), autn_bytes.begin(), autn_bytes.end());
            response = build_nas_msg(NasMsgType::AUTH_REQUEST, auth_req);
            LOG_INFO("NAS", "-> AuthenticationRequest sent");
            Bytes auth_resp;
            receive_message(response, auth_resp);
            break;
        }
        case NasMsgType::AUTH_REQUEST: {
            Bytes rand_bytes(payload.begin(), payload.begin()+16);
            Bytes autn_bytes(payload.begin()+16, payload.begin()+32);
            Bytes res;
            authenticate(rand_bytes, autn_bytes, res);
            response = build_nas_msg(NasMsgType::AUTH_RESPONSE, res);
            LOG_INFO("NAS", "-> AuthenticationResponse sent");
            Bytes sec_resp;
            Bytes sec_cmd = build_nas_msg(NasMsgType::SECURITY_MODE_CMD, {0x01,0x01});
            receive_message(sec_cmd, sec_resp);
            break;
        }
        case NasMsgType::SECURITY_MODE_CMD:
            response = build_nas_msg(NasMsgType::SECURITY_MODE_COMPLETE, {});
            LOG_INFO("NAS", "-> SecurityModeComplete sent");
            {
                Bytes reg_accept = build_nas_msg(NasMsgType::REGISTRATION_ACCEPT, {0x00,0x01});
                Bytes rac_resp;
                receive_message(reg_accept, rac_resp);
            }
            break;
        case NasMsgType::REGISTRATION_ACCEPT:
            reg_state_ = NasRegistrationState::REGISTERED;
            ue_id_.tmsi = 0x12345678;
            LOG_INFO("NAS", "REGISTERED! TMSI=0x12345678");
            LOG_INFO("NAS", "State -> " + get_reg_state_str());
            response = build_nas_msg(NasMsgType::REGISTRATION_COMPLETE, {});
            break;
        case NasMsgType::PDU_SESSION_ESTAB_REQ:
            session_.ip_address = "10.45.0." + std::to_string(1+(std::rand()%254));
            session_.state = NasSessionState::ACTIVE;
            response = build_nas_msg(NasMsgType::PDU_SESSION_ESTAB_ACC, {session_.pdu_session_id});
            LOG_INFO("NAS", "PDU Session established IP=" + session_.ip_address);
            break;
        case NasMsgType::PDU_SESSION_ESTAB_ACC:
            session_.state = NasSessionState::ACTIVE;
            break;
        case NasMsgType::DEREGISTRATION_REQUEST:
            reg_state_ = NasRegistrationState::DEREGISTERED;
            LOG_INFO("NAS", "Deregistered from network");
            break;
        default:
            break;
    }
    return Status::OK;
}
Status NasLayer::request_pdu_session(const std::string& apn) {
    if (reg_state_ != NasRegistrationState::REGISTERED) return Status::INVALID_STATE;
    session_.apn = apn;
    session_.state = NasSessionState::ACTIVATING;
    Bytes payload; payload.push_back(session_.pdu_session_id);
    for (char c : apn) payload.push_back((uint8_t)c);
    Bytes req = build_nas_msg(NasMsgType::PDU_SESSION_ESTAB_REQ, payload);
    Bytes resp;
    LOG_INFO("NAS", "Requesting PDU Session APN=" + apn);
    return receive_message(req, resp);
}
Status NasLayer::initiate_deregistration() {
    if (reg_state_ != NasRegistrationState::REGISTERED) return Status::INVALID_STATE;
    reg_state_ = NasRegistrationState::DEREGISTERING;
    Bytes req = build_nas_msg(NasMsgType::DEREGISTRATION_REQUEST, {0x01});
    Bytes resp;
    LOG_INFO("NAS", "Sending DeregistrationRequest");
    return receive_message(req, resp);
}
