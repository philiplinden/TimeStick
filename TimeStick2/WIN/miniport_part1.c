/*
 * TimeStick NDIS Miniport Driver - Main Implementation
 * Windows USB Ethernet Adapter with PTP Support
 */

#include "miniport.h"

// Global driver handle
NDIS_HANDLE g_NdisDriverHandle = NULL;
DRIVER_OBJECT* g_DriverObject = NULL;

//
// DriverEntry - Main entry point for NDIS miniport
//
NDIS_STATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NDIS_STATUS status;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS miniportCharacteristics;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "DriverEntry called");
    
    g_DriverObject = DriverObject;
    
    // Initialize miniport characteristics structure
    NdisZeroMemory(&miniportCharacteristics, sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS));
    
    miniportCharacteristics.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
    miniportCharacteristics.Header.Size = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_3;
    miniportCharacteristics.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_3;
    
    miniportCharacteristics.MajorNdisVersion = TIMESTICK_NDIS_MAJOR_VERSION;
    miniportCharacteristics.MinorNdisVersion = TIMESTICK_NDIS_MINOR_VERSION;
    
    miniportCharacteristics.MajorDriverVersion = TIMESTICK_MAJOR_VERSION;
    miniportCharacteristics.MinorDriverVersion = TIMESTICK_MINOR_VERSION;
    
    // Set required handler functions
    miniportCharacteristics.InitializeHandlerEx = MiniportInitializeEx;
    miniportCharacteristics.HaltHandlerEx = MiniportHaltEx;
    miniportCharacteristics.UnloadHandler = MiniportDriverUnload;
    miniportCharacteristics.PauseHandler = MiniportPause;
    miniportCharacteristics.RestartHandler = MiniportRestart;
    miniportCharacteristics.OidRequestHandler = MiniportOidRequest;
    miniportCharacteristics.SendNetBufferListsHandler = MiniportSendNetBufferLists;
    miniportCharacteristics.ReturnNetBufferListsHandler = MiniportReturnNetBufferLists;
    miniportCharacteristics.CancelSendHandler = MiniportCancelSend;
    miniportCharacteristics.CheckForHangHandlerEx = MiniportCheckForHangEx;
    miniportCharacteristics.ResetHandlerEx = MiniportResetEx;
    miniportCharacteristics.DevicePnPEventNotifyHandler = MiniportDevicePnPEventNotify;
    miniportCharacteristics.ShutdownHandlerEx = MiniportShutdownEx;
    miniportCharacteristics.CancelOidRequestHandler = MiniportCancelOidRequest;
    
    // Register the miniport driver
    status = NdisMRegisterMiniportDriver(
        DriverObject,
        RegistryPath,
        NULL,  // MiniportDriverContext
        &miniportCharacteristics,
        &g_NdisDriverHandle
    );
    
    if (status != NDIS_STATUS_SUCCESS) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "NdisMRegisterMiniportDriver failed: 0x%08X", status);
        return status;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "NDIS miniport driver registered successfully");
    
    return NDIS_STATUS_SUCCESS;
}

//
// MiniportDriverUnload - Driver unload handler
//
VOID
MiniportDriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportDriverUnload called");
    
    if (g_NdisDriverHandle != NULL) {
        NdisMDeregisterMiniportDriver(g_NdisDriverHandle);
        g_NdisDriverHandle = NULL;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Driver unloaded successfully");
}

