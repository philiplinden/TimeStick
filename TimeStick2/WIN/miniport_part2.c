/*
 * TimeStick NDIS Miniport Driver - Part 2
 * Halt, Pause/Restart, and Packet Handlers
 */

// Continuation of miniport.c

//
// MiniportHaltEx - Halt and cleanup adapter
//
VOID
MiniportHaltEx(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_HALT_ACTION HaltAction
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    UNREFERENCED_PARAMETER(HaltAction);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportHaltEx called");
    
    if (adapter == NULL) {
        return;
    }
    
    // Stop hardware
    if (adapter->HardwareStarted) {
        TimestickHwStop(adapter);
        adapter->HardwareStarted = FALSE;
    }
    
    // Stop receiving
    if (adapter->ReceiveStarted) {
        TimestickReceiveStop(adapter);
        adapter->ReceiveStarted = FALSE;
    }
    
    // Clean up adapter
    TimestickFreeAdapter(adapter);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Adapter halted successfully");
}

//
// MiniportPause - Pause adapter operations
//
NDIS_STATUS
MiniportPause(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_MINIPORT_PAUSE_PARAMETERS PauseParameters
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    UNREFERENCED_PARAMETER(PauseParameters);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportPause called");
    
    // Stop receiving packets
    if (adapter->ReceiveStarted) {
        TimestickReceiveStop(adapter);
        adapter->ReceiveStarted = FALSE;
    }
    
    // Wait for pending sends to complete
    // In production, implement proper synchronization here
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Adapter paused");
    
    return NDIS_STATUS_SUCCESS;
}

//
// MiniportRestart - Restart adapter operations
//
NDIS_STATUS
MiniportRestart(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_MINIPORT_RESTART_PARAMETERS RestartParameters
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(RestartParameters);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportRestart called");
    
    // Start hardware if not already started
    if (!adapter->HardwareStarted) {
        status = TimestickHwStart(adapter);
        if (!NT_SUCCESS(status)) {
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
                "TimestickHwStart failed: 0x%08X", status);
            return NDIS_STATUS_FAILURE;
        }
        adapter->HardwareStarted = TRUE;
    }
    
    // Start receiving packets
    status = TimestickReceiveStart(adapter);
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "TimestickReceiveStart failed: 0x%08X", status);
        return NDIS_STATUS_FAILURE;
    }
    adapter->ReceiveStarted = TRUE;
    
    // Check link status
    TimestickIndicateLinkState(adapter);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Adapter restarted");
    
    return NDIS_STATUS_SUCCESS;
}

//
// MiniportSendNetBufferLists - Send packets
//
VOID
MiniportSendNetBufferLists(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ NDIS_PORT_NUMBER PortNumber,
    _In_ ULONG SendFlags
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    PNET_BUFFER_LIST currentNbl;
    PNET_BUFFER_LIST nextNbl;
    NTSTATUS status;
    BOOLEAN dispatchLevel;
    
    UNREFERENCED_PARAMETER(PortNumber);
    
    dispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_TRACE, 
        "MiniportSendNetBufferLists called");
    
    for (currentNbl = NetBufferLists; currentNbl != NULL; currentNbl = nextNbl) {
        nextNbl = NET_BUFFER_LIST_NEXT_NBL(currentNbl);
        NET_BUFFER_LIST_NEXT_NBL(currentNbl) = NULL;
        
        // Attempt to transmit this NBL
        status = TimestickTransmitNetBufferList(adapter, currentNbl);
        
        if (!NT_SUCCESS(status)) {
            // Failed to queue for transmission, complete with error
            NET_BUFFER_LIST_STATUS(currentNbl) = NDIS_STATUS_FAILURE;
            
            NdisMSendNetBufferListsComplete(
                adapter->AdapterHandle,
                currentNbl,
                dispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
            );
            
            NdisInterlockedIncrement((PLONG)&adapter->Statistics.TransmitErrors);
        }
    }
}

//
// MiniportReturnNetBufferLists - Return received packets
//
VOID
MiniportReturnNetBufferLists(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG ReturnFlags
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    PNET_BUFFER_LIST currentNbl;
    PNET_BUFFER_LIST nextNbl;
    
    UNREFERENCED_PARAMETER(ReturnFlags);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_TRACE, 
        "MiniportReturnNetBufferLists called");
    
    for (currentNbl = NetBufferLists; currentNbl != NULL; currentNbl = nextNbl) {
        nextNbl = NET_BUFFER_LIST_NEXT_NBL(currentNbl);
        
        // Free the NET_BUFFER_LIST
        NdisFreeNetBufferList(currentNbl);
    }
}

//
// MiniportCancelSend - Cancel pending send operations
//
VOID
MiniportCancelSend(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID CancelId
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    UNREFERENCED_PARAMETER(adapter);
    UNREFERENCED_PARAMETER(CancelId);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportCancelSend called");
    
    // Implementation: Walk transmit queue and cancel matching sends
    // Not critical for Phase 1
}

//
// MiniportCheckForHangEx - Check if adapter is hung
//
BOOLEAN
MiniportCheckForHangEx(
    _In_ NDIS_HANDLE MiniportAdapterContext
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    UNREFERENCED_PARAMETER(adapter);
    
    // Check if adapter is responding
    // Return TRUE if hung, FALSE otherwise
    
    return FALSE;
}

