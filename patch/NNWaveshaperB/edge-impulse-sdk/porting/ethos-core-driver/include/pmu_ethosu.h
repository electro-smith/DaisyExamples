/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMU_ETHOSU_H
#define PMU_ETHOSU_H

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include <stdint.h>

#include "ethosu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define ETHOSU_PMU_NCOUNTERS 4

#define ETHOSU_PMU_CNT1_Msk (1UL << 0)
#define ETHOSU_PMU_CNT2_Msk (1UL << 1)
#define ETHOSU_PMU_CNT3_Msk (1UL << 2)
#define ETHOSU_PMU_CNT4_Msk (1UL << 3)
#define ETHOSU_PMU_CCNT_Msk (1UL << 31)

/*****************************************************************************
 * Types
 *****************************************************************************/

/** \brief HW Supported ETHOSU PMU Events
 *
 * Note: These values are symbolic. Actual HW-values may change. I.e. always use API
 *       to set/get actual event-type value.
 * */
enum ethosu_pmu_event_type
{
    ETHOSU_PMU_NO_EVENT = 0,
    ETHOSU_PMU_CYCLE,
    ETHOSU_PMU_NPU_IDLE,
    ETHOSU_PMU_CC_STALLED_ON_BLOCKDEP,
    ETHOSU_PMU_CC_STALLED_ON_SHRAM_RECONFIG,
    ETHOSU_PMU_NPU_ACTIVE,
    ETHOSU_PMU_MAC_ACTIVE,
    ETHOSU_PMU_MAC_ACTIVE_8BIT,
    ETHOSU_PMU_MAC_ACTIVE_16BIT,
    ETHOSU_PMU_MAC_DPU_ACTIVE,
    ETHOSU_PMU_MAC_STALLED_BY_WD_ACC,
    ETHOSU_PMU_MAC_STALLED_BY_WD,
    ETHOSU_PMU_MAC_STALLED_BY_ACC,
    ETHOSU_PMU_MAC_STALLED_BY_IB,
    ETHOSU_PMU_MAC_ACTIVE_32BIT,
    ETHOSU_PMU_MAC_STALLED_BY_INT_W,
    ETHOSU_PMU_MAC_STALLED_BY_INT_ACC,
    ETHOSU_PMU_AO_ACTIVE,
    ETHOSU_PMU_AO_ACTIVE_8BIT,
    ETHOSU_PMU_AO_ACTIVE_16BIT,
    ETHOSU_PMU_AO_STALLED_BY_OFMP_OB,
    ETHOSU_PMU_AO_STALLED_BY_OFMP,
    ETHOSU_PMU_AO_STALLED_BY_OB,
    ETHOSU_PMU_AO_STALLED_BY_ACC_IB,
    ETHOSU_PMU_AO_STALLED_BY_ACC,
    ETHOSU_PMU_AO_STALLED_BY_IB,
    ETHOSU_PMU_WD_ACTIVE,
    ETHOSU_PMU_WD_STALLED,
    ETHOSU_PMU_WD_STALLED_BY_WS,
    ETHOSU_PMU_WD_STALLED_BY_WD_BUF,
    ETHOSU_PMU_WD_PARSE_ACTIVE,
    ETHOSU_PMU_WD_PARSE_STALLED,
    ETHOSU_PMU_WD_PARSE_STALLED_IN,
    ETHOSU_PMU_WD_PARSE_STALLED_OUT,
    ETHOSU_PMU_WD_TRANS_WS,
    ETHOSU_PMU_WD_TRANS_WB,
    ETHOSU_PMU_WD_TRANS_DW0,
    ETHOSU_PMU_WD_TRANS_DW1,
    ETHOSU_PMU_AXI0_RD_TRANS_ACCEPTED,
    ETHOSU_PMU_AXI0_RD_TRANS_COMPLETED,
    ETHOSU_PMU_AXI0_RD_DATA_BEAT_RECEIVED,
    ETHOSU_PMU_AXI0_RD_TRAN_REQ_STALLED,
    ETHOSU_PMU_AXI0_WR_TRANS_ACCEPTED,
    ETHOSU_PMU_AXI0_WR_TRANS_COMPLETED_M,
    ETHOSU_PMU_AXI0_WR_TRANS_COMPLETED_S,
    ETHOSU_PMU_AXI0_WR_DATA_BEAT_WRITTEN,
    ETHOSU_PMU_AXI0_WR_TRAN_REQ_STALLED,
    ETHOSU_PMU_AXI0_WR_DATA_BEAT_STALLED,
    ETHOSU_PMU_AXI0_ENABLED_CYCLES,
    ETHOSU_PMU_AXI0_RD_STALL_LIMIT,
    ETHOSU_PMU_AXI0_WR_STALL_LIMIT,
    ETHOSU_PMU_AXI1_RD_TRANS_ACCEPTED,
    ETHOSU_PMU_AXI1_RD_TRANS_COMPLETED,
    ETHOSU_PMU_AXI1_RD_DATA_BEAT_RECEIVED,
    ETHOSU_PMU_AXI1_RD_TRAN_REQ_STALLED,
    ETHOSU_PMU_AXI1_WR_TRANS_ACCEPTED,
    ETHOSU_PMU_AXI1_WR_TRANS_COMPLETED_M,
    ETHOSU_PMU_AXI1_WR_TRANS_COMPLETED_S,
    ETHOSU_PMU_AXI1_WR_DATA_BEAT_WRITTEN,
    ETHOSU_PMU_AXI1_WR_TRAN_REQ_STALLED,
    ETHOSU_PMU_AXI1_WR_DATA_BEAT_STALLED,
    ETHOSU_PMU_AXI1_ENABLED_CYCLES,
    ETHOSU_PMU_AXI1_RD_STALL_LIMIT,
    ETHOSU_PMU_AXI1_WR_STALL_LIMIT,
    ETHOSU_PMU_AXI_LATENCY_ANY,
    ETHOSU_PMU_AXI_LATENCY_32,
    ETHOSU_PMU_AXI_LATENCY_64,
    ETHOSU_PMU_AXI_LATENCY_128,
    ETHOSU_PMU_AXI_LATENCY_256,
    ETHOSU_PMU_AXI_LATENCY_512,
    ETHOSU_PMU_AXI_LATENCY_1024,
    ETHOSU_PMU_ECC_DMA,
    ETHOSU_PMU_ECC_SB0,
    ETHOSU_PMU_ECC_SB1,

