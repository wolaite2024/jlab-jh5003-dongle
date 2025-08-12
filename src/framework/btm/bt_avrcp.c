/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVRCP_SUPPORT == 1)
#include <string.h>

#include "os_mem.h"
#include "avrcp.h"
#include "btm.h"
#include "bt_avrcp.h"
#include "bt_avrcp_int.h"
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

void bt_avrcp_cback(uint8_t      bd_addr[6],
                    T_AVRCP_MSG  msg_type,
                    void        *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    BTM_PRINT_INFO1("bt_avrcp_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case AVRCP_MSG_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_CONN_IND, &payload);
        }
        else
        {
            bt_avrcp_connect_cfm(bd_addr, false);
        }
        break;

    case AVRCP_MSG_CONN_CMPL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->connected_profile |= AVRCP_PROFILE_MASK;
            bt_mgr_dispatch(BT_MSG_AVRCP_CONN_CMPL, &payload);
            avrcp_get_capability(bd_addr, CAPABILITY_ID_EVENTS_SUPPORTED);
        }
        else
        {
            avrcp_disconnect_req(bd_addr);
        }
        break;

    case AVRCP_MSG_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_CONN_FAIL, &payload);
        }
        break;

    case AVRCP_MSG_DISCONN:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->connected_profile &= ~AVRCP_PROFILE_MASK;
            p_link->avrcp_data.vol_change_registered = false;
            p_link->avrcp_data.play_status_change_registered = false;
            p_link->avrcp_data.track_change_registered = false;
            p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_STOPPED;
            bt_mgr_dispatch(BT_MSG_AVRCP_DISCONN_CMPL, &payload);
        }
        break;

    case AVRCP_MSG_CMD_ABS_VOL:
        if (p_link != NULL)
        {
            payload.msg_buf = (uint8_t *)msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_ABSOLUTE_VOLUME_SET, &payload);
        }
        break;

    case AVRCP_MSG_CMD_VOL_UP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_KEY_VOLUME_UP, &payload);
        }
        break;

    case AVRCP_MSG_CMD_VOL_DOWN:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_KEY_VOLUME_DOWN, &payload);
        }
        break;

    case AVRCP_MSG_CMD_REG_VOL_CHANGE:
        if (p_link != NULL)
        {
            if (p_link->avrcp_data.vol_change_registered != true)
            {
                p_link->avrcp_data.vol_change_registered = true;
            }

            bt_mgr_dispatch(BT_MSG_AVRCP_REG_VOL_CHANGE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_REG_PLAY_STATUS_CHANGE:
        if (p_link != NULL)
        {
            if (p_link->avrcp_data.play_status_change_registered != true)
            {
                p_link->avrcp_data.play_status_change_registered = true;
            }

            bt_mgr_dispatch(BT_MSG_AVRCP_REG_PLAY_STATUS_CHANGE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_POWER:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_POWER, &payload);
        }
        break;

    case AVRCP_MSG_CMD_MUTE:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_MUTE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_PLAY:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_PLAY, &payload);
        }
        break;

    case AVRCP_MSG_CMD_STOP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_STOP, &payload);
        }
        break;

    case AVRCP_MSG_CMD_PAUSE:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_PAUSE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_REWIND_START:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_REWIND_START, &payload);
        }
        break;

    case AVRCP_MSG_CMD_REWIND_STOP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_REWIND_STOP, &payload);
        }
        break;

    case AVRCP_MSG_CMD_FAST_FORWARD_START:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_FAST_FORWARD_START, &payload);
        }
        break;

    case AVRCP_MSG_CMD_FAST_FORWARD_STOP:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_FAST_FORWARD_STOP, &payload);
        }
        break;

    case AVRCP_MSG_CMD_FORWARD:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_FORWARD, &payload);
        }
        break;

    case AVRCP_MSG_CMD_BACKWARD:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_BACKWARD, &payload);
        }
        break;

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
    case AVRCP_MSG_CMD_REG_TRACK_CHANGE:
        if (p_link != NULL)
        {
            if (p_link->avrcp_data.track_change_registered != true)
            {
                p_link->avrcp_data.track_change_registered = true;
            }

            bt_mgr_dispatch(BT_MSG_AVRCP_REG_TRACK_CHANGE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_GET_ELEMENT_ATTRS:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_ELEM_ATTRS, &payload);
        }
        break;

    case AVRCP_MSG_CMD_GET_PLAY_STATUS:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_PLAY_STATUS, &payload);
        }
        break;

    case AVRCP_MSG_CMD_SET_ADDRESSED_PLAYER:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_SET_ADDRESSED_PLAYER, &payload);
        }
        break;

    case AVRCP_MSG_CMD_REG_ADDRESSED_PLAYER_CHANGE:
        if (p_link != NULL)
        {
            if (p_link->avrcp_data.addressed_player_change_registered != true)
            {
                p_link->avrcp_data.addressed_player_change_registered = true;
            }

            bt_mgr_dispatch(BT_MSG_AVRCP_REG_ADDRESSED_PLAYER_CHANGE, &payload);
        }
        break;

    case AVRCP_MSG_CMD_GET_FOLDER_ITEMS:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_FOLDER_ITEMS, &payload);
        }
        break;

    case AVRCP_MSG_CMD_GET_TOTAL_NUM_OF_ITEMS:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_TOTAL_NUM_OF_ITEMS, &payload);
        }
        break;
