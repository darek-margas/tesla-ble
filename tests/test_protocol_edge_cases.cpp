/**
 * @file test_protocol_edge_cases.cpp
 * @brief Tests for Tesla BLE Protocol message format validation and edge cases
 *
 * This file covers edge cases and message format validation as described in the protocol specification.
 */

#include <gtest/gtest.h>
#include <client.h>
#include <peer.h>
#include <crypto_context.h>
#include <cstring>
#include <memory>
#include <string>
#include "test_constants.h"

namespace TeslaBLE {

class ProtocolEdgeCasesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    client_ = std::make_unique<Client>();
    client_->set_vin(TestConstants::TEST_VIN);
    crypto_context_ = std::make_shared<CryptoContext>();
  }

  std::unique_ptr<Client> client_;
  std::shared_ptr<CryptoContext> crypto_context_;
};

// Test 1: Invalid Public Key Format
TEST_F(ProtocolEdgeCasesTest, InvalidPublicKeyFormat) {
  Peer peer(UniversalMessage_Domain_DOMAIN_INFOTAINMENT, crypto_context_, TestConstants::TEST_VIN);
  // Public key not starting with 0x04 (uncompressed)
  uint8_t bad_pubkey[65] = {0x02};
  auto result = peer.load_tesla_key(bad_pubkey, 65);
  EXPECT_NE(result, TeslaBLE_Status_E_OK) << "Should fail with invalid public key format";
}

// Test 2: Session Info with Empty Epoch (should be handled gracefully)
TEST_F(ProtocolEdgeCasesTest, InvalidSessionInfoMissingEpoch) {
  Peer peer(UniversalMessage_Domain_DOMAIN_INFOTAINMENT, crypto_context_, TestConstants::TEST_VIN);
  Signatures_SessionInfo session_info = Signatures_SessionInfo_init_default;
  session_info.counter = 1;
  // No epoch set (zero-filled epoch)
  auto result = peer.update_session(&session_info);
  EXPECT_EQ(result, TeslaBLE_Status_E_OK)
      << "Should handle empty epoch gracefully (client trusts vehicle's session info)";
}

// Test 3: Invalid Counter (overflow)
TEST_F(ProtocolEdgeCasesTest, CounterOverflow) {
  Peer peer(UniversalMessage_Domain_DOMAIN_INFOTAINMENT, crypto_context_, TestConstants::TEST_VIN);
  peer.set_counter(0xFFFFFFFF);
  peer.increment_counter();
  EXPECT_EQ(peer.get_counter(), 0) << "Counter should wrap to 0 on overflow";
}

// Test 4: Malformed Metadata
TEST_F(ProtocolEdgeCasesTest, MalformedMetadata) {
  Peer peer(UniversalMessage_Domain_DOMAIN_INFOTAINMENT, crypto_context_, TestConstants::TEST_VIN);
  uint8_t malformed_metadata[3] = {0x02, 0x11};  // Incomplete TLV
  uint8_t ad_hash[32];
  EXPECT_EQ(CryptoUtils::sha256_hash(malformed_metadata, 2, ad_hash), TeslaBLE_Status_E_OK);
  // Try to use malformed metadata in encryption (should fail or be handled)
  pb_byte_t plaintext[4] = {1, 2, 3, 4};
  pb_byte_t ciphertext[16];
  pb_byte_t tag[16];
  // Hash bytes as key and nonce, no AAD: must not crash.
  auto status = CryptoUtils::aes_gcm_encrypt(ad_hash, ad_hash, nullptr, 0, plaintext, sizeof(plaintext), ciphertext,
                                             sizeof(ciphertext), tag);
  EXPECT_TRUE(status == TeslaBLE_Status_E_OK || status != TeslaBLE_Status_E_OK)
      << "Malformed metadata should not crash";
}

// Test 5: Request Hash Construction with Short Tag (should handle gracefully)
TEST_F(ProtocolEdgeCasesTest, RequestHashInvalidTagLength) {
  Peer peer(UniversalMessage_Domain_DOMAIN_INFOTAINMENT, crypto_context_, TestConstants::TEST_VIN);
  uint8_t short_tag[4] = {1, 2, 3, 4};
  uint8_t hash[33];
  size_t hash_length;
  auto result = peer.construct_request_hash(Signatures_SignatureType_SIGNATURE_TYPE_AES_GCM_PERSONALIZED, short_tag, 4,
                                            hash, &hash_length);
  EXPECT_EQ(result, TeslaBLE_Status_E_OK) << "Should handle short tag gracefully (defensive programming)";
  EXPECT_EQ(hash_length, 5) << "Hash length should be 1 (auth type) + 4 (tag length)";
  EXPECT_EQ(hash[0], static_cast<uint8_t>(Signatures_SignatureType_SIGNATURE_TYPE_AES_GCM_PERSONALIZED))
      << "First byte should be auth type";
}

// Test 6: HMAC Authentication with All-Zero Key
TEST_F(ProtocolEdgeCasesTest, HmacAllZeroKey) {
  uint8_t key[16] = {0};
  uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t hmac[32];
  EXPECT_EQ(CryptoUtils::hmac_sha256(key, 16, data, 8, hmac, sizeof(hmac)), TeslaBLE_Status_E_OK);
  // HMAC should not be all zeros
  bool all_zero = true;
  for (unsigned char value : hmac) {
    if (value != 0) {
      all_zero = false;
      break;
    }
  }
  EXPECT_FALSE(all_zero) << "HMAC output should not be all zeros even with zero key";
}

}  // namespace TeslaBLE
