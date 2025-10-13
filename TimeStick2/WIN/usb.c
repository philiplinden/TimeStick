/*
 * TimeStick USB Communication Layer
 * Handles all USB communication with ASIX hardware
 */

#include "miniport.h"

//
// TimestickUsbInitialize - Initialize USB device
//
NTSTATUS
TimestickUsbInitialize(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    NTSTATUS status;
    WDF_USB_DEVICE_CREATE_CONFIG usbConfig;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    WDFUSBPIPE pipe;
    WDF_USB_PIPE_INFORMATION pipeInfo;
    UCHAR pipeIndex;
    UCHAR numPipes;
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickUsbInitialize");
    
    // Create USB device
    WDF_USB_DEVICE_CREATE_CONFIG_INIT(&usbConfig,
        USBD_CLIENT_CONTRACT_VERSION_602);
    
    status = WdfUsbTargetDeviceCreateWithParameters(
        Adapter->WdfDevice,
        &usbConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &Adapter->UsbInfo.UsbDevice
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "WdfUsbTargetDeviceCreateWithParameters failed: 0x%08X", status);
        return status;
    }
    
    // Get device information
    WDF_USB_DEVICE_INFORMATION_INIT(&Adapter->UsbInfo.DeviceInfo);
    status = WdfUsbTargetDeviceRetrieveInformation(
        Adapter->UsbInfo.UsbDevice,
        &Adapter->UsbInfo.DeviceInfo
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "WdfUsbTargetDeviceRetrieveInformation failed: 0x%08X", status);
        return status;
    }
    
    // Verify this is an ASIX device
    Adapter->HardwareInfo.VendorId = 
        Adapter->UsbInfo.DeviceInfo.Descriptor.idVendor;
    Adapter->HardwareInfo.ProductId = 
        Adapter->UsbInfo.DeviceInfo.Descriptor.idProduct;
    Adapter->HardwareInfo.BcdDevice = 
        Adapter->UsbInfo.DeviceInfo.Descriptor.bcdDevice;
    
    if (Adapter->HardwareInfo.VendorId != USB_VENDOR_ID_ASIX) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Not an ASIX device: VID=0x%04X", 
            Adapter->HardwareInfo.VendorId);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    // Determine chip version
    if (Adapter->HardwareInfo.ProductId == USB_PRODUCT_ID_AX88279) {
        if (Adapter->HardwareInfo.BcdDevice >= 0x0400) {
            Adapter->HardwareInfo.ChipVersion = AX_VERSION_AX88279;
        } else if (Adapter->HardwareInfo.BcdDevice >= 0x0200) {
            Adapter->HardwareInfo.ChipVersion = AX_VERSION_AX88179A;
        } else {
            Adapter->HardwareInfo.ChipVersion = AX_VERSION_AX88179;
        }
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
        "Found ASIX device: VID=0x%04X PID=0x%04X BCD=0x%04X ChipVer=%d",
        Adapter->HardwareInfo.VendorId,
        Adapter->HardwareInfo.ProductId,
        Adapter->HardwareInfo.BcdDevice,
        Adapter->HardwareInfo.ChipVersion);
    
    // Select configuration
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
    
    status = WdfUsbTargetDeviceSelectConfig(
        Adapter->UsbInfo.UsbDevice,
        WDF_NO_OBJECT_ATTRIBUTES,
        &configParams
    );
    
    if (!NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "WdfUsbTargetDeviceSelectConfig failed: 0x%08X", status);
        return status;
    }
    
    Adapter->UsbInfo.UsbInterface = 
        configParams.Types.SingleInterface.ConfiguredUsbInterface;
    
    // Find bulk IN, bulk OUT, and interrupt pipes
    numPipes = WdfUsbInterfaceGetNumConfiguredPipes(Adapter->UsbInfo.UsbInterface);
    
    for (pipeIndex = 0; pipeIndex < numPipes; pipeIndex++) {
        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
        
        pipe = WdfUsbInterfaceGetConfiguredPipe(
            Adapter->UsbInfo.UsbInterface,
            pipeIndex,
            &pipeInfo
        );
        
        if (pipeInfo.PipeType == WdfUsbPipeTypeBulk) {
            if (WdfUsbTargetPipeIsInEndpoint(pipe)) {
                Adapter->UsbInfo.BulkInPipe = pipe;
                TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
                    "Found bulk IN pipe (EP 0x%02X)", 
                    pipeInfo.EndpointAddress);
            } else if (WdfUsbTargetPipeIsOutEndpoint(pipe)) {
                Adapter->UsbInfo.BulkOutPipe = pipe;
                TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
                    "Found bulk OUT pipe (EP 0x%02X)", 
                    pipeInfo.EndpointAddress);
            }
        } else if (pipeInfo.PipeType == WdfUsbPipeTypeInterrupt) {
            if (WdfUsbTargetPipeIsInEndpoint(pipe)) {
                Adapter->UsbInfo.InterruptPipe = pipe;
                TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, 
                    "Found interrupt pipe (EP 0x%02X)", 
                    pipeInfo.EndpointAddress);
            }
        }
    }
    
    // Verify we found required pipes
    if (Adapter->UsbInfo.BulkInPipe == NULL || 
        Adapter->UsbInfo.BulkOutPipe == NULL) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "Required USB pipes not found");
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "USB initialization complete");
    
    return STATUS_SUCCESS;
}