#endif

    case AVRCP_MSG_RSP_PASSTHROUGH:
        if (p_link != NULL)
        {
            T_RSP_PASSTHROUGH *rsp = (T_RSP_PASSTHROUGH *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                //Handle key press response -> Send key release pass through command
                if (rsp->pressed == false) /* released */
                {
                    bool report = false;

                    if (rsp->key == AVRCP_KEY_PAUSE)
                    {
                        p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                        report = true;
                    }
                    else if (rsp->key == AVRCP_KEY_PLAY)
                    {
                        p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
                        report = true;
                    }
                    else if (rsp->key == AVRCP_KEY_STOP)
                    {
                        p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_STOPPED;
                        report = true;
                    }

                    if (report == true)
                    {
                        bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_RSP, &payload);
                    }
                }
                else /* pushed */
                {
                    //Send AVRCP_KEY_FAST_FORWARD/AVRCP_KEY_REWIND release when user key release
                    if ((rsp->key != AVRCP_KEY_FAST_FORWARD) &&
                        (rsp->key != AVRCP_KEY_REWIND))
                    {
                        avrcp_send_pass_through(p_link->bd_addr, rsp->key, false);
                    }
                }
            }
            else
            {
                //BTM_PRINT_INFO0("bt_avrcp_cback: AVRCP_MSG_RSP_PASSTHROUGH fail");
            }
        }
        break;

    case AVRCP_MSG_RSP_GET_CPBS:
        if (p_link != NULL)
        {
            uint8_t cpbs_count;
            uint8_t *cpbs_buf;
            T_RSP_CPBS *tmp;

            tmp = msg_buf;
            cpbs_count = tmp->capability_count;
            cpbs_buf = tmp->p_buf;
            while (cpbs_count != 0)
            {
                avrcp_register_notification(p_link->bd_addr, *cpbs_buf);

                cpbs_count -= 1;
                cpbs_buf += 1;
            }
        }
        break;

    case AVRCP_MSG_RSP_REG_NOTIFICATION:
        if (p_link != NULL)
        {
            T_RSP_REG_NOTIFICATION *rsp = (T_RSP_REG_NOTIFICATION *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                switch (rsp->event_id)
                {
                case EVENT_PLAYBACK_STATUS_CHANGED:
                    {
                        uint8_t play_status = p_link->avrcp_data.play_status;

                        p_link->avrcp_data.play_status = rsp->u.play_status;
                        if (p_link->avrcp_data.play_status == BT_AVRCP_PLAY_STATUS_STOPPED)
                        {
                            p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                        }

                        if (p_link->avrcp_data.play_status != play_status)
                        {
                            payload.msg_buf = &play_status;
                            bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_CHANGED, &payload);
                        }
                    }
                    break;

                case EVENT_TRACK_CHANGED:
                    break;

                case EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
                    {
                        T_BT_AVRCP_RSP_APP_SETTING_CHANGED tmp;

                        tmp.num_of_attr = rsp->u.app_settings.num_of_attr;
                        tmp.p_app_setting = (T_BT_AVRCP_APP_SETTING *)rsp->u.app_settings.app_setting;

                        payload.msg_buf = &tmp;
                        bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_CHANGED, &payload);
                    }
                    break;

                case EVENT_NOW_PLAYING_CONTENT_CHANGED:
                    break;

                case EVENT_AVAILABLE_PLAYERS_CHANGED:
                    break;

                case EVENT_ADDRESSED_PLAYER_CHANGED:
                    {
                        // T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED tmp;

                        // tmp.player_id = rsp->u.addressed_player.player_id;
                        // tmp.uid_counter = rsp->u.addressed_player.uid_counter;

                        // payload.msg_buf = &tmp;
                        // bt_mgr_dispatch(BT_MSG_AVRCP_ADDRESSED_PLAYER_CHANGED, &payload);
                    }
                    break;

                case EVENT_UIDS_CHANGED:
                    break;

                case EVENT_VOLUME_CHANGED:
                    {
                        payload.msg_buf = &(rsp->u.volume);
                        bt_mgr_dispatch(BT_MSG_AVRCP_VOLUME_CHANGED, &payload);
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                //BTM_PRINT_INFO0("bt_avrcp_cback: AVRCP_MSG_RSP_REG_NOTIFICATION fail");
            }
        }
        break;

    case AVRCP_MSG_RSP_GET_PLAYSTATUS:
        if (p_link != NULL)
        {
            T_RSP_GET_PLAY_STATUS *rsp = (T_RSP_GET_PLAY_STATUS *)msg_buf;

            if (rsp->state == AVRCP_RSP_STATE_SUCCESS)
            {
                p_link->avrcp_data.play_status = rsp->play_status;
            }

            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_PLAY_STATUS_RSP, &payload);
        }
        break;

    case AVRCP_MSG_RSP_GET_ELEMENT_ATTR:
        if (p_link != NULL)
        {
            T_RSP_GET_ELEMENT_ATTR *rsp = (T_RSP_GET_ELEMENT_ATTR *)msg_buf;
            T_BT_AVRCP_RSP_GET_ELEMENT_ATTR attr;

            attr.state = rsp->state;
            attr.num_of_attr = rsp->num_of_attr;
            attr.attr = (T_BT_AVRCP_ELEMENT_ATTR *)rsp->attr;

            payload.msg_buf = &attr;
            bt_mgr_dispatch(BT_MSG_AVRCP_GET_ELEM_ATTR_RSP, &payload);
        }
        break;

    case AVRCP_MSG_NOTIF_CHANGED:
        if (p_link != NULL)
        {
            T_NOTIF_CHANGED *rsp = (T_NOTIF_CHANGED *)msg_buf;

            switch (rsp->event_id)
            {
            case EVENT_PLAYBACK_STATUS_CHANGED:
                {
                    p_link->avrcp_data.play_status = rsp->u.play_status;
                    if (p_link->avrcp_data.play_status == BT_AVRCP_PLAY_STATUS_STOPPED)
                    {
                        p_link->avrcp_data.play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
                    }

                    bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_STATUS_CHANGED, &payload);
                }
                break;

            case EVENT_TRACK_CHANGED:
                {
                    payload.msg_buf = &(rsp->u.track_id);
                    bt_mgr_dispatch(BT_MSG_AVRCP_TRACK_CHANGED, &payload);
                }
                break;

            case EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
                {
                    T_BT_AVRCP_RSP_APP_SETTING_CHANGED tmp;

                    tmp.num_of_attr = rsp->u.app_settings.num_of_attr;
                    tmp.p_app_setting = (T_BT_AVRCP_APP_SETTING *)rsp->u.app_settings.app_setting;

                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_CHANGED, &payload);
                }
                break;

            case EVENT_NOW_PLAYING_CONTENT_CHANGED:
                {
                    bt_mgr_dispatch(BT_MSG_AVRCP_PLAYING_CONTENT_CHANGED, &payload);
                }
                break;

            case EVENT_AVAILABLE_PLAYERS_CHANGED:
                {
                    bt_mgr_dispatch(BT_MSG_AVRCP_AVAILABLE_PLAYER_CHANGED, &payload);
                }
                break;

            case EVENT_ADDRESSED_PLAYER_CHANGED:
                {
                    T_BT_AVRCP_RSP_ADDRESSED_PLAYER_CHANGED tmp;

                    tmp.player_id = rsp->u.addressed_player.player_id;
                    tmp.uid_counter = rsp->u.addressed_player.uid_counter;

                    payload.msg_buf = &tmp;
                    bt_mgr_dispatch(BT_MSG_AVRCP_ADDRESSED_PLAYER_CHANGED, &payload);
                }
                break;

            case EVENT_UIDS_CHANGED:
                {
                    payload.msg_buf = &(rsp->u.uid_counter);
                    bt_mgr_dispatch(BT_MSG_AVRCP_UIDS_CHANGED, &payload);
                }
                break;

            case EVENT_VOLUME_CHANGED:
                {
                    payload.msg_buf = &(rsp->u.volume);
                    bt_mgr_dispatch(BT_MSG_AVRCP_VOLUME_CHANGED, &payload);
                }
                break;

            default:
                break;
            }

            avrcp_register_notification(p_link->bd_addr, rsp->event_id);
        }
        break;

    case AVRCP_MSG_RSP_LIST_APP_SETTING_ATTRS:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_LIST_APP_SETTING_ATTRS *rsp = (T_AVRCP_RSP_LIST_APP_SETTING_ATTRS *)msg_buf;
            T_BT_AVRCP_RSP_LIST_APP_SETTING_ATTRS tmp;

            tmp.state = rsp->state;
            tmp.num_of_attr = rsp->num_of_attr;
            tmp.p_attr_id = rsp->attr_id;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_ATTRS, &payload);
        }
        break;

    case AVRCP_MSG_RSP_LIST_APP_SETTING_VALUES:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_LIST_APP_SETTING_VALUES *rsp = (T_AVRCP_RSP_LIST_APP_SETTING_VALUES *)msg_buf;
            T_BT_AVRCP_RSP_LIST_APP_SETTING_VALUES tmp;

            tmp.state = rsp->state;
            tmp.num_of_value = rsp->num_of_value;
            tmp.p_value = rsp->value;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_APP_SETTING_VALUES, &payload);
        }
        break;

    case AVRCP_MSG_RSP_GET_CUR_APP_SETTING_VALUE:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_GET_APP_SETTING_VALUE *rsp = (T_AVRCP_RSP_GET_APP_SETTING_VALUE *)msg_buf;
            T_BT_AVRCP_RSP_GET_APP_SETTING_VALUE tmp;

            tmp.state = rsp->state;
            tmp.num_of_attr = rsp->num_of_attr;
            tmp.p_app_setting = (T_BT_AVRCP_APP_SETTING *)rsp->app_setting;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_CUR_APP_SETTING_VALUE, &payload);
        }
        break;

    case AVRCP_MSG_RSP_SET_ADDRESSED_PLAYER:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_SET_ADDRESSED_PLAYER *rsp = (T_AVRCP_RSP_SET_ADDRESSED_PLAYER *)msg_buf;

            payload.msg_buf = &rsp->status_code;
            bt_mgr_dispatch(BT_MSG_AVRCP_SET_ADDRESSED_PLAYER_RSP, &payload);
        }
        break;

    case AVRCP_MSG_RSP_PLAY_ITEM:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_PLAY_ITEM *rsp = (T_AVRCP_RSP_PLAY_ITEM *)msg_buf;

            payload.msg_buf = &rsp->status_code;
            bt_mgr_dispatch(BT_MSG_AVRCP_PLAY_ITEM, &payload);
        }
        break;

    case AVRCP_MSG_RSP_GET_FOLDER_ITEMS:
        if (p_link != NULL)
        {
            T_RSP_GET_FOLDER_ITEMS *rsp = (T_RSP_GET_FOLDER_ITEMS *)msg_buf;
            T_BT_AVRCP_RSP_GET_FOLDER_ITEMS tmp;

            tmp.status_code = rsp->status_code;
            tmp.uid_counter = rsp->uid_counter;
            tmp.num_of_items = rsp->num_of_items;
            tmp.item_type = rsp->item_type;
            if (tmp.num_of_items > 0)
            {
                switch (tmp.item_type)
                {
                case AVRCP_ITEM_TYPE_MEDIA_PLAYER:
                    tmp.u.p_media_player_items = (T_BT_AVRCP_MEDIA_PLAYER_ITEM *)rsp->u.p_media_player_items;
                    break;

                case AVRCP_ITEM_TYPE_FOLDER:
                    tmp.u.p_folder_items = (T_BT_AVRCP_FOLDER_ITEM *)rsp->u.p_folder_items;
                    break;

                case AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
                    tmp.u.p_media_element_items = (T_BT_AVRCP_MEDIA_ELEMENT_ITEM *)rsp->u.p_media_element_items;
                    break;

                default:
                    {
                        BTM_PRINT_ERROR1("bt_avrcp_cback: unknown item type %d", tmp.item_type);

                        tmp.num_of_items = 0;
                        tmp.item_type = 0;
                    }
                    break;
                }
            }
            else
            {
                tmp.item_type = 0;
            }

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_FOLDER_ITEMS, &payload);
        }
        break;

    case AVRCP_MSG_RSP_GET_ITEM_ATTRS:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_GET_ITEM_ATTRS *rsp = (T_AVRCP_RSP_GET_ITEM_ATTRS *)msg_buf;
            T_BT_AVRCP_RSP_GET_ITEM_ATTR attr;

            attr.state = rsp->status_code;
            attr.num_of_attr = rsp->num_of_attr;
            attr.attr = (T_BT_AVRCP_ELEMENT_ATTR *)rsp->attr;

            payload.msg_buf = &attr;
            bt_mgr_dispatch(BT_MSG_AVRCP_ITEM_ATTR, &payload);
        }
        break;

    case AVRCP_MSG_RSP_SEARCH:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_SEARCH *rsp = (T_AVRCP_RSP_SEARCH *)msg_buf;
            T_BT_AVRCP_RSP_SEARCH search;

            search.status_code = rsp->status_code;
            search.uid_counter = rsp->uid_counter;
            search.num_of_items = rsp->num_of_items;

            payload.msg_buf = &search;
            bt_mgr_dispatch(BT_MSG_AVRCP_SEARCH, &payload);
        }
        break;

    case AVRCP_MSG_RSP_SET_BROWSED_PLAYER:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_SET_BROWSED_PLAYER *rsp = (T_AVRCP_RSP_SET_BROWSED_PLAYER *)msg_buf;
            T_BT_AVRCP_RSP_SET_BROWSED_PLAYER tmp;

            tmp.status_code = rsp->status_code;
            tmp.uid_counter = rsp->uid_counter;
            tmp.num_of_items = rsp->num_of_items;
            tmp.character_set_id = rsp->character_set_id;
            tmp.folder_depth = rsp->folder_depth;
            tmp.p_folders = (T_BT_AVRCP_FOLDER *)rsp->folder;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_SET_BROWSED_PLAYER, &payload);
        }
        break;

    case AVRCP_MSG_RSP_CHANGE_PATH:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_CHANGE_PATH *rsp = (T_AVRCP_RSP_CHANGE_PATH *)msg_buf;
            T_BT_AVRCP_RSP_CHANGE_PATH tmp;

            tmp.status_code = rsp->status_code;
            tmp.num_of_items = rsp->num_of_items;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_CHANGE_PATH, &payload);
        }
        break;

    case AVRCP_MSG_RSP_SET_ABS_VOL:
        if (p_link != NULL)
        {
            T_AVRCP_RSP_SET_ABS_VOL *rsp = (T_AVRCP_RSP_SET_ABS_VOL *)msg_buf;
            T_BT_AVRCP_RSP_SET_ABS_VOL tmp;

            tmp.state = rsp->state;
            tmp.volume = rsp->volume;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_SET_ABSOLUTE_VOLUME_RSP, &payload);
        }
        break;

    case AVRCP_MSG_BROWSING_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_CONN_IND, &payload);
        }
        else
        {
            bt_avrcp_browsing_connect_cfm(bd_addr, false);
        }
        break;

    case AVRCP_MSG_BROWSING_CONN_CMPL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->avrcp_data.browsing_chann_connected = true;
            bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_CONN_CMPL, &payload);
        }
        else
        {
            avrcp_browsing_disconnect_req(bd_addr);
        }
        break;

    case AVRCP_MSG_BROWSING_DISCONN:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->avrcp_data.browsing_chann_connected = false;
            bt_mgr_dispatch(BT_MSG_AVRCP_BROWSING_DISCONN_CMPL, &payload);
        }
        break;

    case AVRCP_MSG_COVER_ART_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->avrcp_data.cover_art_chann_connected = true;
            bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_CONN_CMPL, &payload);
        }
        break;

    case AVRCP_MSG_COVER_ART_DISCONN:
        if (p_link != NULL)
        {
            p_link->avrcp_data.cover_art_chann_connected = false;
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_DISCONN_CMPL, &payload);
        }
        break;

    case AVRCP_MSG_COVER_ART_DATA_IND:
        if (p_link != NULL)
        {
            T_AVRCP_COVER_ART_MSG_DATA *data = (T_AVRCP_COVER_ART_MSG_DATA *)msg_buf;
            T_BT_AVRCP_COVER_ART_DATA_IND tmp;

            tmp.p_data = data->p_data;
            tmp.data_len = data->data_len;
            tmp.data_end = data->data_end;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_AVRCP_COVER_ART_DATA_IND, &payload);
        }
        break;

    case AVRCP_MSG_VENDOR_CMD_IND:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_VENDOR_CMD_IND, &payload);
        }
        break;

    case AVRCP_MSG_VENDOR_RSP:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_AVRCP_VENDOR_RSP, &payload);
        }
        break;

    case AVRCP_MSG_ERR:
        break;

    default:
        break;
    }
}

