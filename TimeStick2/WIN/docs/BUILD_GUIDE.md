# TimeStick NDIS Miniport Driver - Build Guide & Next Steps

## What We've Created (Phase 1 Foundation)

✅ **Core NDIS Structure**
- `miniport.h` - Main header with adapter context and function prototypes
- `miniport_part1.c` - DriverEntry, MiniportInitializeEx
- `miniport_part2.c` - MiniportHaltEx, Pause/Restart, Send/Return handlers
- `asix_hw.h` - Complete hardware register definitions from Linux driver
- `usb.c` - USB communication layer with control transfers
- `hardware.c` - ASIX chip initialization and PHY control

## Current Status

**What works** (should compile and load):
- NDIS driver registration
- USB device detection and enumeration
- MAC address reading from hardware
- PHY initialization
- Basic hardware setup
- Link state monitoring

**What's missing** (for full networking):
- Receive path implementation (Phase 3)
- Transmit path implementation (Phase 2)
- OID query/set handlers
- INF file for installation
- Complete build configuration

## File Organization

Merge the miniport parts and organize like this:

```
E:\repos\philiplinden\TimeStick\TimeStick2\WIN\ndis\
├── src\
│   ├── miniport.c      # Combine miniport_part1.c + miniport_part2.c
│   ├── miniport.h      # Main header
│   ├── asix_hw.h       # Hardware definitions
│   ├── usb.c           # USB communication
│   ├── hardware.c      # Hardware initialization
│   ├── receive.c       # TODO: Phase 3
│   ├── transmit.c      # TODO: Phase 2
│   └── oid.c           # TODO: OID handlers
├── TimeStick.inf       # TODO: Installation file
├── TimeStick.rc        # TODO: Resource file
└── sources             # TODO: Build configuration
```

## Build Configuration Setup

### 1. Create `sources` file (WDK build system)

```makefile
TARGETNAME=TimeStick
TARGETTYPE=DRIVER

KMDF_VERSION_MAJOR=1
KMDF_VERSION_MINOR=33

TARGETLIBS=$(DDK_LIB_PATH)\ndis.lib \
           $(DDK_LIB_PATH)\wdf01000.lib \
           $(DDK_LIB_PATH)\usbd.lib

C_DEFINES=$(C_DEFINES) -DNDIS_MINIPORT_DRIVER -DNDIS683

SOURCES=\
    miniport.c \
    usb.c \
    hardware.c \
    receive.c \
    transmit.c \
    oid.c
```

### 2. Create `TimeStick.inf` file

```inf
;
; TimeStick.inf
;

[Version]
Signature   = "$WINDOWS NT$"
Class       = Net
ClassGUID   = {4d36e972-e325-11ce-bfc1-08002be10318}
Provider    = %ManufacturerName%
CatalogFile = TimeStick.cat
DriverVer   = 10/13/2025,1.0.0.0
PnpLockDown = 1

[Manufacturer]
%ManufacturerName% = Standard,NT$ARCH$.10.0...16299

[Standard.NT$ARCH$.10.0...16299]
%TimeStick.DeviceDesc% = TimeStick.ndi, USB\VID_0B95&PID_1790

[TimeStick.ndi]
AddReg          = TimeStick.Reg
Characteristics = 0x84 ; NCF_PHYSICAL | NCF_HAS_UI
*IfType         = 6    ; IF_TYPE_ETHERNET_CSMACD
*MediaType      = 0    ; NdisMedium802_3
*PhysicalMediaType = 0 ; NdisPhysicalMedium802_3
CopyFiles       = TimeStick.CopyFiles

[TimeStick.ndi.Services]
AddService = TimeStick, 2, TimeStick.Service, TimeStick.EventLog

[TimeStick.Service]
DisplayName    = %TimeStick.Service.DispName%
ServiceType    = 1    ; SERVICE_KERNEL_DRIVER
StartType      = 3    ; SERVICE_DEMAND_START
ErrorControl   = 1    ; SERVICE_ERROR_NORMAL
ServiceBinary  = %13%\TimeStick.sys
LoadOrderGroup = NDIS

[TimeStick.EventLog]
AddReg = TimeStick.AddEventLog.Reg

[TimeStick.AddEventLog.Reg]
HKR, , EventMessageFile, 0x00020000, "%%SystemRoot%%\System32\netevent.dll"
HKR, , TypesSupported,   0x00010001, 7

[TimeStick.Reg]
HKR, Ndi,                   Service,    0, "TimeStick"
HKR, Ndi\Interfaces,        UpperRange, 0, "ndis5"
HKR, Ndi\Interfaces,        LowerRange, 0, "ethernet"

HKR, ,                      NetworkAddress,     0, ""
HKR, ,                      *ReceiveBuffers,    0, "256"
HKR, ,                      *TransmitBuffers,   0, "128"

[TimeStick.CopyFiles]
TimeStick.sys,,,2

[SourceDisksNames]
1 = %DiskName%,,,""

[SourceDisksFiles]
TimeStick.sys = 1,,

[DestinationDirs]
TimeStick.CopyFiles = 13

[Strings]
ManufacturerName = "ASIX Electronics"
DiskName = "TimeStick Installation Disk"
TimeStick.DeviceDesc = "ASIX AX88279 USB 3.0 Gigabit Ethernet Adapter"
TimeStick.Service.DispName = "TimeStick Network Driver"
```

