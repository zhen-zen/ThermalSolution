//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThermalSolution.cpp
//  ThermalSolution
//
//  Created by Zhen on 2020/8/28.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThermalSolution.hpp"
#include "thd_lzma_dec.h"

OSDefineMetaClassAndStructors(ThermalSolution, IOService)

OSString *parseDeciKelvin(uint32_t raw) {
    char temp_str[10];
    if (raw == 0xFFFFFFFF)
        return OSString::withCString("Invalid");

    SInt32 celsius = acpi_deci_kelvin_to_deci_celsius(raw);
    snprintf(temp_str, 10, "%d.%d℃", celsius / 10, celsius % 10);
    return (OSString::withCString(temp_str));
}

bool ThermalSolution::start(IOService *provider) {
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

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
    _notificationServices = OSSet::withCapacity(1);
    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch) {
      IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &ThermalSolution::notificationHandler);

      //
      // Register notifications for availability of any IOService objects wanting to consume our message events
      //
      _publishNotify = addMatchingNotification(gIOFirstPublishNotification,
                                             propertyMatch,
                                             notificationHandler,
                                             this,
                                             0, 10000);

      _terminateNotify = addMatchingNotification(gIOTerminatedNotification,
                                               propertyMatch,
                                               notificationHandler,
                                               this,
                                               0, 10000);

      propertyMatch->release();
    }

    /* Missing IDSP isn't fatal */
    evaluateAvailableMode();
    evaluateGDDV();
    evaluateODVP();

    UInt32 tmp;
    if (dev->evaluateInteger("_TMP", &tmp) == kIOReturnSuccess) {
        tz = new ThermalZone(dev);
        OSDictionary *value = tz->readTrips();
        if (value) {
            setProperty("TZ", value);
            value->release();
        }
    }

    registerService();
    return true;
}

void ThermalSolution::stop(IOService *provider) {
    DebugLog("Stoping");

    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    terminate();
    super::stop(provider);
}

bool ThermalSolution::evaluateAvailableMode() {
    OSObject *result = nullptr;
    if (dev->evaluateObject("IDSP", &result) != kIOReturnSuccess) {
        OSSafeReleaseNULL(result);
        return false;
    }

    OSArray *uuid = OSDynamicCast(OSArray, result);
    OSDictionary *mode = OSDictionary::withCapacity(1);
    OSString *value;
    OSData *entry;

    for (int i = 0; i < uuid->getCount(); i++) {
        if (!(entry = OSDynamicCast(OSData, uuid->getObject(i))) || !(entry->getBytesNoCopy()))
            break;
        uuid_t guid;
        memcpy(guid, entry->getBytesNoCopy(), sizeof(uuid_t));
        if (!uuid_is_null(guid)) {
            *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
            *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
            *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));
            char guid_string[37];
            uuid_unparse_upper(guid, guid_string);
            setPropertyString(mode, guid_string, "Unknown");
        }
    }

    for (int i = 0; i < INT3400_THERMAL_MAXIMUM_UUID; i++) {
        if ((mode->getObject(int3400_thermal_uuids[i]))) {
            uuid_bitmap |= BIT(i);
            switch (i) {
                case INT3400_THERMAL_PASSIVE_1:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_PASSIVE_1");
                    break;

                case INT3400_THERMAL_ACTIVE:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_ACTIVE");
                    break;

                case INT3400_THERMAL_CRITICAL:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_CRITICAL");
                    break;

                case INT3400_THERMAL_ADAPTIVE_PERFORMANCE:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_ADAPTIVE_PERFORMANCE");
                    break;

                case INT3400_THERMAL_EMERGENCY_CALL_MODE:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_EMERGENCY_CALL_MODE");
                    break;

                case INT3400_THERMAL_PASSIVE_2:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_PASSIVE_2");
                    break;

                case INT3400_THERMAL_POWER_BOSS:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_POWER_BOSS");
                    break;

                case INT3400_THERMAL_VIRTUAL_SENSOR:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_VIRTUAL_SENSOR");
                    break;

                case INT3400_THERMAL_COOLING_MODE:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_COOLING_MODE");
                    break;

                case INT3400_THERMAL_HARDWARE_DUTY_CYCLING:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_HARDWARE_DUTY_CYCLING");
                    break;

                case INT3400_THERMAL_ACTIVE_2:
                    setPropertyString(mode, int3400_thermal_uuids[i], "INT3400_THERMAL_ACTIVE_2");
                    break;
            }
        }
    }
    setProperty("Available Mode", mode);
    mode->release();
    OSSafeReleaseNULL(result);
    return true;
}

