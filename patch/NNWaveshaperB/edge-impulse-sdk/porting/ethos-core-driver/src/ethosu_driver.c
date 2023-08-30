/*
 * Copyright (c) 2019-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#if EI_ETHOS

#include "ethosu_driver.h"
#include "ethosu_common.h"
#include "ethosu_config.h"
#include "ethosu_device.h"

#include <assert.h>
#include <cmsis_compiler.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 * Defines
 ******************************************************************************/

#define MACS_PER_CYCLE_LOG2_MASK 0x000F
#define SHRAM_SIZE_MASK 0xFF00
#define SHRAM_SIZE_RIGHT_SHIFT 8
#define BYTES_IN_32_BITS 4
#define CUSTOM_OPTION_LENGTH_32_BIT_WORD 1
#define DRIVER_ACTION_LENGTH_32_BIT_WORD 1
#define OPTIMIZER_CONFIG_LENGTH_32_BIT_WORD 2
#define ETHOSU_FOURCC ('1' << 24 | 'P' << 16 | 'O' << 8 | 'C') // "Custom Operator Payload 1"
#define APB_START_ADDR_MASK 0x0FFF
#define APB_NUM_REG_BIT_SHIFT 12
#define BYTES_1KB 1024
#define PRODUCT_MAJOR_ETHOSU55 (4)
#define MASK_16_BYTE_ALIGN (0xF)
#define FAST_MEMORY_BASE_ADDR_INDEX 2

/******************************************************************************
 * Types
 ******************************************************************************/

// Driver actions
enum DRIVER_ACTION_e
{
    RESERVED         = 0,
    OPTIMIZER_CONFIG = 1,
    COMMAND_STREAM   = 2,
    READ_APB_REG     = 3,
    DUMP_SHRAM       = 4,
    NOP              = 5,
};

// Custom data struct
struct custom_data_s
{
    union
    {
        // Driver action data
        struct
        {
            // Driver action command (valid values in DRIVER_ACTION_e)
            uint8_t driver_action_command;

            // reserved
            uint8_t reserved;

            // Driver action data
            union
            {
                // DA_CMD_OPT_CFG
                struct
                {
                    uint16_t rel_nbr : 4;
                    uint16_t patch_nbr : 4;
                    uint16_t opt_cfg_reserved : 8;
                };

                // DA_CMD_CMSTRM
                struct
                {
                    uint16_t length;
                };

                // DA_CMD_READAPB
                struct
                {
                    uint16_t start_address : 12;
                    uint16_t nbr_reg_minus1 : 4;
                };

                uint16_t driver_action_data;
            };
        };

        uint32_t word;
    };
};

// optimizer config struct
struct opt_cfg_s
{
    struct custom_data_s da_data;
    union
    {
        struct
        {
            uint32_t macs_per_cc : 4;
            uint32_t cmd_stream_version : 4;
            uint32_t shram_size : 8;
            uint32_t reserved0 : 11;
            uint32_t custom_dma : 1;
            uint32_t product : 4;
        };
        uint32_t npu_cfg;
    };
    union
    {
        struct
        {
            uint32_t version_status : 4;
            uint32_t version_minor : 4;
            uint32_t version_major : 4;
            uint32_t product_major : 4;
            uint32_t arch_patch_rev : 4;
            uint32_t arch_minor_rev : 8;
            uint32_t arch_major_rev : 4;
        };
        uint32_t ethosu_id;
    };
};

/******************************************************************************
 * Functions
 ******************************************************************************/

struct ethosu_driver ethosu_drv = {
    .dev = {.base_address = NULL, .proto = 0, .pmccntr = {0}, .pmu_evcntr = {0, 0, 0, 0}, .pmu_evtypr = {0, 0, 0, 0}},
    .abort_inference     = false,
    .status_error        = false,
    .dev_power_always_on = false};

// Registered drivers linked list HEAD
static struct ethosu_driver *registered_drivers = NULL;

/*
 * Following section handles the minimal sempahore and mutex implementation in case of baremetal applications.
 * Weak symbols will be overwritten by RTOS definitions and implement true thread-safety. (Done in application layer)
 */

// Baremetal sempahore implementation
struct ethosu_semaphore_t
{
    int count;
};

// Minimal needed declaration to allow baremetal functionality.
static void *ethosu_mutex;
static void *ethosu_semaphore;

void *__attribute__((weak)) ethosu_mutex_create(void)
{
    return NULL;
}

void __attribute__((weak)) ethosu_mutex_lock(void *mutex)
{
    UNUSED(mutex);
}

void __attribute__((weak)) ethosu_mutex_unlock(void *mutex)
{
    UNUSED(mutex);
}

// Baremetal implementation of creating a semaphore
void *__attribute__((weak)) ethosu_semaphore_create(void)
{
    struct ethosu_semaphore_t *sem = malloc(sizeof(*sem));
    sem->count                     = 1;
    return sem;
}

// Baremetal simulation of waiting/sleeping for and then taking a semaphore using intrisics
void __attribute__((weak)) ethosu_semaphore_take(void *sem)
{
    struct ethosu_semaphore_t *s = sem;
    while (s->count <= 0)
    {
        __WFE();
    }
    s->count--;
}

// Baremetal simulation of giving a semaphore and waking up processes using intrinsics
void __attribute__((weak)) ethosu_semaphore_give(void *sem)
{
    struct ethosu_semaphore_t *s = sem;
    s->count++;
    __SEV();
}
// <--- End of semaphore and mutex implementations

static int ethosu_soft_reset_and_restore(struct ethosu_driver *drv);

void __attribute__((weak)) ethosu_irq_handler(struct ethosu_driver *drv)
{
    uint8_t irq_raised = 0;

    LOG_DEBUG("Interrupt. status=0x%08x, qread=%d\n",
              ethosu_read_reg(&drv->dev, NPU_REG_STATUS),
              ethosu_read_reg(&drv->dev, NPU_REG_QREAD));

    // Verify that interrupt has been raised
    (void)ethosu_is_irq_raised(&drv->dev, &irq_raised);
    assert(irq_raised == 1);
    drv->irq_triggered = true;

    // Clear interrupt
    (void)ethosu_clear_irq_status(&drv->dev);

    // Verify that interrupt has been successfully cleared
    (void)ethosu_is_irq_raised(&drv->dev, &irq_raised);
    assert(irq_raised == 0);

    if (ethosu_status_has_error(&drv->dev))
    {
        ethosu_soft_reset_and_restore(drv);
        drv->status_error = true;
    }

    ethosu_semaphore_give(drv->semaphore);
}

