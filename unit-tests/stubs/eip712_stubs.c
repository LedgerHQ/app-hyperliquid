/* Stub implementations of eip712 hash functions.
 * action_hash() calls these; parse_action() does not.
 * They exist only to satisfy the linker for unit tests.
 */
#include <stdbool.h>
#include <stdint.h>

bool eip712_builder_fee_hash(const uint64_t *chain_id,
                             const char     *chain,
                             const char     *max_fee_rate,
                             const uint8_t  *builder,
                             const uint64_t *nonce,
                             uint8_t        *domain_hash,
                             uint8_t        *message_hash) {
    (void) chain_id; (void) chain; (void) max_fee_rate;
    (void) builder; (void) nonce; (void) domain_hash; (void) message_hash;
    return false;
}

bool eip712_cid_hash(const char    *source,
                     const uint8_t *cid,
                     uint8_t       *domain_hash,
                     uint8_t       *message_hash) {
    (void) source; (void) cid; (void) domain_hash; (void) message_hash;
    return false;
}
