//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThermalSolution.hpp
//  ThermalSolution
//
//  Created by Zhen on 2020/8/28.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThermalSolution_hpp
#define ThermalSolution_hpp

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "common.h"
#include "ThermalZone.hpp"

#define DPTF_OSC_REVISION 1

// from linux/include/linux/acpi.h

/* Indexes into _OSC Capabilities Buffer (DWORDs 2 & 3 are device-specific) */
#define OSC_QUERY_DWORD              0    /* DWORD 1 */
#define OSC_SUPPORT_DWORD            1    /* DWORD 2 */
#define OSC_CONTROL_DWORD            2    /* DWORD 3 */

/* _OSC Capabilities DWORD 1: Query/Control and Error Returns (generic) */
#define OSC_QUERY_ENABLE             0x00000001  /* input */
#define OSC_REQUEST_ERROR            0x00000002  /* return */
#define OSC_INVALID_UUID_ERROR       0x00000004  /* return */
#define OSC_INVALID_REVISION_ERROR   0x00000008  /* return */
#define OSC_CAPABILITIES_MASK_ERROR  0x00000010  /* return */

// from linux/drivers/thermal/intel/int340x_thermal/int3400_thermal.c

#define INT3400_THERMAL_TABLE_CHANGED 0x83
#define INT3400_ODVP_CHANGED 0x88

enum int3400_thermal_uuid {
    INT3400_THERMAL_PASSIVE_1,
    INT3400_THERMAL_ACTIVE,
    INT3400_THERMAL_CRITICAL,
    INT3400_THERMAL_ADAPTIVE_PERFORMANCE,
    INT3400_THERMAL_EMERGENCY_CALL_MODE,
    INT3400_THERMAL_PASSIVE_2,
    INT3400_THERMAL_POWER_BOSS,
    INT3400_THERMAL_VIRTUAL_SENSOR,
    INT3400_THERMAL_COOLING_MODE,
    INT3400_THERMAL_HARDWARE_DUTY_CYCLING,
    INT3400_THERMAL_ACTIVE_2,
    INT3400_THERMAL_MAXIMUM_UUID,
};

static const char *int3400_thermal_uuids[INT3400_THERMAL_MAXIMUM_UUID] = {
    "42A441D6-AE6A-462b-A84B-4A8CE79027D3",     // DPSP
    "3A95C389-E4B8-4629-A526-C52C88626BAE",     // DASP
    "97C68AE7-15FA-499c-B8C9-5DA81D606E0A",     // DCSP
    "63BE270F-1C11-48FD-A6F7-3AF253FF3E2D",     // DAPP
    "5349962F-71E6-431D-9AE8-0A635B710AEE",
    "9E04115A-AE87-4D1C-9500-0F3E340BFE75",     // DP2P
    "F5A35014-C209-46A4-993A-EB56DE7530A1",     // POBP
    "6ED722A7-9240-48A5-B479-31EEF723D7CF",     // DVSP
    "16CAF1B7-DD38-40ED-B1C1-1B8A1913D531",     // DMSP
    "BE84BABF-C4D4-403D-B495-3128FD44dAC1",     // HDCP
    "0e56fab6-bdfc-4e8c-8246-40ecfd4d74ea",     // DA2P
};

// 42496e14-bc1b-46e8-a798-ca915464426f // DPID
// a01dbc39-a15a-4915-a215-9324b4c03366 // DGPS
// b9455b06-7949-40c6-abf2-363a70c8706c // LPSP
// e145970a-e4c1-4d73-900e-c9c5a69dd067 // CTSP

// from intel/thermal_daemon/src/thd_engine_adaptive.h