static inline void wait_for_irq(struct ethosu_driver *drv)
{
    while (1)
    {
        if (drv->irq_triggered || drv->abort_inference)
        {
            drv->irq_triggered = false;
            break;
        }

        ethosu_semaphore_take(drv->semaphore);
    }
}

void __attribute__((weak)) ethosu_inference_begin(struct ethosu_driver *drv, const void *inference_data)
{
    (void)inference_data;
    (void)drv;
}

void __attribute__((weak)) ethosu_inference_end(struct ethosu_driver *drv, const void *inference_data)
{
    (void)inference_data;
    (void)drv;
}

static int handle_optimizer_config(struct ethosu_driver *drv, struct opt_cfg_s *opt_cfg_p);
static int handle_command_stream(struct ethosu_driver *drv,
                                 const uint8_t *cmd_stream,
                                 const int cms_length,
                                 const uint64_t *base_addr,
                                 const size_t *base_addr_size,
                                 const int num_base_addr);
static int read_apb_reg(struct ethosu_driver *drv, uint16_t);
static int dump_shram(struct ethosu_driver *drv);
static void dump_npu_register(struct ethosu_driver *drv, int npu_reg, int npu_reg_end);
static void dump_command_stream(const uint32_t *cmd_stream, const int cms_length, int qread);
static void npu_axi_init(struct ethosu_driver *drv);
static struct ethosu_driver *ethosu_find_and_reserve_driver(void);

int ethosu_init(struct ethosu_driver *drv,
                const void *base_address,
                const void *fast_memory,
                const size_t fast_memory_size,
                uint32_t secure_enable,
                uint32_t privilege_enable)
{
    int return_code = 0;

    LOG_INFO("%s. base_address=%p, fast_memory=%p, fast_memory_size=%zu, secure=%" PRIu32 ", privileged=%" PRIu32 "\n",
             __FUNCTION__,
             base_address,
             fast_memory,
             fast_memory_size,
             secure_enable,
             privilege_enable);

    if (!ethosu_mutex)
    {
        ethosu_mutex = ethosu_mutex_create();
    }

    if (!ethosu_semaphore)
    {
        ethosu_semaphore = ethosu_semaphore_create();
    }

    ethosu_register_driver(drv);

    drv->fast_memory      = (uint32_t)fast_memory;
    drv->fast_memory_size = fast_memory_size;
    drv->irq_triggered    = false;
    drv->semaphore        = ethosu_semaphore_create();

    if (ETHOSU_SUCCESS != ethosu_dev_init(&drv->dev, base_address, secure_enable, privilege_enable))
    {
        LOG_ERR("Failed in ethosu_dev_init");
        return -1;
    }

    if (ETHOSU_SUCCESS !=
        set_clock_and_power_request(drv, ETHOSU_INFERENCE_REQUEST, ETHOSU_CLOCK_Q_DISABLE, ETHOSU_POWER_Q_DISABLE))
    {
        LOG_ERR("Failed to disable clock-q & power-q for Ethos-U\n");
        return -1;
    }

    if (ETHOSU_SUCCESS != ethosu_soft_reset(&drv->dev))
    {
        return -1;
    }

    if (ETHOSU_SUCCESS != ethosu_wait_for_reset(&drv->dev))
    {
        LOG_ERR("Failed reset of Ethos-U\n");
        return -1;
    }

    drv->status_error = false;

    return return_code;
}

int ethosu_get_version(struct ethosu_driver *drv, struct ethosu_version *version)
{
    int return_code = 0;

    if (NULL != version)
    {
        struct ethosu_id id;
        struct ethosu_config cfg;
        (void)ethosu_get_id(&drv->dev, &id);
        (void)ethosu_get_config(&drv->dev, &cfg);

        version->id.version_status      = id.version_status;
        version->id.version_minor       = id.version_minor;
        version->id.version_major       = id.version_major;
        version->id.product_major       = id.product_major;
        version->id.arch_patch_rev      = id.arch_patch_rev;
        version->id.arch_minor_rev      = id.arch_minor_rev;
        version->id.arch_major_rev      = id.arch_major_rev;
        version->id.driver_patch_rev    = ETHOSU_DRIVER_VERSION_PATCH;
        version->id.driver_minor_rev    = ETHOSU_DRIVER_VERSION_MINOR;
        version->id.driver_major_rev    = ETHOSU_DRIVER_VERSION_MAJOR;
        version->cfg.macs_per_cc        = cfg.macs_per_cc;
        version->cfg.cmd_stream_version = cfg.cmd_stream_version;
        version->cfg.shram_size         = cfg.shram_size;
        version->cfg.custom_dma         = cfg.custom_dma;
    }
    else
    {
        return_code = -1;
    }

    return return_code;
}