OSDictionary *ThermalSolution::parsePath(OSDictionary *entry, const char *&path) {
    int x = 1;
    while (path[x] != '/') {
        if (path[x] == '\0') {
            entry->retain();
            return entry;
        }
        x++;
    }
    char *dictname = new char[x];
    strncpy(dictname, path, x);
    dictname[x] = '\0';
    OSDictionary *subentry = OSDynamicCast(OSDictionary, entry->getObject(dictname));
    if (!subentry) {
        subentry = OSDictionary::withCapacity(1);
        entry->setObject(dictname, subentry);
        subentry->release();
    }
    delete [] dictname;
    path += x;
    return parsePath(subentry, path);
}

OSDictionary *ThermalSolution::parseAPAT(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    const uint64Container *version = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(ret, "version", version->value, 64);
    if (version->type != ESIF_DATA_UINT64 || version->value != 2)
        return ret;

    const char* offset = reinterpret_cast<const char *>(data);
    offset += sizeof(uint64Container);
    OSArray *arr = OSArray::withCapacity(1);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        OSDictionary *entry = OSDictionary::withCapacity(1);
        const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "target_id", content->value, 64);
        offset += sizeof(uint64Container);

        const uint64Container *str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "name", offset);
        offset += str->value;

        str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "participant", offset);
        offset += str->value;

        content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "domain", content->value, 64);
        offset += sizeof(uint64Container);

        str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "code", offset);
        offset += str->value;

        str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "argument", offset);
        offset += str->value;

        arr->setObject(entry);
        entry->release();
    }
    ret->setObject("targets", arr);
    arr->release();
    return ret;
}

OSDictionary *ThermalSolution::parseAPCT(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    const uint64Container *version = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(ret, "version", version->value, 64);
    if (version->type != ESIF_DATA_UINT64)
        return ret;

    const char* offset = reinterpret_cast<const char *>(data);
    offset += sizeof(uint64Container);
    OSDictionary *arr = OSDictionary::withCapacity(1);
    switch (version->value) {
        case 1:
        {
            while (offset < reinterpret_cast<const char *>(data) + length) {
                OSArray *condition_set = OSArray::withCapacity(1);
                const uint64Container *target = reinterpret_cast<const uint64Container *>(offset);
                offset += sizeof(uint64Container);
                for (int i = 0; i < 10; i++) {
                    OSDictionary *condition = OSDictionary::withCapacity(1);

                    const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
                    if (content[0].value < ARRAY_SIZE(condition_names))
                        setPropertyString(condition, "condition", condition_names[content[0].value]);
                    else
                        setPropertyNumber(condition, "condition", content[0].value, 64);
                    if (content[1].value < ARRAY_SIZE(comp_strs))
                        setPropertyString(condition, "condition", comp_strs[content[1].value]);
                    else
                        setPropertyNumber(condition, "comparison", content[1].value, 64);
                    setPropertyNumber(condition, "argument", content[2].value, 64);
                    offset += 3 * sizeof(uint64Container);

                    if (i < 9) {
                        switch (content[3].value) {
                            case 1:
                                setPropertyString(condition, "operation", "AND");
                                break;

                            case 2:
                                setPropertyString(condition, "operation", "FOR");
                                break;

                            default:
                                setPropertyNumber(condition, "operation", content[3].value, 64);
                                break;
                        }
                        offset += sizeof(uint64Container);
                        if (content[3].value == FOR) {
                            setPropertyNumber(condition, "unknown2", content[4].value, 64);
                            setPropertyNumber(condition, "time_comparison", content[5].value, 64);
                            setPropertyNumber(condition, "time", content[6].value, 64);
                            setPropertyNumber(condition, "unknown3", content[7].value, 64);
                            offset += 4 * sizeof(uint64Container);
                            i++;
                        }
                    }
                    condition_set->setObject(condition);
                    condition->release();
                }
                char *name = new char[10];
                snprintf(name, 10, "target%llX", target->value);
                arr->setObject(name, condition_set);
                delete [] name;
                condition_set->release();
            }
            break;
        }

        case 2:
        {
            while (offset < reinterpret_cast<const char *>(data) + length) {
                OSArray *condition_set = OSArray::withCapacity(1);
                const uint64Container *target = reinterpret_cast<const uint64Container *>(offset);
                offset += sizeof(uint64Container);
                const uint64Container *count = reinterpret_cast<const uint64Container *>(offset);
                offset += sizeof(uint64Container);
                for (int i = 0; i < count->value; i++) {
                    OSDictionary *condition = OSDictionary::withCapacity(1);

                    const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
                    if (content[0].value < ARRAY_SIZE(condition_names))
                        setPropertyString(condition, "condition", condition_names[content[0].value]);
                    else
                        setPropertyNumber(condition, "condition", content[0].value, 64);
                    offset += sizeof(uint64Container);
                    const uint64Container *str = reinterpret_cast<const uint64Container *>(offset);
                    offset += sizeof(uint64Container);
                    setPropertyString(condition, "device", offset);
                    offset += str->value;

                    content = reinterpret_cast<const uint64Container *>(offset);
                    setPropertyNumber(condition, "unknown0", content[0].value, 64);
                    if (content[1].value < ARRAY_SIZE(comp_strs))
                        setPropertyString(condition, "condition", comp_strs[content[1].value]);
                    else
                        setPropertyNumber(condition, "comparison", content[1].value, 64);
                    setPropertyNumber(condition, "argument", content[2].value, 64);
                    offset += 3 * sizeof(uint64Container);

                    if (i < (count->value - 1)) {
                        switch (content[3].value) {
                            case 1:
                                setPropertyString(condition, "operation", "AND");
                                break;

                            case 2:
                                setPropertyString(condition, "operation", "FOR");
                                break;

                            default:
                                setPropertyNumber(condition, "operation", content[3].value, 64);
                                break;
                        }
                        offset += sizeof(uint64Container);
                        if (content[3].value == FOR) {
                            setPropertyNumber(condition, "unknown1", content[4].value, 64);
                            offset += sizeof(uint64Container);
                            str = reinterpret_cast<const uint64Container *>(offset);
                            offset += sizeof(uint64Container);
                            setPropertyString(condition, "device", offset);
                            offset += str->value;

                            content = reinterpret_cast<const uint64Container *>(offset);
                            setPropertyNumber(condition, "unknown2", content[0].value, 64);
                            setPropertyNumber(condition, "time_comparison", content[1].value, 64);
                            setPropertyNumber(condition, "time", content[2].value, 64);
                            setPropertyNumber(condition, "unknown3", content[3].value, 64);
                            offset += 4 * sizeof(uint64Container);
                            i++;
                        }
                    }
                    condition_set->setObject(condition);
                    condition->release();
                }
                char *name = new char[10];
                snprintf(name, 10, "target%llX", target->value);
                arr->setObject(name, condition_set);
                delete [] name;
                condition_set->release();
            }
            break;
        }

        default:
            break;
    }
    ret->setObject("conditions", arr);
    arr->release();
    return ret;
}

