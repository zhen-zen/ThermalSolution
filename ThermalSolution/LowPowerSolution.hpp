//  SPDX-License-Identifier: GPL-2.0-only
//
//  LowPowerSolution.hpp
//  ThermalSolution
//
//  Created by Zhen on 6/14/21.
//

#ifndef LowPowerSolution_hpp
#define LowPowerSolution_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include "common.h"

#define LOW_POWER_S0_UUID    "c4eb40a0-6cd2-11e2-bcfd-0800200c9a66"
#define LPS0_DSM_REVISION    0

#define LPS0_DSM_CAPABILITY  0
#define LPS0_DSM_CONSTRAINT  1
#define LPS0_DSM_CRASH_DUMP  2
#define LPS0_DSM_SCREEN_OFF  3
#define LPS0_DSM_SCREEN_ON   4
#define LPS0_DSM_ENTRY       5
#define LPS0_DSM_EXIT        6

#define kIOPMNumberPowerStates     2

static IOPMPowerState IOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOServicePowerCapabilityOff, kIOServicePowerCapabilityOff, kIOServicePowerCapabilityOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOServicePowerCapabilityOn, kIOServicePowerCapabilityOn, kIOServicePowerCapabilityOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

class LowPowerSolution : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(LowPowerSolution)

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    const char *name;
    IOACPIPlatformDevice *dev {nullptr};

    /**
     * Evaluate _DSM for specific GUID and function index.
     * @param index Function index
     * @param result The return is a buffer containing one bit for each function index if Function Index is zero, otherwise could be any data object (See 9.1.1 _DSM (Device Specific Method) in ACPI Specification, Version 6.3)
     * @param arg argument array
     * @param uuid Human-readable GUID string (big-endian)
     * @param revision _DSM revision
     *
     * @return *kIOReturnSuccess* upon a successfull *_DSM* parse, otherwise failed when executing *evaluateObject*.
     */
    IOReturn evaluateDSM(UInt32 index, OSObject **result, OSArray *arg=nullptr, const char *uuid=LOW_POWER_S0_UUID, UInt32 revision=LPS0_DSM_REVISION);

    UInt8 functionMask {0};

    bool ready {false};

    void setPropertiesGated(OSObject* props);

public:
    bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    IOReturn setProperties(OSObject *props) APPLE_KEXT_OVERRIDE;
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* LowPowerSolution_hpp */
