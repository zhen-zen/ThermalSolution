//  SPDX-License-Identifier: GPL-2.0-only
//
//  ProcessorSolution.hpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/4.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ProcessorSolution_hpp
#define ProcessorSolution_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOService.h>
#include "common.h"
#include "ThermalZone.hpp"

// from linux/drivers/thermal/intel/int340x_thermal/processor_thermal_device.c

#define DEFAULT_TEMPERATURE         0x0BB8
#define PROC_POWER_CAPABILITY_CHANGED    0x83

class ProcessorSolution : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(ProcessorSolution)

    const char *name;
    IOPCIDevice *dev {nullptr};
    IOACPIPlatformDevice *adev {nullptr};

    UInt32 type {0};

    bool evaluateCEUC();
    bool evaluateCLPO();
    bool evaluateTDPL();
    bool evaluatePCCC();
    bool evaluatePPCC();

    ThermalZone *tz {nullptr};

public:
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;

    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* ProcessorSolution_hpp */
