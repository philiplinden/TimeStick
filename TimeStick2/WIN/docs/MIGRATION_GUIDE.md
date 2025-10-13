# Migration Guide: Old KMDF → New NDIS Driver

## Summary

**Old approach (broken)**: Generic KMDF USB driver with IOCTLs
**New approach (correct)**: NDIS miniport USB driver

Don't try to fix the old code - use the new NDIS structure instead.

## File Mapping

### Keep & Port:
| Old File | New Location | Status | Notes |
|----------|--------------|--------|-------|
| `asix_hw.h` | Use new version | ✅ Replace | New version has complete registers |
| `build.ps1` | Keep as-is | ✅ Keep | Still useful for building |
| `firmware.c` | Save for Phase 6 | ⏸️ Later | Firmware programming can be added later |
| `ptp.c` | Save for Phase 5 | ⏸️ Later | PTP after networking works |

### Delete (Wrong Architecture):
| Old File | Reason | Replacement |
|----------|--------|-------------|
| `driver.c` | KMDF, not NDIS | `miniport.c` |
| `driver.h` | KMDF definitions | `miniport.h` |
| `device.c` | KMDF PnP handlers | Built into `miniport.c` |
| `usb.c` | Wrong WDF APIs | New `usb.c` with correct APIs |
| `ethernet.c` | IOCTL-based, not NDIS | New `receive.c` + `transmit.c` |
| `TimeStick.inf` | For KMDF | New INF for NDIS |

### Assets to Keep:
- `AX88279_Firmware_Image_v1.3.16_R7/ax88279.bin` - ✅ Keep firmware file
- `tests/` - ✅ Keep test framework (update for NDIS)
- `BUILD_SETUP.md` - ✅ Keep as reference
- `README.md` - ✅ Update with new info

## Directory Structure

### Recommended:
```
WIN/
├── old_kmdf/          # Move broken code here
│   ├── driver.c
│   ├── device.c
│   └── ...
├── ndis/              # New NDIS driver (CREATE THIS)
│   ├── src/
│   │   ├── miniport.c
│   │   ├── miniport.h
│   │   ├── asix_hw.h
│   │   ├── usb.c
│   │   ├── hardware.c
│   │   ├── receive.c  (TODO)
│   │   ├── transmit.c (TODO)
│   │   └── oid.c      (TODO)
│   ├── TimeStick.inf
│   ├── TimeStick.rc
│   └── sources
├── firmware/          # Firmware tools (Phase 6)
│   ├── ax88279.bin
│   └── programmer.c   (TODO)
└── tests/             # Test applications
    └── ...
```

## Quick Start: Set Up New Driver

### Step 1: Create Directory Structure
```powershell
cd E:\repos\philiplinden\TimeStick\TimeStick2\WIN

# Move old code
mkdir old_kmdf
move driver.c old_kmdf\
move driver.h old_kmdf\
move device.c old_kmdf\
move usb.c old_kmdf\
move ethernet.c old_kmdf\
move TimeStick.inf old_kmdf\

# Create new structure
mkdir ndis
mkdir ndis\src
```

### Step 2: Copy New Files
Copy these files from the outputs I created:
```powershell
# From your Claude outputs folder
copy miniport.h ndis\src\
copy asix_hw.h ndis\src\
copy usb.c ndis\src\
copy hardware.c ndis\src\

# Combine miniport parts
type miniport_part1.c miniport_part2.c > ndis\src\miniport.c
```

### Step 3: Create Stubs for TODO Files
Create these empty files to avoid linker errors:

**`ndis\src\receive.c`**:
```c
#include "miniport.h"

NTSTATUS TimestickReceiveStart(_In_ PTIMESTICK_ADAPTER Adapter) {
    UNREFERENCED_PARAMETER(Adapter);
    return STATUS_NOT_IMPLEMENTED;
}

VOID TimestickReceiveStop(_In_ PTIMESTICK_ADAPTER Adapter) {
    UNREFERENCED_PARAMETER(Adapter);
}

VOID TimestickReceiveComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);
    UNREFERENCED_PARAMETER(Context);
}
```

**`ndis\src\transmit.c`**:
```c
#include "miniport.h"

NTSTATUS TimestickTransmitNetBufferList(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ PNET_BUFFER_LIST NetBufferList)
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(NetBufferList);
    return STATUS_NOT_IMPLEMENTED;
}

VOID TimestickTransmitComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);
    UNREFERENCED_PARAMETER(Context);
}
```