enum adaptive_condition {
    Default = 0x01,
    Orientation,
    Proximity,
    Motion,
    Dock,
    Workload,
    Cooling_mode,
    Power_source,
    Aggregate_power_percentage,
    Lid_state,
    Platform_type,
    Platform_SKU,
    Utilisation,
    TDP,
    Duty_cycle,
    Power,
    Temperature,
    Display_orientation,
    Oem0,
    Oem1,
    Oem2,
    Oem3,
    Oem4,
    Oem5,
    PMAX,
    PSRC,
    ARTG,
    CTYP,
    PROP,
    Unk1,
    Unk2,
    Battery_state,
    Battery_rate,
    Battery_remaining,
    Battery_voltage,
    PBSS,
    Battery_cycles,
    Battery_last_full,
    Power_personality,
    Battery_design_capacity,
    Screen_state,
    AVOL,
    ACUR,
    AP01,
    AP02,
    AP10,
    Time,
    Temperature_without_hysteresis,
    Mixed_reality,
    User_presence,
    RBHF,
    VBNL,
    CMPP,
    Battery_percentage,
    Battery_count,
    Power_slider
};

static const char *condition_names[] = {
        "Invalid",
        "Default",
        "Orientation",
        "Proximity",
        "Motion",
        "Dock",
        "Workload",
        "Cooling_mode",
        "Power_source",
        "Aggregate_power_percentage",
        "Lid_state",
        "Platform_type",
        "Platform_SKU",
        "Utilisation",
        "TDP",
        "Duty_cycle",
        "Power",
        "Temperature",
        "Display_orientation",
        "Oem0",
        "Oem1",
        "Oem2",
        "Oem3",
        "Oem4",
        "Oem5",
        "PMAX",
        "PSRC",
        "ARTG",
        "CTYP",
        "PROP",
        "Unk1",
        "Unk2",
        "Battery_state",
        "Battery_rate",
        "Battery_remaining",
        "Battery_voltage",
        "PBSS",
        "Battery_cycles",
        "Battery_last_full",
        "Power_personality",
        "Battery_design_capacity",
        "Screen_state",
        "AVOL",
        "ACUR",
        "AP01",
        "AP02",
        "AP10",
        "Time",
        "Temperature_without_hysteresis",
        "Mixed_reality",
        "User_presence",
        "RBHF",
        "VBNL",
        "CMPP",
        "Battery_percentage",
        "Battery_count",
        "Power_slider"
};

static const char *comp_strs[] = {
        "INVALID",
        "ADAPTIVE_EQUAL",
        "ADAPTIVE_LESSER_OR_EQUAL",
        "ADAPTIVE_GREATER_OR_EQUAL"
};

enum adaptive_operation {
    AND = 0x01,
    FOR
};

enum adaptive_comparison {
    ADAPTIVE_EQUAL = 0x01,
    ADAPTIVE_LESSER_OR_EQUAL,
    ADAPTIVE_GREATER_OR_EQUAL,
};

// From Common/esif_sdk_iface_esif.h:
#define ESIF_SERVICE_CONFIG_COMPRESSED  0x40000000/* Payload is Compressed */
//From ESIF/Products/ESIF_LIB/Sources/esif_lib_datavault.c
#define ESIFDV_HEADER_SIGNATURE            0x1FE5
#define ESIFDV_ITEM_KEYS_REV0_SIGNATURE    0xA0D8

/* From esif_lilb_datavault.h */
#define ESIFDV_NAME_LEN                32    // Max DataVault Name (Cache Name) Length (not including NULL)
#define ESIFDV_DESC_LEN                64    // Max DataVault Description Length (not including NULL)

#define SHA256_HASH_BYTES            32

typedef struct __attribute__ ((packed)) {
    uint16_t signature;
    uint16_t headersize;
    union {
        uint32_t raw;
        struct {
            uint16_t revision;
            uint8_t  minor;
            uint8_t  major;
        };
    } version;
    union {
        /* Added in V1 */
        struct {
            uint32_t flags;
        } v1;

        /* Added in V2 */
        struct {
            uint32_t flags;
            char     segmentid[ESIFDV_NAME_LEN];
            char     comment[ESIFDV_DESC_LEN];
            uint8_t  payload_hash[SHA256_HASH_BYTES];
            uint32_t payload_size;
            uint32_t payload_class;
        } v2;
    };
} GDDVHeader;

