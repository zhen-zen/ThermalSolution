//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThermalSolution.cpp
//  ThermalSolution
//
//  Created by Zhen on 2020/8/28.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThermalSolution.hpp"

OSDefineMetaClassAndStructors(ThermalSolution, IOService)

IOService *ThermalSolution::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score) ||
        !(dev = OSDynamicCast(IOACPIPlatformDevice, provider)))
        return nullptr;

    /* Missing IDSP isn't fatal */
    evaluateAvailableMode();
    evaluateGDDV();
    evaluateODVP();

    return this;
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
            }
        }
    }
    setProperty("Available Mode", mode);
    mode->release();
    OSSafeReleaseNULL(result);
    return true;
}

const char * ThermalSolution::parsePath(OSDictionary *entry, const char *name, OSDictionary *keyDesc) {
    int x = 1;
    while (name[x] != '/') {
        if (name[x] == '\0') {
            entry->setObject(name, keyDesc);
            return name;
        }
        x++;
    }
    char *dictname = new char[x];
    strncpy(dictname, name, x);
    dictname[x] = '\0';
    OSDictionary *subentry = OSDynamicCast(OSDictionary, entry->getObject(dictname));
    if (!subentry) {
        subentry = OSDictionary::withCapacity(1);
        entry->setObject(dictname, subentry);
        subentry->release();
    }
    const char *item = name + x;
    return parsePath(subentry, item, keyDesc);
}

void ThermalSolution::parseAPPC(OSDictionary *keyDesc, const void *data, uint32_t length) {
    OSObject *value;
    setPropertyBytes(keyDesc, "val", data, length);

    const uint64Container *version = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(keyDesc, "version", version->value, 64);
}

void ThermalSolution::parsePPCC(OSDictionary *keyDesc, const void *data, uint32_t length) {
    OSObject *value;
    setPropertyBytes(keyDesc, "val", data, length);

    const uint64Container *content = reinterpret_cast<const uint64Container *>(data);
    setPropertyNumber(keyDesc, "power_limit_min", content[2].value, 64);
    setPropertyNumber(keyDesc, "power_limit_max", content[3].value, 64);
    setPropertyNumber(keyDesc, "time_wind_min", content[4].value, 64);
    setPropertyNumber(keyDesc, "time_wind_max", content[5].value, 64);
    setPropertyNumber(keyDesc, "step_size", content[6].value, 64);
}

