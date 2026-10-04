## darek-margas fork: v5.2.0 + guest mode, overheat temperature

Fork of [yoziru/tesla-ble](https://github.com/yoziru/tesla-ble) for [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi). Branch `multicar` carries the fork's changes on top of `main`, which mirrors upstream.

Base: upstream `v5.2.0` plus the upstream `main` commits after it (low power mode and keep accessory power actions).

Added here:
- **Guest mode** action (`guestModeAction`): `Vehicle::set_guest_mode(bool)`
- **Cabin overheat protection temperature** action (`setCopTempAction`, Low / Medium / High): `Vehicle::set_cabin_overheat_protection_temp(1..3)`; other levels are rejected

Message fields follow Tesla's [vehicle-command](https://github.com/teslamotors/vehicle-command) (`SetGuestMode`, `SetCabinOverheatProtectionTemperature`). All library tests pass (255/255, 6 new).

Roll back: use tag `v5.2.0`.
