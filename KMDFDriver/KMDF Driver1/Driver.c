#include "Driver.h"

NTSTATUS DispatchPassTrue(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN alreadyProcessed = FALSE;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    switch (irpsp->MajorFunction) {
    case IRP_MJ_CREATE:
        KdPrint(("Primljen zahtjev za stvaranje\n"));
        status = STATUS_SUCCESS;
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;
    case IRP_MJ_CLOSE:
        KdPrint(("Primljen zahtjev za zatvaranje\n"));
        status = STATUS_SUCCESS;
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;
    }

    return status;
 }


NTSTATUS ReleaseSerialPortDeviceObject() {
    if (SerialPortFileObject != NULL) {
        ObDereferenceObject(SerialPortFileObject);
        SerialPortFileObject = NULL;
    }
    if (SerialPortDeviceObject != NULL)
        SerialPortDeviceObject = NULL;
    return STATUS_SUCCESS;
}


NTSTATUS GetSerialPortDeviceObject() {
    PDEVICE_OBJECT deviceObject = NULL;
    PFILE_OBJECT fileObject = NULL;
    NTSTATUS status;

    if (SerialPortName.Length == 0) {
        KdPrint(("Serial port name is not set.\n"));
        return STATUS_INVALID_PARAMETER;
    }


    ReleaseSerialPortDeviceObject();

    status = IoGetDeviceObjectPointer(&SerialPortName, FILE_ALL_ACCESS, &fileObject, &deviceObject);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to get device object pointer for %wZ, status: %x\n", &SerialPortName, status));
        SerialPortFileObject = NULL;
        SerialPortDeviceObject = NULL;
        return status;
    }

    SerialPortFileObject = fileObject;
    SerialPortDeviceObject = deviceObject;

    KdPrint(("Successfully obtained PDEVICE_OBJECT and PFILE_OBJECT for %wZ\n", &SerialPortName));
    return STATUS_SUCCESS;
}



NTSTATUS ConfigureSerialPort(PDEVICE_OBJECT SerialDeviceObject, PFILE_OBJECT SerialFileObject, PSERIAL_CONFIG SerialConfig) {
    NTSTATUS status;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    SERIAL_BAUD_RATE baudRate;
    SERIAL_LINE_CONTROL lineControl;
    SERIAL_HANDFLOW flowControl;
    SERIAL_CHARS serialChars;
    SERIAL_TIMEOUTS timeouts;

    RtlZeroMemory(&timeouts, sizeof(SERIAL_TIMEOUTS));
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;  
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;  

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_TIMEOUTS, SerialDeviceObject, &timeouts, sizeof(timeouts), NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        KdPrint(("IOCTL_SERIAL_SET_TIMEOUTS: Failed to allocate IRP for setting timeouts\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        KdPrint(("IOCTL_SERIAL_SET_TIMEOUTS: Neuspjesno postavljen timeout za serijski port, status: %x\n", status));
        return status;
    }
    baudRate.BaudRate = SerialConfig->BaudRate;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_BAUD_RATE, SerialDeviceObject, &baudRate, sizeof(baudRate), NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        KdPrint(("IOCTL_SERIAL_SET_BAUD_RATE: neuspjesno postavljen seriski port, status: %x\n", status));
        return status;
    }

    lineControl.WordLength = SerialConfig->WordLength;
    lineControl.StopBits = SerialConfig->StopBits;
    lineControl.Parity = SerialConfig->Parity;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_LINE_CONTROL, SerialDeviceObject, &lineControl, sizeof(lineControl), NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        KdPrint(("IOCTL_SERIAL_SET_LINE_CONTROL: neuspjesno postavljen seriski port"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        KdPrint(("IOCTL_SERIAL_SET_LINE_CONTROL: Neuspjesno postavljen line control, status: %x\n", status));
        return status;
    }

    // Postavljanje RTS
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_RTS, SerialDeviceObject, NULL, 0, NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }
    KdPrint(("IOCTL_SERIAL_SET_RTS: Successfully set RTS\n"));

    //DTS
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_DTR, SerialDeviceObject, NULL, 0, NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }
    KdPrint(("IOCTL_SERIAL_SET_DTR: Successfully set DTR\n"));


    RtlZeroMemory(&flowControl, sizeof(SERIAL_HANDFLOW));
    flowControl.ControlHandShake = SERIAL_DTR_CONTROL; 
    flowControl.FlowReplace = SERIAL_AUTO_TRANSMIT | SERIAL_AUTO_RECEIVE | SERIAL_RTS_CONTROL; 

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_HANDFLOW, SerialDeviceObject, &flowControl, sizeof(flowControl), NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(&serialChars, sizeof(SERIAL_CHARS));
    serialChars.EofChar = 94;
    serialChars.ErrorChar = 2;
    serialChars.BreakChar = 2;
    serialChars.EventChar = 0;
    serialChars.XonChar = 7;
    serialChars.XoffChar = 0;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SERIAL_SET_CHARS, SerialDeviceObject, &serialChars, sizeof(serialChars), NULL, 0, FALSE, &event, &ioStatus);
    if (irp == NULL) {
        KdPrint(("IOCTL_SERIAL_SET_CHARS: Neuspjesno postavljanje IRP\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        KdPrint(("IOCTL_SERIAL_SET_CHARS: Neuspjesno postavljanje serijskih znakova, status: %x\n", status));
        return status;
    }
    return status;
}

NTSTATUS SendDataToSerialPort(PDEVICE_OBJECT SerialDeviceObject, PFILE_OBJECT SerialFileObject, const char* Buffer, ULONG Length) {
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;
    KMUTEX mutex;
    KeInitializeMutex(&mutex, 0);
    char* buffer = ExAllocatePoolWithTag(NonPagedPool, Length + 2, 'tag1');
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(buffer, Buffer, Length);
    buffer[Length] = '\n';
    buffer[Length + 1] = '\0';
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    KeWaitForSingleObject(&mutex, Executive, KernelMode, FALSE, NULL);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, SerialDeviceObject, buffer, Length + 2, NULL, &event, &ioStatus);
    if (irp == NULL) {
        KeReleaseMutex(&mutex, FALSE);
        ExFreePoolWithTag(buffer, 'tag1');
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        if (!NT_SUCCESS(status)) {
            IoCancelIrp(irp);
        }
        else {
            status = ioStatus.Status;
        }
    }
    if (buffer) {
        ExFreePoolWithTag(buffer, 'tag1');
        buffer = NULL;  
    }
    KeReleaseMutex(&mutex, FALSE);

    return status;
}


