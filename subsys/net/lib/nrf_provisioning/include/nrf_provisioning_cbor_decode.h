/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of CONFIG_NRF_PROVISIONING_CBOR_RECORDS
 */

#ifndef NRF_PROVISIONING_CBOR_DECODE_H__
#define NRF_PROVISIONING_CBOR_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "nrf_provisioning_cbor_decode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != CONFIG_NRF_PROVISIONING_CBOR_RECORDS
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_commands(const uint8_t *payload, size_t payload_len, struct commands *result,
			 size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CBOR_DECODE_H__ */
