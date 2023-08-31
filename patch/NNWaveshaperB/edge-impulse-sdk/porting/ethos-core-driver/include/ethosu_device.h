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

#ifndef ETHOSU_DEVICE_H
#define ETHOSU_DEVICE_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Defines
 ******************************************************************************/

#define ETHOSU_DRIVER_VERSION_MAJOR 0  ///< Driver major version
#define ETHOSU_DRIVER_VERSION_MINOR 16 ///< Driver minor version
#define ETHOSU_DRIVER_VERSION_PATCH 0  ///< Driver patch version
#define ETHOSU_DRIVER_BASEP_INDEXES 8  ///< Number of base pointer indexes

#ifndef ETHOSU_PMU_NCOUNTERS
#define ETHOSU_PMU_NCOUNTERS 4
#endif

/******************************************************************************
 * Types
 ******************************************************************************/

enum ethosu_error_codes
{
    ETHOSU_SUCCESS         = 0,  ///< Success
    ETHOSU_GENERIC_FAILURE = -1, ///< Generic failure
    ETHOSU_INVALID_PARAM   = -2  ///< Invalid parameter
};

struct ethosu_device
{
    volatile uintptr_t base_address;
    uint32_t proto;
    uint32_t pmcr;
    uint32_t pmccntr[2];
    uint32_t pmcnten;
    uint32_t pmint;
    uint32_t pmccntr_cfg;
    uint32_t pmu_evcntr[ETHOSU_PMU_NCOUNTERS];
    uint32_t pmu_evtypr[ETHOSU_PMU_NCOUNTERS];
    uint32_t secure;
    uint32_t privileged;
};

struct ethosu_id
{
    uint32_t version_status; ///< Version status
    uint32_t version_minor;  ///< Version minor
    uint32_t version_major;  ///< Version major
    uint32_t product_major;  ///< Product major
    uint32_t arch_patch_rev; ///< Architecture version patch
    uint32_t arch_minor_rev; ///< Architecture version minor
    uint32_t arch_major_rev; ///< Architecture version major
};

struct ethosu_config
{
    struct
    {
        uint32_t macs_per_cc;        ///< MACs per clock cycle
        uint32_t cmd_stream_version; ///< NPU command stream version
        uint32_t shram_size;         ///< SHRAM size
        uint32_t custom_dma;         ///< Custom DMA enabled
    };
};

/**
 * Memory type parameter for set_regioncfg_reg:
 *   Counter{0,1}: Outstanding transactions for
 *   AXI port 0 for memory type/region a=0,b=1
 *   Counter{2,3}: Outstanding transactions for
 *   AXI port 1 for memory type/region a=2,b=3
 */
enum ethosu_memory_type
{
    ETHOSU_AXI0_OUTSTANDING_COUNTER0 = 0, ///< NPU axi0_outstanding_counter0
    ETHOSU_AXI0_OUTSTANDING_COUNTER1 = 1, ///< NPU axi0_outstanding_counter1
    ETHOSU_AXI1_OUTSTANDING_COUNTER2 = 2, ///< NPU axi1_outstanding_counter2
    ETHOSU_AXI1_OUTSTANDING_COUNTER3 = 3  ///< NPU axi1_outstanding_counter3
};

enum ethosu_axi_limit_beats
{
    ETHOSU_AXI_LIMIT_64_BYTES  = 0, ///< NPU AXI limit 64 byte burst split alignment.
    ETHOSU_AXI_LIMIT_128_BYTES = 1, ///< NPU AXI limit 128 byte burst split alignment.
    ETHOSU_AXI_LIMIT_256_BYTES = 2  ///< NPU AXI limit 256 byte burst split alignment.
};

enum ethosu_axi_limit_mem_type
{
    ETHOSU_MEM_TYPE_DEVICE_NON_BUFFERABLE                 = 0,
    ETHOSU_MEM_TYPE_DEVICE_BUFFERABLE                     = 1,
    ETHOSU_MEM_TYPE_NORMAL_NON_CACHEABLE_NON_BUFFERABLE   = 2,
    ETHOSU_MEM_TYPE_NORMAL_NON_CACHEABLE_BUFFERABLE       = 3,
    ETHOSU_MEM_TYPE_WRITE_THROUGH_NO_ALLOCATE             = 4,
    ETHOSU_MEM_TYPE_WRITE_THROUGH_READ_ALLOCATE           = 5,
    ETHOSU_MEM_TYPE_WRITE_THROUGH_WRITE_ALLOCATE          = 6,
    ETHOSU_MEM_TYPE_WRITE_THROUGH_READ_AND_WRITE_ALLOCATE = 7,
    ETHOSU_MEM_TYPE_WRITE_BACK_NO_ALLOCATE                = 8,
    ETHOSU_MEM_TYPE_WRITE_BACK_READ_ALLOCATE              = 9,
    ETHOSU_MEM_TYPE_WRITE_BACK_WRITE_ALLOCATE             = 10,
    ETHOSU_MEM_TYPE_WRITE_BACK_READ_AND_WRITE_ALLOCATE    = 11
};

