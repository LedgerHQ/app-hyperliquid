#include <string.h>
#include "os_print.h"
#include "tlv_library.h"
#include "format.h"  // format_u64
#include "write.h"
#include "cmp.h"
#include "action.h"
#include "hl_context.h"
#include "eip712_builder_fee.h"
#include "eip712_cid.h"

#define STRUCT_TYPE 0x2c

#define MAX_SERIALIZED_ACTION_SIZE 1024

typedef struct {
    TLV_reception_t received_tags;
    s_action action;
    union {
        s_bulk_order_ctx bulk_order_ctx;
        s_bulk_modify_ctx bulk_modify_ctx;
        s_bulk_cancel_ctx bulk_cancel_ctx;
        s_update_leverage_ctx update_leverage_ctx;
        s_approve_builder_fee_ctx approve_builder_fee_ctx;
        s_update_isolated_margin_ctx update_isolated_margin_ctx;
    };
} s_action_ctx;

static uint8_t g_raw_buf[MAX_SERIALIZED_ACTION_SIZE];

static bool handle_struct_type(const tlv_data_t *data, s_action_ctx *out) {
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

static bool handle_struct_version(const tlv_data_t *data, s_action_ctx *out) {
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

static bool handle_action_type(const tlv_data_t *data, s_action_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->action.type)) {
        return false;
    }
    switch (out->action.type) {
        case ACTION_TYPE_BULK_ORDER:
        case ACTION_TYPE_BULK_MODIFY:
        case ACTION_TYPE_BULK_CANCEL:
        case ACTION_TYPE_UPDATE_LEVERAGE:
        case ACTION_TYPE_APPROVE_BUILDER_FEE:
        case ACTION_TYPE_UPDATE_ISOLATED_MARGIN:
            break;
        default:
            PRINTF("Error: unsupported action type (%u)!\n", out->action.type);
            return false;
    }
    return true;
}

static bool handle_nonce(const tlv_data_t *data, s_action_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->action.nonce);
}

static bool handle_action(const tlv_data_t *, s_action_ctx *);

#define ACTION_TAGS(X)                                                     \
    X(0x01, TAG_STRUCT_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCT_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0xd0, TAG_ACTION_TYPE, handle_action_type, ENFORCE_UNIQUE_TAG)       \
    X(0xda, TAG_NONCE, handle_nonce, ENFORCE_UNIQUE_TAG)                   \
    X(0xdb, TAG_ACTION, handle_action, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(ACTION_TAGS, NULL, action_tlv_parser);

static bool handle_action(const tlv_data_t *data, s_action_ctx *out) {
    bool ret;

    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ACTION_TYPE)) {
        PRINTF("Error: received an action structure before receiving its type!\n");
        return false;
    }

    switch (out->action.type) {
        case ACTION_TYPE_BULK_ORDER:
            out->bulk_order_ctx.bulk_order = &out->action.bulk_order;
            ret = parse_bulk_order(&data->value, &out->bulk_order_ctx);
            break;
        case ACTION_TYPE_BULK_MODIFY:
            out->bulk_modify_ctx.bulk_modify = &out->action.bulk_modify;
            ret = parse_bulk_modify(&data->value, &out->bulk_modify_ctx);
            break;
        case ACTION_TYPE_BULK_CANCEL:
            out->bulk_cancel_ctx.bulk_cancel = &out->action.bulk_cancel;
            ret = parse_bulk_cancel(&data->value, &out->bulk_cancel_ctx);
            break;
        case ACTION_TYPE_UPDATE_LEVERAGE:
            out->update_leverage_ctx.update_leverage = &out->action.update_leverage;
            ret = parse_update_leverage(&data->value, &out->update_leverage_ctx);
            break;
        case ACTION_TYPE_APPROVE_BUILDER_FEE:
            out->approve_builder_fee_ctx.approve_builder_fee = &out->action.approve_builder_fee;
            ret = parse_approve_builder_fee(&data->value, &out->approve_builder_fee_ctx);
            break;
        case ACTION_TYPE_UPDATE_ISOLATED_MARGIN:
            out->update_isolated_margin_ctx.update_isolated_margin =
                &out->action.update_isolated_margin;
            ret = parse_update_isolated_margin(&data->value, &out->update_isolated_margin_ctx);
            break;
        default:
            ret = false;
    }
    return ret;
}

