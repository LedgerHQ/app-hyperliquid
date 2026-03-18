#include <stddef.h>
#include "os_print.h"
#include "cx.h"
#include "tlv_library.h"
#include "os_pki.h"
#include "ledger_pki.h"
#include "format.h"
#include "action_metadata.h"
#include "hl_context.h"

#define CERTIFICATE_PUBLIC_KEY_USAGE_PERPS_DATA 0x11

#define STRUCT_TYPE 0x2b

typedef struct {
    TLV_reception_t received_tags;
    cx_sha256_t hash_ctx;
    buffer_t signature;
    s_action_metadata metadata;
} s_action_metadata_ctx;

static bool handle_struct_type(const tlv_data_t *data, s_action_metadata_ctx *out) {
    uint8_t struct_type;

    (void) out;
    if (!get_uint8_t_from_tlv_data(data, &struct_type)) {
        return false;
    }
    if (struct_type != STRUCT_TYPE) {
        PRINTF("Error: unknown struct type (0x%02x)!\n", struct_type);
        return false;
    }
    return true;
}

static bool handle_struct_version(const tlv_data_t *data, s_action_metadata_ctx *out) {
    uint8_t struct_version;

    (void) out;
    if (!get_uint8_t_from_tlv_data(data, &struct_version)) {
        return false;
    }
    if (struct_version != 1) {
        PRINTF("Error: unsupported struct version (%u)!\n", struct_version);
        return false;
    }
    return true;
}

static bool handle_operation_type(const tlv_data_t *data, s_action_metadata_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->metadata.op_type)) {
        return false;
    }
    switch (out->metadata.op_type) {
        case OP_TYPE_ORDER:
        case OP_TYPE_MODIFY:
        case OP_TYPE_CANCEL:
        case OP_TYPE_CLOSE:
        case OP_TYPE_UPDATE_MARGIN:
            break;
        case OP_TYPE_UPDATE_LEVERAGE:
        default:
            PRINTF("Error: unknown operation type (%u)!\n", out->metadata.op_type);
            return false;
    }
    return true;
}

static bool handle_asset_id(const tlv_data_t *data, s_action_metadata_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->metadata.asset_id);
}

static bool handle_asset_ticker(const tlv_data_t *data, s_action_metadata_ctx *out) {
    return get_string_from_tlv_data(data,
                                    out->metadata.asset_ticker,
                                    1,
                                    sizeof(out->metadata.asset_ticker));
}

static bool handle_network(const tlv_data_t *data, s_action_metadata_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->metadata.network)) {
        return false;
    }
    switch (out->metadata.network) {
        case NETWORK_MAINNET:
        case NETWORK_TESTNET:
            break;
        default:
            PRINTF("Error: unknown network (%u)!\n", out->metadata.network);
            return false;
    }
    return true;
}

static bool handle_builder_addr(const tlv_data_t *data, s_action_metadata_ctx *out) {
    buffer_t buf;

    if (!get_buffer_from_tlv_data(data,
                                  &buf,
                                  sizeof(out->metadata.builder_addr),
                                  sizeof(out->metadata.builder_addr))) {
        return false;
    }
    memcpy(out->metadata.builder_addr, buf.ptr, sizeof(out->metadata.builder_addr));
    out->metadata.has_builder_addr = true;
    return true;
}

static bool handle_margin(const tlv_data_t *data, s_action_metadata_ctx *out) {
    if (!get_uint64_t_from_tlv_data(data, &out->metadata.margin)) {
        return false;
    }
    out->metadata.has_margin = true;
    return true;
}

static bool handle_leverage(const tlv_data_t *data, s_action_metadata_ctx *out) {
    if (!get_uint32_t_from_tlv_data(data, &out->metadata.leverage)) {
        return false;
    }
    out->metadata.has_leverage = true;
    return true;
}

static bool handle_signature(const tlv_data_t *data, s_action_metadata_ctx *out) {
    return get_buffer_from_tlv_data(data, &out->signature, 70, 72);
}

