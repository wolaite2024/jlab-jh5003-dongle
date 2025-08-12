#include <stdint.h>
#include <string.h>
#include "ftl_ext.h"
#include "ftl.h"
#include "app_le_ext_ftl.h"
#include "storage.h"
#include "trace.h"

uint32_t lea_ext_ftl_load_data(void *data, uint32_t offset, uint32_t len);

uint32_t lea_ext_ftl_save_data(void *data, uint32_t offset, uint32_t len);

bool lea_ext_find_dev_by_addr(uint8_t *bd_addr, uint8_t bd_type, T_LEA_FTL_DEV *p_dev,
                              uint32_t *p_offset)
{
    if ((!bd_addr) || (!p_dev))
    {
        return false;
    }

    T_LEA_FTL_DEV ftl_dev;
    uint32_t offset = 0;
    uint32_t ret = 0;

    for (uint8_t i = 0; i < LEA_DEV_RECORD_MAX; i++)
    {
        offset = i * sizeof(T_LEA_FTL_DEV);
        ret = lea_ext_ftl_load_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        if (ret)
        {
            APP_PRINT_INFO1("lea_ext_find_dev_by_addr read ret %x", ret);
            return false;
        }

        if (ftl_dev.sync_word != LEA_FTL_SYNC_WORD)
        {
            continue;
        }

        if (!memcmp(bd_addr, ftl_dev.bd_addr, BD_ADDR_LEN) && (bd_type == ftl_dev.bd_type))
        {
            APP_PRINT_INFO4("lea_ext_find_dev_by_addr addr %b idx %x hash %b size %x",
                            TRACE_BDADDR(bd_addr), i, TRACE_BINARY(CSIS_HASH_LEN, ftl_dev.csis_hash), ftl_dev.csis_size);
            memcpy(p_dev, &ftl_dev, sizeof(T_LEA_FTL_DEV));
            *p_offset = offset;
            return true;
        }
    }
    return false;
}

bool lea_ext_find_dev_by_hash(uint8_t *hash, uint8_t dev_idx, T_LEA_FTL_DEV *p_dev)
{
    if ((!hash) || (!p_dev))
    {
        return false;
    }

    T_LEA_FTL_DEV ftl_dev;
    uint32_t offset = 0;
    uint32_t ret = 0;
    uint8_t hash_match_count = 0;   /* di */

    for (uint8_t i = 0; i < LEA_DEV_RECORD_MAX; i++)
    {
        offset = i * sizeof(T_LEA_FTL_DEV);
        ret = lea_ext_ftl_load_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        if (ret)
        {
            APP_PRINT_INFO1("lea_ext_find_dev_by_hash read ret %x", ret);
            return false;
        }
        if (ftl_dev.sync_word != LEA_FTL_SYNC_WORD)
        {
            return false;
        }
        if (!memcmp(hash, ftl_dev.csis_hash, CSIS_HASH_LEN))
        {
            if (hash_match_count == dev_idx)
            {
                APP_PRINT_INFO4("lea_ext_find_dev_by_hash hash %b addr %b idx %x  size %x",
                                TRACE_BINARY(CSIS_HASH_LEN, hash), TRACE_BDADDR(ftl_dev.bd_addr), i, ftl_dev.csis_size);
                memcpy(p_dev, &ftl_dev, sizeof(T_LEA_FTL_DEV));
                return true;
            }
            hash_match_count++;
        }
    }
    return false;
}

bool lea_ext_clear_hash(void)
{
    T_LEA_FTL_DEV ftl_dev;
    uint32_t offset = 0;
    uint32_t ret = 0;

    for (uint8_t i = 0; i < LEA_DEV_RECORD_MAX; i++)
    {
        offset = i * sizeof(T_LEA_FTL_DEV);
        ret = lea_ext_ftl_load_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        if (ret)
        {
            APP_PRINT_INFO1("lea_ext_save_le_dev read ret %x", ret);
            return false;
        }

        memset(&ftl_dev, 0, sizeof(T_LEA_FTL_DEV));
        lea_ext_ftl_save_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
    }
    return true;
}

uint8_t lea_ext_find_same_hash_count(uint8_t *hash)
{
    if (!hash)
    {
        return false;
    }

    T_LEA_FTL_DEV ftl_dev;
    uint32_t offset = 0;
    uint32_t ret = 0;
    uint8_t same_hash_count = 0;

    for (uint8_t i = 0; i < LEA_DEV_RECORD_MAX; i++)
    {
        offset = i * sizeof(T_LEA_FTL_DEV);
        ret = lea_ext_ftl_load_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        if (ret)
        {
            APP_PRINT_INFO1("lea_ext_save_le_dev read ret %x", ret);
            return false;
        }

        if (!memcmp(hash, ftl_dev.csis_hash, CSIS_HASH_LEN))
        {
            same_hash_count++;
        }
    }
    return same_hash_count;
}