static bool verify_action(const s_action_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_STRUCT_TYPE,
                                 TAG_STRUCT_VERSION,
                                 TAG_ACTION_TYPE,
                                 TAG_NONCE,
                                 TAG_ACTION)) {
        PRINTF("Error: incomplete action struct received!\n");
        return false;
    }
    return true;
}

static void dump_action(const s_action *action) {
    char tmp[20 + 1];

    PRINTF(">>> ACTION >>>\n");
    PRINTF("type = %u\n", action->type);
    format_u64(tmp, sizeof(tmp), action->nonce);
    PRINTF("nonce = %s\n", tmp);
    switch (action->type) {
        case ACTION_TYPE_BULK_ORDER:
            dump_bulk_order(&action->bulk_order);
            break;
        case ACTION_TYPE_BULK_MODIFY:
            dump_bulk_modify(&action->bulk_modify);
            break;
        case ACTION_TYPE_BULK_CANCEL:
            dump_bulk_cancel(&action->bulk_cancel);
            break;
        case ACTION_TYPE_UPDATE_LEVERAGE:
            dump_update_leverage(&action->update_leverage);
            break;
        case ACTION_TYPE_APPROVE_BUILDER_FEE:
            dump_approve_builder_fee(&action->approve_builder_fee);
            break;
        case ACTION_TYPE_UPDATE_ISOLATED_MARGIN:
            dump_update_isolated_margin(&action->update_isolated_margin);
            break;
        default:
            PRINTF("Error: cannot dump unknown action type\n");
    }
    PRINTF("<<< ACTION <<<\n");
}

/**
 * Validate that the action type being pushed is consistent with the operation
 * type declared in the metadata. This prevents a malicious host from injecting
 * an unrelated action (e.g. a BULK_ORDER) into a cancel session: the UI is
 * built from the action fetched by type, but signing iterates by index, so a
 * rogue action at a lower index would be signed without ever being displayed.
 *
 * For OP_TYPE_MODIFY the list must contain exactly one action (either
 * BULK_MODIFY or BULK_ORDER, not both), so we also reject a second compatible
 * action if one is already present.
 */
static bool is_action_compatible(e_action_type action_type, e_operation_type op_type) {
    switch (op_type) {
        case OP_TYPE_ORDER:
            // UPDATE_LEVERAGE and APPROVE_BUILDER_FEE may accompany a bulk
            // order. The builder fee approval is signed silently by design —
            // see the comment in bulk_order.c handle_builder().
            return action_type == ACTION_TYPE_BULK_ORDER ||
                   action_type == ACTION_TYPE_UPDATE_LEVERAGE ||
                   action_type == ACTION_TYPE_APPROVE_BUILDER_FEE;
        case OP_TYPE_MODIFY:
            // A modify is represented by exactly one action: either BULK_MODIFY
            // or BULK_ORDER.  Reject if a compatible action is already present
            // to prevent a second action from being signed silently.
            if (action_type != ACTION_TYPE_BULK_MODIFY && action_type != ACTION_TYPE_BULK_ORDER) {
                return false;
            }
            return ctx_get_action(ACTION_TYPE_BULK_MODIFY) == NULL &&
                   ctx_get_action(ACTION_TYPE_BULK_ORDER) == NULL;
        case OP_TYPE_CANCEL:
        case OP_TYPE_CANCEL_SL:
        case OP_TYPE_CANCEL_TP:
        case OP_TYPE_CANCEL_TP_SL:
            return action_type == ACTION_TYPE_BULK_CANCEL;
        case OP_TYPE_CLOSE:
            return action_type == ACTION_TYPE_BULK_ORDER;
        case OP_TYPE_UPDATE_MARGIN:
            return action_type == ACTION_TYPE_UPDATE_ISOLATED_MARGIN;
        default:
            break;
    }
    return false;
}

bool parse_action(const buffer_t *payload) {
    s_action_ctx out = {0};
    const s_action_metadata *metadata;

    if (payload == NULL) {
        return false;
    }
    if ((metadata = ctx_get_action_metadata()) == NULL) {
        PRINTF("Error: received an action without a prior metadata!\n");
        return false;
    }
    if (!action_tlv_parser(payload, &out, &out.received_tags)) {
        return false;
    }
    if (!verify_action(&out)) {
        return false;
    }
    if (!is_action_compatible(out.action.type, metadata->op_type)) {
        PRINTF("Error: action type %u is incompatible with op_type %u!\n",
               out.action.type,
               metadata->op_type);
        return false;
    }
    dump_action(&out.action);
    if (!ctx_push_action(&out.action)) {
        return false;
    }
    return true;
}

