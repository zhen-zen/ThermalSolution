//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThermalZone.hpp
//  ThermalSolution
//
//  Created by Zhen on 2020/9/5.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThermalZone_hpp
#define ThermalZone_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "common.h"

// from linux/include/linux/units.h

#define ABSOLUTE_ZERO_MILLICELSIUS -273150
#define MILLIDEGREE_PER_DECIDEGREE 100
#define MILLIDEGREE_PER_DEGREE 1000

#define ACPI_ABSOLUTE_ZERO_DECI_CELSIUS -2732

static inline long milli_kelvin_to_millicelsius(long t)
{
    return t + ABSOLUTE_ZERO_MILLICELSIUS;
}

static inline long deci_kelvin_to_millicelsius(long t)
{
    return milli_kelvin_to_millicelsius(t * MILLIDEGREE_PER_DECIDEGREE);
}

static inline long deci_kelvin_to_celsius(long t)
{
    return deci_kelvin_to_millicelsius(t) / MILLIDEGREE_PER_DEGREE;
}

static inline SInt32 acpi_deci_kelvin_to_deci_celsius(UInt32 t)
{
    return t + ACPI_ABSOLUTE_ZERO_DECI_CELSIUS;
}

// from linux/drivers/thermal/intel/int340x_thermal/int340x_thermal_zone.c

#define MAX_ACT_TRIP_COUNT    10

struct active_trip {
    UInt32 temp;
    int id;
    bool valid;
};

class ThermalZone {
    IOACPIPlatformDevice *dev {nullptr};

    // TODO: LPAT table parse

    struct active_trip act_trips[MAX_ACT_TRIP_COUNT];
    UInt32 *aux_trips {nullptr};

    int aux_trip_nr {0};
    UInt32 cr3_temp {0};
    UInt32 crt_temp {0};
    int crt_trip_id {-1};
    UInt32 hot_temp {0};
    int hot_trip_id {-1};
    UInt32 psv_temp {0};
    int psv_trip_id {-1};

    UInt32 tsp {0};

public:
    ThermalZone(IOACPIPlatformDevice *dev) : dev(dev) {};
    ~ThermalZone();

//    UInt32 getTripConfig();
//    UInt32 getTripHyst(UInt32 trip);
//    UInt32 getTripTemp(UInt32 trip);
//    UInt32 getTripType(UInt32 trip);
    IOReturn getZoneTemp(UInt32 *temp);

//    bool setTripTemp(UInt32 trip, UInt32 temp);

    OSDictionary *readTrips();
};
#endif /* ThermalZone_hpp */
