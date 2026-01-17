/*
 * Extended UEFI definitions for standalone EFI applications
 * Self-contained header with common protocols
 * Based on UEFI Specification 2.9
 */

#ifndef _EFI_H_
#define _EFI_H_

#include <stdint.h>
#include <stddef.h>

/*==============================================================================
 * Basic Types
 *============================================================================*/

typedef uint64_t UINTN;
typedef int64_t  INTN;
typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t   INT8;
typedef int16_t  INT16;
typedef int32_t  INT32;
typedef int64_t  INT64;
typedef uint8_t  BOOLEAN;
typedef uint16_t CHAR16;
typedef void     VOID;

typedef UINTN    EFI_STATUS;
typedef VOID*    EFI_HANDLE;
typedef VOID*    EFI_EVENT;
typedef UINT64   EFI_PHYSICAL_ADDRESS;
typedef UINT64   EFI_VIRTUAL_ADDRESS;
typedef UINT64   EFI_LBA;
typedef UINTN    EFI_TPL;

#define IN
#define OUT
#define OPTIONAL
#define CONST const
#define EFIAPI

#define TRUE  1
#define FALSE 0
#ifndef NULL
#define NULL  ((void*)0)
#endif

/*==============================================================================
 * Status Codes
 *============================================================================*/

#define EFI_SUCCESS               0ULL
#define EFI_ERROR_MASK            0x8000000000000000ULL
#define EFI_ERROR(s)              (((INTN)(s)) < 0)

#define EFI_LOAD_ERROR            (EFI_ERROR_MASK | 1)
#define EFI_INVALID_PARAMETER     (EFI_ERROR_MASK | 2)
#define EFI_UNSUPPORTED           (EFI_ERROR_MASK | 3)
#define EFI_BAD_BUFFER_SIZE       (EFI_ERROR_MASK | 4)
#define EFI_BUFFER_TOO_SMALL      (EFI_ERROR_MASK | 5)
#define EFI_NOT_READY             (EFI_ERROR_MASK | 6)
#define EFI_DEVICE_ERROR          (EFI_ERROR_MASK | 7)
#define EFI_WRITE_PROTECTED       (EFI_ERROR_MASK | 8)
#define EFI_OUT_OF_RESOURCES      (EFI_ERROR_MASK | 9)
#define EFI_NOT_FOUND             (EFI_ERROR_MASK | 14)
#define EFI_ACCESS_DENIED         (EFI_ERROR_MASK | 15)
#define EFI_TIMEOUT               (EFI_ERROR_MASK | 18)
#define EFI_ABORTED               (EFI_ERROR_MASK | 21)

/*==============================================================================
 * GUIDs
 *============================================================================*/

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a} }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    { 0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

/*==============================================================================
 * Table Header
 *============================================================================*/

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

/*==============================================================================
 * Memory Types and Descriptors
 *============================================================================*/

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    UINT32                Type;
    EFI_PHYSICAL_ADDRESS  PhysicalStart;
    EFI_VIRTUAL_ADDRESS   VirtualStart;
    UINT64                NumberOfPages;
    UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

/* Memory attribute definitions */
#define EFI_MEMORY_UC            0x0000000000000001ULL
#define EFI_MEMORY_WC            0x0000000000000002ULL
#define EFI_MEMORY_WT            0x0000000000000004ULL
#define EFI_MEMORY_WB            0x0000000000000008ULL
#define EFI_MEMORY_UCE           0x0000000000000010ULL
#define EFI_MEMORY_WP            0x0000000000001000ULL
#define EFI_MEMORY_RP            0x0000000000002000ULL
#define EFI_MEMORY_XP            0x0000000000004000ULL
#define EFI_MEMORY_RUNTIME       0x8000000000000000ULL

/*==============================================================================
 * Reset Types
 *============================================================================*/

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

/*==============================================================================
 * Simple Text Input Protocol
 *============================================================================*/

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    IN BOOLEAN                        ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    OUT EFI_INPUT_KEY                  *Key
);

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET    Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT          WaitForKey;
};