    ETHOSU_PMU_SENTINEL // End-marker (not event)
};

/*****************************************************************************
 * Functions
 *****************************************************************************/

/**
 * \brief   Enable the PMU
 */
void ETHOSU_PMU_Enable(struct ethosu_driver *drv);

/**
 * \brief   Disable the PMU
 */
void ETHOSU_PMU_Disable(struct ethosu_driver *drv);

/**
 * \brief   Set event to count for PMU eventer counter
 * \param [in]    num     Event counter (0-ETHOSU_PMU_NCOUNTERS) to configure
 * \param [in]    type    Event to count
 */
void ETHOSU_PMU_Set_EVTYPER(struct ethosu_driver *drv, uint32_t num, enum ethosu_pmu_event_type type);

/**
 * \brief   Get event to count for PMU eventer counter
 * \param [in]    num     Event counter (0-ETHOSU_PMU_NCOUNTERS) to configure
 * \return        type    Event to count
 */
enum ethosu_pmu_event_type ETHOSU_PMU_Get_EVTYPER(struct ethosu_driver *drv, uint32_t num);

/**
 * \brief  Reset cycle counter
 */
void ETHOSU_PMU_CYCCNT_Reset(struct ethosu_driver *drv);

/**
 * \brief  Reset all event counters
 */
void ETHOSU_PMU_EVCNTR_ALL_Reset(struct ethosu_driver *drv);

/**
 * \brief  Enable counters
 * \param [in]     mask    Counters to enable
 * \note   Enables one or more of the following:
 *         - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *         - cycle counter  (bit 31)
 */
