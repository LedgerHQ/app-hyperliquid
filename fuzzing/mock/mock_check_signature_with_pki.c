/*
 * Pure-validation gate override for fuzz builds.
 *
 * The real check_signature_with_pki() (lib_pki/ledger_pki.c) verifies a
 * PKI certificate before letting handler_provide_action_metadata() accept
 * the operation descriptor. Under fuzzing, the certificate machinery is
 * stubbed (os_pki_* return defaults) so the real check would always fail
 * with MISSING_CERTIFICATE, leaving the entire SET_ACTION / SIGN_ACTION
 * pipeline unreachable.
 *
 * This is a pure-validation gate: it does not copy or arithmetically
 * combine the fuzzer-controlled bytes. Returning SUCCESS unconditionally
 * exposes the downstream TLV parsers and EIP-712 hashing to the fuzzer
 * without weakening any memory-sensitive code paths.
 */

#include "ledger_pki.h"
#include "buffer.h"
#include "ox_ec.h"

check_signature_with_pki_status_t check_signature_with_pki(const buffer_t hash,
                                                           const uint8_t *expected_key_usage,
                                                           const cx_curve_t *expected_curve,
                                                           const buffer_t signature) {
    (void) hash;
    (void) expected_key_usage;
    (void) expected_curve;
    (void) signature;
    return CHECK_SIGNATURE_WITH_PKI_SUCCESS;
}
