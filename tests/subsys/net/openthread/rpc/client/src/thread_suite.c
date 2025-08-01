/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/thread.h>

static bool cb_called;

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();
}

/* Test serialization of otThreadDiscover() */
ZTEST(ot_rpc_thread, test_otThreadDiscover)
{
	otError error;
	uint32_t scan_channels = 0x7fff800;
	uint16_t pan_id = 0x4321;
	bool joiner = false;
	bool enable_eui64_filtering = true;
	otHandleActiveScanResult cb = (otHandleActiveScanResult)0xdeadbeef;
	void *cb_ctx = (void *)0xcafeface;

	int slot = nrf_rpc_cbkproxy_in_set(cb);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER, CBOR_UINT32(0x7fff800),
					   CBOR_UINT16(0x4321), CBOR_FALSE, CBOR_TRUE, slot,
					   CBOR_UINT32(0xcafeface)),
				   RPC_RSP(OT_ERROR_NONE));
	error = otThreadDiscover(NULL, scan_channels, pan_id, joiner, enable_eui64_filtering, cb,
				 cb_ctx);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

static void discover_cb(otActiveScanResult *result, void *ctx)
{
	uint8_t ext_addr[] = {INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)};
	uint8_t nwk_name[] = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0};
	uint8_t ext_pan_id[] = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)};
	uint8_t steer_data[] = {INT_SEQUENCE(OT_STEERING_DATA_MAX_LENGTH)};

	cb_called = true;
	zassert_not_null(result);
	zassert_not_null(ctx);
	zassert_mem_equal(result->mExtAddress.m8, ext_addr, OT_EXT_ADDRESS_SIZE);
	zassert_mem_equal(result->mNetworkName.m8, nwk_name, OT_NETWORK_NAME_MAX_SIZE + 1);
	zassert_mem_equal(result->mExtendedPanId.m8, ext_pan_id, OT_EXT_PAN_ID_SIZE);
	zassert_mem_equal(result->mSteeringData.m8, steer_data, OT_STEERING_DATA_MAX_LENGTH);
	zassert_equal(result->mPanId, 0x1234);
	zassert_equal(result->mJoinerUdpPort, 0x4321);
	zassert_equal(result->mChannel, 0x22);
	zassert_equal(result->mRssi, 0x33);
	zassert_equal(result->mLqi, 0x44);
	zassert_equal(result->mVersion, 0x0f);
	zassert_false(!!result->mIsNative);
	zassert_true(!!result->mDiscover);
	zassert_false(!!result->mIsJoinable);
	zassert_equal_ptr(ctx, (void *)0xdeadbeef);
}

/* Test reception of discover callback with active scan result. */
ZTEST(ot_rpc_thread, test_discover_cb_handler)
{

	int in_slot = nrf_rpc_cbkproxy_in_set(discover_cb);

	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, EXT_ADDR, NWK_NAME,
					EXT_PAN_ID, STEERING_DATA, CBOR_UINT16(0x1234),
					CBOR_UINT16(0x4321), CBOR_UINT8(0x22), CBOR_UINT8(0x33),
					CBOR_UINT8(0x44), 0x0f, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE,
					CBOR_UINT32(0xdeadbeef), in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

static void discover_empty_cb(otActiveScanResult *result, void *ctx)
{
	cb_called = true;
	zassert_is_null(result);
	zassert_not_null(ctx);
	zassert_equal_ptr(ctx, (void *)0xdeadbeef);
}

/* Test reception of discover callback with empty active scan result. */
ZTEST(ot_rpc_thread, test_discover_cb_handler_empty)
{
	int in_slot = nrf_rpc_cbkproxy_in_set(discover_empty_cb);

	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, CBOR_NULL,
					CBOR_UINT32(0xdeadbeef), in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

/* Test serialization of otThreadSetEnabled(false) */
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_false)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_FALSE),
				   RPC_RSP(0x00));
	error = otThreadSetEnabled(NULL, false);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otThreadSetEnabled(true) */
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_true)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE),
				   RPC_RSP(0x00));
	error = otThreadSetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otThreadSetEnabled(true) */
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_true_failed)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE),
				   RPC_RSP(0x01));
	error = otThreadSetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otThreadGetDeviceRole() returning 'disabled' */