//
// MiniportInitializeEx - Initialize a new adapter instance
//
NDIS_STATUS
MiniportInitializeEx(
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE MiniportDriverContext,
    _In_ PNDIS_MINIPORT_INIT_PARAMETERS MiniportInitParameters
)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PTIMESTICK_ADAPTER adapter = NULL;
    NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES registrationAttributes;
    NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES generalAttributes;
    NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParams;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDEVICE device;
    NTSTATUS ntStatus;
    
    UNREFERENCED_PARAMETER(MiniportDriverContext);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportInitializeEx called");
    
    do {
        // Allocate adapter context
        adapter = (PTIMESTICK_ADAPTER)NdisAllocateMemoryWithTagPriority(
            MiniportAdapterHandle,
            sizeof(TIMESTICK_ADAPTER),
            TIMESTICK_POOL_TAG,
            NormalPoolPriority
        );
        
        if (adapter == NULL) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, "Failed to allocate adapter context");
            status = NDIS_STATUS_RESOURCES;
            break;
        }
        
        NdisZeroMemory(adapter, sizeof(TIMESTICK_ADAPTER));
        adapter->AdapterHandle = MiniportAdapterHandle;
        
        // Initialize spin locks
        NdisAllocateSpinLock(&adapter->ReceiveLock);
        NdisAllocateSpinLock(&adapter->TransmitLock);
        NdisAllocateSpinLock(&adapter->AdapterLock);
        
        // Initialize lists
        InitializeListHead(&adapter->ReceiveFreeList);
        InitializeListHead(&adapter->ReceiveBusyList);
        InitializeListHead(&adapter->TransmitFreeList);
        InitializeListHead(&adapter->TransmitBusyList);
        
        // Set registration attributes
        NdisZeroMemory(&registrationAttributes, 
            sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES));
        registrationAttributes.Header.Type = 
            NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        registrationAttributes.Header.Revision = 
            NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2;
        registrationAttributes.Header.Size = 
            NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2;
        registrationAttributes.MiniportAdapterContext = adapter;
        registrationAttributes.AttributeFlags = 
            NDIS_MINIPORT_ATTRIBUTES_HARDWARE_DEVICE |
            NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER |
            NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK;
        registrationAttributes.InterfaceType = NdisInterfaceInternal;
        
        status = NdisMSetMiniportAttributes(
            MiniportAdapterHandle,
            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&registrationAttributes
        );
        
        if (status != NDIS_STATUS_SUCCESS) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "NdisMSetMiniportAttributes (registration) failed: 0x%08X", status);
            break;
        }
        
        // Create WDF device for USB communication
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = NULL;
        
        ntStatus = WdfDeviceCreate(
            (PWDFDEVICE_INIT*)&MiniportInitParameters->DeviceContext,
            &attributes,
            &device
        );
        
        if (!NT_SUCCESS(ntStatus)) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "WdfDeviceCreate failed: 0x%08X", ntStatus);
            status = NDIS_STATUS_FAILURE;
            break;
        }
        
        adapter->WdfDevice = device;
        
        // Initialize USB layer
        ntStatus = TimestickUsbInitialize(adapter);
        if (!NT_SUCCESS(ntStatus)) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "TimestickUsbInitialize failed: 0x%08X", ntStatus);
            status = NDIS_STATUS_ADAPTER_NOT_FOUND;
            break;
        }
        
        // Initialize hardware and read MAC address
        ntStatus = TimestickHwInitialize(adapter);
        if (!NT_SUCCESS(ntStatus)) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "TimestickHwInitialize failed: 0x%08X", ntStatus);
            status = NDIS_STATUS_FAILURE;
            break;
        }
        
        // Set general attributes
        NdisZeroMemory(&generalAttributes, 
            sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));
        generalAttributes.Header.Type = 
            NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        generalAttributes.Header.Revision = 
            NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        generalAttributes.Header.Size = 
            NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        
        generalAttributes.MediaType = NdisMedium802_3;
        generalAttributes.PhysicalMediumType = NdisPhysicalMedium802_3;
        generalAttributes.MtuSize = TIMESTICK_MAX_PACKET_SIZE - TIMESTICK_HEADER_SIZE;
        generalAttributes.MaxXmitLinkSpeed = TIMESTICK_LINK_SPEED_1G;
        generalAttributes.MaxRcvLinkSpeed = TIMESTICK_LINK_SPEED_1G;
        generalAttributes.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        generalAttributes.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        generalAttributes.MediaConnectState = MediaConnectStateDisconnected;
        generalAttributes.MediaDuplexState = MediaDuplexStateUnknown;
        generalAttributes.LookaheadSize = TIMESTICK_MAX_PACKET_SIZE;
        
        generalAttributes.MacOptions = 
            NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
            NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
            NDIS_MAC_OPTION_NO_LOOPBACK;
        
        generalAttributes.SupportedPacketFilters = 
            NDIS_PACKET_TYPE_DIRECTED |
            NDIS_PACKET_TYPE_MULTICAST |
            NDIS_PACKET_TYPE_BROADCAST |
            NDIS_PACKET_TYPE_PROMISCUOUS |
            NDIS_PACKET_TYPE_ALL_MULTICAST;
        
        generalAttributes.MaxMulticastListSize = TIMESTICK_MAX_MULTICAST_LIST;
        generalAttributes.MacAddressLength = 6;
        
        NdisMoveMemory(
            generalAttributes.PermanentMacAddress,
            adapter->HardwareInfo.PermanentMacAddress,
            6
        );
        
        NdisMoveMemory(
            generalAttributes.CurrentMacAddress,
            adapter->HardwareInfo.CurrentMacAddress,
            6
        );
        
        generalAttributes.RecvScaleCapabilities = NULL;
        generalAttributes.AccessType = NET_IF_ACCESS_BROADCAST;
        generalAttributes.DirectionType = NET_IF_DIRECTION_SENDRECEIVE;
        generalAttributes.ConnectionType = NET_IF_CONNECTION_DEDICATED;
        generalAttributes.IfType = IF_TYPE_ETHERNET_CSMACD;
        generalAttributes.IfConnectorPresent = TRUE;
        generalAttributes.SupportedStatistics = 
            NDIS_STATISTICS_XMIT_OK_SUPPORTED |
            NDIS_STATISTICS_RCV_OK_SUPPORTED |
            NDIS_STATISTICS_XMIT_ERROR_SUPPORTED |
            NDIS_STATISTICS_RCV_ERROR_SUPPORTED |
            NDIS_STATISTICS_RCV_NO_BUFFER_SUPPORTED;
        
        generalAttributes.SupportedPauseFunctions = NdisPauseFunctionsUnsupported;
        generalAttributes.DataBackFillSize = 0;
        generalAttributes.ContextBackFillSize = 0;
        
        generalAttributes.SupportedOidList = NULL;
        generalAttributes.SupportedOidListLength = 0;
        
        generalAttributes.AutoNegotiationFlags = 
            NDIS_LINK_STATE_XMIT_LINK_SPEED_AUTO_NEGOTIATED |
            NDIS_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED |
            NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;
        
        generalAttributes.PowerManagementCapabilitiesEx = NULL;
        
        status = NdisMSetMiniportAttributes(
            MiniportAdapterHandle,
            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&generalAttributes
        );
        
        if (status != NDIS_STATUS_SUCCESS) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "NdisMSetMiniportAttributes (general) failed: 0x%08X", status);
            break;
        }
        
        // Create NET_BUFFER_LIST pool for receives
        NdisZeroMemory(&nblPoolParams, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
        nblPoolParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        nblPoolParams.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        nblPoolParams.Header.Size = sizeof(NET_BUFFER_LIST_POOL_PARAMETERS);
        nblPoolParams.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
        nblPoolParams.fAllocateNetBuffer = TRUE;
        nblPoolParams.PoolTag = TIMESTICK_NBL_POOL_TAG;
        
        adapter->NetBufferListPool = NdisAllocateNetBufferListPool(
            MiniportAdapterHandle,
            &nblPoolParams
        );
        
        if (adapter->NetBufferListPool == NULL) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "NdisAllocateNetBufferListPool failed");
            status = NDIS_STATUS_RESOURCES;
            break;
        }
        
        adapter->AdapterInitialized = TRUE;
        
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
            "Adapter initialized successfully, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            adapter->HardwareInfo.CurrentMacAddress[0],
            adapter->HardwareInfo.CurrentMacAddress[1],
            adapter->HardwareInfo.CurrentMacAddress[2],
            adapter->HardwareInfo.CurrentMacAddress[3],
            adapter->HardwareInfo.CurrentMacAddress[4],
            adapter->HardwareInfo.CurrentMacAddress[5]
        );
        
    } while (FALSE);
    
    if (status != NDIS_STATUS_SUCCESS) {
        if (adapter != NULL) {
            TimestickFreeAdapter(adapter);
        }
    }
    
    return status;
}

// Continue in next file...