## Build Steps

### Using Visual Studio + WDK

1. **Open Visual Studio 2022**

2. **Create New Project**
   - File → New → Project
   - Select "Kernel Mode Driver, Empty (KMDF)"
   - Name: TimeStick
   - Location: `E:\repos\philiplinden\TimeStick\TimeStick2\WIN\ndis`

3. **Add Source Files**
   - Add all `.c` and `.h` files to project
   - Add `TimeStick.inf` to project
   - Set configuration to x64, Debug

4. **Project Properties**
   - Configuration: All Configurations
   - Platform: x64
   - Driver Settings → Target OS Version: Windows 10 or higher
   - Driver Settings → Target Platform: Desktop
   - Driver Model Settings → Type of Driver: KMDF
   - Driver Model Settings → KMDF Version: 1.33
   - Linker → Input → Additional Dependencies: Add `ndis.lib;usbd.lib`

5. **Build**
   - Build → Build Solution (Ctrl+Shift+B)
   - Output: `x64\Debug\TimeStick.sys`

### Using Command Line (DDK/WDK Build)

```cmd
cd E:\repos\philiplinden\TimeStick\TimeStick2\WIN\ndis
set WDKPATH=C:\Program Files (x86)\Windows Kits\10
"%WDKPATH%\bin\x86\devcon.exe" update TimeStick.inf "USB\VID_0B95&PID_1790"
```

## Installing for Testing

### 1. Enable Test Signing

**Run as Administrator**:
```cmd
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
```

Reboot required.

### 2. Install Driver

```cmd
pnputil /add-driver TimeStick.inf /install
```

### 3. Verify Installation

```cmd
pnputil /enum-drivers | findstr TimeStick
devcon status "USB\VID_0B95&PID_1790"
```

## Testing Phase 1

### What to Test:
1. **Driver loads** - Check Device Manager
2. **MAC address** - Verify reads from hardware
3. **Link detection** - Plug/unplug cable
4. **No crashes** - Stability test

### Debug Output:
View kernel debug messages with DebugView:
- Download DebugView from Sysinternals
- Run as Administrator
- Capture → Capture Kernel
- Look for "TimeStick:" messages