bool lea_ext_save_le_dev(T_LEA_FTL_DEV *p_dev)
{
    if (!p_dev)
    {
        return false;
    }

    T_LEA_FTL_DEV ftl_dev;
    uint32_t offset = 0;
    uint32_t ret = 0;

    p_dev->sync_word = LEA_FTL_SYNC_WORD;
    if (lea_ext_find_dev_by_addr(p_dev->bd_addr, p_dev->bd_type, &ftl_dev, &offset))
    {
        if (memcmp(&ftl_dev, p_dev, sizeof(T_LEA_FTL_DEV)))
        {
            APP_PRINT_INFO3("lea_ext_save_le_dev update addr %b  hash %b size %x",
                            TRACE_BDADDR(p_dev->bd_addr), TRACE_BINARY(CSIS_HASH_LEN, p_dev->csis_hash), p_dev->csis_size);
            memcpy(&ftl_dev, p_dev, sizeof(T_LEA_FTL_DEV));
            return lea_ext_ftl_save_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        }
        return true;
    }

    if (lea_ext_find_same_hash_count(p_dev->csis_hash) >= p_dev->csis_size)
    {
        ret = lea_ext_clear_hash();
        if (!ret)
        {
            APP_PRINT_WARN1("lea_ext_clear_hash ret %x", ret);
            return false;
        }
    }

    for (uint8_t i = 0; i < LEA_DEV_RECORD_MAX; i++)
    {
        offset = i * sizeof(T_LEA_FTL_DEV);
        ret = lea_ext_ftl_load_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
        if (ret)
        {
            APP_PRINT_INFO1("lea_ext_save_le_dev read ret %x", ret);
            return false;
        }

        if (memcmp(p_dev->csis_hash, ftl_dev.csis_hash, CSIS_HASH_LEN))
        {
            memcpy(&ftl_dev, p_dev, sizeof(T_LEA_FTL_DEV));
            ret = lea_ext_ftl_save_data(&ftl_dev, offset, sizeof(T_LEA_FTL_DEV));
            ret = ret ? false : true;
            APP_PRINT_INFO5("lea_ext_save_le_dev addr %b idx %x hash %b size %x ret %x",
                            TRACE_BDADDR(p_dev->bd_addr), i,
                            TRACE_BINARY(CSIS_HASH_LEN, p_dev->csis_hash), p_dev->csis_size, ret);
            return ret;
        }
    }
    return false;
}


bool lea_ext_save_broadcast_id(uint8_t *broadcast_id)
{
    if (!broadcast_id)
    {
        return false;
    }

    T_LEA_BST lea_bst;
    uint32_t ret = 0;
    ret = lea_ext_ftl_load_data(&lea_bst, LEA_BROADCAST_ID_OFFSET, sizeof(T_LEA_BST));
    if (ret)
    {
        APP_PRINT_INFO1("lea_ext_save_broadcast_id read ret %x", ret);
        return false;
    }

    if ((lea_bst.sync_word == LEA_FTL_SYNC_WORD) &&
        (!memcmp(lea_bst.broadcast_id, broadcast_id, 3)))
    {
        return true;
    }
    lea_bst.sync_word = LEA_FTL_SYNC_WORD;
    memcpy(lea_bst.broadcast_id, broadcast_id, 3);
    lea_bst.rsv = 0;
    return lea_ext_ftl_save_data(&lea_bst, LEA_BROADCAST_ID_OFFSET, sizeof(T_LEA_BST));
}

bool lea_ext_read_broadcast_id(uint8_t *broadcast_id)
{
    if (!broadcast_id)
    {
        return false;
    }

    T_LEA_BST lea_bst;
    uint32_t ret = 0;
    ret = lea_ext_ftl_load_data(&lea_bst, LEA_BROADCAST_ID_OFFSET, sizeof(T_LEA_BST));
    if (ret)
    {
        APP_PRINT_INFO1("lea_ext_read_broadcast_id read ret %x", ret);
        return false;
    }

    if (lea_bst.sync_word != LEA_FTL_SYNC_WORD)
    {
        return false;
    }
    memcpy(broadcast_id, lea_bst.broadcast_id, 3);
    return true;
}

/* block must be write before read, init all le_ext_ftl block when first bootup */
void lea_ext_ftl_init_all_block(void)
{
    uint8_t block_buf[LEA_EXT_FTL_BLOCK_LEN] = {0xFF};
    uint32_t ret = 0;
    uint32_t offset = 0;

    while (offset < LEA_EXT_FTL_SIZE)
    {
        memset(block_buf, 0xFF, LEA_EXT_FTL_BLOCK_LEN);
        ret = ftl_load_from_module(LEA_EXT_FTL_PARTITION_NAME, (void *)block_buf, offset,
                                   LEA_EXT_FTL_BLOCK_LEN);
        if (ret == ENOF)
        {
            ftl_save_to_module(LEA_EXT_FTL_PARTITION_NAME, (void *)block_buf, offset, LEA_EXT_FTL_BLOCK_LEN);
        }
        offset += LEA_EXT_FTL_BLOCK_LEN;
    }
}

uint32_t lea_ext_ftl_load_data(void *data, uint32_t offset, uint32_t len)
{
    return storage_read(LEA_EXT_FTL_PARTITION_NAME, offset, len, data, NULL, NULL);
}

