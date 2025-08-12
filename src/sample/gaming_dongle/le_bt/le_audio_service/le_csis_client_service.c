#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "le_audio_service.h"
#include "aes_api.h"
#include "le_bst_src_service.h"
#include "app_usb_layer.h"
#include "le_unicast_src_service.h"
#include "dev_mgr.h"
#include "le_csis_client_service.h"
#include "ual_adapter.h"
#include "cap.h"
#include "le_audio_service.h"
#if(LE_EXT_FTL_SUPPORT == 1)
#include "app_le_ext_ftl.h"
#endif
#include "mics_def.h"

typedef struct
{
    T_UALIST_HEAD         csis_list;
    int                   client_id;
    T_BT_BOND_STATE       bond_state;
    uint8_t               discv_hash[3];
} T_LE_CSIS_DB;

static T_LE_CSIS_DB le_csis_mgr;
static const uint8_t const_prand[3] = {0x53, 0x52, 0x55};

static void csis_client_service_scan_callback(uint8_t cb_type, void *result)
{
    T_LE_ADV_INFO *p_report = NULL;

    if (!result)
    {
        return;
    }

    if (cb_type != SCAN_RESULT_LE)
    {
        return;
    }

    p_report = (T_LE_ADV_INFO *)result;

    if (p_report->rssi < app_cfg_const.dongle_autopair_rssi)
    {
        return;
    }
    set_coordinator_check_adv_rsi(p_report->data_len, p_report->p_data, p_report->bd_addr,
                                  p_report->bd_type);
}

static T_LE_AUDIO_CSIS *find_csis_group_by_hash(uint8_t hash[3])
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_LE_AUDIO_CSIS *p_csis = NULL;

    if (ualist_empty(&le_csis_mgr.csis_list))
    {
        return NULL;
    }

    ualist_for_each_safe(pos, n, &le_csis_mgr.csis_list)
    {
        p_csis = ualist_entry(pos, struct t_le_audio_csis, list);
        if (!memcmp(p_csis->hash, hash, 3))
        {
            return p_csis;
        }
    }
    return NULL;
}


static T_LE_AUDIO_CSIS *find_or_alloc_csis_group_by_hash(uint8_t hash[3])
{
    T_LE_AUDIO_CSIS *p_csis = find_csis_group_by_hash(hash);

    if (p_csis != NULL)
    {
        APP_PRINT_INFO1("find_or_alloc_csis_group_by_hash: coor set %p already exists",
                        p_csis);
        return p_csis;
    }

    p_csis = calloc(1, sizeof(T_LE_AUDIO_CSIS));
    if (p_csis == NULL)
    {
        return NULL;
    }
    init_ualist_head(&p_csis->list);
    memcpy(p_csis->hash, hash, 3);
    ualist_add_tail(&p_csis->list, &le_csis_mgr.csis_list);

    APP_PRINT_INFO1("find_or_alloc_csis_group_by_hash: coor set %p", p_csis);

    return p_csis;
}

void le_csis_client_remove_all_groups(void)
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_LE_AUDIO_CSIS *p_csis = NULL;

    APP_PRINT_INFO0("le_csis_client_remove_all_groups");
    if (ualist_empty(&le_csis_mgr.csis_list))
    {
        return;
    }

    ualist_for_each_safe(pos, n, &le_csis_mgr.csis_list)
    {
        p_csis = ualist_entry(pos, struct t_le_audio_csis, list);
        ble_audio_group_release(&(p_csis->group_handle));
        ualist_del(&p_csis->list);
        free(p_csis);
    }
    le_csis_client_stop_scan();
}

static void remove_coor_set_by_hash(uint8_t hash[3])
{
    T_UALIST_HEAD *pos = NULL;
    T_UALIST_HEAD *n = NULL;
    T_LE_AUDIO_CSIS *p_csis = NULL;

    if (ualist_empty(&le_csis_mgr.csis_list))
    {
        return;
    }
    ualist_for_each_safe(pos, n, &le_csis_mgr.csis_list)
    {
        p_csis = ualist_entry(pos, struct t_le_audio_csis, list);
        if (!memcmp(p_csis->hash, hash, 3))
        {
            APP_PRINT_INFO1("remove_coor_set_by_hash: Remove coor set %p",
                            p_csis);
            ble_audio_group_release(&(p_csis->group_handle));
            ualist_del(&p_csis->list);
            free(p_csis);
            return;
        }
    }
}

