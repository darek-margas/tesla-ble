#pragma once

// Lets the tests build against Mbed TLS 3.6 (ESP-IDF 5.x) and 4.x / TF-PSA-Crypto
// (ESP-IDF 6.x). Mbed TLS 4 removed the legacy GCM / MD / SHA / PK-RNG APIs, so
// tests that compare against them as an independent reference only do so on 3.6.
#include <psa/crypto.h>
#if __has_include(<tf-psa-crypto/build_info.h>)
#include <tf-psa-crypto/build_info.h>
#endif

#ifdef TF_PSA_CRYPTO_VERSION_MAJOR
#define TESLABLE_TEST_HAS_LEGACY_CRYPTO 0
#else
#define TESLABLE_TEST_HAS_LEGACY_CRYPTO 1
#endif