OSDictionary *ThermalSolution::parseAPPC(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    const uint64Container *version = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(ret, "version", version->value, 64);
    if (version->type != ESIF_DATA_UINT64 || version->value != 1)
        return ret;

    const char* offset = reinterpret_cast<const char *>(data);
    offset += sizeof(uint64Container);
    OSArray *arr = OSArray::withCapacity(1);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        OSDictionary *entry = OSDictionary::withCapacity(1);
        const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
        if (content[0].value < ARRAY_SIZE(condition_names))
            setPropertyString(entry, "condition", condition_names[content[0].value]);
        else
            setPropertyNumber(entry, "condition", content[0].value, 64);
        offset += sizeof(uint64Container);

        const uint64Container *str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "name", offset);
        offset += str->value;

        str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "participant", offset);
        offset += str->value;

        content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "domain", content[0].value, 64);
        setPropertyNumber(entry, "type", content[1].value, 64);
        offset += 2 * sizeof(uint64Container);

        arr->setObject(entry);
        entry->release();
    }
    ret->setObject("custom_conditions", arr);
    arr->release();
    return ret;
}

OSDictionary *ThermalSolution::parsePPCC(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;
    char iname[10];

    const uint64Container *content = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(ret, "version", content[0].value, 64);
    setPropertyNumber(ret, "unknown1", content[1].value, 64);
    setPropertyNumber(ret, "power_limit_min", content[2].value, 64);
    setPropertyNumber(ret, "power_limit_max", content[3].value, 64);
    setPropertyNumber(ret, "time_wind_min", content[4].value, 64);
    setPropertyNumber(ret, "time_wind_max", content[5].value, 64);
    setPropertyNumber(ret, "step_size", content[6].value, 64);
    for (int i = 7; i < length / sizeof(uint64Container); i++) {
        snprintf(iname, 10, "unknown%02X", i);
        setPropertyNumber(ret, iname, content[i].value, 64);
    }
    return ret;
}