bool bt_avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                         uint8_t vol)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.vol_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_volume_change_register_rsp(bd_addr, vol);
            }
        }
    }

    return false;
}

bool bt_avrcp_volume_change_req(uint8_t bd_addr[6],
                                uint8_t vol)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.vol_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_notify_volume_change(bd_addr, vol);
            }
        }
    }

    return false;
}

bool bt_avrcp_play_status_change_register_rsp(uint8_t                bd_addr[6],
                                              T_BT_AVRCP_PLAY_STATUS play_status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.play_status_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_play_status_change_register_rsp(bd_addr, play_status);
            }
        }
    }

    return false;
}

bool bt_avrcp_play_status_change_req(uint8_t                bd_addr[6],
                                     T_BT_AVRCP_PLAY_STATUS play_status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.play_status_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_notify_play_status_change(bd_addr, play_status);
            }
        }
    }

    return false;
}

bool bt_avrcp_get_element_attr_req(uint8_t  bd_addr[6],
                                   uint8_t  attr_num,
                                   uint8_t *p_attr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_get_element_attr(bd_addr, attr_num, p_attr);
        }
    }

    return false;
}

bool bt_avrcp_get_play_status_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_get_play_status(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_play(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_PLAY, true);
        }
    }

    return false;
}

bool bt_avrcp_pause(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_PAUSE, true);
        }
    }

    return false;
}