ZTEST(ot_rpc_thread, test_otThreadGetDeviceRole_disabled)
{
	otDeviceRole role;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE), RPC_RSP(0x00));
	role = otThreadGetDeviceRole(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(role, OT_DEVICE_ROLE_DISABLED);
}

/* Test serialization of otThreadGetDeviceRole() returning 'leader' */
ZTEST(ot_rpc_thread, test_otThreadGetDeviceRole_leader)
{
	otDeviceRole role;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE), RPC_RSP(0x04));
	role = otThreadGetDeviceRole(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(role, OT_DEVICE_ROLE_LEADER);
}

/* Test serialization of otThreadSetLinkMode(-) */
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_none)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x00), RPC_RSP(0x00));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otThreadSetLinkMode(rdn) */
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_rdn)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));
	mode.mRxOnWhenIdle = true;
	mode.mDeviceType = true;
	mode.mNetworkData = true;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x07), RPC_RSP(0x00));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otThreadSetLinkMode(n) */
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_n)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));
	mode.mNetworkData = true;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x04), RPC_RSP(0x07));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otThreadGetLinkMode() returning '-' */
ZTEST(ot_rpc_thread, test_otThreadGetLinkMode_none)
{
	otLinkModeConfig mode;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE), RPC_RSP(0x00));
	mode = otThreadGetLinkMode(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_false(mode.mRxOnWhenIdle);
	zassert_false(mode.mDeviceType);
	zassert_false(mode.mNetworkData);
}

/* Test serialization of otThreadGetLinkMode() returning 'rdn' */
ZTEST(ot_rpc_thread, test_otThreadGetLinkMode_rdn)
{
	otLinkModeConfig mode;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE), RPC_RSP(0x07));
	mode = otThreadGetLinkMode(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_true(mode.mRxOnWhenIdle);
	zassert_true(mode.mDeviceType);
	zassert_true(mode.mNetworkData);
}

/* Test serialization of otThreadGetVersion() returning 1.4 */
ZTEST(ot_rpc_thread, test_otThreadGetVersion)
{
	uint16_t version;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_VERSION),
				   RPC_RSP(OT_THREAD_VERSION_1_4));
	version = otThreadGetVersion();
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(version, OT_THREAD_VERSION_1_4);
}

/* Test serialization of otThreadGetExtendedPanId() */
ZTEST(ot_rpc_thread, test_otThreadGetExtendedPanId)
{
	const otExtendedPanId exp_version = {{INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)}};
	const otExtendedPanId *version;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_EXTENDED_PANID),
				   RPC_RSP(EXT_PAN_ID));
	version = otThreadGetExtendedPanId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_mem_equal(version, &exp_version, OT_EXT_PAN_ID_SIZE);
}

/* Test serialization of otThreadGetLeaderRouterId() */
ZTEST(ot_rpc_thread, test_otThreadGetLeaderRouterId)
{
	uint8_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LEADER_ROUTER_ID),
				   RPC_RSP(CBOR_UINT8(UINT8_MAX)));
	id = otThreadGetLeaderRouterId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, UINT8_MAX);
}

/* Test serialization of otThreadGetLeaderWeight() */
ZTEST(ot_rpc_thread, test_otThreadGetLeaderWeight)
{
	uint8_t weight;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LEADER_WEIGHT),
				   RPC_RSP(CBOR_UINT8(UINT8_MAX)));
	weight = otThreadGetLeaderWeight(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(weight, UINT8_MAX);
}

