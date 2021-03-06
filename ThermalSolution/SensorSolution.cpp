//  SPDX-License-Identifier: GPL-2.0-only
//
//  SensorSolution.cpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/3.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "SensorSolution.hpp"

OSDefineMetaClassAndStructors(SensorSolution, IOService)

bool SensorSolution::start(IOService *provider) {
    if (!super::start(provider) || !(dev = OSDynamicCast(IOACPIPlatformDevice, provider)))
        return false;

    name = dev->getName();
    DebugLog("Starting");

    UInt32 tmp;
    if (dev->evaluateInteger("_TMP", &tmp) == kIOReturnSuccess)
        type = INT3403_TYPE_SENSOR;
    else if (dev->evaluateInteger("PTYP", &type) != kIOReturnSuccess)
        return false;

    switch (type) {
        case INT3403_TYPE_SENSOR:
            setProperty("Type", "Sensor");
            break;

        case INT3403_TYPE_BATTERY:
            setProperty("Type", "Battery");
            break;

        case INT3403_TYPE_CHARGER:
            setProperty("Type", "Charger");
            break;

        default:
            char Unknown[11];
            snprintf(Unknown, 11, "Unknown:%2x", type);
            setProperty("Type", Unknown);
            break;
    }

    if (type == INT3403_TYPE_SENSOR) {
        tz = new ThermalZone(dev);
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

IOReturn SensorSolution::message(UInt32 type, IOService *provider, void *argument) {
    switch (type) {
        case kThermal_getDeviceType:
            *(reinterpret_cast<UInt32 *>(argument)) = this->type;
            break;

        case kThermal_getTemperature:
            if (this->type != INT3403_TYPE_SENSOR)
                break;
            UInt32 tmp;
            if (dev->evaluateInteger("_TMP", &tmp) != kIOReturnSuccess)
                tmp = DEFAULT_TEMPERATURE;
            *(reinterpret_cast<UInt32 *>(argument)) = tmp;
            break;

        case kIOACPIMessageDeviceNotification:
            if (argument) {
                switch (*(UInt32 *) argument) {
                    case INT3403_PERF_CHANGED_EVENT:
                        AlwaysLog("ACPI notification: performance changed");
                        break;

                    case INT3403_PERF_TRIP_POINT_CHANGED:
                        AlwaysLog("ACPI notification: performance trip point changed");
                        break;

                    case INT3403_THERMAL_EVENT:
                        DebugLog("ACPI notification: thermal event");
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
