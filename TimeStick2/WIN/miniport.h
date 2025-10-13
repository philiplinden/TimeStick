/*
 * TimeStick NDIS Miniport Driver - Main Header
 * Windows USB Ethernet Adapter with PTP Support
 */

#ifndef _TIMESTICK_MINIPORT_H_
#define _TIMESTICK_MINIPORT_H_

#include <ndis.h>
#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>
#include "asix_hw.h"

// Driver version
#define TIMESTICK_MAJOR_VERSION         1
#define TIMESTICK_MINOR_VERSION         0
#define TIMESTICK_DRIVER_VERSION        ((TIMESTICK_MAJOR_VERSION << 16) | TIMESTICK_MINOR_VERSION)

// NDIS version we're targeting (NDIS 6.83 for Windows 10 2004+)
#define TIMESTICK_NDIS_MAJOR_VERSION    6
#define TIMESTICK_NDIS_MINOR_VERSION    83

// Pool tags for memory allocation
#define TIMESTICK_POOL_TAG              'kstT'
#define TIMESTICK_NBL_POOL_TAG          'lbNT'

// Adapter identification
#define TIMESTICK_VENDOR_DESC           "ASIX Electronics"
#define TIMESTICK_ADAPTER_DESC          "ASIX AX88279 USB 3.0 Gigabit Ethernet Adapter"

// Default MAC address if none in hardware
#define TIMESTICK_DEFAULT_MAC           {0x00, 0x0E, 0xC6, 0x87, 0x72, 0x00}

// Media parameters
#define TIMESTICK_MAX_PACKET_SIZE       1514    // 1500 + 14 byte header
#define TIMESTICK_MIN_PACKET_SIZE       60
#define TIMESTICK_MAX_FRAME_SIZE        1514
#define TIMESTICK_HEADER_SIZE           14
#define TIMESTICK_MAX_MULTICAST_LIST    AX_MAX_MCAST
#define TIMESTICK_LINK_SPEED_1G         1000000000ULL
#define TIMESTICK_LINK_SPEED_100M       100000000ULL
#define TIMESTICK_LINK_SPEED_10M        10000000ULL

// USB Endpoint configuration
#define TIMESTICK_BULK_IN_PIPE_INDEX    0
#define TIMESTICK_BULK_OUT_PIPE_INDEX   1
#define TIMESTICK_INTERRUPT_PIPE_INDEX  2

// Receive configuration
#define TIMESTICK_NUM_RX_URBS           16      // Number of RX URBs
#define TIMESTICK_RX_BUFFER_SIZE        AX_RX_URB_SIZE

// Transmit configuration
#define TIMESTICK_NUM_TX_URBS           8       // Number of TX URBs
#define TIMESTICK_TX_BUFFER_SIZE        2048

// Forward declarations
typedef struct _TIMESTICK_ADAPTER TIMESTICK_ADAPTER, *PTIMESTICK_ADAPTER;

//
// Receive Buffer Descriptor
//
typedef struct _RX_BUFFER_DESC {
    LIST_ENTRY                  ListEntry;
    PTIMESTICK_ADAPTER         Adapter;
    WDFREQUEST                  UsbRequest;
    WDFMEMORY                   BufferMemory;
    PVOID                       Buffer;
    ULONG                       BufferLength;
    BOOLEAN                     InUse;
} RX_BUFFER_DESC, *PRX_BUFFER_DESC;

//
// Transmit Buffer Descriptor  
//
typedef struct _TX_BUFFER_DESC {
    LIST_ENTRY                  ListEntry;
    PTIMESTICK_ADAPTER         Adapter;
    WDFREQUEST                  UsbRequest;
    WDFMEMORY                   BufferMemory;
    PVOID                       Buffer;
    ULONG                       BufferLength;
    PNET_BUFFER_LIST            NetBufferList;
    BOOLEAN                     InUse;
} TX_BUFFER_DESC, *PTX_BUFFER_DESC;