/* Test serialization of otThreadGetMeshLocalPrefix() */
ZTEST(ot_rpc_thread, test_otThreadGetMeshLocalPrefix)
{
	const otMeshLocalPrefix exp_prefix = {{MESH_LOCAL_PREFIX}};
	const otMeshLocalPrefix *prefix;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_MESH_LOCAL_PREFIX),
				   RPC_RSP(CBOR_MESH_LOCAL_PREFIX));
	prefix = otThreadGetMeshLocalPrefix(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_mem_equal(prefix, &exp_prefix, OT_MESH_LOCAL_PREFIX_SIZE);
}

/* Test serialization of otThreadGetNetworkName() */
ZTEST(ot_rpc_thread, test_otThreadGetNetworkName)
{
	const char exp_name[] = {STR_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), '\0'};
	const char *name;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_NETWORK_NAME),
				   RPC_RSP(0x50, STR_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE)));
	name = otThreadGetNetworkName(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_str_equal(name, exp_name);
}

/* Test serialization of otThreadGetPartitionId() */
ZTEST(ot_rpc_thread, test_otThreadGetPartitionId)
{
	uint32_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_PARTITION_ID),
				   RPC_RSP(CBOR_UINT32(UINT32_MAX)));
	id = otThreadGetPartitionId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, UINT32_MAX);
}

/* Test serialization of otThreadGetRloc16() */
ZTEST(ot_rpc_thread, test_otThreadGetRloc16)
{
	uint16_t rloc16;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_RLOC16),
				   RPC_RSP(CBOR_UINT16(UINT16_MAX)));
	rloc16 = otThreadGetRloc16(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(rloc16, UINT16_MAX);
}

/* Test serialization of otThreadGetMleCounters() */
ZTEST(ot_rpc_thread, test_otThreadGetMleCounters)
{
	const otMleCounters *counters;

#define COUNTER_16(i)	   (UINT16_MAX - i)
#define COUNTER_64(i)	   (UINT64_MAX - i)
#define CBOR_COUNTER_16(i) CBOR_UINT16(COUNTER_16(i))
#define CBOR_COUNTER_64(i) CBOR_UINT64(COUNTER_64(i))

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_MLE_COUNTERS),
		RPC_RSP(CBOR_COUNTER_16(0), CBOR_COUNTER_16(1), CBOR_COUNTER_16(2),
			CBOR_COUNTER_16(3), CBOR_COUNTER_16(4), CBOR_COUNTER_16(5),
			CBOR_COUNTER_16(6), CBOR_COUNTER_16(7), CBOR_COUNTER_16(8),
			CBOR_COUNTER_64(0), CBOR_COUNTER_64(1), CBOR_COUNTER_64(2),
			CBOR_COUNTER_64(3), CBOR_COUNTER_64(4), CBOR_COUNTER_64(5)));
	counters = otThreadGetMleCounters(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(counters->mDisabledRole, COUNTER_16(0));
	zassert_equal(counters->mDetachedRole, COUNTER_16(1));
	zassert_equal(counters->mChildRole, COUNTER_16(2));
	zassert_equal(counters->mRouterRole, COUNTER_16(3));
	zassert_equal(counters->mLeaderRole, COUNTER_16(4));
	zassert_equal(counters->mAttachAttempts, COUNTER_16(5));
	zassert_equal(counters->mPartitionIdChanges, COUNTER_16(6));
	zassert_equal(counters->mBetterPartitionAttachAttempts, COUNTER_16(7));
	zassert_equal(counters->mParentChanges, COUNTER_16(8));
	zassert_equal(counters->mDisabledTime, COUNTER_64(0));
	zassert_equal(counters->mDetachedTime, COUNTER_64(1));
	zassert_equal(counters->mChildTime, COUNTER_64(2));
	zassert_equal(counters->mRouterTime, COUNTER_64(3));
	zassert_equal(counters->mLeaderTime, COUNTER_64(4));
	zassert_equal(counters->mTrackedTime, COUNTER_64(5));

#undef COUNTER_16
#undef COUNTER_64
#undef CBOR_COUNTER_16
#undef CBOR_COUNTER_64
}

ZTEST_SUITE(ot_rpc_thread, NULL, NULL, tc_setup, NULL, NULL);