bool bt_avrcp_stop(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_STOP, true);
        }
    }

    return false;
}

bool bt_avrcp_forward(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_FORWARD, true);
        }
    }

    return false;
}

bool bt_avrcp_backward(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_BACKWARD, true);
        }
    }

    return false;
}

bool bt_avrcp_mute(uint8_t *bd_addr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_MUTE, true);
        }
    }

    return false;
}

bool bt_avrcp_power(uint8_t *bd_addr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_POWER, true);
        }
    }

    return false;
}

bool bt_avrcp_rewind_start(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_REWIND, true);
        }
    }

    return false;
}

bool bt_avrcp_rewind_stop(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_REWIND, false);
        }
    }

    return false;
}

bool bt_avrcp_fast_forward_start(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_FAST_FORWARD, true);
        }
    }

    return false;
}

bool bt_avrcp_fast_forward_stop(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_pass_through(p_link->bd_addr, AVRCP_KEY_FAST_FORWARD, false);
        }
    }

    return false;
}

bool bt_avrcp_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_connect_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_disconnect_req(bd_addr);
        }

        return avrcp_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_avrcp_connect_cfm(uint8_t bd_addr[6],
                          bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avrcp_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_avrcp_absolute_volume_set(uint8_t bd_addr[6],
                                  uint8_t volume)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_set_absolute_volume(bd_addr, volume);
        }
    }

    return false;
}

