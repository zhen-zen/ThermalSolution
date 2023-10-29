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

OSString *parse_string(OSData *data) {
  uint32_t size = data->getLength();
  const uint16_t *buf2 = reinterpret_cast<const uint16_t *>(data->getBytesNoCopy());
  if (size % 2 != 0) {
//    errors("invalid size");
    return OSString::withCString("invalid size");
  }
  char *out = reinterpret_cast<char *>(IOMalloc(size+1));
  if (!out) {
//    errors("malloc failed");
    return OSString::withCString("malloc failed");
  }
  bzero(out, size+1);

  uint32_t i, j;
  for (i=0, j=0; i<size/2; ++i) {
    if (buf2[i] == 0) {
      break;
    } else if (buf2[i] < 0x80) {
      out[j++] = buf2[i];
    } else if (buf2[i] < 0x800) {
      out[j++] = 0xC0 | (buf2[i] >> 6);
      out[j++] = 0x80 | (buf2[i] & 0x3F);
    } else if (buf2[i] >= 0xD800 && buf2[i] <= 0xDBFF && i+1 < size/2 && buf2[i+1] >= 0xDC00 && buf2[i+1] <= 0xDFFF) {
      uint32_t c = 0x10000 + ((buf2[i] - 0xD800) << 10) + (buf2[i+1] - 0xDC00);
      ++i;
      out[j++] = 0xF0 | (c >> 18);
      out[j++] = 0x80 | ((c >> 12) & 0x3F);
      out[j++] = 0x80 | ((c >> 6) & 0x3F);
      out[j++] = 0x80 | (c & 0x3F);
    } else if (buf2[i] >= 0xD800 && buf2[i] <= 0xDBFF && i+1 < size/2 && buf2[i+1] >= 0xDC00 && buf2[i+1] <= 0xDFFF) {
      uint32_t c = 0x10000 + ((buf2[i] - 0xD800) << 10) + (buf2[i+1] - 0xDC00);
      ++i;
      out[j++] = 0xF0 | (c >> 18);
      out[j++] = 0x80 | ((c >> 12) & 0x3F);
      out[j++] = 0x80 | ((c >> 6) & 0x3F);
      out[j++] = 0x80 | (c & 0x3F);
    } else {
      out[j++] = 0xE0 | (buf2[i] >> 12);
      out[j++] = 0x80 | ((buf2[i] >> 6) & 0x3F);
      out[j++] = 0x80 | (buf2[i] & 0x3F);
    }
  }
  out[j] = 0;
  OSString *ret = OSString::withCString(out);
  IOFree(out, size+1);
  return ret;
}

bool SensorSolution::start(IOService *provider) {
    if (!super::start(provider) || !(dev = OSDynamicCast(IOACPIPlatformDevice, provider)))
        return false;

    name = dev->getName();
    DebugLog("Starting");

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add commandGate");
        return false;
    }

    OSObject *raw;
    OSData *data;
    if (dev->evaluateObject("_STR", &raw) == kIOReturnSuccess) {
        if ((data = OSDynamicCast(OSData, raw))) {
//            setProperty("_STR len", data->getLength(), 32);
//            setProperty("raw", raw);
            OSString *res = parse_string(data);
            setProperty("_STR", res);
            OSSafeReleaseNULL(res);
        } else {
            setProperty("_STR", raw);
        }
        OSSafeReleaseNULL(raw);
    }

    if (dev->evaluateInteger("_TMP", &tmp) == kIOReturnSuccess) {
        type = INT3403_TYPE_SENSOR;
    } else if (dev->evaluateInteger("PTYP", &type) != kIOReturnSuccess) {
        return false;
    }

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

void SensorSolution::stop(IOService *provider) {
    DebugLog("Stoping");

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    terminate();
    super::stop(provider);
}

IOReturn SensorSolution::message(UInt32 type, IOService *provider, void *argument) {
    switch (type) {
        case kThermal_getDeviceType:
            *(reinterpret_cast<UInt32 *>(argument)) = this->type;
            break;

        case kThermal_getTemperature:
            if (this->type != INT3403_TYPE_SENSOR)
                break;
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

IOReturn SensorSolution::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SensorSolution::setPropertiesGated), props);
    return kIOReturnSuccess;
}

void SensorSolution::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    if (dict->getObject("update") != nullptr) {
        if (dev->evaluateInteger("_TMP", &tmp) == kIOReturnSuccess) {
            setProperty("_TMP", tmp, 32);
            DebugLog("Evaluated _TMP %d %d", tmp, (tmp - 2732) / 10);
        } else {
            AlwaysLog("Failed to evaluate tmp");
        }
    }
}