/* We received the ext adv containing RSI. */
void le_csis_client_handle_mem_rsi(uint8_t *rsi, uint8_t bd_type, uint8_t *bd_addr)
{
    T_CSIS_GROUP_INFO csis_info;
    T_SET_MEM_AVAIL report;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    T_BT_DEVICE *p_dev_rec = NULL;
    T_BLE_AUDIO_GROUP_HANDLE group_handle = set_coordinator_find_by_rsi(rsi);


    /* If the group handle is not null, it means the RSI can be resolved by the
     * SIRK that is associated with the group handle.
     * It means that the device belongs to the group that is found.
     * */
    if (!group_handle)
    {
        APP_PRINT_ERROR3("le_csis_client_handle_mem_rsi: %b %d rsi %b, group handle NULL",
                         TRACE_BDADDR(bd_addr), bd_type, TRACE_BINARY(6, rsi));
        return;
    }

    dev_handle = ble_audio_group_find_dev(group_handle, bd_addr,
                                          bd_type);

    APP_PRINT_INFO1("le_csis_client_handle_mem_rsi: dev_handle %08x",
                    dev_handle);

    /* The device was connected before. */
    if (dev_handle)
    {
        return;
    }

    /* FIXME: Here are two scenarios.
        * A third device occurs with the same SIRK. Or the second device is
        * found.
        * How to distinguish the two cases ?
        * */

    if (!set_coordinator_get_group_info(group_handle, &csis_info))
    {
        APP_PRINT_ERROR0("le_csis_client_handle_mem_rsi can't find csis info");
        return;
    }

    le_csis_client_gen_local_hash(report.hash, csis_info.sirk);
    p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec)
    {
        memcpy(report.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        report.bd_type = p_dev_rec->bd_type;
    }
    else
    {
        memcpy(report.bd_addr, bd_addr, BD_ADDR_LEN);
        report.bd_type = bd_type;
    }

    report.srv_uuid = csis_info.srv_uuid;
    report.rank = 0;
    report.size = csis_info.set_mem_size;
    set_mem_avail_report_callback(&report);
    le_audio_update_unicast_addr(report.bd_addr, report.bd_type);
}

void le_csis_client_gen_local_hash(uint8_t hash[3], uint8_t *p_sirk)
{
    uint8_t rand[16] = {0};
    uint8_t out[16] = {0};

    memset(rand, 0, 16);
    rand[0] = const_prand[0];
    rand[1] = const_prand[1];
    rand[2] = const_prand[2];

    aes128_ecb_encrypt_msb2lsb(rand, p_sirk, out);
    memcpy(hash, out, 3);
}

