#include <string.h>
#include "os_print.h"
#include "buffer.h"
#include "format.h"
#include "approve_builder_fee.h"

static bool handle_signature_chain_id(const tlv_data_t *data, s_approve_builder_fee_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->approve_builder_fee->signature_chain_id);
}

static bool handle_max_fee_rate(const tlv_data_t *data, s_approve_builder_fee_ctx *out) {
    return get_string_from_tlv_data(data,
                                    out->approve_builder_fee->max_fee_rate,
                                    1,
                                    sizeof(out->approve_builder_fee->max_fee_rate));
}

static bool handle_builder(const tlv_data_t *data, s_approve_builder_fee_ctx *out) {
    buffer_t buf;

    if (!get_buffer_from_tlv_data(data,
                                  &buf,
                                  sizeof(out->approve_builder_fee->builder),
                                  sizeof(out->approve_builder_fee->builder))) {
        return false;
    }
    memcpy(out->approve_builder_fee->builder, buf.ptr, sizeof(out->approve_builder_fee->builder));
    return true;
}

#define APPROVE_BUILDER_FEE_TAGS(X)                                                \
    X(0x23, TAG_SIGNATURE_CHAIN_ID, handle_signature_chain_id, ENFORCE_UNIQUE_TAG) \
    X(0xb0, TAG_MAX_FEE_RATE, handle_max_fee_rate, ENFORCE_UNIQUE_TAG)             \
    X(0xd3, TAG_BUILDER_ADDR, handle_builder, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(APPROVE_BUILDER_FEE_TAGS, NULL, approve_builder_fee_tlv_parser);

static bool verify_approve_builder_fee(const s_approve_builder_fee_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_SIGNATURE_CHAIN_ID,
                                 TAG_MAX_FEE_RATE,
                                 TAG_BUILDER_ADDR)) {
        PRINTF("Error: incomplete approve_builder_fee struct received!\n");
        return false;
    }
    return true;
}

bool parse_approve_builder_fee(const buffer_t *payload, s_approve_builder_fee_ctx *out) {
    if (!approve_builder_fee_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_approve_builder_fee(out)) {
        return false;
    }
    return true;
}

void dump_approve_builder_fee(const s_approve_builder_fee *approve_builder_fee) {
    char tmp[20 + 1];

    PRINTF(">>> APPROVE_BUILDER_FEE >>>\n");
    format_u64(tmp, sizeof(tmp), approve_builder_fee->signature_chain_id);
    PRINTF("signature_chain_id = %s\n", tmp);
    PRINTF("max_fee_rate = \"%s\"\n", approve_builder_fee->max_fee_rate);
    PRINTF("builder = 0x%.*h\n",
           sizeof(approve_builder_fee->builder),
           approve_builder_fee->builder);
    PRINTF("<<< APPROVE_BUILDER_FEE <<<\n");
}
