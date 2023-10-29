# ThermalSolution  ![CI](https://github.com/zhen-zen/ThermalSolution/workflows/CI/badge.svg)

## Minimally maintained only

A driver based on [intel/thermal_daemon](https://github.com/intel/thermal_daemon) and [linux/drivers/thermal/intel/int340x_thermal](https://github.com/torvalds/linux/tree/master/drivers/thermal/intel/int340x_thermal)

Currently available functions:
- Set thermal mode by UUID
- Adaptive configuration parsing
- S0ix support [Use at your own risk]
  
  By interfacing with PEPD, certain features such as press keyboard to wake and stop fan on sleep can be achieved.
  
  However, since it's not real S3, there's a risk for overheating since the fan won't be spinning.

  You can test the feature by sending `ioio -s LowPowerSolution SetCap featureMask` and edit `EnabledStates` property in Info.plist for persistence.

  The definition for the bits can be found in [LowPowerSolution.hpp](https://github.com/zhen-zen/ThermalSolution/blob/master/ThermalSolution/LowPowerSolution.hpp)

- Read temperature manually from `INT3403` devices