void le_csis_client_add_group_mem(T_CSIS_CLIENT_SET_MEM_FOUND *set_report)
{
    T_LE_AUDIO_CSIS *p_csis;
    T_SET_MEM_AVAIL report;
    uint8_t hash[3];

    APP_PRINT_INFO1("le_csis_client_add_group_mem: bd_addr %b", TRACE_BDADDR(set_report->bd_addr));

    le_csis_client_gen_local_hash(hash, set_report->sirk);

    p_csis = find_or_alloc_csis_group_by_hash(hash);
    if (p_csis != NULL)
    {
        T_BT_DEVICE *p_dev_rec = ual_find_alloc_device_by_addr(set_report->bd_addr, set_report->addr_type);
        if (!p_dev_rec)
        {
            APP_PRINT_ERROR0("le_csis_client_add_group_mem: alloc fail");
            return;
        }

        memcpy(report.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        report.bd_type = p_dev_rec->bd_type;

        if (p_csis->group_handle == NULL)
        {
            p_csis->group_handle = set_report->group_handle;
        }

        memcpy(report.hash, hash, 3);
        p_csis->srv_uuid = set_report->srv_uuid;
        p_csis->size = set_report->set_mem_size;
        report.srv_uuid = set_report->srv_uuid;
        report.rank = set_report->rank;
        report.size = set_report->set_mem_size;
        set_mem_avail_report_callback(&report);

        APP_PRINT_INFO4("le_csis_client_add_group_mem: bd_addr %b, group handle %p, size %x rank %x",
                        TRACE_BDADDR(report.bd_addr), set_report->group_handle,
                        report.size, report.rank);
#if (LE_EXT_FTL_SUPPORT == 1)
        T_LEA_FTL_DEV ftl_dev;
        memset(&ftl_dev, 0, sizeof(T_LEA_FTL_DEV));
        memcpy(ftl_dev.bd_addr, report.bd_addr, 6);
        ftl_dev.bd_type = report.bd_type;
        ftl_dev.csis_support = true;
        ftl_dev.csis_size = report.size;
        memcpy(ftl_dev.csis_hash, hash, 3);
        lea_ext_save_le_dev(&ftl_dev);
#endif
    }
}

void le_csis_client_remove_group(T_BLE_AUDIO_GROUP_HANDLE group_handle)
{
    T_CSIS_GROUP_INFO csis_info;
    uint8_t hash[3];

    if (set_coordinator_get_group_info(group_handle, &csis_info))
    {
        le_csis_client_gen_local_hash(hash, csis_info.sirk);
        if (memcmp(hash, le_csis_mgr.discv_hash, 3) == 0)
        {
            APP_PRINT_WARN0("le_csis_client_remove_group coordinator removed while scanning");
            le_csis_client_stop_scan();
        }
        remove_coor_set_by_hash(hash);
    }

}

int le_csis_client_start_scan(uint8_t hash[3])
{
    APP_PRINT_TRACE3("le_csis_client_start_scan 0x%x:0x%x:0x%x", hash[0], hash[1], hash[2]);

    if (le_csis_mgr.client_id > 0)
    {
        APP_PRINT_ERROR0("le_csis_client_start_scan aleady start discover");
        return -1;
    }
    T_LE_AUDIO_CSIS *p_csis =  find_csis_group_by_hash(hash);
    if (p_csis == NULL || p_csis->group_handle == NULL)
    {
        APP_PRINT_ERROR0("le_csis_client_start_scan can't find coordinate");
        return -1;
    }

    if (p_csis->group_handle != NULL)
    {
        uint8_t dev_num = ble_audio_group_get_used_dev_num(p_csis->group_handle, true);
        if (p_csis->size != 0 && p_csis->size == dev_num)
        {
            APP_PRINT_ERROR1("le_csis_client_start_scan already bonded all devices %d", dev_num);
            return -1;
        }
        set_coordinator_cfg_discover(p_csis->group_handle, true, 10000);
    }
    else
    {
        APP_PRINT_ERROR0("le_csis_client_start_scan can't find group");
        return -1;
    }

    le_csis_mgr.client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                                    csis_client_service_scan_callback);
    if (le_csis_mgr.client_id > 0)
    {
        memcpy(le_csis_mgr.discv_hash, hash, 3);
        return le_csis_mgr.client_id;
    }
    else
    {
        set_coordinator_cfg_discover(NULL, false, 0);
        le_csis_mgr.client_id = 0;
        return -1;
    }
}

void le_csis_client_stop_scan(void)
{
    APP_PRINT_TRACE1("le_csis_client_stop_scan client id 0x%x", le_csis_mgr.client_id);
    if (le_csis_mgr.client_id > 0)
    {
        bt_adap_stop_discovery(le_csis_mgr.client_id);
    }
    memset(le_csis_mgr.discv_hash, 0, 3);
    set_coordinator_cfg_discover(NULL, false, 0);
    le_csis_mgr.client_id = 0;
}


bool le_csis_client_connect_group_all_mems(uint8_t hash[3])
{
    T_LE_AUDIO_CSIS *p_csis =  find_or_alloc_csis_group_by_hash(hash);
    if (p_csis == NULL || p_csis->group_handle == NULL)
    {
        return false;
    }

    //FIX TODO not call connect_all_set_members
    uint8_t dev_num = ble_audio_group_get_dev_num(p_csis->group_handle);
    if (dev_num == 0)
    {
        APP_PRINT_ERROR0("le_csis_client_connect_group_all_mems: No devs in group");
        return false;
    }

    T_AUDIO_DEV_INFO *p_dev_tbl = calloc(1, dev_num * sizeof(T_AUDIO_DEV_INFO));

    if (!p_dev_tbl)
    {
        APP_PRINT_ERROR0("le_csis_client_connect_group_all_mems: alloc failed");
        return false;
    }

    if (!ble_audio_group_get_info(p_csis->group_handle, &dev_num, p_dev_tbl))
    {
        APP_PRINT_ERROR0("le_csis_client_connect_group_all_mems: get group info err");
        free(p_dev_tbl);
        return false;
    }

    //FIX TODO may be the device num is larger than white list size
    for (uint8_t i = 0; i < dev_num; i++)
    {
        APP_PRINT_INFO3("le_csis_client_connect_group_all_mems: i %u, bd_addr %b, conn_state %d",
                        i, TRACE_BDADDR(p_dev_tbl[i].bd_addr), p_dev_tbl[i].conn_state);
        if (p_dev_tbl[i].is_used && p_dev_tbl[i].conn_state == GAP_CONN_STATE_DISCONNECTED)
        {
            bt_dev_create_bond(p_dev_tbl[i].bd_addr, p_dev_tbl[i].addr_type);
        }
    }

    free(p_dev_tbl);
    return true;
}


