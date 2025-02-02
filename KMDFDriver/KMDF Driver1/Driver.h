#ifndef DRIVER_H
#define DRIVER_H

#define _NTIFS_INCLUDED_
#include <ntddk.h>
#include <ntifs.h>
#include <ntddser.h>


#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define SET_SERIAL_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CONFIGURE_SERIAL_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_RELEASE_DRIVER_HANDLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_STOP_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEVICE_RECEIVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _SERIAL_CONFIG {
    INT32 BaudRate;
    UCHAR WordLength;
    UCHAR StopBits;
    UCHAR Parity;
} SERIAL_CONFIG, * PSERIAL_CONFIG;


UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\devicezavrsni");
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\devicelinkzavrsni");

PUCHAR SerialPortBaseAddress;
PKEVENT StopThreadEvent;
PETHREAD ReceiveThreadHandle;
INT32 ReceivedDataLength;
PDEVICE_OBJECT DeviceObject;
PDEVICE_OBJECT SerialPortDeviceObject;
PFILE_OBJECT SerialPortFileObject;
WCHAR SerialPortNameBuffer[256];
UNICODE_STRING SerialPortName;


NTSTATUS HandleReleaseDriverHandle(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchPassTrue(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS ReleaseSerialPortDeviceObject();
NTSTATUS GetSerialPortDeviceObject();
NTSTATUS ConfigureSerialPort(PDEVICE_OBJECT SerialDeviceObject, PFILE_OBJECT SerialFileObject, PSERIAL_CONFIG SerialConfig);
NTSTATUS SendDataToSerialPort(PDEVICE_OBJECT SerialDeviceObject, PFILE_OBJECT SerialFileObject, const char* Buffer, ULONG Length);
NTSTATUS DispatchDevCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID Unload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
#endif 
