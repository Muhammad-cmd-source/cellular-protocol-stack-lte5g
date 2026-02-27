# Cellular Protocol Stack — LTE/5G NR

A full implementation of a cellular protocol stack simulating LTE and 5G NR protocol layers in C/C++ for embedded Linux systems, based on 3GPP specifications.

## Architecture
```
+---------------------------+
|   NAS - Non-Access Stratum|  Registration, Authentication
+---------------------------+
|   RRC - Radio Resource    |  Connection setup, State machine
+---------------------------+
|   PDCP - Packet Data Conv.|  Header compression (ROHC), Integrity
+---------------------------+
|   RLC - Radio Link Control|  Segmentation, ARQ retransmission
+---------------------------+
|   MAC - Medium Access Ctrl|  Scheduling, HARQ
+---------------------------+
|   PHY - Physical Layer    |  Modulation, MCS, SNR simulation
+---------------------------+
```

## Features
- LTE and 5G NR protocol layer simulation in C++17
- RLC Acknowledged Mode (AM) with ARQ retransmission
- HARQ (Hybrid ARQ) at MAC layer with 8 processes
- RRC State Machine: IDLE → CONNECTED → INACTIVE → CONNECTED
- NAS 5GMM State Machine with AKA Authentication
- PDCP header compression (ROHC IR and UO-0 packets)
- Python log analyzer for debugging protocol flows
- GDB pretty-printers for all protocol layer types
- 12/12 unit tests passing

## Build and Run

### Prerequisites
```bash
sudo apt-get install build-essential g++ python3
```

### Build
```bash
make all
```

### Run Simulation
```bash
./bin/stack_sim
```

### Run Tests
```bash
make test
```

### Analyze Logs with Python
```bash
./bin/stack_sim 2>&1 | python3 scripts/log_analyzer.py
```

## Project Structure
```
cellular-protocol-stack/
├── include/        # Header files for all layers
├── src/
│   ├── phy/        # Physical layer
│   ├── mac/        # MAC layer with HARQ
│   ├── rlc/        # RLC layer with ARQ
│   ├── pdcp/       # PDCP with header compression
│   ├── rrc/        # RRC state machine
│   └── nas/        # NAS registration and authentication
├── tests/          # Unit tests
└── scripts/        # Python debugging tools
```

## Sample Output
```
PHASE 1: NAS REGISTRATION
[INFO][NAS] State -> 5GMM-REGISTERING
[INFO][NAS] AKA: computed RES
[INFO][NAS] REGISTERED! TMSI=0x12345678

PHASE 2: PDU SESSION
[INFO][NAS] PDU Session established IP=10.45.0.130

PHASE 3: RRC CONNECTION
[INFO][RRC] State -> RRC_CONNECTED

PHASE 5: DATA TRANSFER
[DEBUG][PDCP] ROHC: compressed 40 -> 23 bytes
Sent: Hello 5G network!
```

## References
- 3GPP TS 38.300 — 5G NR Overall Description
- 3GPP TS 38.322 — RLC Protocol
- 3GPP TS 38.323 — PDCP Protocol
- 3GPP TS 38.331 — RRC Protocol
- 3GPP TS 24.501 — 5G NAS Protocol
