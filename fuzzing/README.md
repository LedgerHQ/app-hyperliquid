# Hyperliquid Fuzz Integration

One APDU per iteration through `apdu_dispatcher()`; app state is restored from
the input prefix by Absolution, so the harness has no per-iteration setup.

Reference: `$BOLOS_SDK/fuzzing/doc/` (`manifest.dox`, `invariants.dox`,
`running.dox`).

## Quickstart

```bash
export BOLOS_SDK=/absolute/path/to/ledger-secure-sdk
export APP_DIR=/absolute/path/to/app-hyperliquid

BOLOS_SDK="$BOLOS_SDK" FUZZ_TIME=0 \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh --clean --app-dir "$APP_DIR" probe-build

BOLOS_SDK="$BOLOS_SDK" OVERWRITE=1 FUZZ_TIME=300 APP_SANITIZER=address \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh --app-dir "$APP_DIR" baseline
```

`FUZZ_TIME` is per worker. `APP_SANITIZER` takes `address`, `undefined` or
`memory`; the unprefixed `SANITIZER` is ignored. Use `--clean` when switching
sanitizer, or the previous `-fsanitize=` flag persists in the CMake cache.
Artifacts land in `.fuzz-artifacts/<run-name>/`.

## Layout

```text
fuzzing/
  fuzz-manifest.toml         target, coverage key files, dictionary, seeds
  CMakeLists.txt             app sources and include dirs
  harness/fuzz_dispatcher.c  command table and dispatcher adapter
  harness/fuzz_tlv_config.c  per-command TLV grammars
  invariants/                zero-symbols.txt, domain-overrides.txt
  seeds/custom_seeds.py      one seed per INS, payload only
```

`invariants/fuzz_globals.zon` is generated per build and gitignored: it records
absolute source paths, which would tie the corpus compat key to one checkout.

## App notes

- CLA `0xE0`. INS `0x01` GET_ADDRESS, `0x02` PROVIDE_ACTION_METADATA,
  `0x03` SET_ACTION, `0x04` SIGN_ACTION.
- `check_signature_with_pki` is replaced by the SDK's own PKI mock, so
  PROVIDE_ACTION_METADATA reaches the TLV parsing behind it.
- NBGL flows auto-approve through the SDK mock, so `handle_ui()` invokes
  `sign_action()` synchronously. `fuzz_mock_nbgl_reject` selects rejection.
- A prefix byte selects by index into a domain's value list, so the first value
  listed is what a zero prefix restores. `g_ctx.has_metadata` and
  `g_ctx.action_count` list their productive value first; otherwise every seed
  starts with no metadata and no actions and returns before `handle_ui()`.
- `src/eip712_*.c` and `src_msgpack/cmp.c` sit behind `ui_order()`'s structural
  check on the restored action (exactly one limit order, at most one TP and one
  SL, matching `order_count`). Reaching them needs the fuzzer to build that
  structure; constraining it in the invariant would author content rather than
  restore state.
