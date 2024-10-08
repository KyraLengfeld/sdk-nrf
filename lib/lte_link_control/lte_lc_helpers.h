/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>

#define AT_CFUN_READ				"AT+CFUN?"
#define AT_CEREG_5				"AT+CEREG=5"
#define AT_CEREG_READ				"AT+CEREG?"
#define AT_CEREG_REG_STATUS_INDEX		1
#define AT_CEREG_TAC_INDEX			2
#define AT_CEREG_CELL_ID_INDEX			3
#define AT_CEREG_ACT_INDEX			4
#define AT_CEREG_CAUSE_TYPE_INDEX		5
#define AT_CEREG_REJECT_CAUSE_INDEX		6
#define AT_CEREG_ACTIVE_TIME_INDEX		7
#define AT_CEREG_TAU_INDEX			8
#define AT_XSYSTEMMODE_READ			"AT%XSYSTEMMODE?"

/* The indices are for the set command. Add 1 for the read command indices. */
#define AT_XSYSTEMMODE_READ_LTEM_INDEX		1
#define AT_XSYSTEMMODE_READ_NBIOT_INDEX		2
#define AT_XSYSTEMMODE_READ_GPS_INDEX		3
#define AT_XSYSTEMMODE_READ_PREFERENCE_INDEX	4

/* CEDRXS command parameters */
#define AT_CEDRXS_MODE_INDEX
#define AT_CEDRXS_ACTT_WB			4
#define AT_CEDRXS_ACTT_NB			5

/* CEDRXP notification parameters */
#define AT_CEDRXP_ACTT_INDEX			1
#define AT_CEDRXP_REQ_EDRX_INDEX		2
#define AT_CEDRXP_NW_EDRX_INDEX			3
#define AT_CEDRXP_NW_PTW_INDEX			4

/* CSCON command parameters */
#define AT_CSCON_RRC_MODE_INDEX			1
#define AT_CSCON_READ_RRC_MODE_INDEX		2

/* XT3412 command parameters */
#define AT_XT3412_SUB				"AT%%XT3412=1,%d,%d"
#define AT_XT3412_TIME_INDEX			1
#define T3412_MAX				35712000000

/* NCELLMEAS notification parameters */
#define AT_NCELLMEAS_START			"AT%%NCELLMEAS"
#define AT_NCELLMEAS_STOP			"AT%%NCELLMEASSTOP"
#define AT_NCELLMEAS_STATUS_INDEX		1
#define AT_NCELLMEAS_STATUS_VALUE_SUCCESS	0
#define AT_NCELLMEAS_STATUS_VALUE_FAIL		1
#define AT_NCELLMEAS_STATUS_VALUE_INCOMPLETE	2
#define AT_NCELLMEAS_CELL_ID_INDEX		2
#define AT_NCELLMEAS_PLMN_INDEX			3
#define AT_NCELLMEAS_TAC_INDEX			4
#define AT_NCELLMEAS_TIMING_ADV_INDEX		5
#define AT_NCELLMEAS_EARFCN_INDEX		6
#define AT_NCELLMEAS_PHYS_CELL_ID_INDEX		7
#define AT_NCELLMEAS_RSRP_INDEX			8
#define AT_NCELLMEAS_RSRQ_INDEX			9
#define AT_NCELLMEAS_MEASUREMENT_TIME_INDEX	10
#define AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT	11
/* The rest of the parameters are in repeating arrays per neighboring cell.
 * The indices below refer to their index within such a repeating array.
 */
#define AT_NCELLMEAS_N_EARFCN_INDEX		0
#define AT_NCELLMEAS_N_PHYS_CELL_ID_INDEX	1
#define AT_NCELLMEAS_N_RSRP_INDEX		2
#define AT_NCELLMEAS_N_RSRQ_INDEX		3
#define AT_NCELLMEAS_N_TIME_DIFF_INDEX		4
#define AT_NCELLMEAS_N_PARAMS_COUNT		5
#define AT_NCELLMEAS_N_MAX_ARRAY_SIZE		CONFIG_LTE_NEIGHBOR_CELLS_MAX

#define AT_NCELLMEAS_PARAMS_COUNT_MAX					\
	(AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +				\
	 AT_NCELLMEAS_N_PARAMS_COUNT * CONFIG_LTE_NEIGHBOR_CELLS_MAX)

#define AT_NCELLMEAS_GCI_CELL_PARAMS_COUNT	12

