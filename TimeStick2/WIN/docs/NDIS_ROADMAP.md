# TimeStick NDIS Miniport Driver - Implementation Roadmap

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│           Windows TCP/IP Stack                  │
├─────────────────────────────────────────────────┤
│              NDIS 6.x Layer                     │
├─────────────────────────────────────────────────┤
│         TimeStick Miniport Driver               │
│  ┌───────────────────────────────────────────┐  │
│  │  NDIS Miniport Handlers                   │  │
│  │  - Initialize/Halt                        │  │
│  │  - SendNetBufferLists                     │  │
│  │  - ReturnNetBufferLists                   │  │
│  │  - CancelSend                             │  │
│  │  - DevicePnPEventNotify                   │  │
│  │  - Shutdown/Reset                         │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  USB Communication Layer                  │  │
│  │  - Bulk IN/OUT pipes                      │  │
│  │  - Control transfers                      │  │
│  │  - Continuous receive                     │  │
│  └───────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────┐  │
│  │  ASIX Hardware Layer                      │  │
│  │  - Register access                        │  │
│  │  - MAC/PHY control                        │  │
│  │  - PTP timestamping                       │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
                       │
                       ▼
            ┌──────────────────┐
            │  ASIX AX88279    │
            │  USB 3.0 Ethernet│
            └──────────────────┘
```

## Implementation Phases

### Phase 1: Core NDIS Structure ✅ (Now)
**Goal**: Driver loads, binds to NDIS, device detected

**Files to create**:
- `miniport.c` - NDIS miniport handlers
- `miniport.h` - Miniport structures and definitions
- `asix_hw.h` - Hardware register definitions (from Linux)
- `usb.c` - USB communication layer
- `usb.h` - USB definitions
- `timestick.inf` - INF file for installation

**Key functionality**:
- DriverEntry and NDIS registration
- MiniportInitializeEx - device initialization
- MiniportHaltEx - cleanup
- Basic USB device setup
- Read MAC address from hardware

**Success criteria**: Device appears in Device Manager, no network packets yet

---

### Phase 2: Packet Transmission ✅
**Goal**: Can send Ethernet packets

**New functionality**:
- MiniportSendNetBufferLists - send handler
- USB bulk OUT for TX
- TX descriptor management
- Packet formatting for ASIX

**Key challenges**:
- ASIX TX header format (8 bytes)
- USB async sends
- Buffer management

**Success criteria**: `ping` can send packets (won't receive replies yet)

---

### Phase 3: Packet Reception ✅
**Goal**: Can receive Ethernet packets, network works

**New functionality**:
- Continuous USB bulk IN reads
- RX descriptor management
- Packet parsing (ASIX RX format)
- MiniportReturnNetBufferLists
- Indicate received packets to NDIS

**Key challenges**:
- ASIX RX header format (variable length)
- Multiple packets per USB transfer
- Async RX pipeline

**Success criteria**: Full network connectivity - `ping`, web browsing work

---

### Phase 4: Link Management & PHY ✅
**Goal**: Proper link up/down, speed negotiation

**New functionality**:
- PHY (Gigabit Ethernet) control
- Link status monitoring
- Speed/duplex configuration
- Media connect/disconnect events
- Auto-negotiation

**Success criteria**: Link LED works, proper speed reporting (10/100/1000)

---

### Phase 5: PTP Hardware Timestamping 🎯
**Goal**: Hardware timestamps for PTP packets

**New functionality**:
- PTP register access
- TX timestamp capture
- RX timestamp extraction
- OID handlers for PTP configuration
- Timestamp indication to NDIS

**Key challenges**:
- Windows PTP kernel support (NetAdapterCx or custom)
- Timestamp format conversion
- Sync with IEEE 1588 stack

**Success criteria**: PTP daemon can use hardware timestamps

---

### Phase 6: Advanced Features
**Goal**: Full feature parity

- Wake-on-LAN
- Power management (selective suspend)
- Checksum offload
- Jumbo frames
- Statistics/counters
- VLAN support (if needed)

---

## Critical Components to Port from Linux

### From `ax_main.h`:
```c
// USB Vendor/Product IDs
#define USB_VENDOR_ID_ASIX      0x0B95
#define AX_DEVICE_ID_279        0x1790  // with bcdDevice 0x0400

// Register addresses
#define AX_ACCESS_MAC           0x01
#define AX_ACCESS_PHY           0x02
#define AX_NODE_ID              0x10  // MAC address (6 bytes)
#define AX_MEDIUM_STATUS_MODE   0x22  // Speed/duplex control
#define AX_RX_CTL               0x0b  // RX control
#define AX_MONITOR_MODE         0x24

// Medium status bits
#define AX_MEDIUM_GIGAMODE      0x0001
#define AX_MEDIUM_FULL_DUPLEX   0x0002
#define AX_MEDIUM_RECEIVE_EN    0x0100
#define AX_MEDIUM_TXFLOW_CTRLEN 0x0020
#define AX_MEDIUM_RXFLOW_CTRLEN 0x0010