//
// Adapter Statistics
//
typedef struct _ADAPTER_STATISTICS {
    ULONG64                     FramesTransmitted;
    ULONG64                     FramesReceived;
    ULONG64                     BytesTransmitted;
    ULONG64                     BytesReceived;
    ULONG64                     TransmitErrors;
    ULONG64                     ReceiveErrors;
    ULONG64                     ReceiveNoBuffers;
    ULONG64                     ReceiveCrcErrors;
    ULONG64                     ReceiveAlignmentErrors;
    ULONG64                     TransmitCollisions;
} ADAPTER_STATISTICS, *PADAPTER_STATISTICS;

//
// Link State Information
//
typedef struct _LINK_STATE {
    NDIS_MEDIA_CONNECT_STATE    MediaConnectState;
    NDIS_MEDIA_DUPLEX_STATE     MediaDuplexState;
    ULONG64                     LinkSpeed;          // bits per second
    AX_LINK_SPEED               HardwareLinkSpeed;  // Hardware speed enum
    BOOLEAN                     LinkUp;
    BOOLEAN                     FullDuplex;
} LINK_STATE, *PLINK_STATE;

//
// Hardware Information
//
typedef struct _HARDWARE_INFO {
    UCHAR                       PermanentMacAddress[6];
    UCHAR                       CurrentMacAddress[6];
    UCHAR                       MulticastList[AX_MAX_MCAST][6];
    ULONG                       MulticastCount;
    ULONG                       PacketFilter;
    USHORT                      VendorId;
    USHORT                      ProductId;
    USHORT                      BcdDevice;
    AX_CHIP_VERSION             ChipVersion;
    UCHAR                       FirmwareVersion[4];
} HARDWARE_INFO, *PHARDWARE_INFO;

//
// USB Information
//
typedef struct _USB_INFO {
    WDFUSBDEVICE                UsbDevice;
    WDFUSBINTERFACE             UsbInterface;
    WDFUSBPIPE                  BulkInPipe;
    WDFUSBPIPE                  BulkOutPipe;
    WDFUSBPIPE                  InterruptPipe;
    UCHAR                       NumInterfaces;
    UCHAR                       InterfaceNumber;
    WDF_USB_DEVICE_INFORMATION  DeviceInfo;
} USB_INFO, *PUSB_INFO;

//
// Main Adapter Context
//
typedef struct _TIMESTICK_ADAPTER {
    // NDIS handles
    NDIS_HANDLE                 AdapterHandle;
    
    // WDF objects
    WDFDEVICE                   WdfDevice;
    
    // Hardware state
    HARDWARE_INFO               HardwareInfo;
    USB_INFO                    UsbInfo;
    LINK_STATE                  LinkState;
    ADAPTER_STATISTICS          Statistics;
    
    // Adapter state flags
    BOOLEAN                     AdapterInitialized;
    BOOLEAN                     HardwareStarted;
    BOOLEAN                     ReceiveStarted;
    
    // Synchronization
    NDIS_SPIN_LOCK              ReceiveLock;
    NDIS_SPIN_LOCK              TransmitLock;
    NDIS_SPIN_LOCK              AdapterLock;
    
    // Receive resources
    LIST_ENTRY                  ReceiveFreeList;
    LIST_ENTRY                  ReceiveBusyList;
    RX_BUFFER_DESC              ReceiveBuffers[TIMESTICK_NUM_RX_URBS];
    ULONG                       ReceiveFreeCount;
    ULONG                       ReceivePendingCount;
    
    // Transmit resources
    LIST_ENTRY                  TransmitFreeList;
    LIST_ENTRY                  TransmitBusyList;
    TX_BUFFER_DESC              TransmitBuffers[TIMESTICK_NUM_TX_URBS];
    ULONG                       TransmitFreeCount;
    ULONG                       TransmitPendingCount;
    
    // NET_BUFFER_LIST pool
    NDIS_HANDLE                 NetBufferListPool;
    
    // Power management
    NDIS_DEVICE_POWER_STATE     PowerState;
    
    // Work items and timers
    WDFTIMER                    LinkCheckTimer;
    
} TIMESTICK_ADAPTER, *PTIMESTICK_ADAPTER;

//
// Function Prototypes - Miniport Handlers
//

