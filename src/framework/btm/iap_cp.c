/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stddef.h>
#include <trace.h>
#include <iap_cp.h>

void *cp_sem = NULL;

P_CP_WRITE_FUNC cp_write = NULL;
P_CP_READ_FUNC cp_read = NULL;

void cp_init(P_CP_WRITE_FUNC p_cp_write,
             P_CP_READ_FUNC  p_cp_read)
{
    cp_write = p_cp_write;
    cp_read = p_cp_read;
}

/**
  * @brief read cp basic information.
  * @param[out] info cp device info buffer.
  * @return read result.
  */
bool cp_read_cp_info(T_CP_INFO *info)
{
    if (cp_read == NULL)
    {
        return false;
    }
    return cp_read(CP_REG_DEV_VER, (uint8_t *)info, 8);
}

/**
  * @brief read signature info.
  * @param[out] signature info buffer.
  * @note the first 2 bytes in signature buffer are signature data length.
  * @return read result.
  */
bool cp_read_signature_len(uint16_t *p_signature_len)
{
    uint8_t data[2];

    if (cp_read == NULL)
    {
        return false;
    }

    if (cp_read(CP_SIG_LEN, data, 2) == true)
    {
        *p_signature_len = ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
        return true;
    }

    return false;
}

bool cp_read_signature_data(uint8_t  *sig_buf,
                            uint16_t  length)
{
    if (cp_read == NULL)
    {
        return false;
    }
    return cp_read(CP_SIG_DATA, sig_buf, length);
}

/**
  * @brief write challenge info.
  * @param[in] cha_buf challenge info buffer.
  * @param[in] buf_len challenge info buffer length.
  * @note the first 2 bytes in challenge info buffer are challenge data length.
  * @return write result.
  */
bool cp_write_cha_info(uint8_t  *cha_buf,
                       uint16_t  buf_len)
{
    if (cp_write == NULL)
    {
        return false;
    }
    return cp_write(CP_REG_CHA_LEN, cha_buf, buf_len);
}

bool cp_write_cha_len(uint16_t len)
{
    uint8_t len_buf[2];

    if (cp_write == NULL)
    {
        return false;
    }
    len_buf[0] = len >> 8;
    len_buf[1] = (uint8_t)len;
    return cp_write(CP_REG_CHA_LEN, len_buf, 2);
}

bool cp_write_cha_data(uint8_t  *cha_buf,
                       uint16_t  buf_len)
{
    if (cp_write == NULL)
    {
        return false;
    }
    return cp_write(CP_REG_CHA_DATA, cha_buf, buf_len);
}

/**
  * @brief read certificate data length.
  * @param none
  * @return certificate data length.
  */
bool cp_read_crf_data_len(uint16_t *p_crf_data_le)
{
    uint8_t data[2];

    if (cp_read == NULL)
    {
        return false;
    }

    if (cp_read(CP_REG_ACD_LEN, data, 2) == true)
    {
        *p_crf_data_le = ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
        return true;
    }
    return false;
}

/**
  * @brief read certificate data.
  * @param[in] cert_data_page
  * @param[in] data certificate data buffer.
  * @param[in] length certificate data length.
  * @return read result.
  */
bool cp_read_crf_data(uint8_t   cert_data_page,
                      uint8_t  *data,
                      uint16_t  length)
{
    if (cp_read == NULL)
    {
        return false;
    }
    return cp_read(cert_data_page, data, length);
}

/**
  * @brief start cp cpntrol process.
  * @param[in] cmd cp control comamnd.
  * @return write result.
  */
bool cp_ctrl(T_CP_CMD cmd)
{
    uint8_t op = (uint8_t)cmd;

    if (cp_write == NULL)
    {
        return false;
    }
    return cp_write(CP_REG_CS, &op, 1);
}

/**
  * @brief read cp process result.
  * @param none.
  * @return cp process result.
  */
bool cp_read_proc_result(T_CP_PRO_RES *p_cp_pro_res_m)
{
    T_CP_PRO_RES res;

    if (cp_read == NULL)
    {
        return false;
    }

    if (cp_read(CP_REG_CS, &res, 1) == true)
    {
        *p_cp_pro_res_m = res;
        return true;
    }

    return false;
}

/**
  * @brief read cp error code.
  * @param none.
  * @return cp process error code.
  */
bool cp_read_err_code(T_CP_ERROR_CODE *p_cp_err_code_m)
{
    T_CP_ERROR_CODE res;

    if (cp_read == NULL)
    {
        return false;
    }

    if (cp_read(CP_ERR_CODE, &res, 1) == true)
    {
        *p_cp_err_code_m = res;
        return true;
    }

    return false;
}
