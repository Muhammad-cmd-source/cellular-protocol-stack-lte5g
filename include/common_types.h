#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>

using Bytes = std::vector<uint8_t>;

struct PDU {
    Bytes   data;
    uint8_t layer_id;
    uint32_t timestamp;
    PDU() : layer_id(0), timestamp(0) {}
    PDU(Bytes d, uint8_t lid) : data(std::move(d)), layer_id(lid), timestamp(0) {}
};

enum class LayerID : uint8_t { PHY=0, MAC=1, RLC=2, PDCP=3, RRC=4, NAS=5, APP=6 };

enum class Status { OK, ERROR, RETRY, PENDING, BUFFER_FULL, INVALID_STATE };

inline const char* status_str(Status s) {
    switch(s) {
        case Status::OK:            return "OK";
        case Status::ERROR:         return "ERROR";
        case Status::RETRY:         return "RETRY";
        case Status::PENDING:       return "PENDING";
        case Status::BUFFER_FULL:   return "BUFFER_FULL";
        case Status::INVALID_STATE: return "INVALID_STATE";
    }
    return "UNKNOWN";
}

enum class LogLevel { DEBUG, INFO, WARN, ERR };

class Logger {
public:
    static Logger& instance() { static Logger inst; return inst; }
    void log(LogLevel lvl, const std::string& layer, const std::string& msg) {
        const char* lvl_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};
        std::cout << "[" << lvl_str[(int)lvl] << "][" 
                  << std::setw(5) << layer << "] " << msg << "\n";
    }
};

#define LOG_DEBUG(layer, msg) Logger::instance().log(LogLevel::DEBUG, layer, msg)
#define LOG_INFO(layer, msg)  Logger::instance().log(LogLevel::INFO,  layer, msg)
#define LOG_WARN(layer, msg)  Logger::instance().log(LogLevel::WARN,  layer, msg)
#define LOG_ERR(layer, msg)   Logger::instance().log(LogLevel::ERR,   layer, msg)

inline std::string hex_dump(const Bytes& b, size_t max_bytes = 32) {
    std::ostringstream ss;
    size_t n = std::min(b.size(), max_bytes);
    for (size_t i = 0; i < n; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b[i] << " ";
    if (b.size() > max_bytes) ss << "... (" << b.size() << " bytes)";
    return ss.str();
}
