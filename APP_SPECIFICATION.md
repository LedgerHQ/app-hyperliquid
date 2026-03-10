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
| [ACTION](#action) struct         | variable |

###### following chunk

:warning: Multi-chunk payloads are currently not supported by the app, but for future-proofing it was specified this way.

| Description                      | Length   |
| ---                              | ---      |
| [ACTION](#action) struct         | variable |

##### `Output data`

None

### SIGN_ACTION

#### Description

This command signs a previously given hyperliquid action (in the same order).

#### Coding

##### `Command`

| CLA | INS   | P1  | P2  | Lc       | Le       |
| --- | ---   | --- | --- | ---      | ---      |
| E0  |  04   | 00  | 00  | variable | variable |

##### `Input data`

| Description                                             | Length |
| ---                                                     | ---    |
| Number of BIP 32 derivations to perform (max 10 levels) | 1      |
| First derivation index (big endian)                     | 4      |
| ...                                                     | 4      |
| Last derivation index (big endian)                      | 4      |

##### `Output data`

| Description               | Length |
| ---                       | ---    |
| Remaining actions to sign | 1      |
| signature v               | 1      |
| signature r               | 32     |
| signature s               | 32     |

## TLV structs

### ACTION_METADATA

| Name           | Tag  | Type                                 | Optional |
| ----           | ---  | ----                                 | -------- |
| STRUCT_TYPE    | 0x01 | uint8                                |          |
| STRUCT_VERSION | 0x02 | uint8                                |          |
| OPERATION_TYPE | 0xd0 | [OperationType](#operationtype-enum) |          |
| ASSET_ID       | 0xd1 | uint32                               |          |
| ASSET_TICKER   | 0x24 | char[]                               |          |
| NETWORK        | 0xd2 | [Network](#network-enum)             |          |
| BUILDER_ADDR   | 0xd3 | uint8[20]                            | x        |
| MARGIN         | 0xd4 | uint64                               | x        |
| SIGNATURE      | 0x15 | uint8[]                              |          |

#### OperationType enum

| Name                 | Value |
| ----                 | ----- |
| ORDER                | 0x00  |
| MODIFY               | 0x01  |
| CANCEL               | 0x02  |
| UPDATE_LEVERAGE      | 0x03  |
| CLOSE                | 0x04  |
| UPDATE_MARGIN        | 0x05  |

#### Network enum

| Name    | Value |
| ----    | ----- |
| MAINNET | 0x00  |
| TESTNET | 0x01  |

### ACTION

| Name           | Tag  | Type                           | Optional |
| ----           | ---  | ----                           | -------- |
| STRUCT_TYPE    | 0x01 | uint8                          |          |
| STRUCT_VERSION | 0x02 | uint8                          |          |
| ACTION_TYPE    | 0xd0 | [ActionType](#actiontype-enum) |          |
| NONCE          | 0xda | uint64                         |          |
| ACTION         | 0xdb | [BULK_ORDER](#bulk_order) \|<br>[BULK_MODIFY](#bulk_modify) \|<br>[BULK_CANCEL](#bulk_cancel) \|<br>[UPDATE_LEVERAGE](#update_leverage) \|<br>[APPROVE_BUILDER_FEE](#approve_builder_fee) \|<br>[UPDATE_ISOLATED_MARGIN](#update_isolated_margin) |          |

#### ActionType enum

| Name                   | Value |
| ----                   | ----- |
| BULK_ORDER             | 0x00  |
| BULK_MODIFY            | 0x01  |
| BULK_CANCEL            | 0x02  |
| UPDATE_LEVERAGE        | 0x03  |
| APPROVE_BUILDER_FEE    | 0x04  |
| UPDATE_ISOLATED_MARGIN | 0x05  |

### BULK_ORDER

| Name            | Tag  | Type                            | Optional |
| ----            | ---  | ----                            | -------- |
| ORDER           | 0xdd | [ORDER_REQUEST](#order_request) |          |
| GROUPING        | 0xea | [Grouping](#grouping-enum)      |          |
| BUILDER         | 0xeb | [BUILDER_INFO](#builder_info)   | x        |

:information_source: Multiple ORDER tags may be present; up to 5 ORDER tags are supported per BULK_ORDER.

### BUILDER_INFO

| Name            | Tag  | Type                            | Optional |
| ----            | ---  | ----                            | -------- |
| ADDRESS         | 0xd3 | uint8[20]                       |          |
| FEE             | 0xec | uint64                          |          |

#### Grouping enum

| Name          | Value |
| ----          | ----- |
| NA            | 0x00  |
| NORMAL_TPSL   | 0x01  |
| POSITION_TPSL | 0x02  |

### BULK_MODIFY

| Name            | Tag  | Type                              | Optional |
| ----            | ---  | ----                              | -------- |
| MODIFY          | 0xd8 | [MODIFY_REQUEST](#modify_request) |          |

:information_source: Multiple MODIFY tags may be present; up to 5 MODIFY tags are supported per BULK_MODIFY.

### MODIFY_REQUEST

| Name            | Tag  | Type                            | Optional |
| ----            | ---  | ----                            | -------- |
| ORDER           | 0xdd | [ORDER_REQUEST](#order_request) |          |
| OID             | 0xdc | uint64                          |          |

### BULK_CANCEL

| Name            | Tag  | Type                              | Optional |
| ----            | ---  | ----                              | -------- |
| CANCEL          | 0xd9 | [CANCEL_REQUEST](#cancel_request) |          |

:information_source: Multiple CANCEL tags may be present; up to 5 CANCEL tags are supported per BULK_CANCEL.

### CANCEL_REQUEST

| Name            | Tag  | Type      | Optional |
| ----            | ---  | ----      | -------- |
| ASSET           | 0xd1 | uint32    |          |
| OID             | 0xdc | uint64    |          |

### UPDATE_LEVERAGE

| Name            | Tag  | Type      | Optional |
| ----            | ---  | ----      | -------- |
| ASSET           | 0xd1 | uint32    |          |
| IS_CROSS        | 0xde | bool      |          |
| LEVERAGE        | 0xed | uint32    |          |

### ORDER_REQUEST

| Name            | Tag  | Type                             | Optional |
| ----            | ---  | ----                             | -------- |
| ORDER_TYPE      | 0xe0 | [OrderType](#ordertype-enum)     |          |
| ASSET           | 0xd1 | uint32                           |          |
| IS_BUY          | 0xe2 | bool                             |          |
| LIMIT_PX        | 0xe3 | char[]                           |          |
| SZ              | 0xe4 | char[]                           |          |
| REDUCE_ONLY     | 0xe5 | bool                             |          |
| ORDER           | 0xd7 | [LIMIT_ORDER](#limit_order) \|<br>[TRIGGER_ORDER](#trigger_order) |          |

#### OrderType enum

| Name    | Value |
| ----    | ----- |
| LIMIT   | 0x00  |
| TRIGGER | 0x01  |

### LIMIT_ORDER

| Name            | Tag  | Type                             | Optional |
| ----            | ---  | ----                             | -------- |
| TIF             | 0xe6 | [Tif](#tif-enum)                 |          |

#### Tif enum

| Name | Value |
| ---- | ----- |
| ALO  | 0x00  |
| IOC  | 0x01  |
| GTC  | 0x02  |

### TRIGGER_ORDER

| Name            | Tag  | Type                             | Optional |
| ----            | ---  | ----                             | -------- |
| IS_MARKET       | 0xe7 | bool                             |          |
| TRIGGER_PX      | 0xe8 | char[]                           |          |
| TPSL            | 0xe9 | [TriggerType](#triggertype-enum) |          |

#### TriggerType enum

| Name | Value |
| ---- | ----- |
| TP   | 0x00  |
| SL   | 0x01  |

### APPROVE_BUILDER_FEE

| Name               | Tag  | Type      | Optional |
| ----               | ---  | ----      | -------- |
| SIGNATURE_CHAIN_ID | 0x23 | uint64    |          |
| MAX_FEE_RATE       | 0xb0 | char[]    |          |
| BUILDER            | 0xd3 | uint8[20] |          |

### UPDATE_ISOLATED_MARGIN

| Name               | Tag  | Type      | Optional |
| ----               | ---  | ----      | -------- |
| ASSET              | 0xd1 | uint32    |          |
| IS_BUY             | 0xe2 | bool      |          |
| NTLI               | 0xd6 | int64     |          |
