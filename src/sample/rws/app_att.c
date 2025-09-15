
#include "trace.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "bt_att.h"
#include "gap_chann.h"
#include "app_att.h"
#include "app_link_util.h"
#include "app_bt_policy_api.h"
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif

static const uint8_t att_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x25,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_GATT >> 8),
    (uint8_t)(UUID_GATT),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0d,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_ATT >> 8),
    (uint8_t)(PSM_ATT),
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ATT >> 8),
    (uint8_t)(UUID_ATT),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),
};

void app_att_cb(uint8_t *bd_addr, T_BT_ATT_MSG_TYPE msg_type, void *p_msg)
{
    T_APP_BR_LINK *p_link;

    APP_PRINT_TRACE2("app_bt_att_cb: msg_type %d, bd_addr %s", msg_type, TRACE_BDADDR(bd_addr));

    switch (msg_type)
    {
    case BT_ATT_MSG_CONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                T_BT_ATT_CONN_CMPL *param = (T_BT_ATT_CONN_CMPL *)p_msg;

                APP_PRINT_INFO3("BT_ATT_MSG_CONN_CMPL: conn_handle 0x%x, cid 0x%x, mtu_size 0x%x",
                                param->conn_handle, param->cid, param->mtu_size);

                app_bt_policy_msg_prof_conn(p_link->bd_addr, GATT_PROFILE_MASK);
                p_link->mtu_size = param->mtu_size;

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
                gatt_client_start_discovery_all(param->conn_handle, NULL);
#endif
            }
        }
        break;

    case BT_ATT_MSG_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_disconn(p_link->bd_addr, GATT_PROFILE_MASK, 0);
            }
        }
        break;

    default:
        break;
    }
}

void app_att_init(void)
{
    bt_sdp_record_add((void *)att_sdp_record);
    bt_att_init(app_att_cb);
}