//
// TimestickUsbCleanup - Clean up USB resources
//
VOID
TimestickUsbCleanup(
    _In_ PTIMESTICK_ADAPTER Adapter
)
{
    TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_INFO, "TimestickUsbCleanup");
    
    // WDF handles cleanup automatically when device is deleted
    Adapter->UsbInfo.UsbDevice = NULL;
    Adapter->UsbInfo.UsbInterface = NULL;
    Adapter->UsbInfo.BulkInPipe = NULL;
    Adapter->UsbInfo.BulkOutPipe = NULL;
    Adapter->UsbInfo.InterruptPipe = NULL;
}

//
// TimestickUsbReadRegister - Read register via USB control transfer
//
NTSTATUS
TimestickUsbReadRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR Command,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
)
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDFMEMORY memory;
    ULONG bytesTransferred;
    
    // Allocate memory for the transfer
    status = WdfMemoryCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        NonPagedPoolNx,
        TIMESTICK_POOL_TAG,
        Length,
        &memory,
        NULL
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Setup control transfer packet for vendor read
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
        &setupPacket,
        BmRequestDeviceToHost,
        BmRequestToDevice,
        Command,
        Value,
        Index
    );
    
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);
    
    // Send synchronous control transfer
    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
        Adapter->UsbInfo.UsbDevice,
        WDF_NO_HANDLE,
        NULL,
        &setupPacket,
        &memoryDescriptor,
        &bytesTransferred
    );
    
    if (NT_SUCCESS(status)) {
        // Copy data to output buffer
        PVOID srcBuffer = WdfMemoryGetBuffer(memory, NULL);
        RtlCopyMemory(Buffer, srcBuffer, Length);
        
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_TRACE, 
            "USB Read: Cmd=0x%02X Val=0x%04X Idx=0x%04X Len=%d",
            Command, Value, Index, Length);
    } else {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "USB Read failed: Cmd=0x%02X Status=0x%08X",
            Command, status);
    }
    
    WdfObjectDelete(memory);
    
    return status;
}

//
// TimestickUsbWriteRegister - Write register via USB control transfer
//
NTSTATUS
TimestickUsbWriteRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR Command,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
)
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDFMEMORY memory;
    ULONG bytesTransferred;
    PVOID destBuffer;
    
    // Allocate memory for the transfer
    status = WdfMemoryCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        NonPagedPoolNx,
        TIMESTICK_POOL_TAG,
        Length,
        &memory,
        &destBuffer
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Copy data to transfer buffer
    RtlCopyMemory(destBuffer, Buffer, Length);
    
    // Setup control transfer packet for vendor write
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
        &setupPacket,
        BmRequestHostToDevice,
        BmRequestToDevice,
        Command,
        Value,
        Index
    );
    
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);
    
    // Send synchronous control transfer
    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
        Adapter->UsbInfo.UsbDevice,
        WDF_NO_HANDLE,
        NULL,
        &setupPacket,
        &memoryDescriptor,
        &bytesTransferred
    );
    
    if (NT_SUCCESS(status)) {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_TRACE, 
            "USB Write: Cmd=0x%02X Val=0x%04X Idx=0x%04X Len=%d",
            Command, Value, Index, Length);
    } else {
        TIMESTICK_DBGPRINT(TIMESTICK_DEBUG_ERROR, 
            "USB Write failed: Cmd=0x%02X Status=0x%08X",
            Command, status);
    }
    
    WdfObjectDelete(memory);
    
    return status;
}

//
// TimestickUsbReadMacRegister - Read MAC register
//
NTSTATUS
TimestickUsbReadMacRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ USHORT Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
)
{
    return TimestickUsbReadRegister(
        Adapter,
        AX_ACCESS_MAC,
        Offset,
        Length,
        Buffer,
        Length
    );
}

//
// TimestickUsbWriteMacRegister - Write MAC register
//
NTSTATUS
TimestickUsbWriteMacRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ USHORT Offset,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ USHORT Length
)
{
    return TimestickUsbWriteRegister(
        Adapter,
        AX_ACCESS_MAC,
        Offset,
        Length,
        Buffer,
        Length
    );
}

//
// TimestickUsbReadPhyRegister - Read PHY register  
//
NTSTATUS
TimestickUsbReadPhyRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR PhyId,
    _In_ UCHAR Location,
    _Out_ PUSHORT Value
)
{
    USHORT data;
    NTSTATUS status;
    
    status = TimestickUsbReadRegister(
        Adapter,
        AX_ACCESS_PHY,
        PhyId,
        Location,
        &data,
        sizeof(data)
    );
    
    if (NT_SUCCESS(status)) {
        *Value = data;
    }
    
    return status;
}

//
// TimestickUsbWritePhyRegister - Write PHY register
//
NTSTATUS
TimestickUsbWritePhyRegister(
    _In_ PTIMESTICK_ADAPTER Adapter,
    _In_ UCHAR PhyId,
    _In_ UCHAR Location,
    _In_ USHORT Value
)
{
    return TimestickUsbWriteRegister(
        Adapter,
        AX_ACCESS_PHY,
        PhyId,
        Location,
        &Value,
        sizeof(Value)
    );
}
