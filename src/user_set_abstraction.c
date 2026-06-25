#include <string.h>
#include "os_print.h"
#include "buffer.h"
#include "format.h"
#include "user_set_abstraction.h"

static bool handle_signature_chain_id(const tlv_data_t *data, s_user_set_abstraction_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->user_set_abstraction->signature_chain_id);
}

static bool handle_abstraction(const tlv_data_t *data, s_user_set_abstraction_ctx *out) {
    return get_uint8_t_from_tlv_data(data, &out->user_set_abstraction->abstraction);
}

#define USER_SET_ABSTRACTION_TAGS(X)                                               \
    X(0x23, TAG_SIGNATURE_CHAIN_ID, handle_signature_chain_id, ENFORCE_UNIQUE_TAG) \
    X(0xdf, TAG_ABSTRACTION, handle_abstraction, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(USER_SET_ABSTRACTION_TAGS, NULL, user_set_abstraction_tlv_parser);

static bool verify_user_set_abstraction(const s_user_set_abstraction_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_SIGNATURE_CHAIN_ID, TAG_ABSTRACTION)) {
        PRINTF("Error: incomplete user_set_abstraction struct received!\n");
        return false;
    }
    switch (out->user_set_abstraction->abstraction) {
        case ABSTRACTION_DISABLED:
        case ABSTRACTION_UNIFIED_ACCOUNT:
        case ABSTRACTION_PORTFOLIO_MARGIN:
            break;
        default:
            return false;
    }
    return true;
}

bool parse_user_set_abstraction(const buffer_t *payload, s_user_set_abstraction_ctx *out) {
    if (!user_set_abstraction_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_user_set_abstraction(out)) {
        return false;
    }
    return true;
}

void dump_user_set_abstraction(const s_user_set_abstraction *user_set_abstraction) {
    char tmp[20 + 1];

    PRINTF(">>> USER_SET_ABSTRACTION >>>\n");
    format_u64(tmp, sizeof(tmp), user_set_abstraction->signature_chain_id);
    PRINTF("signature_chain_id = %s\n", tmp);
    PRINTF("abstraction = %u\n", user_set_abstraction->abstraction);
    PRINTF("<<< USER_SET_ABSTRACTION <<<\n");
}

const char *get_abstraction_string(const s_user_set_abstraction *user_set_abstraction) {
    switch (user_set_abstraction->abstraction) {
        case ABSTRACTION_DISABLED:
            return "disabled";
        case ABSTRACTION_UNIFIED_ACCOUNT:
            return "unifiedAccount";
        case ABSTRACTION_PORTFOLIO_MARGIN:
            return "portfolioMargin";
        default:
            break;
    }
    return NULL;
}