bool bt_avrcp_vendor_cmd_send(uint8_t           bd_addr[6],
                              T_BT_AVRCP_CTYPE  ctype,
                              uint32_t          company_id,
                              uint8_t          *p_pdu,
                              uint16_t          pdu_length)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_vendor_cmd(bd_addr, AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE, AVRCP_SUBUNIT_ID,
                                         ctype, company_id, p_pdu, pdu_length);
        }
    }

    return false;
}

bool bt_avrcp_vendor_rsp_send(uint8_t              bd_addr[6],
                              T_BT_AVRCP_RESPONSE  response,
                              uint32_t             company_id,
                              uint8_t             *p_pdu,
                              uint16_t             pdu_length)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_send_vendor_rsp(bd_addr, AVRCP_SUBUNIT_TYPE_VENDOR_UNIQUE, AVRCP_SUBUNIT_ID,
                                         response, company_id, p_pdu, pdu_length);
        }
    }

    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
bool bt_avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                        uint64_t track_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.track_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_track_change_register_rsp(bd_addr, track_id);
            }
        }
    }

    return false;
}

bool bt_avrcp_track_change_req(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.track_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_notify_track_change(bd_addr, track_id);
            }
        }
    }

    return false;
}

bool bt_avrcp_get_element_attr_rsp(uint8_t                  bd_addr[6],
                                   uint8_t                  attr_num,
                                   T_BT_AVRCP_ELEMENT_ATTR *p_attr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_get_element_attr_rsp(bd_addr, attr_num, (T_ELEMENT_ATTR *)p_attr);
        }
    }

    return false;
}

