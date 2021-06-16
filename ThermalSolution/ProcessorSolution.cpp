//  SPDX-License-Identifier: GPL-2.0-only
//
//  ProcessorSolution.cpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/4.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ProcessorSolution.hpp"
OSDefineMetaClassAndStructors(ProcessorSolution, IOService)

bool ProcessorSolution::start(IOService *provider) {
    if (!super::start(provider) ||
        !(dev = OSDynamicCast(IOPCIDevice, provider)) ||
        !(adev = OSDynamicCast(IOACPIPlatformDevice, dev->getProperty("acpi-device"))))
        return false;

    name = dev->getName();
    DebugLog("Starting");

    if (adev->evaluateInteger("PTYP", &type) != kIOReturnSuccess ||
        type != INT3401_TYPE_PROCESSOR)
        return false;

    setProperty("Type", "Processor");

    evaluateCEUC();
    evaluateCLPO();
    evaluateTDPL();
    evaluatePCCC();
    evaluatePPCC();

    UInt32 tmp;
    if (adev->evaluateInteger("_TMP", &tmp) == kIOReturnSuccess) {
        tz = new ThermalZone(adev);
        OSDictionary *value = tz->readTrips();
        if (value) {
            setProperty("TZ", value);
            value->release();
        }
    }

    setProperty(kDeliverNotifications, kOSBooleanTrue);
    registerService();
    return true;
}

bool ProcessorSolution::evaluateCEUC() {
    OSObject *result;
    OSArray *package;
    if ((adev->evaluateObject("CEUC", &result) == kIOReturnSuccess) &&
        (package = OSDynamicCast(OSArray, result)))
        setProperty("CEUC", package);
    OSSafeReleaseNULL(result);
    return true;
}

bool ProcessorSolution::evaluateCLPO() {
    OSObject *result;
    OSArray *package;
    if ((adev->evaluateObject("CLPO", &result) == kIOReturnSuccess) &&
        (package = OSDynamicCast(OSArray, result)))
        setProperty("CLPO", package);
    OSSafeReleaseNULL(result);
    return true;
}

bool ProcessorSolution::evaluateTDPL() {
    OSObject *result;
//    OSArray *package;
    if ((adev->evaluateObject("TDPL", &result) == kIOReturnSuccess))// &&
//        (package = OSDynamicCast(OSArray, result)))
        setProperty("TDPL", result);
    OSSafeReleaseNULL(result);
    return true;
}

bool ProcessorSolution::evaluatePCCC() {
    OSObject *result;
    OSArray *package;
    if ((adev->evaluateObject("PCCC", &result) == kIOReturnSuccess) &&
        (package = OSDynamicCast(OSArray, result)))
        setProperty("PCCC", package);
    OSSafeReleaseNULL(result);
    return true;
}

bool ProcessorSolution::evaluatePPCC() {
    OSObject *result;
    OSArray *package;
    if ((adev->evaluateObject("PPCC", &result) == kIOReturnSuccess) &&
        (package = OSDynamicCast(OSArray, result))) {
        OSDictionary *power_limits = OSDictionary::withCapacity(3);
        power_limits->setObject("count", package->getObject(0));
        for (int i=0; i < min(package->getCount(), 2); i++) {
            OSArray *arr;
            OSNumber *index;
            if (!(arr = OSDynamicCast(OSArray, package->getObject(i+1))) ||
                nullptr == (index = OSDynamicCast(OSNumber, arr->getObject(0))) ||
                i != index->unsigned8BitValue())
                continue;
            OSDictionary *power_limit = OSDictionary::withCapacity(6);
            power_limit->setObject("index", arr->getObject(0));
            power_limit->setObject("min_uw", arr->getObject(1));
            power_limit->setObject("max_uw", arr->getObject(2));
            power_limit->setObject("tmin_us", arr->getObject(3));
            power_limit->setObject("tmax_us", arr->getObject(4));
            power_limit->setObject("step_uw", arr->getObject(5));
            char name[11];
            snprintf(name, 11, "processor%1d", i);
            power_limits->setObject(name, power_limit);
            OSSafeReleaseNULL(power_limit);
        }
        setProperty("PPCC", power_limits);
        OSSafeReleaseNULL(power_limits);
    }
    OSSafeReleaseNULL(result);
    return true;
}

IOReturn ProcessorSolution::message(UInt32 type, IOService *provider, void *argument) {
    switch (type) {
        case kThermal_getDeviceType:
            *(reinterpret_cast<UInt32 *>(argument)) = this->type;
            break;

        case kThermal_getTemperature:
            if (this->type != INT3403_TYPE_SENSOR)
                break;
            UInt32 tmp;
            if (adev->evaluateInteger("_TMP", &tmp) != kIOReturnSuccess)
                tmp = DEFAULT_TEMPERATURE;
            *(reinterpret_cast<UInt32 *>(argument)) = tmp;
            break;

        case kIOACPIMessageDeviceNotification:
            if (argument) {
                switch (*(UInt32 *) argument) {
                    case PROC_POWER_CAPABILITY_CHANGED:
                        AlwaysLog("ACPI notification: processor power capability changed");
                        break;

                    default:
                        AlwaysLog("Unknown ACPI notification: argument=0x%04x", *((UInt32 *) argument));
                        break;
                }
            }
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s", type, provider->getName());
            break;
    }

    return kIOReturnSuccess;
}