OSDictionary *ThermalSolution::parsePSVT(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    const char* offset = reinterpret_cast<const char *>(data);

    const uint64Container *version = reinterpret_cast<const uint64Container *>(offset);
    offset += sizeof(uint64Container);
    setPropertyNumber(ret, "version", version->value, 64);
    if (version->type != ESIF_DATA_UINT64 || version->value != 2)
        return ret;

    OSArray *arr = OSArray::withCapacity(1);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        OSDictionary *entry = OSDictionary::withCapacity(1);
        const uint64Container *str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "source", offset);
        offset += str->value;

        str = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "target", offset);
        offset += str->value;

        const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "priority", content[0].value, 64);
        setPropertyNumber(entry, "sample_period", content[1].value, 64);
        setPropertyTemp(entry, "temp", acpi_deci_kelvin_to_deci_celsius((UInt32)content[2].value));
        setPropertyNumber(entry, "domain", content[3].value, 64);
        setPropertyNumber(entry, "control_knob", content[4].value, 64);

        offset += 6 * sizeof(uint64Container);
        if (content[5].type == ESIF_DATA_STRING) {
            setPropertyString(entry, "limit", offset);
            offset += content[5].value;
        } else {
            setPropertyNumber(entry, "limit", content[5].value, 64);
        }

        content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "step_size", content[0].value, 64);
        setPropertyNumber(entry, "limit_coeff", content[1].value, 64);
        setPropertyNumber(entry, "unlimit_coeff", content[2].value, 64);
        setPropertyNumber(entry, "unknown", content[3].value, 64);
        offset += 4 * sizeof(uint64Container);
        arr->setObject(entry);
        entry->release();
    }
    ret->setObject("psvs", arr);
    arr->release();
    return ret;
}

OSDictionary *ThermalSolution::parseIDSP(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    const char* offset = reinterpret_cast<const char *>(data);

    OSArray *arr = OSArray::withCapacity(1);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        char guid_string[37];
        const guidContainer *item = reinterpret_cast<const guidContainer *>(offset);

        if ((reinterpret_cast<const char *>(data) + length - offset) < sizeof(guidContainer))
            break;
        // Should be 5 - ESIF_DATA_GUID instead?
        if (item->type != ESIF_DATA_BINARY)
            break;
        uuid_unparse_upper(item->guid, guid_string);
        value = OSString::withCString(guid_string);
        arr->setObject(value);
        value->release();
        offset += sizeof(guidContainer);
        if (item->length != sizeof(uuid_t))
            offset += item->length - sizeof(uuid_t);
    }
    ret->setObject("idsp", arr);
    arr->release();
    return ret;
}

OSDictionary *ThermalSolution::parseBinary(const void *data, uint32_t length) {
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    uint64_t size, seq = 0;
    char iname[10];

    const char* offset = reinterpret_cast<const char *>(data);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        snprintf(iname, 10, "unknown%02llX", seq++);

        switch (*reinterpret_cast<const uint32_t *>(offset)) {
            case ESIF_DATA_UINT32:
                setPropertyNumber(ret, iname, *reinterpret_cast<const uint32_t *>(offset + sizeof(uint32_t)), 32);
                offset += sizeof(uint32_t) + sizeof(uint32_t);
                break;

            case ESIF_DATA_UINT64:
                setPropertyNumber(ret, iname, *reinterpret_cast<const uint64_t *>(offset + sizeof(uint32_t)), 64);
                offset += sizeof(uint32_t) + sizeof(uint64_t);
                break;

            case ESIF_DATA_BINARY:
                size = *reinterpret_cast<const uint64_t *>(offset + sizeof(uint32_t));
                offset += sizeof(uint32_t) + sizeof(uint64_t);
                setPropertyBytes(ret, iname, offset, (uint32_t) size);
                offset += size;
                break;

            case ESIF_DATA_STRING:
                size = *reinterpret_cast<const uint64_t *>(offset + sizeof(uint32_t));
                offset += sizeof(uint32_t) + sizeof(uint64_t);
                setPropertyString(ret, iname, offset);
                offset += size;
                break;

            default:
                AlwaysLog("Unknown data type %d at", *reinterpret_cast<const uint32_t *>(offset));
                setPropertyBytes(ret, "raw", data, length < 0xff ? length : 0xff);
                return ret;
        }
    }
    return ret;
}

const char *strstr(const char *stack, const char *needle, size_t len) {
    if (len == 0) {
        len = strlen(needle);
        if (len == 0) return stack;
    }

    const char *i = needle;

    while (*stack) {
        if (*stack == *i) {
            i++;
            if (static_cast<size_t>(i - needle) == len)
                return stack - len + 1;
        } else {
            i = needle;
        }
        stack++;
    }

    return nullptr;
}

