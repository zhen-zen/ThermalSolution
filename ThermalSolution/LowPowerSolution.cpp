//  SPDX-License-Identifier: GPL-2.0-only
//
//  LowPowerSolution.cpp
//  ThermalSolution
//
//  Created by Zhen on 6/14/21.
//

#include "LowPowerSolution.hpp"
OSDefineMetaClassAndStructors(LowPowerSolution, IOService)

bool LowPowerSolution::start(IOService *provider) {
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

    OSObject *result;
    OSData *data;
    if ((evaluateDSM(LPS0_DSM_CAPABILITY, &result) == kIOReturnSuccess) &&
        (data = OSDynamicCast(OSData, result)))
        functionMask = *(reinterpret_cast<UInt8 const*>(data->getBytesNoCopy()));

    setProperty("Capability", functionMask, 8);
    OSSafeReleaseNULL(result);

    if ((data = OSDynamicCast(OSData, getProperty("EnabledStates"))))
        functionMask = functionMask & *(reinterpret_cast<UInt8 const*>(data->getBytesNoCopy()));

    OSArray *array;
    if ((evaluateDSM(LPS0_DSM_CONSTRAINT, &result) == kIOReturnSuccess) &&
        (array = OSDynamicCast(OSArray, result))) {
        OSCollectionIterator* iter = OSCollectionIterator::withCollection(array);
        if (iter) {
            OSDictionary *constraints = OSDictionary::withCapacity(1);
            while (OSArray *entry = OSDynamicCast(OSArray, iter->getNextObject())) {
                OSString *name = OSDynamicCast(OSString, entry->getObject(0));
                OSNumber *enabled = OSDynamicCast(OSNumber, entry->getObject(1));
                if (!enabled->unsigned8BitValue())
                    continue;
                OSArray *detail = OSDynamicCast(OSArray, entry->getObject(2));
                if (name && enabled && detail) {
                    OSNumber *revision = OSDynamicCast(OSNumber, detail->getObject(0));
                    OSArray *state = OSDynamicCast(OSArray, detail->getObject(2));

                    OSDictionary *dict = OSDictionary::withCapacity(3);
                    if (enabled->unsigned8BitValue() != 1)                    dict->setObject("Device Enabled", enabled);
                    if (revision->unsigned8BitValue() != 0)
                        dict->setObject("Revision", revision);
                    if (state) {
                        OSDictionary *constraint = OSDictionary::withCapacity(3);
                        constraint->setObject("LPI UID", state->getObject(0));
                        constraint->setObject("Minimum D-state", state->getObject(1));
                        constraint->setObject("Minimum device-specific state", state->getObject(2));
                        dict->setObject("State", constraint);
                        constraint->release();
                    }
                    constraints->setObject(name->getCStringNoCopy(), dict);
                    dict->release();
                }
            }
            setProperty("Constraints", constraints);
            constraints->release();
            iter->release();
        }
    }
    OSSafeReleaseNULL(result);

    if ((evaluateDSM(LPS0_DSM_CRASH_DUMP, &result) == kIOReturnSuccess) &&
        (array = OSDynamicCast(OSArray, result))) {
        OSCollectionIterator* iter = OSCollectionIterator::withCollection(array);
        if (iter) {
            OSDictionary *constraints = OSDictionary::withCapacity(1);
            while (OSArray *entry = OSDynamicCast(OSArray, iter->getNextObject())) {
                OSString *name = OSDynamicCast(OSString, entry->getObject(0));
                OSArray *detail = OSDynamicCast(OSArray, entry->getObject(1));
                if (name && detail) {
                    OSDictionary *dict = OSDictionary::withCapacity(2);
                    dict->setObject("GAS", detail->getObject(0));
                    OSArray *operation = OSDynamicCast(OSArray, detail->getObject(1));
                    if (operation) {
                        OSDictionary *control = OSDictionary::withCapacity(4);
                        control->setObject("ANDMask", operation->getObject(0));
                        control->setObject("Value", operation->getObject(1));
                        control->setObject("Trigger", operation->getObject(2));
                        control->setObject("Delay", operation->getObject(3));
                        dict->setObject("Operation", control);
                        control->release();
                    }
                    constraints->setObject(name->getCStringNoCopy(), dict);
                    dict->release();
                }
            }
            setProperty("Crash Dump", constraints);
            constraints->release();
            iter->release();
        }
    }
    OSSafeReleaseNULL(result);

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    ready = true;

    registerService();
    return true;
}

void LowPowerSolution::stop(IOService *provider) {
    DebugLog("Stoping");

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    ready = false;

    PMstop();

    terminate();
    super::stop(provider);
}

IOReturn LowPowerSolution::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &LowPowerSolution::setPropertiesGated), props);
    return kIOReturnSuccess;
}

void LowPowerSolution::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    OSNumber *index;
    OSObject *result;
    if ((index = OSDynamicCast(OSNumber, dict->getObject("DSM"))) != nullptr) {
        if (evaluateDSM(index->unsigned32BitValue(), &result) == kIOReturnSuccess) {
            if (result) {
                setProperty("raw", result);
                result->release();
            }
            DebugLog("Evaluated _DSM index %d", index->unsigned32BitValue());
        } else {
            AlwaysLog("Failed to evaluate _DSM index %d", index->unsigned32BitValue());
        }
    }
    if ((index = OSDynamicCast(OSNumber, dict->getObject("SetCap"))) != nullptr)
        functionMask = index->unsigned8BitValue();
}

IOReturn LowPowerSolution::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (!(ready && functionMask & (BIT(LPS0_DSM_ENTRY) | BIT(LPS0_DSM_EXIT))))
        return kIOPMAckImplied;

    DebugLog("powerState %ld : %s", powerStateOrdinal, powerStateOrdinal ? "on" : "off");

    IOReturn ret;
    switch (powerStateOrdinal) {
        case 0:
            ret = evaluateDSM(LPS0_DSM_SCREEN_OFF, nullptr);
            DebugLog("LPS0_DSM_SCREEN_OFF 0x%x", ret);
            ret = evaluateDSM(LPS0_DSM_ENTRY, nullptr);
            DebugLog("LPS0_DSM_ENTRY 0x%x", ret);
            break;

        default:
            ret = evaluateDSM(LPS0_DSM_EXIT, nullptr);
            DebugLog("LPS0_DSM_EXIT 0x%x", ret);
            ret = evaluateDSM(LPS0_DSM_SCREEN_ON, nullptr);
            DebugLog("LPS0_DSM_SCREEN_ON 0x%x", ret);
            break;
    }

    return kIOPMAckImplied;
}

IOReturn LowPowerSolution::evaluateDSM(UInt32 index, OSObject **result, OSArray *arg, const char *uuid, UInt32 revision) {
    IOReturn ret;
    uuid_t guid;
    uuid_parse(uuid, guid);

    // convert to mixed-endian
    *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
    *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
    *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));

    OSObject *params[] = {
        OSData::withBytes(guid, 16),
        OSNumber::withNumber(revision, 32),
        OSNumber::withNumber(index, 32),
        arg ? arg : OSArray::withCapacity(1)
    };

    ret = dev->evaluateObject("_DSM", result, params, 4);

    params[0]->release();
    params[1]->release();
    params[2]->release();
    if (!arg)
        params[3]->release();
    return ret;
}