### Expected Results:
- ✅ Device appears in "Network adapters"
- ✅ Shows correct MAC address
- ✅ No yellow exclamation marks
- ❌ No network connectivity yet (that's Phase 2+3)

## Next Steps - Phase 2: Transmit Path

Create `transmit.c` with these functions:

```c
NTSTATUS TimestickTransmitNetBufferList(...)
VOID TimestickTransmitComplete(...)
```

**Key tasks**:
1. Format NET_BUFFER as ASIX TX packet (8-byte header)
2. Submit USB bulk OUT transfer
3. Complete NET_BUFFER_LIST on success
4. Handle errors and queue management

**Reference**: Linux `ax88179_tx_fixup()` in `ax88179_178a.c`

## Next Steps - Phase 3: Receive Path

Create `receive.c` with these functions:

```c
NTSTATUS TimestickReceiveStart(...)
VOID TimestickReceiveStop(...)
VOID TimestickReceiveComplete(...)
```

**Key tasks**:
1. Submit continuous USB bulk IN transfers
2. Parse ASIX RX format (multi-packet per transfer)
3. Create NET_BUFFER_LIST for each packet
4. Indicate to NDIS
5. Resubmit URBs

**Reference**: Linux `ax88179_rx_fixup()` in `ax88179_178a.c`

## Next Steps - Phase 4: OID Handlers

Create `oid.c` implementing:

```c
NDIS_STATUS TimestickQueryInformation(...)
NDIS_STATUS TimestickSetInformation(...)
```

**Required OIDs**:
- OID_GEN_VENDOR_DESCRIPTION
- OID_GEN_VENDOR_ID
- OID_GEN_LINK_SPEED
- OID_GEN_MEDIA_CONNECT_STATUS
- OID_802_3_PERMANENT_ADDRESS
- OID_802_3_CURRENT_ADDRESS
- OID_802_3_MULTICAST_LIST
- OID_GEN_CURRENT_PACKET_FILTER

## Common Issues & Solutions

### Issue: Driver won't load
**Solution**: Check test signing is enabled, verify INF syntax

### Issue: Device shows Code 10
**Solution**: Check DriverEntry succeeds, verify USB VID/PID

### Issue: Can't find device
**Solution**: Verify USB\VID_0B95&PID_1790 in device manager

### Issue: Build errors with NDIS types
**Solution**: Ensure targeting NDIS 6.83, Windows 10 SDK installed

### Issue: Link never comes up
**Solution**: Check PHY initialization, verify cable connected

## Debugging Tips

1. **Use WinDbg** for kernel debugging
2. **Add debug prints** liberally with TIMESTICK_DBGPRINT
3. **Check return codes** - every NTSTATUS/NDIS_STATUS
4. **Test incrementally** - don't add everything at once
5. **Compare with Linux** - when stuck, reference Linux driver code

## Resources

**Microsoft Documentation**:
- [NDIS Miniport Drivers](https://docs.microsoft.com/en-us/windows-hardware/drivers/network/ndis-miniport-drivers2)
- [Writing NDIS Miniport Drivers](https://docs.microsoft.com/en-us/windows-hardware/drivers/network/writing-ndis-miniport-drivers)

**Sample Drivers**:
- `%WDKPATH%\src\network\ndis\usb` - USB NDIS samples
- netvmini - Virtual miniport sample

**Linux Driver Reference**:
- `E:\repos\philiplinden\TimeStick\TimeStick2\DRV\ax88179_178a.c`
- `E:\repos\philiplinden\TimeStick\TimeStick2\DRV\ax_main.c`

## Timeline Estimate

If you're experienced with Windows drivers:
- **Phase 1** (Done): Core structure ✅
- **Phase 2**: TX path - 2-3 days
- **Phase 3**: RX path - 3-4 days  
- **Phase 4**: OID handlers - 1-2 days
- **Phase 5**: PTP - 5-7 days
- **Total**: ~2-3 weeks

If learning NDIS:
- Add 50-100% more time for learning curve
- Budget 4-6 weeks total

## Summary

**You now have**: A solid Phase 1 foundation with proper NDIS architecture, USB communication, and hardware initialization.

**To get networking**: Implement Phase 2 (TX) and Phase 3 (RX) following the Linux driver's packet format.

**To get PTP**: After networking works, implement Phase 5 using hardware timestamp registers.

**You're on the right track now!** The architecture is correct and the foundation is solid.

Want me to help with Phase 2 (transmit path) next?