void le_csis_client_report_mem_info(uint8_t *bd_addr, uint8_t bd_type)
{
    //FIX TODO for now we only care about csis for cap and the address may be mismatch because of RPA
    T_BLE_AUDIO_GROUP_HANDLE group_handle;
    T_BLE_AUDIO_DEV_HANDLE dev_handle;
    T_CSIS_SET_MEM_INFO set_report;
    group_handle = set_coordinator_find_by_addr(bd_addr, bd_type, GATT_UUID_CAS, &dev_handle);
    if (group_handle != NULL)
    {
        if (set_coordinator_get_mem_info(group_handle, dev_handle, &set_report))
        {
            T_SET_MEM_AVAIL report;
            uint8_t hash[3];
            le_csis_client_gen_local_hash(hash, set_report.sirk);
            report.bd_type = set_report.addr_type;
            memcpy(report.bd_addr, bd_addr, BD_ADDR_LEN);
            memcpy(report.hash, hash, 3);
            report.srv_uuid = set_report.srv_uuid;
            report.rank = set_report.rank;
            report.size = set_report.set_mem_size;
            set_mem_avail_report_callback(&report);
        }
    }
}

static void le_csis_client_handle_read_result(T_CSIS_CLIENT_READ_RESULT *read_result)
{
    if (!read_result)
    {
        return;
    }

    T_CSIS_CLIENT_SET_MEM_FOUND mem_found;
    T_LE_AUDIO *p_link = NULL;
    T_BT_DEVICE *p_dev = NULL;

    if ((read_result->cause != GAP_SUCCESS) ||
        (read_result->mem_info.srv_uuid != GATT_UUID_CAS))
    {
        APP_PRINT_WARN2("le_csis_client_handle_read_result: cause %x uuid %x",
                        read_result->cause, read_result->mem_info.srv_uuid);
        return;
    }

    if (read_result->group_handle == NULL)
    {
        if (set_coordinator_add_group(&read_result->group_handle, le_audio_unicast_src_group_cb,
                                      &read_result->dev_handle, &read_result->mem_info) == false)
        {
            return;
        }
    }

    if (read_result->dev_handle == NULL)
    {
        if (set_coordinator_add_dev(read_result->group_handle,
                                    &read_result->dev_handle, &read_result->mem_info) == false)
        {
            return;
        }
    }


    memset(&mem_found, 0, sizeof(T_CSIS_CLIENT_SET_MEM_FOUND));
    memcpy(mem_found.bd_addr, read_result->mem_info.bd_addr, BD_ADDR_LEN);
    mem_found.addr_type = read_result->mem_info.addr_type;
    mem_found.srv_uuid = read_result->mem_info.srv_uuid;
    mem_found.group_handle = read_result->group_handle;
    if (read_result->mem_info.char_exit & CSIS_RANK_FLAG)
    {
        mem_found.rank = read_result->mem_info.rank;
    }
    if (read_result->mem_info.char_exit & CSIS_SIZE_FLAG)
    {
        mem_found.set_mem_size = read_result->mem_info.set_mem_size;
    }
    if (read_result->mem_info.char_exit & CSIS_SIRK_FLAG)
    {
        memcpy(mem_found.sirk, read_result->mem_info.sirk, CSIS_SIRK_LEN);
    }

    if (le_unicast_src_update_group_handle(&mem_found) == false)
    {
        return;
    }
    le_csis_client_add_group_mem(&mem_found);

    p_dev = ual_find_device_by_addr(mem_found.bd_addr);
    if (p_dev)
    {
        p_link = ble_audio_find_by_conn_handle(p_dev->le_conn_handle);
        if (p_link)
        {
            p_link->remote_serv_sup |= LE_AUDIO_CSIS_CLIENT_FLAG;
            p_link->csis_size = mem_found.set_mem_size;
            p_link->csis_ready_result = true;
#if DONGLE_LE_AUDIO
            if (p_link->cap_disc_done)
            {
                extern bool app_le_audio_sync_host_vol_to_remote(uint16_t conn_handle);
                app_le_audio_sync_host_vol_to_remote(p_dev->le_conn_handle);
            }
#endif
        }
    }
}