int ethosu_invoke(struct ethosu_driver *drv,
                  const void *custom_data_ptr,
                  const int custom_data_size,
                  const uint64_t *base_addr,
                  const size_t *base_addr_size,
                  const int num_base_addr)
{
    const struct custom_data_s *data_ptr = custom_data_ptr;
    const struct custom_data_s *data_end = custom_data_ptr + custom_data_size;
    int return_code                      = 0;

    LOG_INFO("%s\n", __FUNCTION__);

    // First word in custom_data_ptr should contain "Custom Operator Payload 1"
    if (data_ptr->word != ETHOSU_FOURCC)
    {
        LOG_ERR("Custom Operator Payload: %" PRIu32 " is not correct, expected %x\n", data_ptr->word, ETHOSU_FOURCC);
        return -1;
    }

    // Custom data length must be a multiple of 32 bits
    if ((custom_data_size % BYTES_IN_32_BITS) != 0)
    {
        LOG_ERR("ethosu_invoke ERROR custom_data_size=0x%x not a multiple of 4\n", custom_data_size);
        return -1;
    }

    ++data_ptr;

    // Adjust base address to fast memory area
    if (drv->fast_memory != 0 && num_base_addr >= FAST_MEMORY_BASE_ADDR_INDEX)
    {
        uint64_t *fast_memory = (uint64_t *)&base_addr[FAST_MEMORY_BASE_ADDR_INDEX];

        if (base_addr_size != NULL && base_addr_size[FAST_MEMORY_BASE_ADDR_INDEX] > drv->fast_memory_size)
        {
            LOG_ERR("Fast memory area too small. fast_memory_size=%u, base_addr_size=%u\n",
                    drv->fast_memory_size,
                    base_addr_size[FAST_MEMORY_BASE_ADDR_INDEX]);
            return -1;
        }

        *fast_memory = drv->fast_memory;
    }

    if (!drv->dev_power_always_on)
    {
        // Only soft reset if securty state or privilege level needs changing
        if (drv->dev.proto != ethosu_read_reg(&drv->dev, NPU_REG_PROT))
        {
            if (ETHOSU_SUCCESS != ethosu_soft_reset(&drv->dev))
            {
                return -1;
            }
        }

        drv->status_error = false;
        set_clock_and_power_request(drv, ETHOSU_INFERENCE_REQUEST, ETHOSU_CLOCK_Q_ENABLE, ETHOSU_POWER_Q_DISABLE);
        ethosu_restore_pmu_config(&drv->dev);
        npu_axi_init(drv);
    }

    drv->status_error = false;

    ethosu_inference_begin(drv, custom_data_ptr);
    while (data_ptr < data_end)
    {
        int ret = 0;
        switch (data_ptr->driver_action_command)
        {
        case OPTIMIZER_CONFIG:
            LOG_INFO("ethosu_invoke OPTIMIZER_CONFIG\n");
            struct opt_cfg_s *opt_cfg_p = (struct opt_cfg_s *)data_ptr;

            ret = handle_optimizer_config(drv, opt_cfg_p);
            data_ptr += DRIVER_ACTION_LENGTH_32_BIT_WORD + OPTIMIZER_CONFIG_LENGTH_32_BIT_WORD;
            break;
        case COMMAND_STREAM:
            LOG_INFO("ethosu_invoke COMMAND_STREAM\n");
            void *command_stream = (uint8_t *)(data_ptr) + sizeof(struct custom_data_s);
            int cms_length       = (data_ptr->reserved << 16) | data_ptr->length;

            drv->abort_inference = false;
            // It is safe to clear this flag without atomic, because npu is not running.
            drv->irq_triggered = false;

            ret = handle_command_stream(drv, command_stream, cms_length, base_addr, base_addr_size, num_base_addr);

            if (return_code == -1 && drv->abort_inference)
            {
                uint32_t qread = 0;
                ethosu_get_qread(&drv->dev, &qread);
                LOG_ERR("NPU timeout\n");
                dump_command_stream(command_stream, cms_length, qread);
                dump_npu_register(drv, 0x200, 0x2BF);
                dump_npu_register(drv, 0x800, 0xB3F);
                dump_shram(drv);
            }

            data_ptr += DRIVER_ACTION_LENGTH_32_BIT_WORD + cms_length;
            break;
        case READ_APB_REG:
            LOG_INFO("ethosu_invoke READ_APB_REG\n");
            ret = read_apb_reg(drv, data_ptr->driver_action_data);
            data_ptr += DRIVER_ACTION_LENGTH_32_BIT_WORD;
            break;
        case DUMP_SHRAM:
            LOG_INFO("ethosu_invoke DUMP_SHRAM\n");
            ret = dump_shram(drv);
            data_ptr += DRIVER_ACTION_LENGTH_32_BIT_WORD;
            break;
        case NOP:
            LOG_INFO("ethosu_invoke NOP\n");
            data_ptr += DRIVER_ACTION_LENGTH_32_BIT_WORD;
            break;
        default:
            LOG_ERR("ethosu_invoke UNSUPPORTED driver_action_command %d \n", data_ptr->driver_action_command);
            ret = -1;
            break;
        }
        if (ret != 0)
        {
            return_code = -1;
            break;
        }
    }
    ethosu_inference_end(drv, custom_data_ptr);

    if (!drv->status_error && !drv->dev_power_always_on)
    {
        ethosu_save_pmu_counters(&drv->dev);
        set_clock_and_power_request(drv, ETHOSU_INFERENCE_REQUEST, ETHOSU_CLOCK_Q_ENABLE, ETHOSU_POWER_Q_ENABLE);
    }

    return return_code;
}

void ethosu_abort(struct ethosu_driver *drv)
{
    drv->abort_inference = true;
}

void ethosu_set_power_mode(struct ethosu_driver *drv, bool always_on)
{
    drv->dev_power_always_on = always_on;

    if (always_on)
    {
        npu_axi_init(drv);
    }
}

int ethosu_register_driver(struct ethosu_driver *drv)
{
    // Safeguard check for if driver is already registered
    struct ethosu_driver *cur = registered_drivers;
    while (cur != NULL)
    {
        if (cur == drv)
        {
            LOG_ERR("%s: NPU driver at address %p is already registered.\n", __FUNCTION__, drv);
            return -1;
        }
        cur = cur->next;
    }

    drv->next = registered_drivers;
    // Designate new registered driver HEAD
    registered_drivers = drv;

    LOG_INFO("%s: New NPU driver at address %p is registered.\n", __FUNCTION__, drv);
    return 0;
}

int ethosu_deregister_driver(struct ethosu_driver *drv)
{
    struct ethosu_driver *cur   = registered_drivers;
    struct ethosu_driver **prev = &registered_drivers;

    while (cur != NULL)
    {
        if (cur == drv)
        {
            *prev = cur->next;
            LOG_INFO("%s: NPU driver at address %p is deregistered.\n", __FUNCTION__, drv);
            return 0;
        }

        prev = &cur->next;
        cur  = cur->next;
    }

    LOG_ERR("%s: NPU driver at address %p does not match a registered driver and therefore may not be deregistered.\n",
            __FUNCTION__,
            drv);

    return -1;
}

struct ethosu_driver *ethosu_reserve_driver(void)
{
    struct ethosu_driver *drv = NULL;

    do
    {
        ethosu_mutex_lock(ethosu_mutex);
        drv = ethosu_find_and_reserve_driver();
        ethosu_mutex_unlock(ethosu_mutex);

        if (drv != NULL)
        {
            break;
        }

        LOG_INFO("%s - Waiting for driver \n", __FUNCTION__);
        ethosu_semaphore_take(ethosu_semaphore);

    } while (1);

    return drv;
}

static struct ethosu_driver *ethosu_find_and_reserve_driver(void)
{
    struct ethosu_driver *drv = registered_drivers;

    while (drv != NULL)
    {
        if (!drv->reserved)
        {
            drv->reserved = true;
            LOG_INFO("%s - Driver %p reserved.\n", __FUNCTION__, drv);
            return drv;
        }
        drv = drv->next;
    }

    LOG_INFO("%s: No available drivers.\n", __FUNCTION__);

