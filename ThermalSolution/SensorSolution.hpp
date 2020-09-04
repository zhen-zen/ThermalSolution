//  SPDX-License-Identifier: GPL-2.0-only
//
//  SensorSolution.hpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/3.
//

#ifndef SensorSolution_hpp
#define SensorSolution_hpp

#include <common.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOService.h>

// from linux/drivers/thermal/intel/int340x_thermal/int3403_thermal.c

#define INT3403_PERF_CHANGED_EVENT        0x80
#define INT3403_PERF_TRIP_POINT_CHANGED   0x81
#define INT3403_THERMAL_EVENT             0x90
#define INT3403_TEMPERATURE_INDICATION    0x91

#define DEFAULT_TEMPERATURE         0x0BB8

class SensorSolution : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(SensorSolution)

    const char *name;
    IOACPIPlatformDevice *dev {nullptr};

    UInt32 type {0};

public:
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;

    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* SensorSolution_hpp */