void ETHOSU_PMU_CNTR_Enable(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief  Disable counters
 * \param [in]     mask    Counters to disable
 * \note   Disables one or more of the following:
 *         - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *         - cycle counter  (bit 31)
 */
void ETHOSU_PMU_CNTR_Disable(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief  Determine counters activation
 *
 * \return                Event count
 * \param [in]     mask    Counters to enable
 * \return  a bitmask where bit-set means:
 *         - event counters activated (bit 0-ETHOSU_PMU_NCOUNTERS)
 *         - cycle counter  activate  (bit 31)
 * \note   ETHOSU specific. Usage breaks CMSIS complience
 */
uint32_t ETHOSU_PMU_CNTR_Status(struct ethosu_driver *drv);

/**
 * \brief  Read cycle counter (64 bit)
 * \return                 Cycle count
 * \note   Two HW 32-bit registers that can increment independently in-between reads.
 *         To work-around raciness yet still avoid turning
 *         off the event both are read as one value twice. If the latter read
 *         is not greater than the former, it means overflow of LSW without
 *         incrementing MSW has occurred, in which case the former value is used.
 */
uint64_t ETHOSU_PMU_Get_CCNTR(struct ethosu_driver *drv);

/**
 * \brief  Set cycle counter (64 bit)
 * \param [in]    val     Conter value
 * \note   Two HW 32-bit registers that can increment independently in-between reads.
 *         To work-around raciness, counter is temporary disabled if enabled.
 * \note   ETHOSU specific. Usage breaks CMSIS complience
 */
void ETHOSU_PMU_Set_CCNTR(struct ethosu_driver *drv, uint64_t val);

/**
 * \brief   Read event counter
 * \param [in]    num     Event counter (0-ETHOSU_PMU_NCOUNTERS)
 * \return                Event count
 */
uint32_t ETHOSU_PMU_Get_EVCNTR(struct ethosu_driver *drv, uint32_t num);

/**
 * \brief   Set event counter value
 * \param [in]    num     Event counter (0-ETHOSU_PMU_NCOUNTERS)
 * \param [in]    val     Conter value
 * \note   ETHOSU specific. Usage breaks CMSIS complience
 */
void ETHOSU_PMU_Set_EVCNTR(struct ethosu_driver *drv, uint32_t num, uint32_t val);

/**
 * \brief   Read counter overflow status
 * \return  Counter overflow status bits for the following:
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS))
 *          - cycle counter  (bit 31)
 */
uint32_t ETHOSU_PMU_Get_CNTR_OVS(struct ethosu_driver *drv);

/**
 * \brief   Clear counter overflow status
 * \param [in]     mask    Counter overflow status bits to clear
 * \note    Clears overflow status bits for one or more of the following:
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *          - cycle counter  (bit 31)
 */
void ETHOSU_PMU_Set_CNTR_OVS(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief   Enable counter overflow interrupt request
 * \param [in]     mask    Counter overflow interrupt request bits to set
 * \note    Sets overflow interrupt request bits for one or more of the following:
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *          - cycle counter  (bit 31)
 */
void ETHOSU_PMU_Set_CNTR_IRQ_Enable(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief   Disable counter overflow interrupt request
 * \param [in]     mask    Counter overflow interrupt request bits to clear
 * \note    Clears overflow interrupt request bits for one or more of the following:
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *          - cycle counter  (bit 31)
 */
void ETHOSU_PMU_Set_CNTR_IRQ_Disable(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief   Get counters overflow interrupt request stiinings
 * \return   mask    Counter overflow interrupt request bits
 * \note    Sets overflow interrupt request bits for one or more of the following:
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *          - cycle counter  (bit 31)
 * \note   ETHOSU specific. Usage breaks CMSIS compliance
 */
uint32_t ETHOSU_PMU_Get_IRQ_Enable(struct ethosu_driver *drv);

/**
 * \brief   Software increment event counter
 * \param [in]     mask    Counters to increment
 *          - event counters (bit 0-ETHOSU_PMU_NCOUNTERS)
 *          - cycle counter  (bit 31)
 * \note    Software increment bits for one or more event counters.
 */
void ETHOSU_PMU_CNTR_Increment(struct ethosu_driver *drv, uint32_t mask);

/**
 * \brief   Set start event number for the cycle counter
 * \param [in]   start_event   Event to trigger start of the cycle counter
 * \note   Sets the event number that starts the cycle counter.
 */
void ETHOSU_PMU_PMCCNTR_CFG_Set_Start_Event(struct ethosu_driver *drv, enum ethosu_pmu_event_type start_event);

/**
 * \brief   Set stop event number for the cycle counter
 * \param [in]   stop_event   Event number
 * \note   Sets the event number that stops the cycle counter.
 */
void ETHOSU_PMU_PMCCNTR_CFG_Set_Stop_Event(struct ethosu_driver *drv, enum ethosu_pmu_event_type stop_event);

#ifdef __cplusplus
}
#endif

#endif /* PMU_ETHOSU_H */