bool ThermalSolution::evaluateGDDV() {
    OSObject *result;
    OSArray *package;
    OSData *buf;
    if ((dev->evaluateObject("GDDV", &result) != kIOReturnSuccess) ||
        !(package = OSDynamicCast(OSArray, result)) ||
        (package->getCount() != 1) ||
        !(buf = OSDynamicCast(OSData, package->getObject(0)))) {
        OSSafeReleaseNULL(result);
        return false;
    }

    const GDDVHeader *hdr = reinterpret_cast<const GDDVHeader *>(buf->getBytesNoCopy());
    OSDictionary *headerDesc = OSDictionary::withCapacity(6);
    OSObject *value;
    OSData *decompressed = 0;
    setPropertyNumber(headerDesc, "Signature", hdr->signature, 16);
    setPropertyNumber(headerDesc, "Major", hdr->version.major, 8);
    setPropertyNumber(headerDesc, "Minor", hdr->version.minor, 8);
    setPropertyNumber(headerDesc, "Revision", hdr->version.revision, 16);
    setPropertyNumber(headerDesc, "Flags", hdr->v1.flags, 32);
    setPropertyNumber(headerDesc, "Length", buf->getLength(), 32);
    setProperty("GDDV", headerDesc);
    OSSafeReleaseNULL(headerDesc);

    if (hdr->signature != ESIFDV_HEADER_SIGNATURE) {
        AlwaysLog("Unsupported signature");
        OSSafeReleaseNULL(result);
        return false;
    }

    int offset = hdr->headersize;

    if (hdr->version.major == 2) {
        if (hdr->headersize != sizeof(GDDVHeader)) {
            AlwaysLog("Header size mismatch");
            OSSafeReleaseNULL(result);
            return false;
        }
        if (hdr->v2.payload_size != buf->getLength() - hdr->headersize) {
            AlwaysLog("Payload size mismatch");
            OSSafeReleaseNULL(result);
            return false;
        }
        char segmentid[ESIFDV_NAME_LEN+1];
        char comment[ESIFDV_DESC_LEN+1];
        char payload_class[5];
        strncpy(segmentid, hdr->v2.segmentid, ESIFDV_NAME_LEN);
        strncpy(comment, hdr->v2.comment, ESIFDV_DESC_LEN);
        strncpy(payload_class, reinterpret_cast<const char *>(&hdr->v2.payload_class), 4);
        segmentid[ESIFDV_NAME_LEN] = 0;
        comment[ESIFDV_DESC_LEN] = 0;
        payload_class[4] = 0;
        headerDesc = OSDictionary::withCapacity(5);
        setPropertyString(headerDesc, "SegmentID", segmentid);
        setPropertyString(headerDesc, "Comment", comment);
        setPropertyBytes(headerDesc, "Hash", hdr->v2.payload_hash, SHA256_HASH_BYTES);
        setPropertyNumber(headerDesc, "PayloadSize", hdr->v2.payload_size, 32);
        setPropertyString(headerDesc, "PayloadClass", payload_class);
        setProperty("GDDV2", headerDesc);
        OSSafeReleaseNULL(headerDesc);
        if (hdr->v2.flags & ESIF_SERVICE_CONFIG_COMPRESSED) {
            int res;
            size_t destlen = 0;
            res = lzma_decompress(NULL, &destlen, reinterpret_cast<const unsigned char *>(buf->getBytesNoCopy(hdr->headersize, hdr->v2.payload_size)), hdr->v2.payload_size);
            AlwaysLog("Decompress res = %d req = %zx", res, destlen);
            if (res != 0) {
                AlwaysLog("Header verification failed");
                OSSafeReleaseNULL(result);
                return false;
            }
            if (destlen >> 32) {
                AlwaysLog("Payload output size exceeded");
                OSSafeReleaseNULL(result);
                return false;
            }
            setProperty("PayloadOutputSize", destlen, 64);
            unsigned char *tmp = reinterpret_cast<unsigned char*>(IOMalloc(destlen));
            if (!tmp) {
                AlwaysLog("Payload output alloc failed");
                OSSafeReleaseNULL(result);
                return false;
            }
            res = lzma_decompress(tmp, &destlen, reinterpret_cast<const unsigned char *>(buf->getBytesNoCopy(hdr->headersize, hdr->v2.payload_size)), hdr->v2.payload_size);
            AlwaysLog("Decompress res = %d req = %zx", res, destlen);
            if (res != 0) {
                AlwaysLog("Decompress failed = %d", res);
                IOFree(tmp, destlen);
                OSSafeReleaseNULL(result);
                return false;
            }
            decompressed = OSData::withBytes(tmp, (unsigned int)destlen);
            IOFree(tmp, destlen);
            if (!decompressed) {
                AlwaysLog("Payload output copy failed");
                OSSafeReleaseNULL(result);
                return false;
            }
            buf = decompressed;
            hdr = reinterpret_cast<const GDDVHeader *>(buf->getBytesNoCopy());
            memset(segmentid, 0, ESIFDV_NAME_LEN);
            memset(comment, 0, ESIFDV_DESC_LEN);
            memset(payload_class, 0, 4);
            strncpy(segmentid, hdr->v2.segmentid, ESIFDV_NAME_LEN);
            strncpy(comment, hdr->v2.comment, ESIFDV_DESC_LEN);
            strncpy(payload_class, reinterpret_cast<const char *>(&hdr->v2.payload_class), 4);
            headerDesc = OSDictionary::withCapacity(11);
            setPropertyNumber(headerDesc, "Signature", hdr->signature, 16);
            setPropertyNumber(headerDesc, "Major", hdr->version.major, 8);
            setPropertyNumber(headerDesc, "Minor", hdr->version.minor, 8);
            setPropertyNumber(headerDesc, "Revision", hdr->version.revision, 16);
            setPropertyNumber(headerDesc, "Flags", hdr->v1.flags, 32);
            setPropertyNumber(headerDesc, "DestLen", destlen, 64);
            setPropertyString(headerDesc, "SegmentID", segmentid);
            setPropertyString(headerDesc, "Comment", comment);
            setPropertyBytes(headerDesc, "Hash", hdr->v2.payload_hash, SHA256_HASH_BYTES);
            setPropertyNumber(headerDesc, "PayloadSize", hdr->v2.payload_size, 32);
            setPropertyString(headerDesc, "PayloadClass", payload_class);
            setProperty("GDDV3", headerDesc);
            OSSafeReleaseNULL(headerDesc);

            if (hdr->signature != ESIFDV_HEADER_SIGNATURE) {
                AlwaysLog("Unsupported signature");
                OSSafeReleaseNULL(result);
                OSSafeReleaseNULL(decompressed);
                return false;
            }
        } else {
            AlwaysLog("Non-uncompress flag not implemented");
            return true;
        }
    }

    OSDictionary *entries = OSDictionary::withCapacity(1);
    while ((offset < buf->getLength())) {
        if (hdr->version.major == 2) {
            const uint16_t *signature = reinterpret_cast<const uint16_t *>(buf->getBytesNoCopy(offset, sizeof(uint16_t)));
            if (*signature == ESIFDV_ITEM_KEYS_REV0_SIGNATURE) {
                offset += sizeof(uint16_t);
            } else if (*signature == ESIFDV_HEADER_SIGNATURE) {
                DebugLog("Found GDDV item at %x", offset);
                const GDDVHeader *hdr_e = reinterpret_cast<const GDDVHeader *>(buf->getBytesNoCopy(offset, sizeof(GDDVHeader)));
                headerDesc = OSDictionary::withCapacity(10);
                setPropertyNumber(headerDesc, "Signature", hdr_e->signature, 16);
                setPropertyNumber(headerDesc, "Major", hdr_e->version.major, 8);
                setPropertyNumber(headerDesc, "Minor", hdr_e->version.minor, 8);
                setPropertyNumber(headerDesc, "Revision", hdr_e->version.revision, 16);
                setPropertyNumber(headerDesc, "Flags", hdr_e->v1.flags, 32);
                if (hdr_e->version.major != 2) {
                    AlwaysLog("Unsupport GDDV version: %x", hdr_e->version.raw);
                    setProperty("GDDV4", headerDesc);
                    OSSafeReleaseNULL(headerDesc);
                    break;
                }
                char segmentid[ESIFDV_NAME_LEN+1];
                char comment[ESIFDV_DESC_LEN+1];
                char payload_class[5];
                strncpy(segmentid, hdr->v2.segmentid, ESIFDV_NAME_LEN);
                strncpy(comment, hdr->v2.comment, ESIFDV_DESC_LEN);
                strncpy(payload_class, reinterpret_cast<const char *>(&hdr->v2.payload_class), 4);
                segmentid[ESIFDV_NAME_LEN] = 0;
                comment[ESIFDV_DESC_LEN] = 0;
                payload_class[4] = 0;
                setPropertyString(headerDesc, "SegmentID", segmentid);
                setPropertyString(headerDesc, "Comment", comment);
                setPropertyBytes(headerDesc, "Hash", hdr_e->v2.payload_hash, SHA256_HASH_BYTES);
                setPropertyNumber(headerDesc, "PayloadSize", hdr_e->v2.payload_size, 32);
                setPropertyString(headerDesc, "PayloadClass", payload_class);
                setPropertyBytes(headerDesc, "Raw", buf->getBytesNoCopy(offset, hdr_e->headersize + hdr_e->v2.payload_size), hdr_e->headersize + hdr_e->v2.payload_size);
                setProperty("GDDV4", headerDesc);
                OSSafeReleaseNULL(headerDesc);
                offset += hdr_e->headersize;
                signature = reinterpret_cast<const uint16_t *>(buf->getBytesNoCopy(offset, sizeof(uint16_t)));
                if (*signature == ESIFDV_ITEM_KEYS_REV0_SIGNATURE) {
                    offset += sizeof(uint16_t);
                } else {
                    AlwaysLog("Unknown signature: %x", *signature);
                    offset += hdr_e->v2.payload_size;
                    continue;
                }
            } else {
                AlwaysLog("Unknown signature: %x", *signature);
                break;
            }
        }

        const GDDVKeyHeader *key = reinterpret_cast<const GDDVKeyHeader *>(buf->getBytesNoCopy(offset, sizeof(GDDVKeyHeader)));
        offset += sizeof(GDDVKeyHeader);

        const char *iname = reinterpret_cast<const char *>(buf->getBytesNoCopy(offset, key->length));
        const char *oname = iname;
        offset += key->length;

        const GDDVKeyHeader *val = reinterpret_cast<const GDDVKeyHeader *>(buf->getBytesNoCopy(offset, sizeof(GDDVKeyHeader)));
        offset += sizeof(GDDVKeyHeader);

        if (iname[0] != '/' || key->flag != 1) {
            entries->setObject(iname, kOSBooleanFalse);
            offset += val->length;
            continue;
        }

        OSDictionary *parent = parsePath(entries, iname);
        OSObject *content = nullptr;

        switch (val->flag) {
            case ESIF_DATA_UINT32:
            case ESIF_DATA_POWER:
                if (val->length == 4)
                    content = OSNumber::withNumber(*(reinterpret_cast<const uint32_t *>(buf->getBytesNoCopy(offset, val->length))), 32);
                else
                    AlwaysLog("Unknown length %d uint32/power data at: %x", val->length, offset);
                break;

            case ESIF_DATA_TEMPERATURE:
                content = parseDeciKelvin(*(reinterpret_cast<const uint32_t *>(buf->getBytesNoCopy(offset, val->length))));
                break;

            case ESIF_DATA_STRING:
                content = OSString::withCString(reinterpret_cast<const char *>(buf->getBytesNoCopy(offset, val->length)));
                break;

            case ESIF_DATA_BINARY:
                if (!strncmp(iname, "/apat", strlen("/apat")))
                    content = parseAPAT(buf->getBytesNoCopy(offset, val->length), val->length);
                else if (!strncmp(iname, "/apct", strlen("/apct")))
                    content = parseAPCT(buf->getBytesNoCopy(offset, val->length), val->length);
                else if (!strncmp(iname, "/appc", strlen("/appc")))
                    content = parseAPPC(buf->getBytesNoCopy(offset, val->length), val->length);
                else if (!strncmp(iname, "/ppcc", strlen("/ppcc")))
                    content = parsePPCC(buf->getBytesNoCopy(offset, val->length), val->length);
                else if (!strncmp(iname, "/psvt", strlen("/psvt")) || !!strstr(oname, "/psvt", 0))
                    content = parsePSVT(buf->getBytesNoCopy(offset, val->length), val->length);
                else if (!strncmp(iname, "/idsp", strlen("/idsp")))
                    content = parseIDSP(buf->getBytesNoCopy(offset, val->length), val->length);
                else {
                    AlwaysLog("Unknown binary %s at %x", iname, offset);
                    content = parseBinary(buf->getBytesNoCopy(offset, val->length), val->length);
                }
                break;

            case ESIF_DATA_JSON:
                content = OSString::withCString(reinterpret_cast<const char *>(buf->getBytesNoCopy(offset, val->length)));
                break;
        }
        if (!content) {
            OSDictionary *keyDesc = OSDictionary::withCapacity(1);
            setPropertyNumber(keyDesc, "type", val->flag, 32);
            setPropertyNumber(keyDesc, "length", val->length, 32);
            if (val->length < 0xff)
                setPropertyBytes(keyDesc, "value", buf->getBytesNoCopy(offset, val->length), val->length);
            content = keyDesc;
#ifdef DEBUG
        } else {
            OSDictionary *keyDesc;
            if ((keyDesc = OSDynamicCast(OSDictionary, content)))
                setPropertyNumber(keyDesc, "length", val->length, 32);
#endif
        }
        offset += val->length;
        parent->setObject(iname, content);
        OSSafeReleaseNULL(content);
        OSSafeReleaseNULL(parent);
    }
    setProperty("GDDVEntry", entries);
    entries->release();
    OSSafeReleaseNULL(result);
    OSSafeReleaseNULL(decompressed);
    return true;
}

