# TeslaBLE - A C++ library for communicating with Tesla vehicles over BLE

> **This is the darek-margas fork** of [yoziru/tesla-ble](https://github.com/yoziru/tesla-ble), used by [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi). It verifies the car's replies, fixes a crash and the first command after a wake, adds media, scheduled departure, guest mode and cabin overheat temperature, and builds on ESP-IDF 6. See [This fork](#this-fork) for what changed and how to use it.

This library is designed to communicate with Tesla vehicles locally via the BLE API. It follows the same principles as the official Tesla [vehicle-command](https://github.com/teslamotors/vehicle-command) library (Golang), and is intended for use in embedded systems.

It exists to:

1. Provide a local and offline alternative to the official Tesla API.
2. Avoid the rate limits of the official Tesla API.

The main purpose of this library is to locally manage charging of the vehicle to enable use cases such as charging during off-peak hours, or to manage charging based on solar production. It is not intended to replace the official Tesla API for all use cases.

## This fork

Upstream `v5.2.0` and the upstream `main` commits after it (low power mode and keep accessory power actions), plus the changes below. Each fork release is a tag `v5.2.0-dm.N`; the [fork release notes](.github/fork-release-notes.md) give the details and the roll-back tag for each step.

### Use it

As an ESP-IDF / ESPHome component (needs ESP-IDF 5.3 or newer; ESPHome 2026.9 has 5.5):

```yaml
esp32:
  framework:
    components:
      - name: tesla-ble
        source: https://github.com/darek-margas/tesla-ble.git
        ref: v5.2.0-dm.9
```

Targets: ESP32, ESP32-S3, ESP32-C3, ESP32-C6 and ESP32-C5.

### What is different from upstream

**Security**
- **Replies from the car are authenticated.** Upstream decrypts them but never checks their AES-GCM tag: the received tag is passed to `mbedtls_gcm_finish()` as its output buffer, which overwrites it instead of comparing. A corrupted or forged reply would be accepted. The fork checks the tag, with the counter the car sends in its reply (`AES_GCM_ResponseData.counter`). (dm.8)
- **Crypto on the PSA API** (P-256 key agreement, AES-GCM, SHA-1/SHA-256, HMAC), so the library builds with Mbed TLS 3.6 (ESP-IDF 5.3+) and Mbed TLS 4 (ESP-IDF 6). Stored private keys keep their format: existing pairings keep working. (dm.8)

**Fixes**
- **No crash from heap churn.** `MessageProcessor::process_messages()` built a `std::queue` on every loop, which allocates even when empty. Under BLE and Wi-Fi load an allocation eventually failed and the ESP32 aborted. It now allocates nothing while idle; the message backlog is capped at 16 instead of 1000. (dm.5)
- **One session request per wake.** A waking car sends a burst of status updates, and each one used to send another infotainment session request: the first command after a wake took about 8 s. (dm.4)
- **An unanswered session request is resent unchanged every second** (up to 10 times), as vehicle-command does. A single request sent the moment the car reports awake is often ignored, and the command then waited the full 25 s auth timeout. (dm.6)
- **A command resend is the identical message.** When a reply is lost, the command is resent as the same bytes instead of being rebuilt with a new counter, so the car sees a duplicate and a toggle (trunk, play / pause) is not carried out twice. (dm.4)
- **A late reply to an earlier request logs at DEBUG**, not as a warning. (dm.9)

**Added vehicle actions** (infotainment domain, message fields as in vehicle-command)

| Action | API | Since |
|---|---|---|
| Media state read | `Vehicle::media_state_poll(wake_policy)`, `Vehicle::set_media_state_callback(cb(const CarServer_MediaState &, const MediaNowPlaying &))` | dm.3 |
| Media volume | `media_volume_up()`, `media_volume_down()`, `set_media_volume(0..10)` | dm.3 |
| Media playback | `media_toggle_playback()`, `media_next_track()`, `media_previous_track()`, `media_next_favorite()`, `media_previous_favorite()` | dm.3 |
| Scheduled departure | `set_scheduled_departure(enabled, departure_minutes, preconditioning_policy, off_peak_policy, off_peak_end_minutes)` | dm.2 |
| Guest mode | `set_guest_mode(bool)` | dm.1 |
| Cabin overheat protection temperature | `set_cabin_overheat_protection_temp(level)`: 1 = 30 °C, 2 = 35 °C, 3 = 40 °C | dm.1 |

Media artist and title are unbounded strings that the generated `CarServer_MediaState` does not keep; they are read from the raw reply into `MediaNowPlaying` (up to 128 bytes each). The generated protobuf code is unchanged. Details per action are in the [fork release notes](.github/fork-release-notes.md).

**Build**
- **Compile-time log level:** define `TESLA_BLE_LOG_LEVEL` (0 = error, 1 = warn, 2 = info, 3 = debug, 4 = verbose, the default). Messages above it are left out of the build and take no flash. (dm.7)
- **Mbed TLS 4 host build:** `cmake -B build -DTESLABLE_MBEDTLS_4=ON` builds and tests against Mbed TLS 4 (TF-PSA-Crypto), as in ESP-IDF 6. CI runs the tests against both. (dm.8)
- **ESP32-C5** in the component targets. (dm.7)

### Branches, releases and upstream

- `multicar` (default): the fork. `main` mirrors upstream, to sync from.
- A change to `FORK_VERSION` on `multicar` runs the tests (Mbed TLS 3.6 and 4) and publishes the tag and release. Tags are never moved.
- The changes are offered upstream: [#90](https://github.com/yoziru/tesla-ble/pull/90) (vehicle actions), [#94](https://github.com/yoziru/tesla-ble/pull/94) (crash fix), [#95](https://github.com/yoziru/tesla-ble/pull/95) (wake and resends), and by [@davidcoulson](https://github.com/davidcoulson) [#91](https://github.com/yoziru/tesla-ble/pull/91) (PSA crypto and verified replies), [#92](https://github.com/yoziru/tesla-ble/pull/92) (ESP32-C5), [#93](https://github.com/yoziru/tesla-ble/pull/93) (log level).
- Problems: please report them at [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi/issues); issues are off on this fork.

## Usage

This project is intended to be used as a library in your own project. It is not a standalone application.

[yoziru/esphome-tesla-ble](https://github.com/yoziru/esphome-tesla-ble) is an ESPHome project that uses this library to control your Tesla vehicle charging. [darek-margas/esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi) uses this fork to control several cars from one ESP32.

Several examples are included for your convenience.

```sh
cd examples/simple/
cmake .
make
```

## Building and Testing

### Quick Start

```bash
# Clone the repository
git clone https://github.com/darek-margas/tesla-ble.git   # this fork (upstream: yoziru/tesla-ble)
cd tesla-ble

# Setup development environment (optional but recommended)
./scripts/setup_dev_environment.sh

# Build the project
cmake -B build
cmake --build build

# Run tests
./scripts/run_tests.sh
```

### Manual Build

```bash
# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the library and tests
cmake --build build --config Release

# Run tests with CTest
cd build
ctest --build-config Release --output-on-failure
```

### Running Tests

#### Using the Test Runner Script

The repository includes a comprehensive test runner script with various options:

```bash
# Run tests in Debug mode (default)
./scripts/run_tests.sh

# Run tests in Release mode
./scripts/run_tests.sh --release

# Run tests with coverage analysis
./scripts/run_tests.sh --coverage

# Run tests with Valgrind (Linux only)
./scripts/run_tests.sh --valgrind

# Clean build and run tests
./scripts/run_tests.sh --clean

# Get help on available options
./scripts/run_tests.sh --help
```

#### Manual Test Execution

```bash
# Build and run all Tesla BLE tests (excludes dependency tests)
cd build
ctest --output-on-failure --verbose

# Run specific test suites
./tests/test_client
./tests/test_key_generation
./tests/test_message_building
./tests/test_message_parsing
./tests/test_session_management
./tests/test_protocol_compliance
./tests/test_utils

# Run the complete test suite
./tests/tesla_ble_tests
```

### Test Coverage

To generate a coverage report:

```bash
# Install coverage tools (Ubuntu/Debian)
sudo apt-get install lcov

# Run tests with coverage
./scripts/run_tests.sh --coverage

# Coverage report will be generated in build/coverage_html/
```

### Static Analysis

The project includes static analysis via cppcheck:

```bash
# Install cppcheck
sudo apt-get install cppcheck  # Ubuntu/Debian
brew install cppcheck          # macOS

# Run static analysis
cppcheck --enable=all --inconclusive \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    src/ include/ examples/
```

### Code Formatting and Linting

The project uses clang-format for code formatting and clang-tidy for static analysis:

```bash
# Install clang tools
sudo apt-get install clang-format clang-tidy  # Ubuntu/Debian
brew install clang-format clang-tidy          # macOS

# Run formatting and linting checks (will configure CMake if needed)
./scripts/lint.sh

# Or run individually:
# Format code
find src include tests examples -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check formatting
find src include tests examples -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror --style=file

# Run clang-tidy on source files (requires compile_commands.json)
find src -name "*.cpp" | xargs clang-tidy
```

### Development Environment Setup

Use the provided script to set up your development environment:

```bash
./scripts/setup_dev_environment.sh
```

This script will:

- Detect your operating system
- Install required dependencies (cmake, compiler, etc.)
- Install optional development tools (lcov, cppcheck, valgrind)
- Set up git hooks for automatic testing

### Continuous Integration

GitHub Actions (`.github/workflows/cmake.yml`) runs on every push and pull request to `main` and `multicar`:

- **Format check**: clang-format 21
- **Tidy check**: clang-tidy 21, warnings are errors
- **Build and test**: the library and all tests, against Mbed TLS 3.6
- **Build and test (Mbed TLS 4)**: the same tests against Mbed TLS 4 (TF-PSA-Crypto)
- **Build examples**: builds and runs `examples/simple`

### Test Structure

The test suite is organized into several categories:

- **`test_client.cpp`**: Core client functionality and initialization
- **`test_key_generation.cpp`**: Private key generation and loading
- **`test_message_building.cpp`**: Message construction for various commands
- **`test_message_parsing.cpp`**: Parsing of received messages
- **`test_session_management.cpp`**: Session handling and peer management
- **`test_utils.cpp`**: Utility functions and helper methods
- **`test_vehicle.cpp`**: Vehicle state management and command processing
- **`test_exponential_backoff.cpp`**: Exponential backoff retry logic

Each test file contains comprehensive unit tests covering both success and failure scenarios, edge cases, and parameter validation.

### State Architecture

The library uses a unified state pattern for command processing with the following states:

- **`IDLE`**: Initial state for new commands
- **`AUTHENTICATING`**: Unified authentication initiation (replaces legacy domain-specific states)
- **`AUTH_RESPONSE_WAITING`**: Unified authentication response waiting
- **`READY`**: Command ready to be sent
- **`WAITING_FOR_RESPONSE`**: Waiting for command response
- **`COMPLETED`**: Command completed successfully
- **`FAILED`**: Command failed

This architecture provides a clean, maintainable approach to command lifecycle management with exponential backoff for retries.

### Dependencies

- [nanopb](https://github.com/nanopb/nanopb)
- [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) 3.6 or 4.x, through its PSA crypto API
  - NOTE: needs ESP-IDF 5.3 or newer (Mbed TLS 3.6). ESP-IDF 6 ships Mbed TLS 4 and works too. Older ESP-IDF versions are not compatible with this fork; `v5.2.0-dm.7` is the last tag that builds on ESP-IDF 5.0–5.2.

## Features

- [x] Implements Tesla's BLE [protocol](https://github.com/teslamotors/vehicle-command/blob/main/pkg/protocol/protocol.md)
- [x] AES-GCM key generation
- [x] [Metadata serialization](https://github.com/teslamotors/vehicle-command/blob/main/pkg/protocol/protocol.md#metadata-serialization)
- [x] Supports `UniversalMessage.RoutableMessage` encoding and decoding
  - [x] Supports Vehicle Security (VSSEC) payload
  - [x] Supports Infotainment payload
- [x] Replies from the car authenticated (AES-GCM tag checked) (fork)
- [x] Session info requests and timed-out commands resent unchanged, as vehicle-command does (fork)
- [x] Media state and controls, scheduled departure, guest mode, cabin overheat protection temperature (fork)

# Credits

This fork builds on the original version by [pmdroid](https://github.com/pmdroid/tesla-ble/tree/main).

The darek-margas fork is maintained for [esphome-tesla-ble-multi](https://github.com/darek-margas/esphome-tesla-ble-multi) on top of [yoziru/tesla-ble](https://github.com/yoziru/tesla-ble). The PSA crypto port and reply authentication, the compile-time log level, ESP32-C5 and the Mbed TLS 4 tests are by [@davidcoulson](https://github.com/davidcoulson).

# IMPORTANT

Please take note that this library does not have official backing from Tesla, and its operational capabilities may be discontinued without prior notice. It's essential to recognize that this library retains private keys and other sensitive data on your device without encryption. I would like to stress that I assume no liability for any possible (although highly unlikely) harm that may befall your vehicle.