bool bt_avrcp_get_play_status_rsp(uint8_t                bd_addr[6],
                                  uint32_t               song_length,
                                  uint32_t               song_pos,
                                  T_BT_AVRCP_PLAY_STATUS play_status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_get_play_status_rsp(bd_addr, song_length, song_pos, play_status);
        }
    }

    return false;
}

bool bt_avrcp_addressed_player_set_rsp(uint8_t                     bd_addr[6],
                                       T_BT_AVRCP_RSP_ERROR_STATUS status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_set_addressed_player_rsp(bd_addr, status);
        }
    }

    return false;
}

bool bt_avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                   uint16_t player_id,
                                                   uint16_t uid_counter)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.addressed_player_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_addressed_player_change_register_rsp(bd_addr, player_id, uid_counter);
            }
        }
    }

    return false;
}

bool bt_avrcp_addressed_player_change_req(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            if (p_link->avrcp_data.addressed_player_change_registered)
            {
                bt_sniff_mode_exit(p_link, false);
                return avrcp_notify_addressed_player_change(bd_addr, player_id, uid_counter);
            }
        }
    }

    return false;
}

bool bt_avrcp_browsing_folder_items_get_rsp(uint8_t bd_addr[6], T_BT_AVRCP_RSP_ERROR_STATUS status,
                                            uint16_t uid_counter, uint16_t num_of_items, void *p_items)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) &&
            (p_link->avrcp_data.browsing_chann_connected == true))
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_browsing_get_folder_items_rsp(bd_addr, status, uid_counter,
                                                       num_of_items, p_items);
        }
    }

    return false;
}

bool bt_avrcp_browsing_total_num_of_items_get_rsp(uint8_t bd_addr[6],
                                                  T_BT_AVRCP_RSP_ERROR_STATUS status,
                                                  uint16_t uid_counter, uint32_t num_of_items)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) &&
            (p_link->avrcp_data.browsing_chann_connected == true))
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_browsing_get_total_num_of_items_rsp(bd_addr, status, uid_counter, num_of_items);
        }
    }

    return false;
}
#else
bool bt_avrcp_track_change_register_rsp(uint8_t bd_addr[6], uint64_t track_id)
{
    return false;
}

bool bt_avrcp_track_change_req(uint8_t bd_addr[6], uint64_t track_id)
{
    return false;
}

bool bt_avrcp_get_element_attr_rsp(uint8_t bd_addr[6], uint8_t attr_num,
                                   T_BT_AVRCP_ELEMENT_ATTR *p_attr)
{
    return false;
}