/*==============================================================================
 * Simple Text Output Protocol
 *============================================================================*/

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    INT32   MaxMode;
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN                         ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16                          *String
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16                          *String
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(
    IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN  UINTN                           ModeNumber,
    OUT UINTN                           *Columns,
    OUT UINTN                           *Rows
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           ModeNumber
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           Attribute
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           Column,
    IN UINTN                           Row
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN                         Visible
);

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET               Reset;
    EFI_TEXT_STRING              OutputString;
    EFI_TEXT_TEST_STRING         TestString;
    EFI_TEXT_QUERY_MODE          QueryMode;
    EFI_TEXT_SET_MODE            SetMode;
    EFI_TEXT_SET_ATTRIBUTE       SetAttribute;
    EFI_TEXT_CLEAR_SCREEN        ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR       EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE      *Mode;
};

/* Text colors */
#define EFI_BLACK        0x00
#define EFI_BLUE         0x01
#define EFI_GREEN        0x02
#define EFI_CYAN         0x03
#define EFI_RED          0x04
#define EFI_MAGENTA      0x05
#define EFI_BROWN        0x06
#define EFI_LIGHTGRAY    0x07
#define EFI_DARKGRAY     0x08
#define EFI_LIGHTBLUE    0x09
#define EFI_LIGHTGREEN   0x0A
#define EFI_LIGHTCYAN    0x0B
#define EFI_LIGHTRED     0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW       0x0E
#define EFI_WHITE        0x0F

#define EFI_BACKGROUND_BLACK     0x00
#define EFI_BACKGROUND_BLUE      0x10
#define EFI_BACKGROUND_GREEN     0x20
#define EFI_BACKGROUND_CYAN      0x30
#define EFI_BACKGROUND_RED       0x40
#define EFI_BACKGROUND_MAGENTA   0x50
#define EFI_BACKGROUND_BROWN     0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70

#define EFI_TEXT_ATTR(Foreground, Background) ((Foreground) | ((Background) << 4))

/*==============================================================================
 * Graphics Output Protocol (GOP)
 *============================================================================*/

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32                    Version;
    UINT32                    HorizontalResolution;
    UINT32                    VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK         PixelInformation;
    UINT32                    PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32                               MaxMode;
    UINT32                               Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                                SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                 FrameBufferBase;
    UINTN                                FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    IN  UINT32                                ModeNumber,
    OUT UINTN                                 *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32                       ModeNumber
);

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer OPTIONAL,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN                             SourceX,
    IN UINTN                             SourceY,
    IN UINTN                             DestinationX,
    IN UINTN                             DestinationY,
    IN UINTN                             Width,
    IN UINTN                             Height,
    IN UINTN                             Delta OPTIONAL
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT        Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE       *Mode;
};

/*==============================================================================
 * Boot Services
 *============================================================================*/

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
    IN EFI_ALLOCATE_TYPE        Type,
    IN EFI_MEMORY_TYPE          MemoryType,
    IN UINTN                    Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
);

typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(
    IN EFI_PHYSICAL_ADDRESS Memory,
    IN UINTN                Pages
);

typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
    IN OUT UINTN              *MemoryMapSize,
    IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT UINTN                 *MapKey,
    OUT UINTN                 *DescriptorSize,
    OUT UINT32                *DescriptorVersion
);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    IN EFI_MEMORY_TYPE PoolType,
    IN UINTN           Size,
    OUT VOID           **Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(
    IN VOID *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    IN EFI_GUID *Protocol,
    IN VOID     *Registration OPTIONAL,
    OUT VOID    **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(
    IN EFI_HANDLE ImageHandle,
    IN UINTN      MapKey
);

typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(
    IN UINTN  Timeout,
    IN UINT64 WatchdogCode,
    IN UINTN  DataSize,
    IN CHAR16 *WatchdogData OPTIONAL
);

typedef EFI_STATUS (EFIAPI *EFI_STALL)(
    IN UINTN Microseconds
);

typedef struct {
    EFI_TABLE_HEADER        Hdr;
    /* Task Priority Services */
    VOID                    *RaiseTPL;
    VOID                    *RestoreTPL;
    /* Memory Services */
    EFI_ALLOCATE_PAGES      AllocatePages;
    EFI_FREE_PAGES          FreePages;
    EFI_GET_MEMORY_MAP      GetMemoryMap;
    EFI_ALLOCATE_POOL       AllocatePool;
    EFI_FREE_POOL           FreePool;
    /* Event & Timer Services */
    VOID                    *CreateEvent;
    VOID                    *SetTimer;
    VOID                    *WaitForEvent;
    VOID                    *SignalEvent;
    VOID                    *CloseEvent;
    VOID                    *CheckEvent;
    /* Protocol Handler Services */
    VOID                    *InstallProtocolInterface;
    VOID                    *ReinstallProtocolInterface;
    VOID                    *UninstallProtocolInterface;
    VOID                    *HandleProtocol;
    VOID                    *Reserved;
    VOID                    *RegisterProtocolNotify;
    VOID                    *LocateHandle;
    VOID                    *LocateDevicePath;
    VOID                    *InstallConfigurationTable;
    /* Image Services */
    VOID                    *LoadImage;
    VOID                    *StartImage;
    VOID                    *Exit;
    VOID                    *UnloadImage;
    EFI_EXIT_BOOT_SERVICES  ExitBootServices;
    /* Miscellaneous Services */
    VOID                    *GetNextMonotonicCount;
    EFI_STALL               Stall;
    EFI_SET_WATCHDOG_TIMER  SetWatchdogTimer;
    /* DriverSupport Services */
    VOID                    *ConnectController;
    VOID                    *DisconnectController;
    /* Open and Close Protocol Services */
    VOID                    *OpenProtocol;
    VOID                    *CloseProtocol;
    VOID                    *OpenProtocolInformation;
    /* Library Services */
    VOID                    *ProtocolsPerHandle;
    VOID                    *LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL     LocateProtocol;
    VOID                    *InstallMultipleProtocolInterfaces;
    VOID                    *UninstallMultipleProtocolInterfaces;
    /* 32-bit CRC Services */
    VOID                    *CalculateCrc32;
    /* Miscellaneous Services */
    VOID                    *CopyMem;
    VOID                    *SetMem;
    VOID                    *CreateEventEx;
} EFI_BOOT_SERVICES;

/*==============================================================================
 * Runtime Services
 *============================================================================*/

typedef EFI_STATUS (EFIAPI *EFI_RESET_SYSTEM)(
    IN EFI_RESET_TYPE ResetType,
    IN EFI_STATUS     ResetStatus,
    IN UINTN          DataSize,
    IN VOID           *ResetData OPTIONAL
);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    VOID             *GetTime;
    VOID             *SetTime;
    VOID             *GetWakeupTime;
    VOID             *SetWakeupTime;
    VOID             *SetVirtualAddressMap;
    VOID             *ConvertPointer;
    VOID             *GetVariable;
    VOID             *GetNextVariableName;
    VOID             *SetVariable;
    VOID             *GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
    VOID             *UpdateCapsule;
    VOID             *QueryCapsuleCapabilities;
    VOID             *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

/*==============================================================================
 * System Table
 *============================================================================*/

typedef struct {
    EFI_TABLE_HEADER                  Hdr;
    CHAR16                            *FirmwareVendor;
    UINT32                            FirmwareRevision;
    EFI_HANDLE                        ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL    *ConIn;
    EFI_HANDLE                        ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut;
    EFI_HANDLE                        StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *StdErr;
    EFI_RUNTIME_SERVICES              *RuntimeServices;
    EFI_BOOT_SERVICES                 *BootServices;
    UINTN                             NumberOfTableEntries;
    VOID                              *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif /* _EFI_H_ */
