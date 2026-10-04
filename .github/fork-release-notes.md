## darek-margas fork: v5.2.0 + guest mode, overheat temperature, scheduled departure

Fork of [yoziru/tesla-ble](https://github.com/yoziru/tesla-ble) for [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi). Branch `multicar` carries the fork's changes on top of `main`, which mirrors upstream.

Base: upstream `v5.2.0` plus the upstream `main` commits after it (low power mode and keep accessory power actions).

Added in this fork:
- **Guest mode** (`guestModeAction`): `Vehicle::set_guest_mode(bool)` - since v5.2.0-dm.1
- **Cabin overheat protection temperature** (`setCopTempAction`, Low / Medium / High): `Vehicle::set_cabin_overheat_protection_temp(1..3)` - since v5.2.0-dm.1
- **Scheduled departure** (`scheduledDepartureAction`): departure time, preconditioning and off-peak charging (off / all week / weekdays) and off-peak end time: `Vehicle::set_scheduled_departure(...)`; times are minutes after midnight, out-of-range times are rejected - new in v5.2.0-dm.2

Message fields follow Tesla's [vehicle-command](https://github.com/teslamotors/vehicle-command) (`SetGuestMode`, `SetCabinOverheatProtectionTemperature`, `ScheduleDeparture` / `ClearScheduledDeparture`). All library tests pass (256/256).

Roll back: tag `v5.2.0-dm.1` (without scheduled departure) or `v5.2.0` (upstream).