/* XMODEMSLEEP command parameters. */
#define AT_XMODEMSLEEP_SUB			"AT%%XMODEMSLEEP=1,%d,%d"
#define AT_XMODEMSLEEP_PARAMS_COUNT_MAX		4
#define AT_XMODEMSLEEP_TYPE_INDEX		1
#define AT_XMODEMSLEEP_TIME_INDEX		2

/* CONEVAL command parameters */
#define AT_CONEVAL_READ				"AT%CONEVAL"
#define AT_CONEVAL_PARAMS_MAX			19
#define AT_CONEVAL_RESULT_INDEX			1
#define AT_CONEVAL_RRC_STATE_INDEX		2
#define AT_CONEVAL_ENERGY_ESTIMATE_INDEX	3
#define AT_CONEVAL_RSRP_INDEX			4
#define AT_CONEVAL_RSRQ_INDEX			5
#define AT_CONEVAL_SNR_INDEX			6
#define AT_CONEVAL_CELL_ID_INDEX		7
#define AT_CONEVAL_PLMN_INDEX			8
#define AT_CONEVAL_PHYSICAL_CELL_ID_INDEX	9
#define AT_CONEVAL_EARFCN_INDEX			10
#define AT_CONEVAL_BAND_INDEX			11
#define AT_CONEVAL_TAU_TRIGGERED_INDEX		12
#define AT_CONEVAL_CE_LEVEL_INDEX		13
#define AT_CONEVAL_TX_POWER_INDEX		14
#define AT_CONEVAL_TX_REPETITIONS_INDEX		15
#define AT_CONEVAL_RX_REPETITIONS_INDEX		16
#define AT_CONEVAL_DL_PATHLOSS_INDEX		17

/* MDMEV command parameters */
#define AT_MDMEV_ENABLE_1			"AT%%MDMEV=1"
#define AT_MDMEV_ENABLE_2			"AT%%MDMEV=2"
#define AT_MDMEV_DISABLE			"AT%%MDMEV=0"
#define AT_MDMEV_RESPONSE_PREFIX		"%MDMEV: "
#define AT_MDMEV_OVERHEATED			"ME OVERHEATED\r\n"
#define AT_MDMEV_BATTERY_LOW			"ME BATTERY LOW\r\n"
#define AT_MDMEV_SEARCH_STATUS_1		"SEARCH STATUS 1\r\n"
#define AT_MDMEV_SEARCH_STATUS_2		"SEARCH STATUS 2\r\n"
#define AT_MDMEV_RESET_LOOP			"RESET LOOP\r\n"
#define AT_MDMEV_NO_IMEI			"NO IMEI\r\n"
#define AT_MDMEV_CE_LEVEL_0			"PRACH CE-LEVEL 0\r\n"
#define AT_MDMEV_CE_LEVEL_1			"PRACH CE-LEVEL 1\r\n"
#define AT_MDMEV_CE_LEVEL_2			"PRACH CE-LEVEL 2\r\n"
#define AT_MDMEV_CE_LEVEL_3			"PRACH CE-LEVEL 3\r\n"

/* RAI notification parameters */
#define AT_RAI_RESPONSE_PREFIX			"%RAI"
#define AT_RAI_PARAMS_COUNT_MAX			5
#define AT_RAI_CELL_ID_INDEX			1
#define AT_RAI_PLMN_INDEX			2
#define AT_RAI_AS_INDEX				3
#define AT_RAI_CP_INDEX				4

/* @brief Parses an AT command response, and returns the current RRC mode.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param mode Pointer to where the RRC mode is stored.
 * @param mode_index Parameter index for mode.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_rrc_mode(const char *at_response,
		   enum lte_lc_rrc_mode *mode,
		   size_t mode_index);

/* @brief Parses an AT command response and returns the current eDRX configuration.
 *
 * @note It is assumed that the network only reports valid eDRX values when
 *	 in each mode (LTE-M and NB1). There is no sanity check of these values.
 *
 * @param[in] at_response Pointer to buffer with AT response.
 * @param[in] cfg Pointer to where the eDRX configuration is stored.
 * @param[out] edrx_str eDRX value as a string. Must be 5 characters long buffer.
 * @param[out] ptw_str PTW as a string. Must be 5 characters long buffer.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg, char *edrx_str, char *ptw_str);

/* @brief Parses PSM configuration from periodic TAU timer and active time strings.
 *
 * @param active_time_str Pointer to active time string.
 * @param tau_ext_str Pointer to TAU (T3412 extended) string.
 * @param tau_legacy_str Pointer to TAU (T3412) string.
 * @param psm_cfg Pointer to PSM configuraion struct where the parsed values
 *		  are stored.
 *
 * @retval 0 if PSM configuration was successfully parsed.
 * @retval -EINVAL if parsing failed.
 */
