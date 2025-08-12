#ifndef _VHCI_H_
#define _VHCI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup VHCI VHCI interface
 *
 * VHCI interface between upper stack and lower stack.
 * @{
 */

typedef enum VHCI_PKT_TYPE_
{
    VHCI_CMD_PKT = 1,
    VHCI_ACL_PKT = 2,
    VHCI_SCO_PKT = 3,
    VHCI_EVT_PKT = 4,
    VHCI_ISO_PKT = 5
} VHCI_PKT_TYPE;

/**
 * @brief VHCI filter function type
 * @param type  Packet type (refer to #VHCI_PKT_TYPE)
 * @param buf  Packet which conforms with Bluetooth standard HCI packet format
 * @return  \c TRUE if the packet can be sent to upper stack; \c FALSE if the
 * packet needs to be dropped.
 */
typedef bool (*VHCI_FILTER_CALLBACK)(VHCI_PKT_TYPE type, void *buf);

/**
 * @brief Set VHCI filter
 *
 * \p filter will be applied on every packet sent from lower stack to upper
 * stack.\n
 * If you want to clear the filter, set \p filter to \c NULL.\n
 * If no filter is set, all packets are sent from lower stack to upper stack
 * unconditionally.
 * @param filter  Filter function (refer to #VHCI_FILTER_CALLBACK)
 */
void vhci_set_filter(VHCI_FILTER_CALLBACK filter);

#ifdef __cplusplus
}
#endif

#endif /* _VHCI_H_ */
