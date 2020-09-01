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

#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "common.h"

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
    INT3400_THERMAL_MAXIMUM_UUID,
};

static const char *int3400_thermal_uuids[INT3400_THERMAL_MAXIMUM_UUID] = {
    "42A441D6-AE6A-462b-A84B-4A8CE79027D3",
    "3A95C389-E4B8-4629-A526-C52C88626BAE",
    "97C68AE7-15FA-499c-B8C9-5DA81D606E0A",
    "63BE270F-1C11-48FD-A6F7-3AF253FF3E2D",
    "5349962F-71E6-431D-9AE8-0A635B710AEE",
    "9E04115A-AE87-4D1C-9500-0F3E340BFE75",
    "F5A35014-C209-46A4-993A-EB56DE7530A1",
    "6ED722A7-9240-48A5-B479-31EEF723D7CF",
    "16CAF1B7-DD38-40ED-B1C1-1B8A1913D531",
    "BE84BABF-C4D4-403D-B495-3128FD44dAC1",
};

typedef struct __attribute__ ((packed)) {
    uint16_t signature;
    uint16_t headersize;
    uint32_t version;
    uint32_t flags;
} GDDVHeader;

typedef struct __attribute__ ((packed)) {
    uint32_t flag;
    uint32_t length;
} GDDVKeyHeader;

#define type_uint64 4
#define type_string 8

typedef struct __attribute__ ((packed)) {
    uint32_t type;
    uint64_t value;
} uint64Container;

typedef struct __attribute__ ((packed)) {
    uint32_t signature;
    uint64_t version;
} appcHeader;

class ThermalSolution : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(ThermalSolution)

    IOACPIPlatformDevice *dev;
    bool evaluateAvailableMode();
    const char *parsePath(OSDictionary *entry, const char *name, OSDictionary *keyDesc);
    
    void parseAPPC(OSDictionary *keyDesc, const void *data, uint32_t length);
    void parsePPCC(OSDictionary *keyDesc, const void *data, uint32_t length);
    void parsePSVT(OSDictionary *keyDesc, const void *data, uint32_t length);

    bool evaluateGDDV();
    bool evaluateODVP();

public:
    IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* DPTFSolution_hpp */