int parse_psm(const char *active_time_str, const char *tau_ext_str,
	      const char *tau_legacy_str, struct lte_lc_psm_cfg *psm_cfg);

/* @brief Encode Periodic TAU timer and active time strings.
 *
 * @param[out] tau_ext_str TAU (T3412 extended) string. Must be at least 9 bytes.
 * @param[out] active_time_str Active time string buffer. Must be at least 9 bytes.
 * @param rptau[in] Requested Periodic TAU value to be encoded.
 * @param rat[in] Requested active time value to be encoded.
 *
 * @retval 0 if PSM configuration was successfully parsed.
 * @retval -EINVAL if parsing failed.
 */
int encode_psm(char *tau_ext_str, char *active_time_str, int rptau, int rat);

/* @brief Parses a +CEREG notification and returns network registration status,
 *	  cell information, LTE mode and PSM configuration. The function always
 *	  initializes the return values. The destination pointers must be non-NULL.
 *
 * @param[in] at_response AT notification.
 * @param[out] reg_status Registration status.
 * @param[out] cell Cell information.
 * @param[out] lte_mode LTE mode.
 * @param[out] psm_cfg PSM configuration.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_cereg(const char *at_response,
		enum lte_lc_nw_reg_status *reg_status,
		struct lte_lc_cell *cell,
		enum lte_lc_lte_mode *lte_mode,
		struct lte_lc_psm_cfg *psm_cfg);

/* @brief Parses an XT3412 response and extracts the time until next TAU.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param time Pointer to integer that the time until next TAU will be written to.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_xt3412(const char *at_response, uint64_t *time);

/* @brief Get the number of neighboring cells reported in an NCELLMEAS response.
 *
 * @param at_response Pointer to buffer with AT response to parse.
 *
 * @return The number of neighbor cells found in the response.
 */
uint32_t neighborcell_count_get(const char *at_response);

/* @brief Parses an NCELLMEAS notification and stores neighboring cell
 *	  information in a struct.
 *
 * 18446744073709551614 is the maximum value for timing_advance_meas_time and
 * measurement_time in @ref lte_lc_cells_info.
 * This value could be represented with uint64_t but cannot be stored by at_parser,
 * which internally uses int64_t value for all integers.
 * Hence, the maximum value for these fields is represented by 63 bits and is
 * 9223372036854775807, which still represents millions of years.
 *
 * @param at_response AT response.
 * @param cells Neighbor cell structure.
 *	        The current cell information is valid if the current cell ID is
 *	        not set to LTE_LC_CELL_EUTRAN_ID_INVALID.
 *
 * @return Zero on success or (negative) error code otherwise.
 * @retval 1 Measurement failure.
 * @retval -E2BIG The static buffers set by CONFIG_LTE_NEIGHBOR_CELLS_MAX
 *         are too small for the modem response. The associated data is still valid,
 *         but not complete.
 */
int parse_ncellmeas(const char *at_response, struct lte_lc_cells_info *cells);

/* @brief Parses a NCELLMEAS notification for GCI search types, and stores neighboring cell
 *	  and measured GCI cell information in a struct.
 *
 * 18446744073709551614 is the maximum value for timing_advance_meas_time and
 * measurement_time in @ref lte_lc_cells_info.
 * This value could be represented with uint64_t but cannot be stored by at_parser,
 * which internally uses int64_t value for all integers.
 * Hence, the maximum value for these fields is represented by 63 bits and is
 * 9223372036854775807, which still represents millions of years.
 *
 * @param params Neighbor cell measurement parameters.
 * @param at_response AT response.
 * @param cells Neighbor cell structure.
 *	        The current cell information is valid if the current cell ID is
 *	        not set to LTE_LC_CELL_EUTRAN_ID_INVALID.
 *
 * @return Zero on success or (negative) error code otherwise.
 * @retval 1 Measurement failure.
 * @retval -E2BIG The static buffers set by CONFIG_LTE_NEIGHBOR_CELLS_MAX
 *         are too small for the modem response. The associated data is still valid,
 *         but not complete.
 */
int parse_ncellmeas_gci(struct lte_lc_ncellmeas_params *params,
	const char *at_response, struct lte_lc_cells_info *cells);

