#pragma once

/**
 * Instruction class of the Boilerplate application.
 */
#define CLA 0xE0

/**
 * Size of an address in bytes
 */
#define ADDRESS_LENGTH 20

/**
 * Max size of an asset ticker
 */
#define ASSET_TICKER_LENGTH 48

/**
 * Max size of a numeric value represented as a string (64-bit floating number)
 */
#define NUMERIC_STRING_LENGTH 21

/**
 * Max size of a bulk operation
 */
#define BULK_MAX_SIZE 5

/**
 * Decimals used for margins when expressed as 64-bit integers
 */
#define MARGIN_DECIMALS 6

#define COUNTERVALUE_TICKER "USDC"

#define SHORT_LONG_STRING_LENGTH 5

#define LIMIT_MARKET_STRING_LENGTH 6

#define UINT32_STRING_LENGTH 10

#define PRICE_STRING_LENGTH    (NUMERIC_STRING_LENGTH + 1 + ASSET_TICKER_LENGTH)
#define SIZE_STRING_LENGTH     PRICE_STRING_LENGTH
#define MARGIN_STRING_LENGTH   PRICE_STRING_LENGTH
#define LEVERAGE_STRING_LENGTH (UINT32_STRING_LENGTH + 1)