// RX control bits
#define AX_RX_CTL_START         0x0080
#define AX_RX_CTL_AP            0x0020  // Accept physical address
#define AX_RX_CTL_AM            0x0010  // Accept multicast
#define AX_RX_CTL_AB            0x0008  // Accept broadcast
#define AX_RX_CTL_PRO           0x0001  // Promiscuous
```

### From `ax88179_178a.c`:
- `ax88179_bind()` - initialization sequence
- `ax88179_rx_fixup()` - RX packet parsing
- `ax88179_tx_fixup()` - TX packet formatting
- PHY initialization sequence

### TX/RX Packet Format:
```
TX format (to device):
[8-byte header][Ethernet frame][padding to 4-byte boundary]

Header format:
  u32 pkt_len;     // Bits 0-15: packet length, Bit 31: auto-padding
  u32 tx_options;  // TX options/checksum

RX format (from device):  
[4-byte header][Ethernet frame][4-byte header][Ethernet frame]...

Header format:
  u16 pkt_len;     // Packet length
  u16 flags;       // Status flags
```

---

## File Structure

```
WIN/ndis/
├── miniport.c          # Main NDIS miniport handlers
├── miniport.h          # Miniport data structures
├── asix_hw.h           # ASIX register definitions
├── asix_hw.c           # Hardware abstraction layer
├── usb.c               # USB communication layer
├── usb.h               # USB definitions
├── packet.c            # TX/RX packet handling
├── packet.h            # Packet structures
├── phy.c               # PHY control (link/speed)
├── phy.h               # PHY definitions
├── ptp.c               # PTP timestamping (Phase 5)
├── ptp.h               # PTP definitions
├── timestick.inf       # Driver installation file
├── timestick.rc        # Resource file
└── sources             # Build configuration (DDK/WDK)
```

---

## Development Environment Requirements

### Required Tools:
1. **Visual Studio 2019 or 2022**
   - Desktop development with C++
   - Windows SDK

2. **Windows Driver Kit (WDK) 10**
   - Download from Microsoft
   - Must match Windows SDK version

3. **Windows 11 SDK** (latest)

4. **Test Environment**:
   - Windows 10/11 VM with test signing enabled
   - Physical test machine for final validation
   - USB 3.0 port for device

### Build Configuration:
- Target: Windows 10 version 2004 or later
- Architecture: x64 (ARM64 optional later)
- NDIS version: 6.83 (Windows 10 2004+)

---

## Testing Strategy

### Phase 1 Tests:
- Device Manager shows adapter
- Driver loads without errors
- MAC address reads correctly
- Device doesn't crash on load/unload

### Phase 2 Tests:
- Can send ARP packets (use Wireshark on another port)
- `arp -d *` followed by `ping` shows ARP going out
- TX completion works without leaks

### Phase 3 Tests:
- `ping` works (both directions)
- TCP connections work (browser, SSH)
- Large file transfers stable
- No memory leaks over time

### Phase 4 Tests:
- Link up/down detection works
- Speed changes work (force 100Mbps, etc)
- Auto-negotiation succeeds
- Works with different switches/cables

### Phase 5 Tests:
- PTP daemon can query timestamps
- Hardware timestamps match software (within spec)
- Sub-microsecond precision achieved
- Continuous operation stable

---

## Known Challenges

### 1. NDIS Complexity
**Challenge**: NDIS has steep learning curve
**Mitigation**: Use Microsoft NDIS samples as reference (netvmini, usb8023)

### 2. USB Async Model
**Challenge**: NDIS expects sync API, USB is async
**Mitigation**: Queue management, completion routines, careful locking

### 3. Packet Format
**Challenge**: ASIX uses custom headers
**Mitigation**: Port Linux parsing code exactly, add validation

### 4. PTP in Windows
**Challenge**: Limited/no kernel PTP infrastructure
**Mitigation**: May need custom solution or NetAdapterCx framework

### 5. Driver Signing
**Challenge**: Modern Windows requires signed drivers
**Mitigation**: Test signing during development, proper signing for release

---

## Development Timeline Estimate

**Phase 1**: 3-5 days (core structure)
**Phase 2**: 2-3 days (TX path)
**Phase 3**: 3-5 days (RX path + debugging)
**Phase 4**: 2-3 days (PHY/link)
**Phase 5**: 5-7 days (PTP - most complex)
**Phase 6**: 3-5 days (polish)

**Total**: ~3-4 weeks for experienced Windows driver dev
**Total**: ~5-8 weeks if learning NDIS

---

## Next Steps

1. **Set up build environment** - Install VS2022 + WDK
2. **Create Phase 1 skeleton** - I'll generate the files
3. **Port register definitions** - From Linux `ax_main.h`
4. **Implement basic init** - Get driver loading
5. **Add USB layer** - Control transfers, bulk pipes
6. **Iterate through phases** - Build incrementally

**Ready to start?** I'll begin creating the Phase 1 files.