uint32_t lea_ext_ftl_save_data(void *data, uint32_t offset, uint32_t len)
{
    return storage_write(LEA_EXT_FTL_PARTITION_NAME, offset, len, data, NULL, NULL);
}

int32_t lea_ext_ftl_partition_init(const T_STORAGE_PARTITION_INFO *info)
{
    return ftl_init_module(LEA_EXT_FTL_PARTITION_NAME, LEA_EXT_FTL_SIZE, LEA_EXT_FTL_BLOCK_LEN);
}

int32_t lea_ext_ftl_partition_write(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                    uint32_t len, void *buf)
{
    if ((!buf) || (!len))
    {
        return -1;
    }

    if (offset + len > LEA_EXT_FTL_SIZE)
    {
        return -2;
    }

    uint8_t *p_buf = (uint8_t *)buf;
    uint8_t *data = NULL;
    uint32_t remain_len = len;
    uint32_t block_addr = (offset / LEA_EXT_FTL_BLOCK_LEN) * (LEA_EXT_FTL_BLOCK_LEN);
    uint32_t block_offset = offset % LEA_EXT_FTL_BLOCK_LEN;
    uint32_t write_len = 0;
    uint32_t ret = 0;

    data = calloc(1, LEA_EXT_FTL_BLOCK_LEN);
    if (!data)
    {
        return -3;
    }
    /* FIXME: if write fail in same block, what to do with write successed block before*/
    while (remain_len)
    {
        if (remain_len > LEA_EXT_FTL_BLOCK_LEN - block_offset)
        {
            write_len = LEA_EXT_FTL_BLOCK_LEN - block_offset;
        }
        else
        {
            write_len = remain_len;
        }
        ret = ftl_load_from_module(LEA_EXT_FTL_PARTITION_NAME, (void *)data, block_addr,
                                   LEA_EXT_FTL_BLOCK_LEN);
        if (ret)
        {
            free(data);
            return -4;
        }
        memcpy(data + block_offset, buf, write_len);
        ret = ftl_save_to_module(LEA_EXT_FTL_PARTITION_NAME, (void *)data, block_addr,
                                 LEA_EXT_FTL_BLOCK_LEN);
        if (ret)
        {
            free(data);
            return -5;
        }

        remain_len -= write_len;
        p_buf += write_len;
        block_addr += LEA_EXT_FTL_BLOCK_LEN;
        block_offset = 0;
    }

    free(data);
    return 0;
}

int32_t lea_ext_ftl_partition_read(const T_STORAGE_PARTITION_INFO *info, uint32_t offset,
                                   uint32_t len, void *buf)
{
    if ((!buf) || (!len))
    {
        return -1;
    }

    if (offset + len > LEA_EXT_FTL_SIZE)
    {
        return -2;
    }

    uint8_t *p_buf = (uint8_t *)buf;
    uint8_t *data = NULL;
    uint32_t remain_len = len;
    uint32_t block_addr = (offset / LEA_EXT_FTL_BLOCK_LEN) * (LEA_EXT_FTL_BLOCK_LEN);
    uint32_t block_offset = offset % LEA_EXT_FTL_BLOCK_LEN;
    uint32_t read_len = 0;
    uint32_t ret = 0;

    data = calloc(1, LEA_EXT_FTL_BLOCK_LEN);
    if (!data)
    {
        return -3;
    }
    /* FIXME: if write fail in same block, what to do with write successed block before*/
    while (remain_len)
    {
        if (remain_len > LEA_EXT_FTL_BLOCK_LEN - block_offset)
        {
            read_len = LEA_EXT_FTL_BLOCK_LEN - block_offset;
        }
        else
        {
            read_len = remain_len;
        }
        ret = ftl_load_from_module(LEA_EXT_FTL_PARTITION_NAME, (void *)data, block_addr,
                                   LEA_EXT_FTL_BLOCK_LEN);
        if (ret)
        {
            free(data);
            return -4;
        }
        memcpy(buf, data + block_offset, read_len);

        remain_len -= read_len;
        p_buf += read_len;
        block_addr += LEA_EXT_FTL_BLOCK_LEN;
        block_offset = 0;
    }

    free(data);
    return 0;
}

static const T_STORAGE_PARTITION_INFO  lea_ext_partitions =
{
    .name = LEA_EXT_FTL_PARTITION_NAME,
    .address = NULL,
    .size = NULL,
    .perm = STORAGE_PERMISSION_READ | STORAGE_PERMISSION_WRITE,
    .media_type = STORAGE_MEDIA_TYPE_NOR,
    .content_type = STORAGE_CONTENT_TYPE_RW_DATA,
    .init = lea_ext_ftl_partition_init,
    .read = lea_ext_ftl_partition_read,
    .write = lea_ext_ftl_partition_write,
    .erase = NULL,
    .async_read = NULL,
    .async_write = NULL,
    .async_erase = NULL,
};

bool lea_ext_ftl_init(void)
{
    if (storage_partition_init(&lea_ext_partitions, 1) == 0)
    {
        lea_ext_ftl_init_all_block();
        return true;
    }
    APP_PRINT_ERROR0("lea_ext_ftl_init:failed");
    return false;
}
