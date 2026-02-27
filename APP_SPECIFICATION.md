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

### SET_ACTION

#### Description

This command gives a hyperliquid action.

#### Coding

##### `Command`

| CLA | INS   | P1                   | P2  | Lc       | Le       |
| --- | ---   | -------------------- | --- | ---      | ---      |
| E0  |  03   | 01: first chunk      | 00  | variable | variable |
|     |       | 00: following chunk  |     |          |          |

##### `Input data`

###### first chunk

| Description                      | Length   |
| ---                              | ---      |
| Struct length (big endian)       | 2        |
| [SET_ACTION](#set_action) struct | variable |

###### following chunk

:warning: Multi-chunk payloads are currently not supported by the app, but for future-proofing it was specified this way.

| Description                      | Length   |
| ---                              | ---      |
| [SET_ACTION](#set_action) struct | variable |

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

### SET_ACTION

| Name           | Tag  | Type                           | Optional |
| ----           | ---  | ----                           | -------- |
| STRUCT_TYPE    | 0x01 | uint8                          |          |
| STRUCT_VERSION | 0x02 | uint8                          |          |
| ACTION_TYPE    | 0xd0 | [ActionType](#actiontype-enum) |          |
| NONCE          | 0xda | uint64                         |          |
| ACTION         | 0xdb | [BULK_ORDER](#bulk_order) \|<br>[BULK_MODIFY](#bulk_modify) |          |

### BULK_ORDER

| Name            | Tag  | Type                       | Optional |
| ----            | ---  | ----                       | -------- |
| ORDER           | 0xdd | [ORDER](#order)            |          |
| GROUPING        | 0xea | [Grouping](#grouping-enum) |          |
| BUILDER_ADDRESS | 0xeb | uint8[20]                  | x        |
| BUILDER_FEE     | 0xec | uint64                     | x        |

#### Grouping enum

| Name          | Value |
| ----          | ----- |
| NA            | 0x00  |
| NORMAL_TPSL   | 0x01  |
| POSITION_TPSL | 0x02  |

### BULK_MODIFY

| Name            | Tag  | Type            | Optional |
| ----            | ---  | ----            | -------- |
| ORDER           | 0xdd | [ORDER](#order) |          |
| OID             | 0xdc | uint64          |          |

### ORDER

| Name            | Tag  | Type                             | Optional |
| ----            | ---  | ----                             | -------- |
| ORDER_TYPE      | 0xe0 | [OrderType](#ordertype-enum)     |          |
| ASSET           | 0xe1 | uint32                           |          |
| IS_BUY          | 0xe2 | bool                             |          |
| LIMIT_PX        | 0xe3 | char[]                           |          |
| SZ              | 0xe4 | char[]                           |          |
| REDUCE_ONLY     | 0xe5 | bool                             |          |
| TIF             | 0xe6 | [Tif](#tif-enum)                 | x        |
| IS_MARKET       | 0xe7 | bool                             | x        |
| TRIGGER_PX      | 0xe8 | char[]                           | x        |
| TRIGGER_TYPE    | 0xe9 | [TriggerType](#triggertype-enum) | x        |

#### OrderType enum

| Name    | Value |
| ----    | ----- |
| LIMIT   | 0x00  |
| TRIGGER | 0x01  |

#### Tif enum

| Name | Value |
| ---- | ----- |
| ALO  | 0x00  |
| IOC  | 0x01  |
| GTC  | 0x02  |

#### TriggerType enum

| Name | Value |
| ---- | ----- |
| TP   | 0x00  |
| SL   | 0x01  |