NTSTATUS ReceiveDataFromUsb(PDEVICE_OBJECT SerialDeviceObject, ULONG Length, char* Buffer) {
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;
    char* buffer;

    buffer = (char*)ExAllocatePoolWithTag(NonPagedPool, Length, 'tag1');
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    LARGE_INTEGER offset;
    offset.QuadPart = 3;
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, SerialDeviceObject, buffer, Length, &offset , &event, &ioStatus);
    if (irp == NULL) {
        ExFreePoolWithTag(buffer, 'tag1');
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = IoCallDriver(SerialDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to receive data from USB device, status: 0x%08X\n", status));
    }
    else {
        KdPrint(("Primljeni podatci sa USB uredaja: %s\n", buffer));
        RtlCopyMemory(Buffer, buffer, ioStatus.Information);
        Buffer[ioStatus.Information] = '\0';  
    }
    if (buffer) {
        ExFreePoolWithTag(buffer, 'tag1');
    }

    return status;
}





NTSTATUS DispatchDevCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS status = STATUS_SUCCESS;

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
    case DEVICE_SEND:
        if (buffer != NULL && inLength > 0 && SerialPortDeviceObject != NULL && SerialPortFileObject != NULL) {
            status = SendDataToSerialPort(SerialPortDeviceObject, SerialPortFileObject, buffer, inLength);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    case SET_SERIAL_PORT:
        if (buffer != NULL && inLength > 0 && inLength < sizeof(SerialPortNameBuffer)) {
            RtlCopyMemory(SerialPortNameBuffer, buffer, inLength);
            SerialPortNameBuffer[inLength / sizeof(WCHAR)] = UNICODE_NULL;
            RtlInitUnicodeString(&SerialPortName, SerialPortNameBuffer);
            status = GetSerialPortDeviceObject();
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    case CONFIGURE_SERIAL_PORT:
        if (buffer != NULL && inLength >= sizeof(SERIAL_CONFIG)) {
            status = ConfigureSerialPort(SerialPortDeviceObject, SerialPortFileObject, (PSERIAL_CONFIG)buffer);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    case IOCTL_RELEASE_DRIVER_HANDLE:
        status = HandleReleaseDriverHandle(DeviceObject, Irp);
        break;
    case IOCTL_STOP_THREAD:
        if (StopThreadEvent != NULL) {
            KeSetEvent(StopThreadEvent, 0, FALSE);
        }
        if (SymLinkName.Length != 0) {
            status = IoDeleteSymbolicLink(&SymLinkName);
            if (!NT_SUCCESS(status)) {
                break;
            }
            else
                KdPrint(("IOCTL_STOP_THREAD: Obrisan SymLinkName\n"));
        }
        status = ReleaseSerialPortDeviceObject();
        if (!NT_SUCCESS(status)) {
            break;
        }
        break;
    case DEVICE_RECEIVE:
        if (outLength > 0 && SerialPortDeviceObject != NULL) {

            status = ReceiveDataFromUsb(SerialPortDeviceObject, outLength, (char*)buffer);

            if (NT_SUCCESS(status)) {
              
                Irp->IoStatus.Information = outLength; 
            }
            else {
                Irp->IoStatus.Information = 0;
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
        }
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS HandleReleaseDriverHandle(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    KdPrint(("HandleReleaseDriverHandle\n"));

    NTSTATUS status = STATUS_SUCCESS;

    if (SymLinkName.Length != 0) {
        status = IoDeleteSymbolicLink(&SymLinkName);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Neuspjesno obrisan SymLinkName\n"));
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }
        else {
            KdPrint(("IoDeleteSymbolicLink\n"));
        }
    }

    status = ReleaseSerialPortDeviceObject();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Neuspjesno oslobodjen PDEVICE_OBJECT i PFILE_OBJECT\n"));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DeviceObject);
        KdPrint(("Neuspjesno kreiranje SymLinkName\n"));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    else {
        KdPrint(("Uspjesno kreiranje SymLinkName\n"));
    }

    return status;
    
}

VOID Unload(_In_ PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    KdPrint(("Driver unloading\n"));

    if (SerialPortFileObject != NULL) {
        ObDereferenceObject(SerialPortFileObject);
        SerialPortFileObject = NULL;
    }
    if (SerialPortDeviceObject != NULL) {
        ObDereferenceObject(SerialPortDeviceObject);
        SerialPortDeviceObject = NULL;
    }

    if (SymLinkName.Length != 0) {
        KdPrint(("Deleting symbolic link\n"));
        IoDeleteSymbolicLink(&SymLinkName);
    }

    if (DeviceObject != NULL) {

        IoDeleteDevice(DeviceObject);
  
        DeviceObject = NULL;
    }

    KdPrint(("Driver unloaded\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;
    HANDLE threadHandle;

    status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Failed to create device\n"));
        return status;
    }

    status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(DeviceObject);
        return status;
    }

    for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DispatchPassTrue;
    }
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevCTL;
    DriverObject->DriverUnload = Unload;

    KdPrint(("Driver loaded\n"));
    return STATUS_SUCCESS;
}

//g_StopThreadEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'Stop');
//if (stopThreadEvent == NULL) {
//    IoDeleteSymbolicLink(&SymLinkName);
//    IoDeleteDevice(DeviceObject);
//    return STATUS_INSUFFICIENT_RESOURCES;
//}
//KeInitializeEvent(stopThreadEvent, NotificationEvent, FALSE);


//// Kreiranje niti za primanje podataka
//status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS , NULL, NULL, NULL, receiveThreadFunction, NULL);
//if (!NT_SUCCESS(status)) {
//    ExFreePoolWithTag(stopThreadEvent, 'Stop');
//    IoDeleteSymbolicLink(&SymLinkName);
//    IoDeleteDevice(DeviceObject);
//    return status;
//}

//status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&receiveThreadHandle, NULL);
//if (!NT_SUCCESS(status)) {
//    ZwClose(threadHandle);
//    ExFreePoolWithTag(stopThreadEvent, 'Stop');
//    IoDeleteSymbolicLink(&SymLinkName);
//    IoDeleteDevice(DeviceObject);
//    return status;
//}
//ZwClose(threadHandle);