bool ThermalSolution::evaluateODVP() {
    OSObject *result;
    OSArray *package;
    if ((dev->evaluateObject("ODVP", &result) == kIOReturnSuccess) &&
        (package = OSDynamicCast(OSArray, result)))
        setProperty("ODVP", package);
    OSSafeReleaseNULL(result);
    return true;
}

bool ThermalSolution::changeMode(int i, bool enable) {
    if (!(uuid_bitmap & BIT(i))) {
        AlwaysLog("Mode %s is not available", int3400_thermal_uuids[i]);
        return false;
    }

    uuid_t guid;
    uuid_parse(int3400_thermal_uuids[i], guid);

    // convert to mixed-endian
    *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
    *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
    *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));

    UInt32 buf[2];
    buf[OSC_QUERY_DWORD] = 0;
    buf[OSC_SUPPORT_DWORD] = enable;

    OSObject *params[] = {
        OSData::withBytes(guid, 16),
        OSNumber::withNumber(DPTF_OSC_REVISION, 32),
        OSNumber::withNumber(sizeof(buf)/sizeof(UInt32), 32),
        OSData::withBytes(buf, sizeof(buf)),
    };

    OSObject *result;

    IOReturn ret = dev->evaluateObject("_OSC", &result, params, 4);
    params[0]->release();
    params[1]->release();
    params[2]->release();
    params[3]->release();
    if (ret == kIOReturnSuccess) {
        OSData *data;
        const UInt32 *rbuf;
        
        if ((data = OSDynamicCast(OSData, result)) &&
            (rbuf = reinterpret_cast<const UInt32 *>(data->getBytesNoCopy()))) {
            UInt32 error = rbuf[0] & ~BIT(OSC_QUERY_ENABLE);
            if (error & OSC_REQUEST_ERROR)
                AlwaysLog("_OSC request failed");
            if (error & OSC_INVALID_UUID_ERROR)
                AlwaysLog("_OSC invalid UUID");
            if (error & OSC_INVALID_REVISION_ERROR)
                AlwaysLog("_OSC invalid revision");
            if (!error || error & OSC_CAPABILITIES_MASK_ERROR)
                evaluateODVP();
            else
                ret = kIOReturnInvalid;
        }
    } else {
        AlwaysLog("_OSC evaluate failed");
    }

    if (ret != kIOReturnSuccess)
        return false;

    setProperty("currentUUID", int3400_thermal_uuids[i]);
    return true;
}