enum ethosu_clock_q_request
{
    ETHOSU_CLOCK_Q_DISABLE = 0, ///< Disble NPU signal ready for clock off.
    ETHOSU_CLOCK_Q_ENABLE  = 1  ///< Enable NPU signal ready for clock off when stop+idle state reached.
};

enum ethosu_power_q_request
{
    ETHOSU_POWER_Q_DISABLE = 0, ///< Disble NPU signal ready for power off.
    ETHOSU_POWER_Q_ENABLE  = 1  ///< Enable NPU signal ready for power off when stop+idle state reached.
};

/******************************************************************************
 * Prototypes
 ******************************************************************************/

/**
 * Initialize the device.
 */
enum ethosu_error_codes ethosu_dev_init(struct ethosu_device *dev,
                                        const void *base_address,
                                        uint32_t secure_enable,
                                        uint32_t privilege_enable);

/**
 * Get device id.
 */
enum ethosu_error_codes ethosu_get_id(struct ethosu_device *dev, struct ethosu_id *id);

/**
 * Get device configuration.
 */
enum ethosu_error_codes ethosu_get_config(struct ethosu_device *dev, struct ethosu_config *config);

/**
 * Execute a given command stream on NPU.
 * \param[in] cmd_stream_ptr   Pointer to the command stream
 * \param[in] cms_length       Command stream length
 * \param[in] base_addr        Pointer to array of base addresses
 *                             - 0: weight tensor
 *                             - 1: scratch tensor
 *                             - All input tensors
 *                             - All output tensors
 * \param[in] num_base_addr    Number of base addresses.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_run_command_stream(struct ethosu_device *dev,
                                                  const uint8_t *cmd_stream_ptr,
                                                  uint32_t cms_length,
                                                  const uint64_t *base_addr,
                                                  int num_base_addr);

/**
 * Check if IRQ is raised.
 * \param[out] irq_status      Pointer to IRQ status
 *                             - 0 IRQ not raised
 *                             - 1 IRQ raised
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_is_irq_raised(struct ethosu_device *dev, uint8_t *irq_status);

/**
 * Clear IRQ status.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_clear_irq_status(struct ethosu_device *dev);

/**
 * Get the 16 bit status mask.
 * \param[out] irq_status_mask     Pointer to the status mask.
 *                                 The lower 16 bits of status reg are returned.
 *                                 bit0: state
 *                                 bit1: irq_raised
 *                                 bit2: bus_status
 *                                 bit3: reset_status
 *                                 bit4: cmd_parse_error
 *                                 bit5: cmd_end_reached
 *                                 bit6: pmu_irq_raised
 *                                 bit7-15: reserved
 * \return                         \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_get_status_mask(struct ethosu_device *dev, uint16_t *status_mask);

/**
 * Get the 16 bit IRQ history mask.
 * \param[out] irq_history_mask    Pointer to the IRQ history mask.
 * \return                         \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_get_irq_history_mask(struct ethosu_device *dev, uint16_t *irq_history_mask);

/**
 * Clear the given bits in the
 *                                     IRQ history mask.
 * \param[in] irq_history_clear_mask   16 bit mask indicating which bits to
 *                                     clear in the IRQ history mask.
 * \return                             \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_clear_irq_history_mask(struct ethosu_device *dev, uint16_t irq_history_clear_mask);

/**
 * Perform a NPU soft reset.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_soft_reset(struct ethosu_device *dev);

/**
 * Wait for reset ready.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_wait_for_reset(struct ethosu_device *dev);

/**
 * Read and return the content of a given NPU APB
 *                             register range.
 * \param[in] start_address    Start address.
 * \param[in] num_reg          Number of registers to read.
 * \param[out] reg_p           Pointer to a output area, allocated by the
 *                             caller, where the register content shall be
 *                             written.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_read_apb_reg(struct ethosu_device *dev,
                                            uint32_t start_address,
                                            uint16_t num_reg,
                                            uint32_t *reg_p);

/**
 * Set qconfig register. I.e.
 *                             AXI configuration for the command stream.
 * \param[in] memory_type      Memory_type to use for command stream:
 *                             enum ethosu_memory_type.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_qconfig(struct ethosu_device *dev, enum ethosu_memory_type memory_type);

/**
 * Set register REGIONCFG.
 *                             Base pointer configuration.
 *                             Bits[2*k+1:2*k] give the memory type for BASEP[k].
 * \param[in] region           Region field to set: 0 - 7.
 * \param[in] memory_type      Memory_type to use for region: enum ethosu_memory_type.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_regioncfg(struct ethosu_device *dev,
                                             uint8_t region,
                                             enum ethosu_memory_type memory_type);

/**
 * Set AXI limit parameters for port 0 counter 0.
 * \param[in] max_beats        Burst split alignment, \ref ethosu_axi_limit_beats.
 * \param[in] memtype          Cache policy \ref ethosu_axi_limit_mem_type
 * \param[in] max_reads        Maximum number of outstanding reads.
 * \param[in] max_writes       Maximum number of outstanding writes.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_axi_limit0(struct ethosu_device *dev,
                                              enum ethosu_axi_limit_beats max_beats,
                                              enum ethosu_axi_limit_mem_type memtype,
                                              uint8_t max_reads,
                                              uint8_t max_writes);
/**
 * Set AXI limit parameters for port 0 counter 1.
 * \param[in] max_beats        Burst split alignment, \ref ethosu_axi_limit_beats.
 * \param[in] memtype          Cache policy \ref ethosu_axi_limit_mem_type
 * \param[in] max_reads        Maximum number of outstanding reads.
 * \param[in] max_writes       Maximum number of outstanding writes.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_axi_limit1(struct ethosu_device *dev,
                                              enum ethosu_axi_limit_beats max_beats,
                                              enum ethosu_axi_limit_mem_type memtype,
                                              uint8_t max_reads,
                                              uint8_t max_writes);
/**
 * Set AXI limit parameters for port 1 counter 2.
 * \param[in] max_beats        Burst split alignment, \ref ethosu_axi_limit_beats.
 * \param[in] memtype          Cache policy \ref ethosu_axi_limit_mem_type
 * \param[in] max_reads        Maximum number of outstanding reads.
 * \param[in] max_writes       Maximum number of outstanding writes.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_axi_limit2(struct ethosu_device *dev,
                                              enum ethosu_axi_limit_beats max_beats,
                                              enum ethosu_axi_limit_mem_type memtype,
                                              uint8_t max_reads,
                                              uint8_t max_writes);
/**
 * Set AXI limit parameters for port 1 counter 3.
 * \param[in] max_beats        Burst split alignment, \ref ethosu_axi_limit_beats.
 * \param[in] memtype          Cache policy \ref ethosu_axi_limit_mem_type
 * \param[in] max_reads        Maximum number of outstanding reads.
 * \param[in] max_writes       Maximum number of outstanding writes.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_axi_limit3(struct ethosu_device *dev,
                                              enum ethosu_axi_limit_beats max_beats,
                                              enum ethosu_axi_limit_mem_type memtype,
                                              uint8_t max_reads,
                                              uint8_t max_writes);

/**
 * Get current command stream queue read position.
 * \param[out] qread           Pointer to queue read.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_get_qread(struct ethosu_device *dev, uint32_t *qread);

/**
 * Get revision of NPU
 * \param[out] revision        Pointer to revision read.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_get_revision(struct ethosu_device *dev, uint32_t *revision);

/**
 * Issue run command for the currently programmed
 *                             command stream, starting at current queue read
 *                             position.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_command_run(struct ethosu_device *dev);

/**
 * Dump a 1KB section of SHRAM.
 * \param[in] section          Section offset to 1KB section in SHRAM.
 * \param[out] shram_p         Pointer to a output area, allocated by the
 *                             caller, where the SHRAM content shall be
 *                             written.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_get_shram_data(struct ethosu_device *dev, int section, uint32_t *shram_p);

/**
 * Set clock and power q request enable bits.
 * \param[in] clock_q          Clock q ENABLE/DISABLE \ref clock_q_request.
 * \param[in] power_q          Power q ENABLE/DISABLE \ref power_q_request.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_set_clock_and_power(struct ethosu_device *dev,
                                                   enum ethosu_clock_q_request clock_q,
                                                   enum ethosu_power_q_request power_q);

/**
 * Read register.
 * \param[in] address          Address to read.
 * \return                     Register value.
 */
