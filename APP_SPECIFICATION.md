# Technical Specification

> **Warning**
This documentation is a template and shall be updated with your own APDUs.

## About

This documentation describes the APDU messages interface to communicate with the Hyperliquid application.

The application interface can be accessed over HID or BLE

## APDUs

### GET_ADDRESS

#### Description

This command returns the derived address for the given BIP-32 path.

#### Coding

##### `Command`

| CLA | INS   | P1  | P2  | Lc       | Le       |
| --- | ---   | --- | --- | ---      | ---      |
| E0  |  01   | 00  | 00  | variable | variable |
|     |       |     |     |          |          |

##### `Input data`

| Description                                             | Length |
| ---                                                     | ---    |
| Number of BIP 32 derivations to perform (max 10 levels) | 1      |
| First derivation index (big endian)                     | 4      |
| ...                                                     | 4      |
| Last derivation index (big endian)                      | 4      |

##### `Output data`

| Description     | Length |
| ---             | ---    |
| Derived address | 20     |

### PROVIDE_ACTION_METADATA

#### Description

This command provides metadata about a hyperliquid action (which by itself does not contain enough information
for proper clear-signing). The metadata includes a signature, which is cryptographically verified by the app.

#### Coding

##### `Command`

| CLA | INS   | P1                   | P2  | Lc       | Le       |
| --- | ---   | -------------------- | --- | ---      | ---      |
| E0  |  02   | 01: first chunk      | 00  | variable | variable |
|     |       | 00: following chunk  |     |          |          |

##### `Input data`

###### first chunk

| Description                                | Length   |
| ---                                        | ---      |
| Struct length (big endian)                 | 2        |
| [ACTION_METADATA](#action_metadata) struct | variable |

###### following chunk

:warning: Multi-chunk payloads are currently not supported by the app, but for future-proofing it was specified this way.

| Description                                | Length   |
| ---                                        | ---      |
| [ACTION_METADATA](#action_metadata) struct | variable |

##### `Output data`

None

## TLV structs

### ACTION_METADATA

| Name           | Tag  | Type                           | Optional |
| ----           | ---  | ----                           | -------- |
| STRUCT_TYPE    | 0x01 | uint8                          |          |
| STRUCT_VERSION | 0x02 | uint8                          |          |
| ACTION_TYPE    | 0xd0 | [ActionType](#actiontype-enum) |          |
| ASSET_ID       | 0xd1 | uint32                         |          |
| ASSET_TICKER   | 0x24 | char[]                         |          |
| NETWORK        | 0xd2 | [Network](#network-enum)       |          |
| BUILDER_ADDR   | 0xd3 | uint8[20]                      | x        |
| SIGNATURE      | 0x15 | uint8[]                        |          |

#### ActionType enum

| Name                 | Value |
| ----                 | ----- |
| ORDER                | 0x00  |
| MODIFY               | 0x01  |
| CANCEL               | 0x02  |
| LEVERAGE             | 0x03  |

#### Network enum

| Name    | Value |
| ----    | ----- |
| MAINNET | 0x00  |
| TESTNET | 0x01  |