IOReturn ThermalSolution::message(UInt32 type, IOService *provider, void *argument) {
    switch (type) {
        case kIOACPIMessageDeviceNotification:
            if (argument) {
                switch (*(UInt32 *) argument) {
                    case INT3400_THERMAL_TABLE_CHANGED:
                        AlwaysLog("ACPI notification: thermal table changed");
                        break;

                    case INT3400_ODVP_CHANGED:
                        AlwaysLog("ACPI notification: ODVP changed");
                        evaluateODVP();
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

IOReturn ThermalSolution::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &ThermalSolution::setPropertiesGated), props);
    return kIOReturnSuccess;
}

void ThermalSolution::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    for (int i = 0; i < INT3400_THERMAL_MAXIMUM_UUID; i++) {
        if ((dict->getObject(int3400_thermal_uuids[i]))) {
            OSBoolean *value = OSDynamicCast(OSBoolean, dict->getObject(int3400_thermal_uuids[i]));
            if (value == nullptr)
                AlwaysLog("Invald status");

            if (changeMode(i, value->getValue()))
                DebugLog("%s mode %s", value->getValue() ? "Enabled" : "Disabled", int3400_thermal_uuids[i]);
            return;
        }
    }
    AlwaysLog("Could not find known policy UUID");
}

void ThermalSolution::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void ThermalSolution::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        AlwaysLog("No available notification consumer");
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &ThermalSolution::dispatchMessageGated), &message, data);
}

void ThermalSolution::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        DebugLog("Notification consumer published: %s", newService->getName());
        _notificationServices->setObject(newService);
        UInt32 type;
        newService->message(kThermal_getDeviceType, this, &type);
        if (type == INT3400_THERMAL_VIRTUAL_SENSOR)
            DebugLog("Sensor consumer published: %s", newService->getName());
    }

    if (notifier == _terminateNotify) {
        DebugLog("Notification consumer terminated: %s", newService->getName());
        _notificationServices->removeObject(newService);
    }
}

bool ThermalSolution::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &ThermalSolution::notificationHandlerGated), newService, notifier);
    return true;
}