    return NULL;
}

void ethosu_release_driver(struct ethosu_driver *drv)
{
    ethosu_mutex_lock(ethosu_mutex);
    if (drv != NULL && drv->reserved)
    {
        drv->reserved = false;
        LOG_INFO("%s - Driver %p released\n", __FUNCTION__, drv);
        ethosu_semaphore_give(ethosu_semaphore);
    }
    ethosu_mutex_unlock(ethosu_mutex);
}

static int ethosu_soft_reset_and_restore(struct ethosu_driver *drv)
{

    if (ETHOSU_SUCCESS != ethosu_soft_reset(&drv->dev))
    {
        return -1;
    }

    set_clock_and_power_request(drv, ETHOSU_INFERENCE_REQUEST, ETHOSU_CLOCK_Q_ENABLE, ETHOSU_POWER_Q_DISABLE);

    npu_axi_init(drv);
    ethosu_restore_pmu_config(&drv->dev);

    return 0;
}

enum ethosu_error_codes set_clock_and_power_request(struct ethosu_driver *drv,
                                                    enum ethosu_request_clients client,
                                                    enum ethosu_clock_q_request clock_request,
                                                    enum ethosu_power_q_request power_request)
{
    // Set clock request bit for client
    if (clock_request == ETHOSU_CLOCK_Q_DISABLE)
    {
        drv->clock_request |= (1 << client);
    }
    else
    {
        drv->clock_request &= ~(1 << client);
    }
    // Get current clock request (ENABLE if both PMU and INFERENCE asks for clock request, else DISABLE)
    clock_request = drv->clock_request == 0 ? ETHOSU_CLOCK_Q_ENABLE : ETHOSU_CLOCK_Q_DISABLE;

    // Set power request bit for client
    if (power_request == ETHOSU_POWER_Q_DISABLE)
    {
        drv->power_request |= (1 << client);
    }
    else
    {
        drv->power_request &= ~(1 << client);
    }
    // Get current power request (ENABLE if both PMU and INFERENCE asks for power request, else DISABLE)
    power_request = drv->power_request == 0 ? ETHOSU_POWER_Q_ENABLE : ETHOSU_POWER_Q_DISABLE;

    // Set clock and power
    enum ethosu_error_codes ret = ethosu_set_clock_and_power(&drv->dev, clock_request, power_request);

    return ret;
}

static int handle_optimizer_config(struct ethosu_driver *drv, struct opt_cfg_s *opt_cfg_p)
{
    struct ethosu_config cfg;
    struct ethosu_id id;
    int return_code = 0;

    LOG_INFO("handle_optimizer_config:\n");
    LOG_INFO("Optimizer release nbr: %d patch: %d\n", opt_cfg_p->da_data.rel_nbr, opt_cfg_p->da_data.patch_nbr);
    LOG_INFO("Optimizer config cmd_stream_version: %d macs_per_cc: %d shram_size: %d custom_dma: %d\n",
             opt_cfg_p->cmd_stream_version,
             opt_cfg_p->macs_per_cc,
             opt_cfg_p->shram_size,
             opt_cfg_p->custom_dma);
    LOG_INFO("Optimizer config Ethos-U version: %d.%d.%d\n",
             opt_cfg_p->arch_major_rev,
             opt_cfg_p->arch_minor_rev,
             opt_cfg_p->arch_patch_rev);

    (void)ethosu_get_config(&drv->dev, &cfg);
    (void)ethosu_get_id(&drv->dev, &id);
    LOG_INFO("Ethos-U config cmd_stream_version: %" PRIu32 " macs_per_cc: %" PRIu32 " shram_size: %" PRIu32
             " custom_dma: %" PRIu32 "\n",
             cfg.cmd_stream_version,
             cfg.macs_per_cc,
             cfg.shram_size,
             cfg.custom_dma);
    LOG_INFO("Ethos-U version: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
             id.arch_major_rev,
             id.arch_minor_rev,
             id.arch_patch_rev);

    if ((cfg.macs_per_cc != opt_cfg_p->macs_per_cc) || (cfg.shram_size != opt_cfg_p->shram_size) ||
        (cfg.cmd_stream_version != opt_cfg_p->cmd_stream_version) || (!cfg.custom_dma && opt_cfg_p->custom_dma))
    {
        if (cfg.macs_per_cc != opt_cfg_p->macs_per_cc)
        {
            LOG_ERR("NPU config mismatch: npu.macs_per_cc=%" PRIu32 " optimizer.macs_per_cc=%d\n",
                    cfg.macs_per_cc,
                    opt_cfg_p->macs_per_cc);
        }
        if (cfg.shram_size != opt_cfg_p->shram_size)
        {
            LOG_ERR("NPU config mismatch: npu.shram_size=%" PRIu32 " optimizer.shram_size=%d\n",
                    cfg.shram_size,
                    opt_cfg_p->shram_size);
        }
        if (cfg.cmd_stream_version != opt_cfg_p->cmd_stream_version)
        {
            LOG_ERR("NPU config mismatch: npu.cmd_stream_version=%" PRIu32 " optimizer.cmd_stream_version=%d\n",
                    cfg.cmd_stream_version,
                    opt_cfg_p->cmd_stream_version);
        }
        if (!cfg.custom_dma && opt_cfg_p->custom_dma)
        {
            LOG_ERR("NPU config mismatch: npu.custom_dma=%" PRIu32 " optimize.custom_dma=%d\n",
                    cfg.custom_dma,
                    opt_cfg_p->custom_dma);
        }
        LOG_ERR("Did you choose the correct target core? This model was compiled for a different Ethos configuration\n");
        return_code = -1;
    }

    if ((id.arch_major_rev != opt_cfg_p->arch_major_rev) || (id.arch_minor_rev < opt_cfg_p->arch_minor_rev))
    {
        LOG_ERR("NPU arch mismatch: npu.arch=%" PRIu32 ".%" PRIu32 ".%" PRIu32 " optimizer.arch=%d.%d.%d\n",
                id.arch_major_rev,
                id.arch_minor_rev,
                id.arch_patch_rev,
                opt_cfg_p->arch_major_rev,
                opt_cfg_p->arch_minor_rev,
                opt_cfg_p->arch_patch_rev);
        return_code = -1;
    }

#if !defined(LOG_ENABLED)
    UNUSED(opt_cfg_p);
#endif
    return return_code;
}

