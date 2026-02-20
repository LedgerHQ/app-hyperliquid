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

## TLV structs