// Required miniport handlers
MINIPORT_INITIALIZE             MiniportInitializeEx;
MINIPORT_HALT                   MiniportHaltEx;
MINIPORT_UNLOAD                 MiniportDriverUnload;
MINIPORT_PAUSE                  MiniportPause;
MINIPORT_RESTART                MiniportRestart;
MINIPORT_SEND_NET_BUFFER_LISTS  MiniportSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS MiniportReturnNetBufferLists;
MINIPORT_CANCEL_SEND            MiniportCancelSend;
MINIPORT_CHECK_FOR_HANG         MiniportCheckForHangEx;
MINIPORT_RESET                  MiniportResetEx;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY MiniportDevicePnPEventNotify;
MINIPORT_SHUTDOWN               MiniportShutdownEx;
MINIPORT_OID_REQUEST            MiniportOidRequest;
MINIPORT_CANCEL_OID_REQUEST     MiniportCancelOidRequest;

//
// Function Prototypes - USB Layer
//
NTSTATUS
TimestickUsbInitialize(
    _In_ PTIMESTICK_ADAPTER Adapter
);

VOID
TimestickUsbCleanup(
    _In_ PTIMESTICK_ADAPTER Adapter
);

NTSTATUS
TimestickUsbReadRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR Command,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
);

NTSTATUS
TimestickUsbWriteRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR Command,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
);

//
// Function Prototypes - Hardware Layer
//
NTSTATUS
TimestickHwInitialize(
    _In_ PTIMESTICK_ADAPTER Adapter
);

NTSTATUS
TimestickHwStart(
    _In_ PTIMESTICK_ADAPTER Adapter
);

VOID
TimestickHwStop(
    _In_ PTIMESTICK_ADAPTER Adapter
);

NTSTATUS
TimestickHwReadMacAddress(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _Out_writes_bytes_(6) PUCHAR MacAddress
);

NTSTATUS
TimestickHwWriteMacAddress(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_reads_bytes_(6) PUCHAR MacAddress
);

NTSTATUS
TimestickHwSetPacketFilter(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ ULONG PacketFilter
);

NTSTATUS
TimestickHwSetMulticastList(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_reads_bytes_(Count * 6) PUCHAR MulticastAddresses,
    _In_ ULONG Count
);

//
// Function Prototypes - Receive Path
//
NTSTATUS
TimestickReceiveStart(
    _In_ PTIMESTICK_ADAPTER Adapter
);

VOID
TimestickReceiveStop(
    _In_ PTIMESTICK_ADAPTER Adapter
);

VOID
TimestickReceiveComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
);

//
// Function Prototypes - Transmit Path
//
NTSTATUS
TimestickTransmitNetBufferList(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ PNET_BUFFER_LIST NetBufferList
);

VOID
TimestickTransmitComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
);

//
// Function Prototypes - Link Management
//
VOID
TimestickLinkCheckWorker(
    _In_ WDFTIMER Timer
);

NTSTATUS
TimestickPhyInit(
    _In_ PTIMESTICK_ADAPTER Adapter
);

NTSTATUS
TimestickPhyReadStatus(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _Out_ PLINK_STATE LinkState
);

VOID
TimestickIndicateLinkState(
    _In_ PTIMESTICK_ADAPTER Adapter
);

//
// Function Prototypes - Utility
//
VOID
TimestickFreeAdapter(
    _In_ PTIMESTICK_ADAPTER Adapter
);

NDIS_STATUS
TimestickQueryInformation(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded
);

NDIS_STATUS
TimestickSetInformation(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesRead,
    _Out_ PULONG BytesNeeded
);

//
// Debug Macros
//
#if DBG
#define TIMESTICK_DBGPRINT(Level, Fmt, ...) \
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, Level, "TimeStick: " Fmt "\n", __VA_ARGS__)
#else
#define TIMESTICK_DBGPRINT(Level, Fmt, ...)
#endif

#define TIMESTICK_DEBUG_ERROR       DPFLTR_ERROR_LEVEL
#define TIMESTICK_DEBUG_WARNING     DPFLTR_WARNING_LEVEL
#define TIMESTICK_DEBUG_INFO        DPFLTR_INFO_LEVEL
#define TIMESTICK_DEBUG_TRACE       DPFLTR_TRACE_LEVEL

#endif // _TIMESTICK_MINIPORT_H_
