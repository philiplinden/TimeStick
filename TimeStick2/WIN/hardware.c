/*
 * TimeStick Hardware Layer
 * ASIX chip initialization and configuration
 */

#include "miniport.h"

//
// TimestickHwInitialize - Initialize ASIX hardware
//
NTSTATUS
TimestickHwInitialize(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    NTSTATUS status;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickHwInitialize");
    
    // Read MAC address from hardware
    status = TimestickHwReadMacAddress(
        Adapter,
        Adapter->HardwareInfo.PermanentMacAddress
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_WARNING, 
            "Failed to read MAC address from hardware: 0x%08X, using default", 
            status);
        
        // Use default MAC address
        UCHAR defaultMac[] = TIMESTICK_DEFAULT_MAC;
        NdisMoveMemory(
            Adapter->HardwareInfo.PermanentMacAddress,
            defaultMac,
            6
        );
    }
    
    // Copy to current MAC address
    NdisMoveMemory(
        Adapter->HardwareInfo.CurrentMacAddress,
        Adapter->HardwareInfo.PermanentMacAddress,
        6
    );
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
        Adapter->HardwareInfo.PermanentMacAddress[0],
        Adapter->HardwareInfo.PermanentMacAddress[1],
        Adapter->HardwareInfo.PermanentMacAddress[2],
        Adapter->HardwareInfo.PermanentMacAddress[3],
        Adapter->HardwareInfo.PermanentMacAddress[4],
        Adapter->HardwareInfo.PermanentMacAddress[5]
    );
    
    // Read firmware version
    UCHAR fwVersion[4] = {0};
    status = TimestickUsbReadMacRegister(Adapter, 0x23, fwVersion, sizeof(fwVersion));
    if (NT_SUCCESS(status)) {
        NdisMoveMemory(Adapter->HardwareInfo.FirmwareVersion, fwVersion, 4);
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
            "Firmware Version: %d.%d.%d.%d",
            fwVersion[0], fwVersion[1], fwVersion[2], fwVersion[3]);
    }
    
    // Initialize PHY
    status = TimestickPhyInit(Adapter);
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_WARNING, 
            "PHY initialization failed: 0x%08X", status);
        // Non-fatal, continue
    }
    
    // Set initial link state to disconnected
    Adapter->LinkState.MediaConnectState = MediaConnectStateDisconnected;
    Adapter->LinkState.MediaDuplexState = MediaDuplexStateUnknown;
    Adapter->LinkState.LinkSpeed = 0;
    Adapter->LinkState.LinkUp = FALSE;
    
    return STATUS_SUCCESS;
}