bool bt_avrcp_get_play_status_rsp(uint8_t bd_addr[6], uint32_t song_length, uint32_t song_pos,
                                  T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_addressed_player_set_rsp(uint8_t bd_addr[6], T_BT_AVRCP_RSP_ERROR_STATUS status)
{
    return false;
}

bool bt_avrcp_addressed_player_change_register_rsp(uint8_t bd_addr[6], uint16_t player_id,
                                                   uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_addressed_player_change_req(uint8_t bd_addr[6], uint16_t player_id,
                                          uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get_rsp(uint8_t bd_addr[6], T_BT_AVRCP_RSP_ERROR_STATUS status,
                                            uint16_t uid_counter, uint16_t num_of_items, void *p_items)
{
    return false;
}

bool bt_avrcp_browsing_total_num_of_items_get_rsp(uint8_t bd_addr[6],
                                                  T_BT_AVRCP_RSP_ERROR_STATUS status,
                                                  uint16_t uid_counter, uint32_t num_of_items)
{
    return false;
}
#endif

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) != 0 &&
            p_link->avrcp_data.browsing_chann_connected == false)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_browsing_connect_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->avrcp_data.browsing_chann_connected)
        {
            return avrcp_browsing_disconnect_req(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return avrcp_browsing_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_list_app_setting_attrs(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_list_app_setting_values(bd_addr, attr_id);
        }
    }

    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_get_current_app_setting_value(bd_addr, attr_num, attr_list);
        }
    }

    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_set_app_setting_value(bd_addr, attr_num, (T_AVRCP_APP_SETTING *)attr_list);
        }
    }

    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_req_continuing_rsp(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_abort_continuing_rsp(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_set_addressed_player(bd_addr, player_id);
        }
    }

    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_play_item(bd_addr, (T_AVRCP_SCOPE_ID)scope, uid, uid_counter);
        }
    }

    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & AVRCP_PROFILE_MASK)
        {
            bt_sniff_mode_exit(p_link, false);
            return avrcp_navigate_group(bd_addr, next, true);
        }
    }

    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint8_t  *p_attr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_GET_FOLDER_ITEMS tmp;
        uint8_t i;
        uint8_t *p;

        tmp.scope = scope;
        tmp.start_item = start_item;
        tmp.end_item = end_item;
        tmp.attr_count = attr_num;

        p = p_attr;
        for (i = 0; i < attr_num; i++)
        {
            tmp.attr_id[i] = *p++;
        }

        bt_sniff_mode_exit(p_link, false);
        if (!p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_connect_req(bd_addr);
        }

        return avrcp_browsing_get_folder_items(bd_addr, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint8_t  *p_attr)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_GET_ITEM_ATTRS tmp;
        uint8_t i;
        uint8_t *p;

        tmp.scope = scope;
        tmp.uid = uid;
        tmp.uid_counter = uid_counter;
        tmp.num_of_attr = attr_num;

        p = p_attr;
        for (i = 0; i < attr_num; i++)
        {
            tmp.attr_id[i] = *p++;
        }

        bt_sniff_mode_exit(p_link, false);
        if (!p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_connect_req(bd_addr);
        }

        return avrcp_browsing_get_item_attrs(bd_addr, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *p_search_str)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_SEARCH tmp;

        tmp.length = length;
        tmp.p_search_str = p_search_str;

        bt_sniff_mode_exit(p_link, false);
        if (!p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_connect_req(bd_addr);
        }

        return avrcp_browsing_search(bd_addr, &tmp);
    }

    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        if (!p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_connect_req(bd_addr);
        }

        return avrcp_browsing_set_browsed_player(bd_addr, player_id);
    }

    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_CHANGE_PATH tmp;

        tmp.uid_counter = uid_counter;
        tmp.direction = direction;
        tmp.folder_uid = folder_uid;

        bt_sniff_mode_exit(p_link, false);

        if (!p_link->avrcp_data.browsing_chann_connected)
        {
            avrcp_browsing_connect_req(bd_addr);
        }

        return avrcp_browsing_change_path(bd_addr, &tmp);
    }

    return false;
}
#else
bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    return false;
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint8_t  *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint8_t  *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *p_search_str)
{
    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    return false;
}
#endif

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & AVRCP_PROFILE_MASK) != 0 &&
            p_link->avrcp_data.cover_art_chann_connected == false)
        {
            bt_sniff_mode_exit(p_link, false);

            return avrcp_cover_art_conn_over_l2c(bd_addr, l2c_psm);
        }
    }

    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->avrcp_data.cover_art_chann_connected)
        {
            return avrcp_cover_art_disconnect(bd_addr);
        }
    }

    return false;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (!p_link->avrcp_data.cover_art_chann_connected)
        {
            BTM_PRINT_ERROR0("bt_avrcp_cover_art_get: cover art connection not exist");
            return false;
        }

        bt_sniff_mode_exit(p_link, false);
        return avrcp_cover_art_get_linked_thumbnail(bd_addr, &image_handle[0]);
    }

    return false;
}
#else
bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    return false;
}
#endif

bool bt_avrcp_supported_features_set(uint8_t ct_features,
                                     uint8_t tg_features)
{
    return avrcp_set_supported_features(ct_features, tg_features);
}

bool bt_avrcp_init(uint8_t link_num)
{
    return avrcp_init(link_num, COMPANY_BT_SIG, bt_avrcp_cback);
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_avrcp.h"

bool bt_avrcp_init(uint8_t link_num)
{
    return false;
}

bool bt_avrcp_supported_features_set(uint8_t ct_features,
                                     uint8_t tg_features)
{
    return false;
}

bool bt_avrcp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_connect_cfm(uint8_t bd_addr[6],
                          bool    accept)
{
    return false;
}

bool bt_avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                   bool    accept)
{
    return false;
}

bool bt_avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                         uint8_t vol)
{
    return false;
}

bool bt_avrcp_volume_change_req(uint8_t bd_addr[6],
                                uint8_t vol)
{
    return false;
}

