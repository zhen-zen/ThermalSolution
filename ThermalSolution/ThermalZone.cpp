//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThermalZone.cpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/5.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThermalZone.hpp"

IOReturn ThermalZone::getZoneTemp(UInt32 *temp) {
    UInt32 tmp;
    IOReturn ret = dev->evaluateInteger("_TMP", &tmp);

    if (ret == kIOReturnSuccess)
        *temp = acpi_deci_kelvin_to_deci_celsius(tmp);

    return ret;
}

OSDictionary *ThermalZone::readTrips() {
    UInt32 trip_cnt;
    OSDictionary *ret = OSDictionary::withCapacity(1);
    OSObject *value;

    if (dev->evaluateInteger("PATC", &trip_cnt) == kIOReturnSuccess) {
        aux_trips = new UInt32[trip_cnt];
        aux_trip_nr = trip_cnt;
        setPropertyNumber(ret, "PATC", trip_cnt, 32);
    } else {
        trip_cnt = 0;
    }

    if (dev->evaluateInteger("_CR3", &cr3_temp) == kIOReturnSuccess)
        setPropertyTemp(ret, "Warm/Standby Temperature", acpi_deci_kelvin_to_deci_celsius(cr3_temp));

    if (dev->evaluateInteger("_TSP", &tsp) == kIOReturnSuccess)
        setPropertyNumber(ret, "Thermal Sampling Period", tsp, 32);

    if (dev->evaluateInteger("_CRT", &crt_temp) == kIOReturnSuccess) {
        crt_trip_id = trip_cnt++;
        setPropertyTemp(ret, "Critical Temperature", acpi_deci_kelvin_to_deci_celsius(crt_temp));
    }

    if (dev->evaluateInteger("_HOT", &hot_temp) == kIOReturnSuccess) {
        hot_trip_id = trip_cnt++;
        setPropertyTemp(ret, "Hot Temperature", acpi_deci_kelvin_to_deci_celsius(hot_temp));
    }

    if (dev->evaluateInteger("_PSV", &psv_temp) == kIOReturnSuccess) {
        psv_trip_id = trip_cnt++;
        setPropertyTemp(ret, "Passive Temperature", acpi_deci_kelvin_to_deci_celsius(psv_temp));
    }

    char name[5];
    OSDictionary *arr = OSDictionary::withCapacity(1);
    for (int i = 0; i < MAX_ACT_TRIP_COUNT; i++) {
        snprintf(name, 5, "_AC%1d", i);
        if (dev->evaluateInteger(name, &act_trips[i].temp) != kIOReturnSuccess)
            continue;

        act_trips[i].id = trip_cnt++;
        act_trips[i].valid = true;
        setPropertyTemp(arr, name, acpi_deci_kelvin_to_deci_celsius(act_trips[i].temp));
    }
    ret->setObject("Active Cooling Temperature", arr);
    arr->release();
    return ret;
}

ThermalZone::~ThermalZone() {
    if (aux_trips)
        delete [] aux_trips;
}