static void npu_axi_init(struct ethosu_driver *drv)
{
    ethosu_set_qconfig(&drv->dev, NPU_QCONFIG);

    ethosu_set_regioncfg(&drv->dev, 0, NPU_REGIONCFG_0);
    ethosu_set_regioncfg(&drv->dev, 1, NPU_REGIONCFG_1);
    ethosu_set_regioncfg(&drv->dev, 2, NPU_REGIONCFG_2);
    ethosu_set_regioncfg(&drv->dev, 3, NPU_REGIONCFG_3);
    ethosu_set_regioncfg(&drv->dev, 4, NPU_REGIONCFG_4);
    ethosu_set_regioncfg(&drv->dev, 5, NPU_REGIONCFG_5);
    ethosu_set_regioncfg(&drv->dev, 6, NPU_REGIONCFG_6);
    ethosu_set_regioncfg(&drv->dev, 7, NPU_REGIONCFG_7);

    (void)ethosu_set_axi_limit0(&drv->dev,
                                AXI_LIMIT0_MAX_BEATS_BYTES,
                                AXI_LIMIT0_MEM_TYPE,
                                AXI_LIMIT0_MAX_OUTSTANDING_READS,
                                AXI_LIMIT0_MAX_OUTSTANDING_WRITES);
    (void)ethosu_set_axi_limit1(&drv->dev,
                                AXI_LIMIT1_MAX_BEATS_BYTES,
                                AXI_LIMIT1_MEM_TYPE,
                                AXI_LIMIT1_MAX_OUTSTANDING_READS,
                                AXI_LIMIT1_MAX_OUTSTANDING_WRITES);
    (void)ethosu_set_axi_limit2(&drv->dev,
                                AXI_LIMIT2_MAX_BEATS_BYTES,
                                AXI_LIMIT2_MEM_TYPE,
                                AXI_LIMIT2_MAX_OUTSTANDING_READS,
                                AXI_LIMIT2_MAX_OUTSTANDING_WRITES);
    (void)ethosu_set_axi_limit3(&drv->dev,
                                AXI_LIMIT3_MAX_BEATS_BYTES,
                                AXI_LIMIT3_MEM_TYPE,
                                AXI_LIMIT3_MAX_OUTSTANDING_READS,
                                AXI_LIMIT3_MAX_OUTSTANDING_WRITES);
}

/* Default implementation to flush the data cache. Override if available on the targeted device.
 * Passing NULL as p argument expects the whole cache to be flushed.
 */
void __attribute__((weak)) ethosu_flush_dcache(uint32_t *p, size_t bytes)
{
    (void)p;
    (void)bytes;
}

/* Default implementation to invalidate the data cache. Override if available on the targeted device.
 * Passing NULL as p argument expects the whole cache to be flushed.
 */
void __attribute__((weak)) ethosu_invalidate_dcache(uint32_t *p, size_t bytes)
{
    (void)p;
    (void)bytes;
}

static int handle_command_stream(struct ethosu_driver *drv,
                                 const uint8_t *cmd_stream,
                                 const int cms_length,
                                 const uint64_t *base_addr,
                                 const size_t *base_addr_size,
                                 const int num_base_addr)
{
    uint32_t qread           = 0;
    uint32_t cms_bytes       = cms_length * BYTES_IN_32_BITS;
    ptrdiff_t cmd_stream_ptr = (ptrdiff_t)cmd_stream;

    LOG_INFO("handle_command_stream: cmd_stream=%p, cms_length %d\n", cmd_stream, cms_length);

    if (0 != ((ptrdiff_t)cmd_stream & MASK_16_BYTE_ALIGN))
    {
        LOG_ERR("Error: Command stream addr %p not aligned to 16 bytes\n", cmd_stream);
        return -1;
    }

    bool base_addr_invalid = false;
    for (int i = 0; i < num_base_addr; i++)
    {
        if (0 != (base_addr[i] & MASK_16_BYTE_ALIGN))
        {
            LOG_ERR("Error: Base addr %d: 0x%llx not aligned to 16 bytes\n", i, base_addr[i]);
            base_addr_invalid = true;
        }
    }

    if (base_addr_invalid)
    {
        return -1;
    }

    /* Flush the cache if available on our CPU.
     * The upcasting to uin32_t* is ok since the pointer never is dereferenced.
     * The base_addr_size is null if invoking from prior to invoke_V2, in that case
     * the whole cache is being flushed.
     */

    if (base_addr_size != NULL)
    {
        ethosu_flush_dcache((uint32_t *)cmd_stream_ptr, cms_bytes);
        for (int i = 0; i < num_base_addr; i++)
        {
            ethosu_flush_dcache((uint32_t *)(uintptr_t)base_addr[i], base_addr_size[i]);
        }
    }
    else
    {
        ethosu_flush_dcache(NULL, 0);
    }

    if (ETHOSU_SUCCESS != ethosu_run_command_stream(&drv->dev, cmd_stream, cms_bytes, base_addr, num_base_addr))
    {
        return -1;
    }

    wait_for_irq(drv);

    if (drv->status_error)
    {
        return -1;
    }

    if (base_addr_size != NULL)
    {
        for (int i = 0; i < num_base_addr; i++)
        {
            ethosu_invalidate_dcache((uint32_t *)(uintptr_t)base_addr[i], base_addr_size[i]);
        }
    }
    else
    {
        ethosu_invalidate_dcache(NULL, 0);
    }

    (void)ethosu_get_qread(&drv->dev, &qread);
    if (qread != cms_bytes)
    {
        LOG_WARN(
            "Failure: IRQ received but qread (%" PRIu32 ") not at end of stream (%" PRIu32 ").\n", qread, cms_bytes);
        return -1;
    }

    return 0;
}

static int read_apb_reg(struct ethosu_driver *drv, uint16_t da_data)
{
    uint32_t *reg_p;
    uint32_t start_address = (uint32_t)(da_data & APB_START_ADDR_MASK);
    uint16_t num_reg       = (da_data >> APB_NUM_REG_BIT_SHIFT) + 1;

    reg_p = (uint32_t *)malloc(num_reg * sizeof(uint32_t));
    if (reg_p == NULL)
    {
        LOG_INFO("read_apb_reg, Error! memory not allocated.");
        return -1;
    }

    if (ETHOSU_SUCCESS == ethosu_read_apb_reg(&drv->dev, start_address, num_reg, reg_p))
    {
        for (int i = 0; i < num_reg; i++)
        {
            LOG_INFO(
                "NPU_REG ADDR 0x%04" PRIu32 " = 0x%08" PRIu32 "\n", (start_address + (i * BYTES_IN_32_BITS)), reg_p[i]);
        }
    }
    else
    {
        free(reg_p);
        return -1;
    }

    free(reg_p);
    return 0;
}

