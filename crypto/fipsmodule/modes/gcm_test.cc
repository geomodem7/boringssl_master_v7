/* Copyright (c) 2019, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <gtest/gtest.h>

#include <openssl/aes.h>

#include "../../internal.h"
#include "../../test/abi_test.h"
#include "../aes/internal.h"
#include "internal.h"


#if defined(SUPPORTS_ABI_TEST) && !defined(OPENSSL_NO_ASM)
TEST(GCMTest, ABI) {
  static const uint64_t kH[2] = {
      UINT64_C(0x66e94bd4ef8a2c3b),
      UINT64_C(0x884cfa59ca342b2e),
  };
  static const size_t kBlockCounts[] = {1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 31, 32};
  uint8_t buf[16 * 32];
  OPENSSL_memset(buf, 42, sizeof(buf));

  uint8_t X[16] = {0x92, 0xa3, 0xb3, 0x60, 0xce, 0xda, 0x88, 0x03,
                   0x78, 0xfe, 0xb2, 0x71, 0xb9, 0xc2, 0x28, 0xf3};

  alignas(16) u128 Htable[16];
#if defined(GHASH_ASM_X86) || defined(GHASH_ASM_X86_64)
  if (CRYPTO_is_SSSE3_capable()) {
    CHECK_ABI_SEH(gcm_init_ssse3, Htable, kH);
    CHECK_ABI_SEH(gcm_gmult_ssse3, X, Htable);
    for (size_t blocks : kBlockCounts) {
      CHECK_ABI_SEH(gcm_ghash_ssse3, X, Htable, buf, 16 * blocks);
    }
  }

  if (crypto_gcm_clmul_enabled()) {
    CHECK_ABI_SEH(gcm_init_clmul, Htable, kH);
    CHECK_ABI_SEH(gcm_gmult_clmul, X, Htable);
    for (size_t blocks : kBlockCounts) {
      CHECK_ABI_SEH(gcm_ghash_clmul, X, Htable, buf, 16 * blocks);
    }

#if defined(GHASH_ASM_X86_64)
    if (CRYPTO_is_AVX_capable() && CRYPTO_is_MOVBE_capable()) {
      CHECK_ABI_SEH(gcm_init_avx, Htable, kH);
      CHECK_ABI_SEH(gcm_gmult_avx, X, Htable);
      for (size_t blocks : kBlockCounts) {
        CHECK_ABI_SEH(gcm_ghash_avx, X, Htable, buf, 16 * blocks);
      }

      if (hwaes_capable()) {
        AES_KEY aes_key;
        static const uint8_t kKey[16] = {0};
        uint8_t iv[16] = {0};

        aes_hw_set_encrypt_key(kKey, 128, &aes_key);
        for (size_t blocks : kBlockCounts) {
          CHECK_ABI_SEH(aesni_gcm_encrypt, buf, buf, blocks * 16, &aes_key, iv,
                        Htable, X);
          CHECK_ABI_SEH(aesni_gcm_encrypt, buf, buf, blocks * 16 + 7, &aes_key,
                        iv, Htable, X);
        }
        aes_hw_set_decrypt_key(kKey, 128, &aes_key);
        for (size_t blocks : kBlockCounts) {
          CHECK_ABI_SEH(aesni_gcm_decrypt, buf, buf, blocks * 16, &aes_key, iv,
                        Htable, X);
          CHECK_ABI_SEH(aesni_gcm_decrypt, buf, buf, blocks * 16 + 7, &aes_key,
                        iv, Htable, X);
        }
      }
    }
#endif  // GHASH_ASM_X86_64
  }
#endif  // GHASH_ASM_X86 || GHASH_ASM_X86_64

#if defined(GHASH_ASM_ARM)
  if (gcm_neon_capable()) {
    CHECK_ABI(gcm_init_neon, Htable, kH);
    CHECK_ABI(gcm_gmult_neon, X, Htable);
    for (size_t blocks : kBlockCounts) {
      CHECK_ABI(gcm_ghash_neon, X, Htable, buf, 16 * blocks);
    }
  }

  if (gcm_pmull_capable()) {
    CHECK_ABI(gcm_init_v8, Htable, kH);
    CHECK_ABI(gcm_gmult_v8, X, Htable);
    for (size_t blocks : kBlockCounts) {
      CHECK_ABI(gcm_ghash_v8, X, Htable, buf, 16 * blocks);
    }
  }
#endif  // GHASH_ASM_ARM

#if defined(OPENSSL_AARCH64) && defined(HW_GCM)
  if (hwaes_capable() && gcm_pmull_capable()) {
    static const uint8_t kKey[16] = {0};
    uint8_t iv[16] = {0};

    for (size_t key_bits = 128; key_bits <= 256; key_bits += 64) {
      AES_KEY aes_key;
      aes_hw_set_encrypt_key(kKey, key_bits, &aes_key);
      CHECK_ABI(aes_gcm_enc_kernel, buf, sizeof(buf) * 8, buf, X, iv, &aes_key,
                Htable);
      CHECK_ABI(aes_gcm_dec_kernel, buf, sizeof(buf) * 8, buf, X, iv, &aes_key,
                Htable);
    }
  }
#endif
}
#endif  // SUPPORTS_ABI_TEST && !OPENSSL_NO_ASM