void ThermalSolution::parsePSVT(OSDictionary *keyDesc, const void *data, uint32_t length) {
    OSObject *value;
//    setPropertyBytes(keyDesc, "val", data, length);

    const char* offset = reinterpret_cast<const char *>(data);

    const uint64Container *version = reinterpret_cast<const uint64Container *>(offset);
    offset += sizeof(uint64Container);
    setPropertyNumber(keyDesc, "version", version->value, 64);

    OSArray *arr = OSArray::withCapacity(1);
    while (offset < reinterpret_cast<const char *>(data) + length) {
        OSDictionary *entry = OSDictionary::withCapacity(1);
        const uint64Container *len1 = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "source", offset);
        offset += len1->value;

        const uint64Container *len2 = reinterpret_cast<const uint64Container *>(offset);
        offset += sizeof(uint64Container);
        setPropertyString(entry, "target", offset);
        offset += len2->value;

        const uint64Container *content = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "priority", content[0].value, 64);
        setPropertyNumber(entry, "sample_period", content[1].value, 64);
        setPropertyNumber(entry, "temp", content[2].value, 64);
        setPropertyNumber(entry, "domain", content[3].value, 64);
        setPropertyNumber(entry, "control_knob", content[4].value, 64);

        offset += 6 * sizeof(uint64Container);
        if (content[5].type == type_string) {
            setPropertyString(entry, "limit", offset);
            offset += content[5].value;
        } else {
            setPropertyNumber(entry, "limit", content[5].value, 64);
        }

        const uint64Container *content2 = reinterpret_cast<const uint64Container *>(offset);
        setPropertyNumber(entry, "step_size", content2[0].value, 64);
        setPropertyNumber(entry, "limit_coeff", content2[1].value, 64);
        setPropertyNumber(entry, "unlimit_coeff", content2[2].value, 64);
        setPropertyNumber(entry, "placeholder", content2[3].value, 64);
        offset += 4 * sizeof(uint64Container);
        arr->setObject(entry);
        entry->release();
    }
    keyDesc->setObject("PSVT", arr);
    arr->release();
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
    OSDictionary *headerDesc = OSDictionary::withCapacity(4);
    OSObject *value;
    setPropertyNumber(headerDesc, "Signature", hdr->signature, 16);
    int ver = OSSwapInt32(hdr->version);
    setPropertyNumber(headerDesc, "Version", ver, 32);
    setPropertyNumber(headerDesc, "Flags", hdr->flags, 32);
    setPropertyNumber(headerDesc, "Length", buf->getLength(), 32);
    setProperty("GDDV", headerDesc);
    headerDesc->release();

    if (ver == 2) {
        IOLog("Uncompress not implemented\n");
        return true;
    }

    int offset = hdr->headersize;

    OSDictionary *entries = OSDictionary::withCapacity(1);
    while ((offset < buf->getLength())) {
        OSDictionary *keyDesc = OSDictionary::withCapacity(1);

        const GDDVKeyHeader *key = reinterpret_cast<const GDDVKeyHeader *>(buf->getBytesNoCopy(offset, sizeof(GDDVKeyHeader)));
        offset += sizeof(GDDVKeyHeader);

        if (key->flag != 1)
            setPropertyNumber(keyDesc, "keyflags", key->flag, 32);
        const char *name = reinterpret_cast<const char *>(buf->getBytesNoCopy(offset, key->length));
        offset += key->length;

        if (name[0] != '/')
            entries->setObject(name, keyDesc);
        else
            name = parsePath(entries, name, keyDesc);

        const GDDVKeyHeader *val = reinterpret_cast<const GDDVKeyHeader *>(buf->getBytesNoCopy(offset, sizeof(GDDVKeyHeader)));
        offset += sizeof(GDDVKeyHeader);

        if (val->flag == 0x8) {
            const char *str = reinterpret_cast<const char *>(buf->getBytesNoCopy(offset, val->length));
            setPropertyString(keyDesc, "string", str);
        } else if (val->flag == 0x1a) {
            if (val->length == 4)
                setPropertyNumber(keyDesc, "val", *(reinterpret_cast<const uint32_t *>(buf->getBytesNoCopy(offset, val->length))), 32);
            else
                setPropertyBytes(keyDesc, "val", buf->getBytesNoCopy(offset, val->length), val->length);
        } else {
            setPropertyNumber(keyDesc, "valtype", val->flag, 32);
            setPropertyNumber(keyDesc, "vallength", val->length, 32);
            if (!strncmp(name, "/appc", strlen("/appc")))
                parseAPPC(keyDesc, buf->getBytesNoCopy(offset, val->length), val->length);
            else if (!strncmp(name, "/ppcc", strlen("/ppcc")))
                parsePPCC(keyDesc, buf->getBytesNoCopy(offset, val->length), val->length);
            else if (!strncmp(name, "/psvt", strlen("/psvt")))
                parsePSVT(keyDesc, buf->getBytesNoCopy(offset, val->length), val->length);
            else if (val->length < 0x30)
                setPropertyBytes(keyDesc, "val", buf->getBytesNoCopy(offset, val->length), val->length);
        }
        offset += val->length;
        keyDesc->release();
    }
    setProperty("GDDVEntry", entries);
    entries->release();
    OSSafeReleaseNULL(result);
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

IOReturn ThermalSolution::message(UInt32 type, IOService *provider, void *argument) {
    if (argument) {
        switch (*(UInt32 *) argument) {
            case INT3400_THERMAL_TABLE_CHANGED :
                IOLog("message: provider=%s, thermal table changed\n", provider->getName());
                break;

            case INT3400_ODVP_CHANGED:
                IOLog("message: provider=%s, ODVP changed\n", provider->getName());
                evaluateODVP();
                break;

            default:
                IOLog("message: type=%x, provider=%s, argument=0x%04x\n", type, provider->getName(), *((UInt32 *) argument));
                break;
        }
    } else {
        IOLog("message: type=%x, provider=%s\n", type, provider->getName());
    }
    return kIOReturnSuccess;
}