static int dump_shram(struct ethosu_driver *drv)
{
    struct ethosu_config cfg;
    uint32_t *shram_p;
    (void)ethosu_get_config(&drv->dev, &cfg);

    LOG_INFO("dump_shram size = %" PRIu32 " KB\n", cfg.shram_size);

    shram_p = (uint32_t *)malloc(BYTES_1KB);
    if (shram_p == NULL)
    {
        LOG_ERR("read_shram, Error! memory not allocated.");
        return -1;
    }

    for (uint32_t i = 0; i < cfg.shram_size; i++)
    {
        ethosu_get_shram_data(&drv->dev, i, (uint32_t *)shram_p);
        // Output 1KB of SHRAM
        LOG_INFO("***SHRAM SECTION %" PRIu32 "***\n", i);
        for (int j = 0; j < (BYTES_1KB / BYTES_IN_32_BITS); j++)
        {
            LOG_INFO("[0x%04" PRIx32 "] %" PRIx32 "\n", (i * 1024 + j * 4), shram_p[j]);
        }
    }
    free(shram_p);

    return 0;
}

typedef struct
{
    int number;
    const char *name;
} name_lookup_t;

static const name_lookup_t npu_reg_name_tbl[] = {
    {0x200, "KERNEL_X"},
    {0x204, "KERNEL_Y"},
    {0x208, "KERNEL_W_M1"},
    {0x20C, "KERNEL_H_M1"},
    {0x210, "OFM_CBLK_WIDTH_M1"},
    {0x214, "OFM_CBLK_HEIGHT_M1"},
    {0x218, "OFM_CBLK_DEPTH_M1"},
    {0x21c, "IFM_CBLK_DEPTH_M1"},
    {0x220, "OFM_X"},
    {0x224, "OFM_Y"},
    {0x228, "OFM_Z"},
    {0x22C, "IFM_Z"},
    {0x230, "PAD_TOP"},
    {0x234, "PAD_LEFT"},
    {0x238, "IFM_CBLK_WIDTH"},
    {0x23C, "IFM_CBLK_HEIGHT"},
    {0x240, "DMA_IFM_SRC"},
    {0x244, "DMA_IFM_SRC_HI"},
    {0x248, "DMA_IFM_DST"},
    {0x24c, "DMA_OFM_SRC"},
    {0x250, "DMA_OFM_DST"},
    {0x254, "DMA_OFM_DST_HI"},
    {0x258, "DMA_WEIGHT_SRC"},
    {0x25c, "DMA_WEIGHT_SRC_HI"},
    {0x260, "DMA_CMD_SRC"},
    {0x264, "DMA_CMD_SRC_HI"},
    {0x268, "DMA_CMD_SIZE"},
    {0x26c, "DMA_M2M_SRC"},
    {0x270, "DMA_M2M_SRC_HI"},
    {0x274, "DMA_M2M_DST"},
    {0x278, "DMA_M2M_DST_HI"},
    {0x27c, "CURRENT_QREAD"},
    {0x280, "DMA_SCALE_SRC"},
    {0x284, "DMA_SCALE_SRC_HI"},
    {0x2BC, "CURRENT_CMD"},
    {0x800, "IFM_PAD_TOP"},
    {0x804, "IFM_PAD_LEFT"},
    {0x808, "IFM_PAD_RIGHT"},
    {0x80C, "IFM_PAD_BOTTOM"},
    {0x810, "IFM_DEPTH_M1"},
    {0x814, "IFM_PRECISION"},
    {0x81C, "IFM_UPSCALE"},
    {0x824, "IFM_ZERO_POINT"},
    {0x828, "IFM_WIDTH0_M1"},
    {0x82C, "IFM_HEIGHT0_M1"},
    {0x830, "IFM_HEIGHT1_M1"},
    {0x834, "IFM_IB_END"},
    {0x83C, "IFM_REGION"},
    {0x844, "OFM_WIDTH_M1"},
    {0x848, "OFM_HEIGHT_M1"},
    {0x84C, "OFM_DEPTH_M1"},
    {0x850, "OFM_PRECISION"},
    {0x854, "OFM_BLK_WIDTH_M1"},
    {0x858, "OFM_BLK_HEIGHT_M1"},
    {0x85C, "OFM_BLK_DEPTH_M1"},
    {0x860, "OFM_ZERO_POINT"},
    {0x868, "OFM_WIDTH0_M1"},
    {0x86C, "OFM_HEIGHT0_M1"},
    {0x870, "OFM_HEIGHT1_M1"},
    {0x87C, "OFM_REGION"},
    {0x880, "KERNEL_WIDTH_M1"},
    {0x884, "KERNEL_HEIGHT_M1"},
    {0x888, "KERNEL_STRIDE"},
    {0x88C, "PARALLEL_MODE"},
    {0x890, "ACC_FORMAT"},
    {0x894, "ACTIVATION"},
    {0x898, "ACTIVATION_MIN"},
    {0x89C, "ACTIVATION_MAX"},
    {0x8A0, "WEIGHT_REGION"},
    {0x8A4, "SCALE_REGION"},
    {0x8B4, "AB_START"},
    {0x8BC, "BLOCKDEP"},
    {0x8C0, "DMA0_SRC_REGION"},
    {0x8C4, "DMA0_DST_REGION"},
    {0x8C8, "DMA0_SIZE0"},
    {0x8CC, "DMA0_SIZE1"},
    {0x900, "IFM2_BROADCAST"},
    {0x904, "IFM2_SCALAR"},
    {0x924, "IFM2_ZERO_POINT"},
    {0x928, "IFM2_WIDTH0_M1"},
    {0x92C, "IFM2_HEIGHT0_M1"},
    {0x930, "IFM2_HEIGHT1_M1"},
    {0x934, "IFM2_IB_START"},
    {0x93C, "IFM2_REGION"},
    {0xA00, "IFM_BASE0"},
    {0xA04, "IFM_BASE0_HI"},
    {0xA08, "IFM_BASE1"},
    {0xA0C, "IFM_BASE1_HI"},
    {0xA10, "IFM_BASE2"},
    {0xA14, "IFM_BASE2_HI"},
    {0xA18, "IFM_BASE3"},
    {0xA1C, "IFM_BASE3_HI"},
    {0xA20, "IFM_STRIDE_X"},
    {0xA24, "IFM_STRIDE_X_HI"},
    {0xA28, "IFM_STRIDE_Y"},
    {0xA2C, "IFM_STRIDE_Y_HI"},
    {0xA30, "IFM_STRIDE_C"},
    {0xA34, "IFM_STRIDE_C_HI"},
    {0xA40, "OFM_BASE0"},
    {0xA44, "OFM_BASE0_HI"},
    {0xA48, "OFM_BASE1"},
    {0xA4C, "OFM_BASE1_HI"},
    {0xA50, "OFM_BASE2"},
    {0xA54, "OFM_BASE2_HI"},
    {0xA58, "OFM_BASE3"},
    {0xA5C, "OFM_BASE3_HI"},
    {0xA60, "OFM_STRIDE_X"},
    {0xA64, "OFM_STRIDE_X_HI"},
    {0xA68, "OFM_STRIDE_Y"},
    {0xA6C, "OFM_STRIDE_Y_HI"},
    {0xA70, "OFM_STRIDE_C"},
    {0xA74, "OFM_STRIDE_C_HI"},
    {0xA80, "WEIGHT_BASE"},
    {0xA84, "WEIGHT_BASE_HI"},
    {0xA88, "WEIGHT_LENGTH"},
    {0xA8C, "WEIGHT_LENGTH_HI"},
    {0xA90, "SCALE_BASE"},
    {0xA94, "SCALE_BASE_HI"},
    {0xA98, "SCALE_LENGTH"},
    {0xAA0, "OFM_SCALE"},
    {0xAA4, "OFM_SCALE_SHIFT"},
    {0xAA8, "OPA_SCALE "},
    {0xAB0, "OPB_SCALE"},
    {0xAC0, "DMA0_SRC"},
    {0xAC4, "DMA0_SRC_HI"},
    {0xAC8, "DMA0_DST"},
    {0xACC, "DMA0_DST_HI"},
    {0xAD0, "DMA0_LEN"},
    {0xAD4, "DMA0_LEN_HI"},
    {0xAD8, "DMA0_SKIP0"},
    {0xADC, "DMA0_SKIP0_HI"},
    {0xAE0, "DMA0_SKIP1"},
    {0xAE4, "DMA0_SKIP1_HI"},
    {0xB00, "IFM2_BASE0"},
    {0xB04, "IFM2_BASE0_HI"},
    {0xB08, "IFM2_BASE1"},
    {0xB0C, "IFM2_BASE1_HI"},
    {0xB10, "IFM2_BASE2"},
    {0xB14, "IFM2_BASE2_HI"},
    {0xB18, "IFM2_BASE3"},
    {0xB1C, "IFM2_BASE3_HI"},
    {0xB20, "IFM2_STRIDE_X"},
    {0xB24, "IFM2_STRIDE_X_HI"},
    {0xB28, "IFM2_STRIDE_Y"},
    {0xB2C, "IFM2_STRIDE_Y_HI"},
    {0xB30, "IFM2_STRIDE_C"},
    {0xB34, "IFM2_STRIDE_C_HI"},
    {0xB40, "WEIGHT1_BASE"},
    {0xB44, "WEIGHT1_BASE_HI"},
    {0xB48, "WEIGHT1_LENGTH"},
    {0xB4C, "WEIGHT1_LENGTH_HI"},
    {0xB50, "SCALE1_BASE"},
    {0xB54, "SCALE1_BASE_HI"},
    {0xB58, "SCALE1_LENGTH"},
};