typedef struct __attribute__ ((packed)) {
    uint32_t flag;
    uint32_t length;
} GDDVKeyHeader;

/* From esif_sdk_data_type.h */
typedef enum esif_data_type {
    ESIF_DATA_ANGLE = 41,
    ESIF_DATA_AUTO = 36,
    ESIF_DATA_BINARY = 7,
    ESIF_DATA_BLOB = 34,
    ESIF_DATA_DECIBEL = 39,
    ESIF_DATA_DSP = 33,
    ESIF_DATA_ENUM = 19,
    ESIF_DATA_FREQUENCY = 40,
    ESIF_DATA_GUID = 5,
    ESIF_DATA_HANDLE = 20,
    ESIF_DATA_INSTANCE = 30,
    ESIF_DATA_INT16 = 12,
    ESIF_DATA_INT32 = 13,
    ESIF_DATA_INT64 = 14,
    ESIF_DATA_INT8 = 11,
    ESIF_DATA_IPV4 = 16,
    ESIF_DATA_IPV6 = 17,
    ESIF_DATA_JSON = 42,
    ESIF_DATA_PERCENT = 29,
    ESIF_DATA_POINTER = 18,
    ESIF_DATA_POWER = 26,
    ESIF_DATA_QUALIFIER = 28,
    ESIF_DATA_REGISTER = 15,
    ESIF_DATA_STRING = 8,
    ESIF_DATA_STRUCTURE = 32,
    ESIF_DATA_TABLE = 35,
    ESIF_DATA_TEMPERATURE = 6,
    ESIF_DATA_TIME = 31,
    ESIF_DATA_UINT16 = 2,
    ESIF_DATA_UINT32 = 3,
    ESIF_DATA_UINT64 = 4,
    ESIF_DATA_UINT8 = 1,
    ESIF_DATA_UNICODE = 9,
    ESIF_DATA_VOID = 24,
    ESIF_DATA_XML = 38,
} esif_data_type_t;

typedef struct __attribute__ ((packed)) {
    uint32_t type;
    uint64_t value;
} uint64Container;

typedef struct __attribute__ ((packed)) {
    uint32_t type;
    uint64_t length;
    uuid_t guid;
} guidContainer;


class ThermalSolution : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(ThermalSolution)

    const char *name;
    IOACPIPlatformDevice *dev {nullptr};

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    IONotifier* _publishNotify {nullptr};
    IONotifier* _terminateNotify {nullptr};
    OSSet* _notificationServices {nullptr};
    OSDictionary* _SensorServices {nullptr};
    const OSSymbol* _deliverNotification {nullptr};

    void dispatchMessage(int message, void* data);
    void dispatchMessageGated(int* message, void* data);
    bool notificationHandler(void * refCon, IOService * newService, IONotifier * notifier);
    void notificationHandlerGated(IOService * newService, IONotifier * notifier);

    bool evaluateAvailableMode();
    uint32_t uuid_bitmap {0};
    bool changeMode(int i, bool enable);

    OSDictionary *parsePath(OSDictionary *entry, const char *&path);
    
    OSDictionary *parseAPAT(const void *data, uint32_t length);
    OSDictionary *parseAPCT(const void *data, uint32_t length);
    OSDictionary *parseAPPC(const void *data, uint32_t length);
    OSDictionary *parsePPCC(const void *data, uint32_t length);
    OSDictionary *parsePSVT(const void *data, uint32_t length);
    OSDictionary *parseIDSP(const void *data, uint32_t length);
    OSDictionary *parseBinary(const void *data, uint32_t length);

    bool evaluateGDDV();
    bool evaluateODVP();

    ThermalZone *tz {nullptr};

    void setPropertiesGated(OSObject* props);

public:
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    IOReturn setProperties(OSObject *props) APPLE_KEXT_OVERRIDE;
};
#endif /* DPTFSolution_hpp */