#define METADATA_TAGS(X)                                                   \
    X(0x01, TAG_STRUCT_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCT_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0xd0, TAG_OPERATION_TYPE, handle_operation_type, ENFORCE_UNIQUE_TAG) \
    X(0xd1, TAG_ASSET_ID, handle_asset_id, ENFORCE_UNIQUE_TAG)             \
    X(0x24, TAG_ASSET_TICKER, handle_asset_ticker, ENFORCE_UNIQUE_TAG)     \
    X(0xd2, TAG_NETWORK, handle_network, ENFORCE_UNIQUE_TAG)               \
    X(0xd3, TAG_BUILDER_ADDR, handle_builder_addr, ENFORCE_UNIQUE_TAG)     \
    X(0xd4, TAG_MARGIN, handle_margin, ENFORCE_UNIQUE_TAG)                 \
    X(0xd5, TAG_LEVERAGE, handle_leverage, ENFORCE_UNIQUE_TAG)             \
    X(0x15, TAG_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

static bool handle_common(const tlv_data_t *, s_action_metadata_ctx *);

DEFINE_TLV_PARSER(METADATA_TAGS, &handle_common, metadata_tlv_parser);

static bool handle_common(const tlv_data_t *data, s_action_metadata_ctx *metadata) {
    if (data->tag != TAG_SIGNATURE) {
        if (cx_hash_update((cx_hash_t *) &metadata->hash_ctx, data->raw.ptr, data->raw.size) !=
            CX_OK) {
            return false;
        }
    }
    return true;
}

static bool verify_action_metadata(const s_action_metadata_ctx *out) {
    uint8_t hash[32];
    buffer_t hash_buf;
    uint8_t key_usage = CERTIFICATE_PUBLIC_KEY_USAGE_PERPS_DATA;
    cx_curve_t curve = CX_CURVE_SECP256K1;

    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_STRUCT_TYPE,
                                 TAG_STRUCT_VERSION,
                                 TAG_OPERATION_TYPE,
                                 TAG_ASSET_ID,
                                 TAG_ASSET_TICKER,
                                 TAG_NETWORK,
                                 TAG_SIGNATURE)) {
        PRINTF("Error: incomplete action metadata struct received!\n");
        return false;
    }
    if (cx_hash_final((cx_hash_t *) &out->hash_ctx, hash) != CX_OK) {
        PRINTF("Error: could not finalize struct hash!\n");
        return false;
    }
    hash_buf.ptr = hash;
    hash_buf.size = sizeof(hash);
    hash_buf.offset = 0;
    if (check_signature_with_pki(hash_buf, &key_usage, &curve, out->signature) !=
        CHECK_SIGNATURE_WITH_PKI_SUCCESS) {
        PRINTF("Error: could not successfully verify struct signature!\n");
        return false;
    }
    return true;
}

static void dump_action_metadata(const s_action_metadata *action_metadata) {
    // bigger value than necessary until the SDK function is fixed
    char tmp[NUMERIC_STRING_LENGTH + MARGIN_DECIMALS + 1] = {0};

    (void) action_metadata;  // to prevent warnings for release builds
    PRINTF(">>> ACTION_METADATA >>>\n");
    PRINTF("operation_type = %u\n", action_metadata->op_type);
    PRINTF("asset_id = %u\n", action_metadata->asset_id);
    PRINTF("asset_ticker = \"%s\"\n", action_metadata->asset_ticker);
    PRINTF("network = %u\n", action_metadata->network);
    if (action_metadata->has_builder_addr) {
        PRINTF("builder_addr = 0x%.*h\n",
               sizeof(action_metadata->builder_addr),
               action_metadata->builder_addr);
    }
    if (action_metadata->has_margin) {
        format_fpu64_trimmed(tmp, sizeof(tmp), action_metadata->margin, MARGIN_DECIMALS);
        PRINTF("margin = %s\n", tmp);
    }
    if (action_metadata->has_leverage) {
        PRINTF("leverage = %u\n", action_metadata->leverage);
    }
    PRINTF("<<< ACTION_METADATA <<<\n");
}

bool parse_action_metadata(const buffer_t *payload) {
    s_action_metadata_ctx out = {0};

    if (payload == NULL) {
        return false;
    }
    if (cx_sha256_init_no_throw(&out.hash_ctx) != CX_OK) {
        return false;
    }
    if (!metadata_tlv_parser(payload, &out, &out.received_tags)) {
        return false;
    }
    if (!verify_action_metadata(&out)) {
        return false;
    }
    if (!ctx_current_action_is_first()) {
        // a signature flow was started but not finished, could be a bug or something malicious
        // return false to indicate failure but still reset so that a new one can be started after
        // (otherwise the app could be stuck in this state)
        ctx_reset();
        return false;
    }
    dump_action_metadata(&out.metadata);
    ctx_reset();
    ctx_save_action_metadata(&out.metadata);
    return true;
}