//
// TimestickHwStart - Start hardware operations
//
NTSTATUS
TimestickHwStart(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    NTSTATUS status;
    USHORT mediumMode;
    USHORT rxControl;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickHwStart");
    
    // Configure RX control - accept unicast, broadcast, multicast
    rxControl = AX_RX_CTL_START | AX_RX_CTL_AB | AX_RX_CTL_AM | AX_RX_CTL_AP;
    
    status = TimestickUsbWriteMacRegister(
        Adapter,
        AX_RX_CTL,
        &rxControl,
        sizeof(rxControl)
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Failed to set RX control: 0x%08X", status);
        return status;
    }
    
    // Set medium mode - enable receive, 1G capable, full duplex
    mediumMode = AX_MEDIUM_RECEIVE_EN | 
                 AX_MEDIUM_GIGAMODE | 
                 AX_MEDIUM_FULL_DUPLEX |
                 AX_MEDIUM_RXFLOW_CTRLEN |
                 AX_MEDIUM_TXFLOW_CTRLEN;
    
    status = TimestickUsbWriteMacRegister(
        Adapter,
        AX_MEDIUM_STATUS_MODE,
        &mediumMode,
        sizeof(mediumMode)
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Failed to set medium mode: 0x%08X", status);
        return status;
    }
    
    // Write MAC address to hardware
    status = TimestickHwWriteMacAddress(
        Adapter,
        Adapter->HardwareInfo.CurrentMacAddress
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Failed to write MAC address: 0x%08X", status);
        return status;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Hardware started successfully");
    
    return STATUS_SUCCESS;
}

//
// TimestickHwStop - Stop hardware operations
//
VOID
TimestickHwStop(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    USHORT rxControl;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickHwStop");
    
    // Stop RX
    rxControl = AX_RX_CTL_STOP;
    TimestickUsbWriteMacRegister(
        Adapter,
        AX_RX_CTL,
        &rxControl,
        sizeof(rxControl)
    );
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Hardware stopped");
}

//
// TimestickHwReadMacAddress - Read MAC address from hardware
//
NTSTATUS
TimestickHwReadMacAddress(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _Out_writes_bytes_(6) PUCHAR MacAddress
)
{
    NTSTATUS status;
    UCHAR buffer[8];
    
    // Read node ID register (contains MAC address)
    status = TimestickUsbReadMacRegister(
        Adapter,
        AX_NODE_ID,
        buffer,
        6
    );
    
    if (NT_SUCCESS(status)) {
        NdisMoveMemory(MacAddress, buffer, 6);
    }
    
    return status;
}

//
// TimestickHwWriteMacAddress - Write MAC address to hardware
//
NTSTATUS
TimestickHwWriteMacAddress(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_reads_bytes_(6) PUCHAR MacAddress
)
{
    return TimestickUsbWriteMacRegister(
        Adapter,
        AX_NODE_ID,
        MacAddress,
        6
    );
}

//
// TimestickHwSetPacketFilter - Set packet filter
//
NTSTATUS
TimestickHwSetPacketFilter(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ ULONG PacketFilter
)
{
    USHORT rxControl = AX_RX_CTL_START;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "TimestickHwSetPacketFilter: 0x%08X", PacketFilter);
    
    // Build RX control based on filter
    if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) {
        rxControl |= AX_RX_CTL_PRO;
    }
    
    if (PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) {
        rxControl |= AX_RX_CTL_AMALL;
    }
    
    if (PacketFilter & NDIS_PACKET_TYPE_BROADCAST) {
        rxControl |= AX_RX_CTL_AB;
    }
    
    if (PacketFilter & NDIS_PACKET_TYPE_MULTICAST) {
        rxControl |= AX_RX_CTL_AM;
    }
    
    if (PacketFilter & NDIS_PACKET_TYPE_DIRECTED) {
        rxControl |= AX_RX_CTL_AP;
    }
    
    // Write RX control register
    return TimestickUsbWriteMacRegister(
        Adapter,
        AX_RX_CTL,
        &rxControl,
        sizeof(rxControl)
    );
}

//
// TimestickHwSetMulticastList - Set multicast address list
//
NTSTATUS
TimestickHwSetMulticastList(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_reads_bytes_(Count * 6) PUCHAR MulticastAddresses,
    _In_ ULONG Count
)
{
    UCHAR multicastFilter[AX_MCAST_FILTER_SIZE];
    ULONG i;
    ULONG crc;
    ULONG bitNumber;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "TimestickHwSetMulticastList: count=%d", Count);
    
    // Clear multicast filter
    NdisZeroMemory(multicastFilter, sizeof(multicastFilter));
    
    // Build multicast hash filter
    for (i = 0; i < Count && i < TIMESTICK_MAX_MULTICAST_LIST; i++) {
        // Calculate CRC32 of address
        crc = RtlComputeCrc32(0, &MulticastAddresses[i * 6], 6);
        
        // Use upper 6 bits as hash
        bitNumber = (crc >> 26) & 0x3F;
        
        // Set bit in filter
        multicastFilter[bitNumber / 8] |= (1 << (bitNumber % 8));
    }
    
    // Write multicast filter to hardware
    return TimestickUsbWriteMacRegister(
        Adapter,
        AX_MULTI_FILTER_ARRY,
        multicastFilter,
        sizeof(multicastFilter)
    );
}

//
// TimestickPhyInit - Initialize PHY (Physical layer)
//
NTSTATUS
TimestickPhyInit(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    NTSTATUS status;
    USHORT phyControl;
    USHORT phyStatus;
    UCHAR phyId = 3;  // ASIX typically uses PHY ID 3
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickPhyInit");
    
    // Read PHY status to verify PHY is responding
    status = TimestickUsbReadPhyRegister(
        Adapter,
        phyId,
        GMII_PHY_STATUS,
        &phyStatus
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Failed to read PHY status: 0x%08X", status);
        return status;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "PHY Status: 0x%04X", phyStatus);
    
    // Enable auto-negotiation
    phyControl = GMII_CONTROL_ENABLE_AUTO | GMII_CONTROL_START_AUTO;
    
    status = TimestickUsbWritePhyRegister(
        Adapter,
        phyId,
        GMII_PHY_CONTROL,
        phyControl
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Failed to start auto-negotiation: 0x%08X", status);
        return status;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "PHY initialized successfully");
    
    return STATUS_SUCCESS;
}