uint32_t ethosu_read_reg(struct ethosu_device *dev, uint32_t address);

/**
 * Write register.
 * \param[in] address          Address to read.
 * \param[in] value            Value to be written.
 */
void ethosu_write_reg(struct ethosu_device *dev, uint32_t address, uint32_t value);

/**
 * Write register with shadow variable.
 * \param[in] address          Address to read.
 * \param[in] value            Value to be written.
 */
void ethosu_write_reg_shadow(struct ethosu_device *dev, uint32_t address, uint32_t value, uint32_t *shadow);

/**
 * Save the PMU configuration to ethosu_device struct.
 * \param[in] dev              Ethos-U device where the PMU configuration is
 *                             saved.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_save_pmu_config(struct ethosu_device *dev);

/**
 * Restore the PMU configuration from a ethosu_device struct.
 * \param[in] dev              Ethos-U device where the PMU configuration is
 *                             stored.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_restore_pmu_config(struct ethosu_device *dev);

/**
 * Save PMU counters to shadow variables in memory.
 * \param[in] dev              Ethos-U device where the PMU configuration is
 *                             stored.
 * \return                     \ref ethosu_error_codes
 */
enum ethosu_error_codes ethosu_save_pmu_counters(struct ethosu_device *dev);

/**
 * Check if the STATUS register has any error bits set or not.
 * \param[in] dev              Ethos-U device to check.
 * \return                     true if any error bits set,
 *                             false otherwise.
 */
bool ethosu_status_has_error(struct ethosu_device *dev);

#ifdef __cplusplus
}
#endif

#endif // ETHOSU_DEVICE_H
