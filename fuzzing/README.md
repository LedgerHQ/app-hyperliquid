# Hyperliquid Fuzz Integration

Hyperliquid plugs into the standard `ledger-secure-sdk` fuzz framework. One
APDU per iteration is dispatched through `apdu_dispatcher()` from
`src/apdu/dispatcher.c`; Absolution drives global state via the
invariant.

See:

- App contract: `$BOLOS_SDK/fuzzing/docs/APP_CONTRACT.md`
- Campaign workflow: `$BOLOS_SDK/fuzzing/docs/CAMPAIGN_WORKFLOW.md`

## Quickstart

```bash
export BOLOS_SDK=/absolute/path/to/ledger-secure-sdk
export APP_DIR=/absolute/path/to/app-hyperliquid

# Build-only probe.
BOLOS_SDK="$BOLOS_SDK" WARMUP_SEC=0 MAIN_SEC=0 \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --clean --app-dir "$APP_DIR" probe-build

# Smoke campaign.
BOLOS_SDK="$BOLOS_SDK" OVERWRITE=1 WARMUP_SEC=10 MAIN_SEC=5 \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --clean --app-dir "$APP_DIR" smoke

# Baseline campaign.
BOLOS_SDK="$BOLOS_SDK" OVERWRITE=1 WARMUP_SEC=30 MAIN_SEC=120 \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir "$APP_DIR" baseline
```

Artifacts land under `app-hyperliquid/.fuzz-artifacts/<run-name>/`.

## App-specific notes

- CLA = `0xE0`. INS handled: `0x01` GET_ADDRESS, `0x02` PROVIDE_ACTION_METADATA,
  `0x03` SET_ACTION, `0x04` SIGN_ACTION.
- `PROVIDE_ACTION_METADATA` is gated by a PKI-signed metadata blob. The
  signature verification (`check_signature_with_pki`) is replaced with a
  pass-through stub in `mock/mock_check_signature_with_pki.c` so the
  fuzzer can reach the downstream TLV parsing and action machinery.
- The session state machine lives in `g_ctx` (static in `src/hl_context.c`).
  Its `action_count` / `action_index` fields are constrained in
  `invariants/domain-overrides.txt` so Absolution explores valid bounded
  values instead of random integers.
- NBGL flows are auto-approved by the SDK NBGL mock; `handle_ui()` therefore
  triggers `sign_action()` synchronously.
