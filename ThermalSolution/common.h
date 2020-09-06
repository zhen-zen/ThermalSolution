//  SPDX-License-Identifier: GPL-2.0-only
//
//  common.h
//  YogaSMC
//
//  Created by Zhen on 8/23/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef common_h
#define common_h

#include <IOKit/IOMessage.h>

#define kDeliverNotifications   "ThermalNotifications"

#define INT3401_TYPE_PROCESSOR      0x00 // aka B0D4
#define INT3403_TYPE_SENSOR         0x03
#define INT3403_TYPE_CHARGER        0x0B
#define INT3403_TYPE_BATTERY        0x0C
#define INT340D_TYPE_DGFX_CORE      0x24
#define INT340D_TYPE_DGFX_MEM       0x25
#define INT3530_TYPE_MULTI_CHIP     0x26

enum
{
    // Thermal message types
    kThermal_getDeviceType  = iokit_vendor_specific_msg(900),   // get temperature from sensor (data is UInt32*)
    kThermal_getTemperature = iokit_vendor_specific_msg(901),   // get temperature from sensor (data is UInt32*)
};

#ifdef DEBUG
#define DebugLog(str, ...) do { IOLog("%s::%s " str, getName(), name, ## __VA_ARGS__); } while (0)
#else
#define DebugLog(str, ...) do { } while (0)
#endif
#define AlwaysLog(str, ...) do { IOLog("%s::%s " str, getName(), name, ## __VA_ARGS__); } while (0)

#define ARRAY_SIZE(names) (sizeof(names)/sizeof((names)[0]))

#define BIT(nr) (1U << (nr))

#define getPropertyBoolean(prompt)     \
    do { \
        value = OSDynamicCast(OSBoolean, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define getPropertyNumber(prompt)     \
    do { \
        value = OSDynamicCast(OSNumber, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define getPropertyString(prompt)     \
    do { \
        value = OSDynamicCast(OSString, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define setPropertyBoolean(dict, name, boolean) \
    do { dict->setObject(name, boolean ? kOSBooleanTrue : kOSBooleanFalse); } while (0)

// define a OSNumber(OSObject) *value before use
#define setPropertyNumber(dict, name, number, bits) \
    do { \
        value = OSNumber::withNumber(number, bits); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#define setPropertyString(dict, name, str) \
    do { \
        value = OSString::withCString(str); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#define setPropertyBytes(dict, name, data, len) \
    do { \
        value = OSData::withBytes(data, len); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#define setPropertyTemp(dict, name, number) \
    do { \
        char temp_str[10]; \
        snprintf(temp_str, 10, "%d.%d℃", number / 10, number % 10); \
        setPropertyString(dict, name, temp_str); \
    } while (0)

#endif /* common_h */
