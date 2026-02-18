#include "os_print.h"
#include "tlv_library.h"
#include "format.h"  // format_u64
#include "write.h"
#include "action.h"

#define STRUCT_TYPE 0x2c

typedef struct {
    TLV_reception_t received_tags;
    s_action action;
    union {
    };
} s_action_ctx;

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
        default:
            ret = false;
    }
    return ret;
}

static bool verify_action(s_action_ctx *out) {
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
        default:
            PRINTF("Error: cannot dump unknown action type\n");
    }
    PRINTF("<<< ACTION <<<\n");
}

bool parse_action(const buffer_t *payload) {
    s_action_ctx out = {0};

    if (!action_tlv_parser(payload, &out, &out.received_tags)) {
        return false;
    }
    if (!verify_action(&out)) {
        return false;
    }
    dump_action(&out.action);
    return true;
}