uint16_t le_csis_client_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    APP_PRINT_INFO1("le_csis_client_handle_msg: msg %x", msg);
    switch (msg)
    {
    case LE_AUDIO_MSG_CSIS_CLIENT_DIS_DONE:
        {
            T_CSIS_CLIENT_DIS_DONE *dis_done = buf;

            le_unicast_src_csis_disc_done(dis_done);
        }
        break;
    case LE_AUDIO_MSG_CSIS_CLIENT_READ_RESULT:
        {
            T_CSIS_CLIENT_READ_RESULT *read_result = (T_CSIS_CLIENT_READ_RESULT *)buf;
            le_csis_client_handle_read_result(read_result);
        }
        break;
    case LE_AUDIO_MSG_CSIS_CLIENT_SET_MEM_FOUND:
        {
            T_CSIS_CLIENT_SET_MEM_FOUND *set_report = (T_CSIS_CLIENT_SET_MEM_FOUND *)buf;

            if ((set_report == NULL) || (set_report->srv_uuid != GATT_UUID_CAS))
            {
                break;
            }

            APP_PRINT_INFO2("le_csis_client_handle_msg: csis mem bd_addr %b type %d find",
                            TRACE_BDADDR(set_report->bd_addr), set_report->addr_type);
            T_BT_DEVICE *p_dev_rec = NULL;
            uint8_t bd_addr[6];
            uint8_t bd_type;

            le_csis_client_add_group_mem(set_report);

            p_dev_rec = ual_find_device_by_addr(set_report->bd_addr);
            if (!p_dev_rec)
            {
                APP_PRINT_ERROR1("le_csis_client_handle_msg: cannot find bd_addr %b",
                                 TRACE_BDADDR(set_report->bd_addr));
                break;
            }

            memcpy(bd_addr, p_dev_rec->pseudo_addr, 6);
            bd_type = p_dev_rec->bd_type;
            le_audio_update_unicast_addr(bd_addr, bd_type);
            le_csis_client_stop_scan();
        }
        break;

    case LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_DONE:
        {
            set_coordinator_cfg_discover(NULL, false, 0);
            le_csis_client_stop_scan();
        }
        break;
    case LE_AUDIO_MSG_CSIS_CLIENT_SEARCH_TIMEOUT:
        {
            T_CSIS_CLIENT_SEARCH_TIMEOUT *p_search = (T_CSIS_CLIENT_SEARCH_TIMEOUT *)buf;
            T_CSIS_GROUP_INFO csis_info;
            uint8_t hash[3] = {0x0};
            if (set_coordinator_get_group_info(p_search->group_handle, &csis_info))
            {
                le_csis_client_gen_local_hash(hash, csis_info.sirk);
                le_csis_client_stop_scan();
                le_csis_client_start_scan(hash);
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}


void le_csis_client_handle_bond_state_change(T_BT_BOND_INFO *p_bond_info)
{
    uint8_t zero_hash[3] = {0};
    le_csis_mgr.bond_state = p_bond_info->state;
    if (p_bond_info->state != BT_BOND_STATE_BONDING &&
        memcmp(le_csis_mgr.discv_hash, zero_hash, 3))
    {
        if (le_csis_mgr.client_id == 0)
        {
            le_csis_mgr.client_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                                            csis_client_service_scan_callback);
            if (le_csis_mgr.client_id < 0)
            {
                APP_PRINT_ERROR0("le_csis_client_handle_bond_state_change bonded restart discover scan fail");
                memset(le_csis_mgr.discv_hash, 0, 3);
                set_coordinator_cfg_discover(NULL, false, 0);
                le_csis_mgr.client_id = 0;
            }
        }
    }
}

void le_csis_client_init(void)
{
    init_ualist_head(&le_csis_mgr.csis_list);
    le_csis_mgr.client_id = 0;
}