/* @brief Parses an XMODEMSLEEP response and extracts the sleep type and time.
 *
 * @note If the time parameter -1 after this API call, time shall be considered infinite.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param modem_sleep Pointer to a structure holding modem sleep data.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_xmodemsleep(const char *at_response, struct lte_lc_modem_sleep *modem_sleep);

/* @brief Parses a CONEVAL response and populates a struct with parameters from the response.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param params Pointer to a structure that will be populated with CONEVAL parameters.
 *
 * @return Zero on success, negative errno code if the API call fails, and a positive error
 *         code if the API call succeeds but connection evalution fails due to modem/network related
 *         reasons.
 *
 * @retval 0 Evaluation succeeded.
 * @retval 1 Evaluation failed, no cell available.
 * @retval 2 Evaluation failed, UICC not available.
 * @retval 3 Evaluation failed, only barred cells available.
 * @retval 4 Evaluation failed, radio busy (e.g GNSS activity)
 * @retval 5 Evaluation failed, aborted due to higher priority operation.
 * @retval 6 Evaluation failed, UE not registered to network.
 * @retval 7 Evaluation failed, Unspecified.
 */
int parse_coneval(const char *at_response, struct lte_lc_conn_eval_params *params);

/* @brief Parses an MDMEV response and populates an enum with the corresponding
 *	  modem event type.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param modem_evt Pointer to enum to hold modem event.
 *
 * @return Zero on success, negative errno code on failure.
 *
 * @retval 0 Parsing succeeded.
 * @retval -EIO If the AT response is not a valid MDMEV response.
 * @retval -ENODATA If no modem event type was found in the AT response.
 */
int parse_mdmev(const char *at_response, enum lte_lc_modem_evt *modem_evt);

/* @brief Parse a RAI response and extract RAI configuration.
 *
 * @param at_response AT response.
 * @param rai_cfg RAI configuration.
 *
 * @return Zero on success, negative errno code on failure.
 */
int parse_rai(const char *at_response, struct lte_lc_rai_cfg *rai_cfg);

/* @brief Add the handler in the event handler list if not already present.
 *
 *  @param handler Event handler.
 *
 * @return Zero on success, negative errno code if the API call fails.
 */
int event_handler_list_append_handler(lte_lc_evt_handler_t handler);

/* @brief Remove the handler from the event handler list if present.
 *
 *  @param handler Event handler.
 *
 * @return Zero on success, negative errno code if the API call fails.
 */
int event_handler_list_remove_handler(lte_lc_evt_handler_t handler);

/* @brief Dispatch events for the registered event handlers.
 *
 *  @param evt Event.
 *
 * @return Zero on success, negative errno code if the API call fails.
 */
void event_handler_list_dispatch(const struct lte_lc_evt *const evt);

/* @brief Test if the handler list is empty.
 *
 * @return a boolean, true if it's empty, false otherwise
 */
bool event_handler_list_is_empty(void);

/* @brief Convert string to integer with a chosen base.
 *
 * @param str_buf Pointer to null-terminated string.
 * @param base The base to use when converting the string.
 * @param output Pointer to an integer where the result is stored.
 *
 * @retval 0 if conversion was successful.
 * @retval -ENODATA if conversion failed.
 */
int string_to_int(const char *str_buf, int base, int *output);

/* @brief Get periodic search pattern string to be used in AT%PERIODICSEARCHCONF from
 *	  a pattern struct.
 *
 * @param buf Buffer to store the string.
 * @param buf_size Size of the provided buffer.
 * @param pattern Pointer to pattern struct.
 *
 * @return Pointer to the buffer where the pattern string is stored.
 */
char *periodic_search_pattern_get(char *const buf, size_t buf_size,
				  const struct lte_lc_periodic_search_pattern *const pattern);

/* @brief Parse a periodic search pattern from an AT%PERIODICSEARCHCONF response
 *	  and populate a pattern struct with the result.
 *	  The pattern string is expected to be without quotation marks and null-terminated.
 *
 * @param pattern_str Pointer to pattern string.
 * @param pattern Pointer to storage for the parsed pattern.
 *
 * @retval 0 if parsing was successful.
 * @retval -EBADMSG if pattern could not be parsed.
 */
int parse_periodic_search_pattern(const char *const pattern_str,
				  struct lte_lc_periodic_search_pattern *pattern);

/* @brief Set RAI based on @kconfig{CONFIG_LTE_RAI_REQ}.
 *
 * If enabling of RAI is requested, AT%RAI=2 is used to order unsolicited RAI notifications.
 * If setting that fails, AT%RAI=1 is used.
 *
 * @retval 0 Setting RAI succeeded.
 * @retval -EFAULT Setting RAI failed.
 */
int rai_set(void);
