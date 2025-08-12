/*
 * Copyright (c) 2023, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "hal_pinmux.h"
#include "section.h"
#include "rtl876x_pinmux.h"
#include "os_mem.h"
#include "io_dlps.h"
#include "slist.h"
#include "os_sync.h"

typedef struct HAL_PAD_WAKE_CB_PKT_NODE_
{
    uint8_t                       pin_index;
    P_PIN_WAKEUP_CALLBACK         wake_up_callback;
    uint32_t                      context;
    SIMPLEQ_ENTRY(struct HAL_PAD_WAKE_CB_PKT_NODE_) q_next;

} HAL_PAD_WAKE_CB_PKT_NODE;

SIMPLEQ_HEAD(HAL_PAD_WAKE_PKT_QUEUE, HAL_PAD_WAKE_CB_PKT_NODE);

HAL_PAD_WAKE_PKT_QUEUE  hal_pad_wake_cb_queue = SIMPLEQ_HEAD_INITIALIZER(hal_pad_wake_cb_queue);

void hal_pad_wake_up_enable(uint8_t pin_index, T_HAL_WAKE_UP_POLARITY polarity)
{
    PAD_WAKEUP_POL_VAL pad_wake_polarity = polarity == HAL_WAKE_UP_POLARITY_HIGH ? PAD_WAKEUP_POL_HIGH :
                                           PAD_WAKEUP_POL_LOW;
    Pad_WakeupPolarityValue(pin_index, pad_wake_polarity);
    Pad_WakeupEnableValue(pin_index, 1);
}

void hal_wake_up_pad_wake_up_disable(uint8_t pin_index)
{
    Pad_WakeupEnableValue(pin_index, 0);
}

HAL_PAD_WAKE_CB_PKT_NODE *hal_pad_wake_up_new(void)
{
    return (HAL_PAD_WAKE_CB_PKT_NODE *)os_mem_zalloc(RAM_TYPE_DATA_ON,
                                                     sizeof(HAL_PAD_WAKE_CB_PKT_NODE));
}

void hal_pad_wake_up_free(HAL_PAD_WAKE_CB_PKT_NODE *node)
{
    os_mem_free(node);
}

void hal_pad_wake_up_enqueue(HAL_PAD_WAKE_CB_PKT_NODE *node)
{
    uint32_t s = os_lock();
    SIMPLEQ_INSERT_TAIL(&hal_pad_wake_cb_queue, node, q_next);
    os_unlock(s);
}

void hal_pad_wake_up_dequeue(HAL_PAD_WAKE_CB_PKT_NODE *node)
{
    uint32_t s = os_lock();
    SIMPLEQ_REMOVE(&hal_pad_wake_cb_queue, node, HAL_PAD_WAKE_CB_PKT_NODE, q_next);
    os_unlock(s);
}

static void hal_wake_up_pad_cb_in_q(uint8_t pin_index, P_PIN_WAKEUP_CALLBACK p_callback,
                                    uint32_t context)
{
    HAL_PAD_WAKE_CB_PKT_NODE *p_wake_up_item = hal_pad_wake_up_new();

    if (p_wake_up_item)
    {
        p_wake_up_item->pin_index = pin_index;
        p_wake_up_item->wake_up_callback = p_callback;
        p_wake_up_item->context = context;
        hal_pad_wake_up_enqueue(p_wake_up_item);
    }
}

void hal_pad_wake_dlps_exit_callback(void)
{
    if (NVIC_GetPendingIRQ(System_IRQn))
    {
        HAL_PAD_WAKE_CB_PKT_NODE *cur;
        SIMPLEQ_FOREACH(cur, &hal_pad_wake_cb_queue, q_next)
        {
            if (System_WakeUpInterruptValue(cur->pin_index) == SET)
            {
                cur->wake_up_callback(cur->context);
            }
        }
    }
}

void hal_pad_register_pin_wake_callback(uint8_t pin_index, P_PIN_WAKEUP_CALLBACK callback,
                                        uint32_t context)
{
    if (pin_index >= TOTAL_PIN_NUM || !callback)
    {
        return;
    }
    static bool is_wake_up_unregister = true;
    if (is_wake_up_unregister)
    {
        io_dlps_register_exit_cb(hal_pad_wake_dlps_exit_callback);
        is_wake_up_unregister = false;
    }
    hal_wake_up_pad_cb_in_q(pin_index, callback, context);
}

void hal_wake_up_unregister_pad_wake_callback(uint8_t pin_index)
{
    HAL_PAD_WAKE_CB_PKT_NODE *cur = hal_pad_wake_cb_queue.sqh_first;
    SIMPLEQ_FOREACH(cur, &hal_pad_wake_cb_queue, q_next)
    {
        if (cur->pin_index == pin_index)
        {
            hal_pad_wake_up_dequeue(cur);
            hal_pad_wake_up_free(cur);
            break;
        }
    }
}
