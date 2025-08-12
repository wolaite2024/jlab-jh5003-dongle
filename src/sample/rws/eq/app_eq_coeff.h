#ifndef _EQ_COEFF_H_
#define _EQ_COEFF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define PI                   ((double)3.1415926535897932384626433832795)
#define SQRT_2               ((double)1.4142135623730950488016887242096)
#define MAX_PARAEQ_STAGENUM  10
#define MAX_APP_EQ_NUM       10

typedef enum
{
    EQ_FILTER_PEAK,
    EQ_FILTER_SHELVING_LP,
    EQ_FILTER_SHELVING_HP,
    EQ_FILTER_LOW_PASS,
    EQ_FILTER_HIGH_PASS,
    EQ_FILTER_PEAK2,
    EQ_FILTER_ALL_EQ_TYPE
} T_EQ_FILTER_TYPE;

typedef enum
{
    EQ_MCU_TO_SDK_SIG_PARAM,
    EQ_MCU_TO_SDK_SPK_EQ_PARAM,
    EQ_MCU_TO_SDK_MIC_EQ_PARAM,
} T_EQ_MCU_TO_SDK_CMD_ID;

typedef struct
{
    int order;
    double gain;
    double num[3];
    double den[3];
} T_EQ_FILTER_COEFF;

typedef struct
{
    unsigned int stage1_eq_type : 3;
    unsigned int stage2_eq_type : 3;
    unsigned int stage3_eq_type : 3;
    unsigned int stage4_eq_type : 3;
    unsigned int stage5_eq_type : 3;
    unsigned int resv : 9;
    unsigned int id : 8;
} T_EQ_MCU_TO_SDK_CMD_HDR;

typedef struct
{
    uint16_t coeff_len;
    union
    {
        struct
        {
            T_EQ_MCU_TO_SDK_CMD_HDR sdk_cmd_hdr;
            uint32_t stage: 4;
            int32_t guad_bit: 4;
            int32_t gain: 24;
            int32_t coeff[MAX_PARAEQ_STAGENUM][5];
        };
        uint8_t value[208];
    };
} T_EQ_COEFF;

double app_eq_coeff_db_to_linear(double value);

void app_eq_coeff_calculate(int32_t stage_num, double global_gain, uint32_t fs, uint32_t *freq,
                            double *gain, double *q, T_EQ_FILTER_TYPE *biquad_type, T_EQ_COEFF *result);

void app_eq_coeff_check_para(int32_t stage_num, double *global_gain, uint32_t *freq, double *gain,
                             double *q, T_EQ_FILTER_TYPE *biquad_type);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