static bool action_serialize(const s_action *action, cmp_ctx_t *cmp_ctx) {
    bool ret;

    switch (action->type) {
        case ACTION_TYPE_BULK_ORDER:
            ret = bulk_order_serialize(&action->bulk_order, cmp_ctx);
            break;
        case ACTION_TYPE_BULK_MODIFY:
            ret = bulk_modify_serialize(&action->bulk_modify, cmp_ctx);
            break;
        case ACTION_TYPE_BULK_CANCEL:
            ret = bulk_cancel_serialize(&action->bulk_cancel, cmp_ctx);
            break;
        case ACTION_TYPE_UPDATE_LEVERAGE:
            ret = update_leverage_serialize(&action->update_leverage, cmp_ctx);
            break;
        case ACTION_TYPE_UPDATE_ISOLATED_MARGIN:
            ret = update_isolated_margin_serialize(&action->update_isolated_margin, cmp_ctx);
            break;
        default:
            ret = false;
    }
    return ret;
}

static size_t ser_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    buffer_t *buf = ctx->buf;

    if (buf == NULL) {
        return 0;
    }
    if ((buf->offset + count) > buf->size) {
        PRINTF("Error: serialization buffer too small!\n");
        return 0;
    }
    memcpy(&buf->ptr[buf->offset], data, count);
    buf->offset += count;
    return count;
}

static bool compute_connection_id(const s_action *action, uint8_t end_byte, uint8_t *out) {
    cmp_ctx_t cmp_ctx;
    buffer_t buf;
    uint8_t nonce_buf[sizeof(action->nonce)];
    cx_sha3_t hash_ctx;

    buf.ptr = g_raw_buf;
    buf.size = sizeof(g_raw_buf);
    buf.offset = 0;
    cmp_init(&cmp_ctx, &buf, NULL, NULL, &ser_writer);
    if (!action_serialize(action, &cmp_ctx)) {
        return false;
    }

    if (cx_keccak_init_no_throw(&hash_ctx, 256) != CX_OK) {
        return false;
    }

    if (cx_hash_update((cx_hash_t *) &hash_ctx, buf.ptr, buf.offset) != CX_OK) {
        return false;
    }

    write_u64_be(nonce_buf, 0, action->nonce);
    if (cx_hash_update((cx_hash_t *) &hash_ctx, nonce_buf, sizeof(nonce_buf)) != CX_OK) {
        return false;
    }

    if (cx_hash_update((cx_hash_t *) &hash_ctx, &end_byte, sizeof(end_byte)) != CX_OK) {
        return false;
    }

    if (cx_hash_final((cx_hash_t *) &hash_ctx, out) != CX_OK) {
        return false;
    }
    PRINTF("connection_id = 0x%.*h\n", 32, out);
    return true;
}

bool action_hash(const s_action *action,
                 const s_action_metadata *metadata,
                 uint8_t *domain_hash,
                 uint8_t *message_hash) {
    uint8_t connection_id[32];

    if ((action == NULL) || (metadata == NULL) || (domain_hash == NULL) || (message_hash == NULL)) {
        return false;
    }
    if (action->type == ACTION_TYPE_APPROVE_BUILDER_FEE) {
        if (!eip712_builder_fee_hash(&action->approve_builder_fee.signature_chain_id,
                                     (metadata->network == NETWORK_MAINNET) ? "Mainnet" : "Testnet",
                                     action->approve_builder_fee.max_fee_rate,
                                     action->approve_builder_fee.builder,
                                     &action->nonce,
                                     domain_hash,
                                     message_hash)) {
            return false;
        }
    } else {
        if (!compute_connection_id(action, 0, connection_id)) {
            return false;
        }
        if (!eip712_cid_hash((metadata->network == NETWORK_MAINNET) ? "a" : "b",
                             connection_id,
                             domain_hash,
                             message_hash)) {
            return false;
        }
    }
    return true;
}