static const char *lookup_name(const name_lookup_t *lookup_table, int lookup_table_count, int find)
{
    int n;
    for (n = 0; n < lookup_table_count; n++)
    {
        if (lookup_table[n].number == find)
        {
            return lookup_table[n].name;
        }
    }
    // Not found
    return 0;
}

static void dump_npu_register(struct ethosu_driver *drv, int npu_reg, int npu_reg_end)
{
    unsigned int reg_val;
    const char *reg_name;
    int npu_reg_name_tbl_count = sizeof(npu_reg_name_tbl) / sizeof(npu_reg_name_tbl[0]);

    LOG_INFO("dump_register %X - %X\n", npu_reg, npu_reg_end);
    for (; npu_reg <= npu_reg_end; npu_reg += sizeof(int))
    {
        reg_val  = ethosu_read_reg(&drv->dev, npu_reg);
        reg_name = lookup_name(npu_reg_name_tbl, npu_reg_name_tbl_count, npu_reg);
        LOG_INFO("[0x%.4X] 0x%.8X\t%s\n", npu_reg, reg_val, (reg_name) ? reg_name : "");
    }
}

static const name_lookup_t cmd0_name_tbl[] = {
    {0x000, "NPU_OP_STOP"},
    {0x001, "NPU_OP_IRQ"},
    {0x002, "NPU_OP_CONV"},
    {0x003, "NPU_OP_DEPTHWISE"},
    {0x004, "NPU_OP_VECTOR_PROD"},
    {0x005, "NPU_OP_POOL"},
    {0x006, "NPU_OP_ELEMENTWISE"},
    {0x010, "NPU_OP_DMA_START"},
    {0x011, "NPU_OP_DMA_WAIT"},
    {0x012, "NPU_OP_KERNEL_WAIT"},
    {0x100, "NPU_SET_IFM_PAD_TOP"},
    {0x101, "NPU_SET_IFM_PAD_LEFT"},
    {0x102, "NPU_SET_IFM_PAD_RIGHT"},
    {0x103, "NPU_SET_IFM_PAD_BOTTOM"},
    {0x104, "NPU_SET_IFM_DEPTH_M1"},
    {0x105, "NPU_SET_IFM_PRECISION"},
    {0x107, "NPU_SET_IFM_UPSCALE"},
    {0x109, "NPU_SET_IFM_ZERO_POINT"},
    {0x10A, "NPU_SET_IFM_WIDTH0_M1"},
    {0x10B, "NPU_SET_IFM_HEIGHT0_M1"},
    {0x10C, "NPU_SET_IFM_HEIGHT1_M1"},
    {0x10D, "NPU_SET_IFM_IB_END"},
    {0x10F, "NPU_SET_IFM_REGION"},
    {0x110, "NPU_SET_OFM_BATCH_SIZE_M1"},
    {0x111, "NPU_SET_OFM_WIDTH_M1"},
    {0x112, "NPU_SET_OFM_HEIGHT_M1"},
    {0x113, "NPU_SET_OFM_DEPTH_M1"},
    {0x114, "NPU_SET_OFM_PRECISION"},
    {0x115, "NPU_SET_OFM_BLK_WIDTH_M1"},
    {0x116, "NPU_SET_OFM_BLK_HEIGHT_M1"},
    {0x117, "NPU_SET_OFM_BLK_DEPTH_M1"},
    {0x118, "NPU_SET_OFM_ZERO_POINT"},
    {0x11A, "NPU_SET_OFM_WIDTH0_M1"},
    {0x11B, "NPU_SET_OFM_HEIGHT0_M1"},
    {0x11C, "NPU_SET_OFM_HEIGHT1_M1"},
    {0x11F, "NPU_SET_OFM_REGION"},
    {0x120, "NPU_SET_KERNEL_WIDTH_M1"},
    {0x121, "NPU_SET_KERNEL_HEIGHT_M1"},
    {0x122, "NPU_SET_KERNEL_STRIDE"},
    {0x124, "NPU_SET_ACC_FORMAT"},
    {0x125, "NPU_SET_ACTIVATION"},
    {0x126, "NPU_SET_ACTIVATION_MIN"},
    {0x127, "NPU_SET_ACTIVATION_MAX"},
    {0x128, "NPU_SET_WEIGHT_REGION"},
    {0x129, "NPU_SET_SCALE_REGION"},
    {0x12D, "NPU_SET_AB_START"},
    {0x12F, "NPU_SET_BLOCKDEP"},
    {0x130, "NPU_SET_DMA0_SRC_REGION"},
    {0x131, "NPU_SET_DMA0_DST_REGION"},
    {0x180, "NPU_SET_IFM2_BROADCAST"},
    {0x181, "NPU_SET_IFM2_SCALAR"},
    {0x185, "NPU_SET_IFM2_PRECISION"},
    {0x189, "NPU_SET_IFM2_ZERO_POINT"},
    {0x18A, "NPU_SET_IFM2_WIDTH0_M1"},
    {0x18B, "NPU_SET_IFM2_HEIGHT0_M1"},
    {0x18C, "NPU_SET_IFM2_HEIGHT1_M1"},
    {0x18D, "NPU_SET_IFM2_IB_START"},
    {0x18F, "NPU_SET_IFM2_REGION"},
};