bool bt_avrcp_play_status_change_register_rsp(uint8_t                bd_addr[6],
                                              T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_play_status_change_req(uint8_t                bd_addr[6],
                                     T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                        uint64_t track_id)
{
    return false;
}

bool bt_avrcp_track_change_req(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    return false;
}

bool bt_avrcp_get_element_attr_req(uint8_t  bd_addr[6],
                                   uint8_t  attr_num,
                                   uint8_t *p_attr)
{
    return false;
}

bool bt_avrcp_get_element_attr_rsp(uint8_t                  bd_addr[6],
                                   uint8_t                  attr_num,
                                   T_BT_AVRCP_ELEMENT_ATTR *p_attr)
{
    return false;
}

bool bt_avrcp_get_play_status_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_get_play_status_rsp(uint8_t                bd_addr[6],
                                  uint32_t               song_length,
                                  uint32_t               song_pos,
                                  T_BT_AVRCP_PLAY_STATUS play_status)
{
    return false;
}

bool bt_avrcp_play(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_pause(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_forward(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_backward(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_mute(uint8_t *bd_addr)
{
    return false;
}

bool bt_avrcp_power(uint8_t *bd_addr)
{
    return false;
}

bool bt_avrcp_rewind_start(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_rewind_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_fast_forward_start(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_fast_forward_stop(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_attrs_list(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_app_setting_values_list(uint8_t bd_addr[6],
                                      uint8_t attr_id)
{
    return false;
}

bool bt_avrcp_app_setting_value_get(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_app_setting_value_set(uint8_t  bd_addr[6],
                                    uint8_t  attr_num,
                                    uint8_t *attr_list)
{
    return false;
}

bool bt_avrcp_continuing_rsp_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_continuing_rsp_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_absolute_volume_set(uint8_t bd_addr[6],
                                  uint8_t volume)
{
    return false;
}

bool bt_avrcp_addressed_player_set(uint8_t  bd_addr[6],
                                   uint16_t player_id)
{
    return false;
}

bool bt_avrcp_addressed_player_set_rsp(uint8_t                     bd_addr[6],
                                       T_BT_AVRCP_RSP_ERROR_STATUS status)
{
    return false;
}

bool bt_avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                   uint16_t player_id,
                                                   uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_addressed_player_change_req(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_item_play(uint8_t  bd_addr[6],
                        uint8_t  scope,
                        uint64_t uid,
                        uint16_t uid_counter)
{
    return false;
}

bool bt_avrcp_group_navigate(uint8_t bd_addr[6],
                             bool    next)
{
    return false;
}

bool bt_avrcp_vendor_cmd_send(uint8_t           bd_addr[6],
                              T_BT_AVRCP_CTYPE  ctype,
                              uint32_t          company_id,
                              uint8_t          *p_pdu,
                              uint16_t          pdu_length)
{
    return false;
}

bool bt_avrcp_vendor_rsp_send(uint8_t              bd_addr[6],
                              T_BT_AVRCP_RESPONSE  response,
                              uint32_t             company_id,
                              uint8_t             *p_pdu,
                              uint16_t             pdu_length)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get(uint8_t   bd_addr[6],
                                        uint8_t   scope,
                                        uint32_t  start_item,
                                        uint32_t  end_item,
                                        uint8_t   attr_num,
                                        uint8_t  *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_folder_items_get_rsp(uint8_t                      bd_addr[6],
                                            T_BT_AVRCP_RSP_ERROR_STATUS  status,
                                            uint16_t                     uid_counter,
                                            uint16_t                     num_of_items,
                                            void                        *p_items)
{
    return false;
}

bool bt_avrcp_browsing_total_num_of_items_get_rsp(uint8_t                     bd_addr[6],
                                                  T_BT_AVRCP_RSP_ERROR_STATUS status,
                                                  uint16_t                    uid_counter,
                                                  uint32_t                    num_of_items)
{
    return false;
}

bool bt_avrcp_browsing_item_attrs_get(uint8_t   bd_addr[6],
                                      uint8_t   scope,
                                      uint64_t  uid,
                                      uint16_t  uid_counter,
                                      uint8_t   attr_num,
                                      uint8_t  *p_attr)
{
    return false;
}

bool bt_avrcp_browsing_search(uint8_t   bd_addr[6],
                              uint16_t  length,
                              uint8_t  *p_search_str)
{
    return false;
}

bool bt_avrcp_browsing_browsed_player_set(uint8_t  bd_addr[6],
                                          uint16_t player_id)
{
    return false;
}

bool bt_avrcp_browsing_path_change(uint8_t  bd_addr[6],
                                   uint16_t uid_counter,
                                   uint8_t  direction,
                                   uint64_t folder_uid)
{
    return false;
}

bool bt_avrcp_cover_art_connect_req(uint8_t  bd_addr[6],
                                    uint16_t l2c_psm)
{
    return false;
}

bool bt_avrcp_cover_art_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_avrcp_cover_art_get(uint8_t bd_addr[6],
                            uint8_t image_handle[16])
{
    return false;
}
#endif