//
// TimestickPhyReadStatus - Read current PHY/link status
//
NTSTATUS
TimestickPhyReadStatus(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _Out_ PLINK_STATE LinkState
)
{
    NTSTATUS status;
    USHORT phyStatus;
    USHORT physr;
    UCHAR phyId = 3;
    
    // Read PHY status register
    status = TimestickUsbReadPhyRegister(
        Adapter,
        phyId,
        GMII_PHY_STATUS,
        &phyStatus
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Check link status
    if (phyStatus & GMII_STATUS_LINK_UP) {
        // Read PHY-specific status register for speed/duplex
        status = TimestickUsbReadPhyRegister(
            Adapter,
            phyId,
            GMII_PHY_PHYSR,
            &physr
        );
        
        if (NT_SUCCESS(status)) {
            LinkState->LinkUp = TRUE;
            LinkState->MediaConnectState = MediaConnectStateConnected;
            
            // Determine speed
            if (physr & GMII_PHY_PHYSR_GIGA) {
                LinkState->LinkSpeed = TIMESTICK_LINK_SPEED_1G;
                LinkState->HardwareLinkSpeed = AX_LINK_SPEED_1000MBPS;
            } else if (physr & GMII_PHY_PHYSR_100) {
                LinkState->LinkSpeed = TIMESTICK_LINK_SPEED_100M;
                LinkState->HardwareLinkSpeed = AX_LINK_SPEED_100MBPS;
            } else {
                LinkState->LinkSpeed = TIMESTICK_LINK_SPEED_10M;
                LinkState->HardwareLinkSpeed = AX_LINK_SPEED_10MBPS;
            }
            
            // Determine duplex
            if (physr & GMII_PHY_PHYSR_FULL) {
                LinkState->FullDuplex = TRUE;
                LinkState->MediaDuplexState = MediaDuplexStateFull;
            } else {
                LinkState->FullDuplex = FALSE;
                LinkState->MediaDuplexState = MediaDuplexStateHalf;
            }
        }
    } else {
        LinkState->LinkUp = FALSE;
        LinkState->MediaConnectState = MediaConnectStateDisconnected;
        LinkState->MediaDuplexState = MediaDuplexStateUnknown;
        LinkState->LinkSpeed = 0;
        LinkState->HardwareLinkSpeed = AX_LINK_SPEED_UNKNOWN;
    }
    
    return STATUS_SUCCESS;
}

//
// TimestickIndicateLinkState - Indicate link state change to NDIS
//
VOID
TimestickIndicateLinkState(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    NDIS_LINK_STATE linkState;
    NDIS_STATUS_INDICATION statusIndication;
    
    // Read current link status
    TimestickPhyReadStatus(Adapter, &Adapter->LinkState);
    
    // Build NDIS link state indication
    NdisZeroMemory(&linkState, sizeof(NDIS_LINK_STATE));
    linkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    linkState.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    linkState.Header.Size = sizeof(NDIS_LINK_STATE);
    
    linkState.MediaConnectState = Adapter->LinkState.MediaConnectState;
    linkState.MediaDuplexState = Adapter->LinkState.MediaDuplexState;
    linkState.XmitLinkSpeed = Adapter->LinkState.LinkSpeed;
    linkState.RcvLinkSpeed = Adapter->LinkState.LinkSpeed;
    linkState.PauseFunctions = NdisPauseFunctionsUnsupported;
    
    // Build status indication
    NdisZeroMemory(&statusIndication, sizeof(NDIS_STATUS_INDICATION));
    statusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    statusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
    statusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);
    statusIndication.SourceHandle = Adapter->AdapterHandle;
    statusIndication.StatusCode = NDIS_STATUS_LINK_STATE;
    statusIndication.StatusBuffer = &linkState;
    statusIndication.StatusBufferSize = sizeof(linkState);
    
    // Indicate to NDIS
    NdisMIndicateStatusEx(Adapter->AdapterHandle, &statusIndication);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "Link state: %s, Speed: %s, Duplex: %s",
        Adapter->LinkState.LinkUp ? "UP" : "DOWN",
        Adapter->LinkState.HardwareLinkSpeed == AX_LINK_SPEED_1000MBPS ? "1000M" :
        Adapter->LinkState.HardwareLinkSpeed == AX_LINK_SPEED_100MBPS ? "100M" :
        Adapter->LinkState.HardwareLinkSpeed == AX_LINK_SPEED_10MBPS ? "10M" : "Unknown",
        Adapter->LinkState.FullDuplex ? "FULL" : "HALF"
    );
}