//
// MiniportResetEx - Reset the adapter
//
NDIS_STATUS
MiniportResetEx(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _Out_ PBOOLEAN AddressingReset
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    NTSTATUS status;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_WARNING, "MiniportResetEx called");
    
    *AddressingReset = TRUE;
    
    // Stop hardware
    TimestickHwStop(adapter);
    
    // Restart hardware
    status = TimestickHwStart(adapter);
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Hardware restart failed during reset: 0x%08X", status);
        return NDIS_STATUS_HARD_ERRORS;
    }
    
    // Restart receive
    status = TimestickReceiveStart(adapter);
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Receive restart failed during reset: 0x%08X", status);
        return NDIS_STATUS_HARD_ERRORS;
    }
    
    return NDIS_STATUS_SUCCESS;
}

//
// MiniportDevicePnPEventNotify - Handle PnP events
//
VOID
MiniportDevicePnPEventNotify(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNET_DEVICE_PNP_EVENT NetDevicePnPEvent
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "MiniportDevicePnPEventNotify: event %d", 
        NetDevicePnPEvent->DevicePnPEvent);
    
    switch (NetDevicePnPEvent->DevicePnPEvent) {
        case NdisDevicePnPEventSurpriseRemoved:
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_WARNING, "Device surprise removed");
            // Device was unplugged - stop all operations
            if (adapter->ReceiveStarted) {
                TimestickReceiveStop(adapter);
                adapter->ReceiveStarted = FALSE;
            }
            break;
            
        case NdisDevicePnPEventPowerProfileChanged:
            TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "Power profile changed");
            break;
            
        default:
            break;
    }
}

//
// MiniportShutdownEx - Shutdown handler
//
VOID
MiniportShutdownEx(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_SHUTDOWN_ACTION ShutdownAction
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    
    UNREFERENCED_PARAMETER(ShutdownAction);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportShutdownEx called");
    
    // Stop hardware gracefully
    if (adapter->HardwareStarted) {
        TimestickHwStop(adapter);
    }
}

//
// MiniportOidRequest - Handle OID requests
//
NDIS_STATUS
MiniportOidRequest(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_OID_REQUEST OidRequest
)
{
    PTIMESTICK_ADAPTER adapter = (PTIMESTICK_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PVOID informationBuffer;
    ULONG informationBufferLength;
    ULONG bytesWritten = 0;
    ULONG bytesNeeded = 0;
    ULONG bytesRead = 0;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_TRACE, 
        "MiniportOidRequest: type %d, OID 0x%08X",
        OidRequest->RequestType,
        OidRequest->DATA.QUERY_INFORMATION.Oid);
    
    switch (OidRequest->RequestType) {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            informationBuffer = OidRequest->DATA.QUERY_INFORMATION.InformationBuffer;
            informationBufferLength = OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
            
            status = TimestickQueryInformation(
                adapter,
                OidRequest->DATA.QUERY_INFORMATION.Oid,
                informationBuffer,
                informationBufferLength,
                &bytesWritten,
                &bytesNeeded
            );
            
            OidRequest->DATA.QUERY_INFORMATION.BytesWritten = bytesWritten;
            OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = bytesNeeded;
            break;
            
        case NdisRequestSetInformation:
            informationBuffer = OidRequest->DATA.SET_INFORMATION.InformationBuffer;
            informationBufferLength = OidRequest->DATA.SET_INFORMATION.InformationBufferLength;
            
            status = TimestickSetInformation(
                adapter,
                OidRequest->DATA.SET_INFORMATION.Oid,
                informationBuffer,
                informationBufferLength,
                &bytesRead,
                &bytesNeeded
            );
            
            OidRequest->DATA.SET_INFORMATION.BytesRead = bytesRead;
            OidRequest->DATA.SET_INFORMATION.BytesNeeded = bytesNeeded;
            break;
            
        case NdisRequestMethod:
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
            
        default:
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }
    
    return status;
}

//
// MiniportCancelOidRequest - Cancel OID request
//
VOID
MiniportCancelOidRequest(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID RequestId
)
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(RequestId);
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "MiniportCancelOidRequest called");
    
    // OID request cancellation not implemented for Phase 1
}

//
// TimestickFreeAdapter - Free adapter resources
//
VOID
TimestickFreeAdapter(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    if (Adapter == NULL) {
        return;
    }
    
    // Free NET_BUFFER_LIST pool
    if (Adapter->NetBufferListPool != NULL) {
        NdisFreeNetBufferListPool(Adapter->NetBufferListPool);
        Adapter->NetBufferListPool = NULL;
    }
    
    // Clean up USB
    TimestickUsbCleanup(Adapter);
    
    // Free spin locks
    NdisFreeSpinLock(&Adapter->ReceiveLock);
    NdisFreeSpinLock(&Adapter->TransmitLock);
    NdisFreeSpinLock(&Adapter->AdapterLock);
    
    // Free adapter memory
    NdisFreeMemoryWithTagPriority(
        Adapter->AdapterHandle,
        Adapter,
        TIMESTICK_POOL_TAG
    );
}
