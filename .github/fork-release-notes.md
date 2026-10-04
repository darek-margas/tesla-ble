## tesla-ble fork v5.2.0-dm.3: media controls and media state, scheduled departure, guest mode, cabin overheat temperature

Fork of [yoziru/tesla-ble](https://github.com/yoziru/tesla-ble), maintained for [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi) (from release `v2026.10.5`). It adds the vehicle actions that the Tesla app has but the upstream library does not send yet. Everything else is upstream code, unchanged.

### Base
- Upstream **`v5.2.0`**
- plus the upstream `main` commits after it: **low power mode** and **keep accessory power** actions (`Vehicle::set_low_power_mode(bool)`, `Vehicle::set_keep_accessory_power_mode(bool)`, from yoziru/tesla-ble#89)

### Added in this fork

| Action | API | Since |
|---|---|---|
| Media state read (`getMediaState`) | `Vehicle::media_state_poll(wake_policy)`, `Vehicle::set_media_state_callback(cb(const CarServer_MediaState &, const MediaNowPlaying &))` | **dm.3** |
| Media volume (`mediaUpdateVolume`) | `Vehicle::media_volume_up()`, `Vehicle::media_volume_down()`, `Vehicle::set_media_volume(0..10)` | **dm.3** |
| Media playback (`mediaPlayAction`, `mediaNextTrack`, `mediaPreviousTrack`, `mediaNextFavorite`, `mediaPreviousFavorite`) | `Vehicle::media_toggle_playback()`, `media_next_track()`, `media_previous_track()`, `media_next_favorite()`, `media_previous_favorite()` | **dm.3** |
| Scheduled departure (`scheduledDepartureAction`) | `Vehicle::set_scheduled_departure(enabled, departure_minutes, preconditioning_policy, off_peak_policy, off_peak_end_minutes)` | dm.2 |
| Guest mode (`guestModeAction`) | `Vehicle::set_guest_mode(bool)` | dm.1 |
| Cabin overheat protection temperature (`setCopTempAction`) | `Vehicle::set_cabin_overheat_protection_temp(level)`, 1 = Low (30 °C), 2 = Medium (35 °C), 3 = High (40 °C) | dm.1 |

**Media:**
- The media state gives volume, maximum volume, volume step, source, playback status (stopped / playing / paused) and whether remote control is enabled.
- Artist and title are unbounded strings, which the generated `CarServer_MediaState` does not keep. They are read from the raw response into `MediaNowPlaying` (at most 128 bytes each, cut on a UTF-8 character boundary). The generated protobuf code is unchanged.
- `media_state_poll()` is not part of `infotainment_poll()`. The car answers it only while the infotainment is awake.
- Volume follows vehicle-command: `SetVolume` takes 0–10 (anything else is rejected and nothing is sent), `VolumeUp` / `VolumeDown` send a step of ±1.

**Scheduled departure:**
- Times are minutes after midnight (0–1439). Out-of-range times are rejected and nothing is sent.
- The policies are `0` = Off, `1` = All week, `2` = Weekdays.
- `enabled = false` clears the schedule.
- The car stores the whole schedule at once, so send every field each time, not only the one that changed.

The message fields follow Tesla's [vehicle-command](https://github.com/teslamotors/vehicle-command) (`SetGuestMode`, `SetCabinOverheatProtectionTemperature`, `ScheduleDeparture` / `ClearScheduledDeparture`, `ToggleMediaPlayback`, `MediaNextTrack` / `MediaPreviousTrack`, `MediaNextFavorite` / `MediaPreviousFavorite`, `SetVolume`, `VolumeUp` / `VolumeDown`). These actions go to the infotainment domain, like the other vehicle controls.

### Tests
There are new tests for the volume builder and for reading the media state, artist and title from a response.

### Use it
ESP-IDF / ESPHome component:

```yaml
esp32:
  framework:
    components:
      - name: tesla-ble
        source: https://github.com/darek-margas/tesla-ble.git
        ref: v5.2.0-dm.3
```

### Branches and roll back
- `multicar`: the fork's changes (the default branch).
- `main`: mirrors upstream, to sync from.
- Tags are never moved.
  - To roll back to dm.2 (no media), use `v5.2.0-dm.2`.
  - To roll back to dm.1 (no scheduled departure), use `v5.2.0-dm.1`.
  - To roll back to plain upstream, use `v5.2.0`.