**`ndis\src\oid.c`**:
```c
#include "miniport.h"

NDIS_STATUS
TimestickQueryInformation(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded)
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(Oid);
    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InformationBufferLength);
    *BytesWritten = 0;
    *BytesNeeded = 0;
    return NDIS_STATUS_NOT_SUPPORTED;
}

NDIS_STATUS
TimestickSetInformation(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesRead,
    _Out_ PULONG BytesNeeded)
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(Oid);
    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InformationBufferLength);
    *BytesRead = 0;
    *BytesNeeded = 0;
    return NDIS_STATUS_NOT_SUPPORTED;
}
```

### Step 4: Add INF File
Copy the INF from BUILD_GUIDE.md to `ndis\TimeStick.inf`

### Step 5: Build Configuration
Copy sources file from BUILD_GUIDE.md to `ndis\sources`

## What Changed & Why

### Architecture Change
**Old**: Custom IOCTL interface, userspace handles networking
```
User App → DeviceIoControl → KMDF Driver → USB
          ↑ Manual packet handling in user mode
```

**New**: Standard NDIS, Windows handles networking
```
TCP/IP Stack → NDIS → Miniport → USB
              ↑ Automatic integration
```

### Key Differences

| Aspect | Old (KMDF) | New (NDIS) |
|--------|-----------|-----------|
| Driver Type | Generic USB | Network Miniport |
| Entry Point | DriverEntry | DriverEntry (different) |
| Device Interface | Custom IOCTLs | NDIS callbacks |
| Packet Handling | Manual via IOCTL | NET_BUFFER_LIST |
| Network Stack | None | Full Windows TCP/IP |
| Appears As | Custom device | Network adapter |
| User Access | DeviceIoControl() | Standard sockets |

### Code Pattern Changes

**Old (KMDF) - Won't work for networking**:
```c
// Old approach - wrong architecture
VOID MyEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode)
{
    // Handle IOCTL_TIMESTICK_SEND_PACKET
    // User must call this for every packet!
}
```

**New (NDIS) - Correct for networking**:
```c
// New approach - proper network driver
VOID MiniportSendNetBufferLists(
    NDIS_HANDLE MiniportAdapterContext,
    PNET_BUFFER_LIST NetBufferLists,
    NDIS_PORT_NUMBER PortNumber,
    ULONG SendFlags)
{
    // NDIS calls this automatically when
    // any application sends network data
}
```

## Salvaging Work from Old Code

### MAC Address Reading - Similar
Old code had the right idea, port the logic:
```c
// Old: Correct register, wrong API
TimeStickUsbReadRegister(DevContext, 0x10, buffer, sizeof(buffer));

// New: Same register, correct API  
TimestickUsbReadMacRegister(Adapter, AX_NODE_ID, buffer, 6);
```

### Firmware Programming - Save for Later
The old `firmware.c` has useful logic for Phase 6:
- Keep the firmware loading code
- Keep the programming sequences
- Port to NDIS adapter context later

### Test Framework - Update References
Old tests used IOCTLs, update to use:
- NDIS OIDs for queries
- Standard socket API for packet tests
- `ipconfig`, `ping` for validation

## Common Mistakes to Avoid

### ❌ Don't:
- Try to "fix" the old KMDF driver
- Mix KMDF and NDIS concepts
- Keep using IOCTLs for networking
- Use old `driver.h` definitions

### ✅ Do:
- Start fresh with NDIS miniport
- Follow Microsoft NDIS samples
- Use NET_BUFFER_LIST for packets
- Reference Linux driver for hardware

## Validation Checklist

After migration, verify:
- [ ] No files from old driver mixed in
- [ ] Using NDIS miniport APIs only
- [ ] MiniportInitializeEx as entry point
- [ ] NET_BUFFER_LIST for packets
- [ ] INF file is for NDIS network class
- [ ] No custom IOCTLs (except future PTP extensions)

## Need Help?

If you see these errors after migration:
- **"unresolved external symbol Miniport..."** → Missing handler implementation
- **"incompatible types: WDFDEVICE"** → Mixed old and new headers
- **"NDIS_STATUS_xxx undeclared"** → Missing `#include <ndis.h>`
- **Link fails** → Missing `ndis.lib` in linker settings

## Summary

**What to do**: Move old code to `old_kmdf/`, set up new `ndis/` structure with files I provided.

**Why**: KMDF approach was fundamentally wrong for network adapter.

**Result**: Clean start with correct NDIS architecture.

**Timeline**: ~1 hour to reorganize, then continue with Phase 2.