static const name_lookup_t cmd1_name_tbl[] = {
    {0x000, "NPU_SET_IFM_BASE0"},     {0x001, "NPU_SET_IFM_BASE1"},     {0x002, "NPU_SET_IFM_BASE2"},
    {0x003, "NPU_SET_IFM_BASE3"},     {0x004, "NPU_SET_IFM_STRIDE_X"},  {0x005, "NPU_SET_IFM_STRIDE_Y"},
    {0x006, "NPU_SET_IFM_STRIDE_C"},  {0x007, "NPU_SET_IFM_STRIDE_N"},  {0x010, "NPU_SET_OFM_BASE0"},
    {0x011, "NPU_SET_OFM_BASE1"},     {0x012, "NPU_SET_OFM_BASE2"},     {0x013, "NPU_SET_OFM_BASE3"},
    {0x014, "NPU_SET_OFM_STRIDE_X"},  {0x015, "NPU_SET_OFM_STRIDE_Y"},  {0x016, "NPU_SET_OFM_STRIDE_C"},
    {0x017, "NPU_SET_OFM_STRIDE_N"},  {0x020, "NPU_SET_WEIGHT_BASE"},   {0x021, "NPU_SET_WEIGHT_LENGTH"},
    {0x022, "NPU_SET_SCALE_BASE"},    {0x023, "NPU_SET_SCALE_LENGTH"},  {0x024, "NPU_SET_OFM_SCALE"},
    {0x025, "NPU_SET_OPA_SCALE"},     {0x026, "NPU_SET_OPB_SCALE"},     {0x030, "NPU_SET_DMA0_SRC"},
    {0x031, "NPU_SET_DMA0_DST"},      {0x032, "NPU_SET_DMA0_LEN"},      {0x080, "NPU_SET_IFM2_BASE0"},
    {0x081, "NPU_SET_IFM2_BASE1"},    {0x082, "NPU_SET_IFM2_BASE2"},    {0x083, "NPU_SET_IFM2_BASE3"},
    {0x084, "NPU_SET_IFM2_STRIDE_X"}, {0x085, "NPU_SET_IFM2_STRIDE_Y"}, {0x086, "NPU_SET_IFM2_STRIDE_C"},
};

static void dump_command_stream(const uint32_t *cmd_stream, const int cms_length, int qread)
{
    int n;
    int offset;
    uint32_t cmd_val;
    const uint8_t *cmd_ptr;
    const char *cmd_name;
    int cmd0_name_tbl_count = sizeof(cmd0_name_tbl) / sizeof(cmd0_name_tbl[0]);
    int cmd1_name_tbl_count = sizeof(cmd1_name_tbl) / sizeof(cmd1_name_tbl[0]);

    LOG_INFO("dump_command_stream cmd_stream = 0x%8p cms_length = %d\n", cmd_stream, cms_length);
    for (n = 0; n < cms_length; n++)
    {
        // Offset
        offset = n * sizeof(int);
        LOG_INFO("[%.4d] ", offset);
        // Command
        cmd_ptr = (const uint8_t *)&cmd_stream[n];
        LOG_INFO("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", cmd_ptr[0], cmd_ptr[1], cmd_ptr[2], cmd_ptr[3]);
        // Command name and payload
        if (cmd_stream[n] & 0x4000)
        {
            cmd_name = lookup_name(cmd1_name_tbl, cmd1_name_tbl_count, cmd_stream[n] & 0x3FF);
            n++;
            cmd_val = cmd_stream[n];
            cmd_ptr = (const uint8_t *)&cmd_stream[n];
            LOG_INFO("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", cmd_ptr[0], cmd_ptr[1], cmd_ptr[2], cmd_ptr[3]);
        }
        else
        {
            cmd_val  = cmd_stream[n] >> 16;
            cmd_name = lookup_name(cmd0_name_tbl, cmd0_name_tbl_count, cmd_stream[n] & 0x3FF);
        }
        if (cmd_name)
        {
            LOG_INFO("\t%s 0x%.8" PRIX32, cmd_name, cmd_val);
        }
        if (offset == qread)
        {
            LOG_INFO(" <<== QREAD\n");
        }
        else
        {
            LOG_INFO("\n");
        }
    }
}

#endif //EI ETHOS
