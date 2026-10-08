## tesla-ble fork v5.2.0-dm.8: car responses authenticated, PSA crypto; log level, ESP32-C5; session request resend; crash fix; wake fix and identical resends; media, scheduled departure, guest mode, cabin overheat temperature

### Security fix in dm.8
- **Responses from the car are now authenticated.** Encrypted responses were decrypted but their AES-GCM tag was never checked: with the Mbed TLS GCM API the received tag was passed as the output buffer of `mbedtls_gcm_finish()`, which overwrote it with the computed tag instead of comparing the two. A corrupted or forged response would have been accepted. The tag is now verified, and the authenticated data uses the counter the car sends in the response (`AES_GCM_ResponseData.counter`), not our own request counter; with the old counter every response fails once the check is on. This affects all earlier versions, upstream included.

### Changed in dm.8
- **Crypto ported to the PSA API** (P-256 key agreement, AES-GCM, SHA-1/SHA-256, HMAC), so the library builds with Mbed TLS 3.6 (ESP-IDF 5.x) and Mbed TLS 4 (ESP-IDF 6). Stored private keys keep their format: existing pairings keep working, no re-pairing (tested on a classic ESP32 with ESP-IDF 5.5 and two paired cars, and on an ESP32-C5 with ESP-IDF 6.1).
- **Needs ESP-IDF 5.3 or newer** (ESPHome 2026.9 uses 5.5).

By @davidcoulson.

### Added in dm.7
- **Compile-time log level** `TESLA_BLE_LOG_LEVEL` (0 = error, 1 = warn, 2 = info, 3 = debug, 4 = verbose, the default). Messages above it are left out of the build, so their texts take no flash. Without the define nothing changes. esphome-tesla-ble-multi sets it from the ESPHome logger level.
- **ESP32-C5** in the component manifest targets.

Both by @davidcoulson.

### Fixed in dm.6
- **First command after a wake failed after 25 s (regression in dm.4).** dm.4 stopped the burst of session requests during a wake, but a single request sent the moment VCSEC reports the car awake is ignored by its still-waking infotainment, so the command waited the full 25 s auth timeout and failed (seen on two cars: the first Charge State Poll after every wake). An unanswered session info request is now resent unchanged every second (at most 10 times, still within the 25 s), as vehicle-command does. Same bytes, same request id: whichever copy the car answers is accepted. This also covers any other lost session request.

### Fixed in dm.5
- **Crash (abort) from heap churn.** The received-message processor built a fresh `std::queue` on every loop, before checking whether anything was queued. A `std::deque` allocates even when empty: a map plus a node holding one ~750-byte message, so every loop did two heap allocations. Under BLE and Wi-Fi load one of them eventually failed, and with exceptions disabled that aborted the ESP32 (seen on a Shelly Plus 1, backtrace in `MessageProcessor::process_messages`). It now allocates nothing when idle and takes messages off the queue one at a time. This code came from upstream.
- **Message backlog capped at 16** instead of 1000 (about 750 KB at full size, more heap than an ESP32 has). The oldest message is dropped first, as before.

### Fixed in dm.4
- **One infotainment session request per wake.** A waking car sends a burst of status updates. Once the infotainment session request was out, each further "awake" status restarted the auth and sent another request (5 in 0.6 s seen on a car), all queued behind each other: the first command after a wake took about 8 s instead of 0.5 s. Now only a command still waiting for the wake reacts.
- **A resend is the same message.** When no reply arrives in time, the request is sent again as the identical bytes, as Tesla's vehicle-command does, instead of being rebuilt with a new counter. If the first one did arrive and only the reply was lost, the car sees a duplicate instead of a new command, so toggles (media play / pause, trunk, volume step, horn) are not executed twice. After a new session the request is built again.

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
There are new tests for the volume builder and for reading the media state, artist and title from a response (dm.3), for a wake status burst sending one session request, and for a timed-out request being resent byte for byte (dm.4), and for the message processor: order, nothing processed when idle, messages queued meanwhile wait, bounded backlog (dm.5), and for an unanswered session request being resent unchanged (dm.6).

### Use it
ESP-IDF / ESPHome component:

```yaml
esp32:
  framework:
    components:
      - name: tesla-ble
        source: https://github.com/darek-margas/tesla-ble.git
        ref: v5.2.0-dm.8
```

### Branches and roll back
- `multicar`: the fork's changes (the default branch).
- `main`: mirrors upstream, to sync from.
- Tags are never moved.
  - To roll back to dm.7 (Mbed TLS GCM API, response tags not verified; also for ESP-IDF older than 5.3), use `v5.2.0-dm.7`.
  - To roll back to dm.6 (no compile-time log level), use `v5.2.0-dm.6`.
  - To roll back to dm.5 (session request not resent: first command after a wake may wait 25 s), use `v5.2.0-dm.5`.
  - To roll back to dm.4 (without the crash fix), use `v5.2.0-dm.4`.
  - To roll back to dm.3 (without the wake fix and identical resends), use `v5.2.0-dm.3`.
  - To roll back to dm.2 (no media), use `v5.2.0-dm.2`.
  - To roll back to dm.1 (no scheduled departure), use `v5.2.0-dm.1`.
  - To roll back to plain upstream, use `v5.2.0`.
