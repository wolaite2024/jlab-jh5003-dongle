#if (F_APP_EQ_COEFF_SUPPORT == 1)

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "app_eq_coeff.h"
#include "app_eq.h"
#include "trace.h"

#include "rtl876x_pinmux.h"
#include "app_cfg.h"
#include "pm.h"

#define BIT(_n)     (uint32_t)(1U << (_n))
#define MAX_FREQ    (44100 / 2)
#define MIN_FREQ    (1)
#define MAX_GAIN    12
#define MIN_GAIN    -12
#define MAX_Q       4
#define MIN_Q       0.2

double app_eq_coeff_db_to_linear(double value)
{
    return pow(10.0, value / (20.0));
};

int32_t app_eq_coeff_signed_quant(double in, int32_t total_bits, int32_t frac_bits)
{
    double tmp;
    double max_value;

    if ((total_bits == 32) && (frac_bits == 30))
    {
        tmp = 1073741824;       // 2^30
        max_value = 2147483648; // 2^31

        if (in >= 2)
        {
            tmp = max_value - 1.0;
        }
        else if (in <= -2)
        {
            tmp = -max_value;
        }
        else
        {
            tmp = round(tmp * in);
        }
    }
    else if ((total_bits == 24) && (frac_bits == 12))
    {
        tmp = 4096;             // 2^12
        max_value = 8388608;    // 2^23

        if (in >= 2048)
        {
            tmp = max_value - 1.0;
        }
        else if (in <= -2048)
        {
            tmp = -max_value;
        }
        else
        {
            tmp = round(tmp * in);
        }
    }
    else
    {
        if (frac_bits >= total_bits)
        {
            frac_bits = total_bits - 1;
        }
        if (total_bits > 32)
        {
            total_bits = 32;
        }

        tmp = pow(2.0, frac_bits);
        max_value = pow(2.0, total_bits - 1);

        tmp = round(tmp * in);
        if (tmp >= max_value)
        {
            tmp = max_value - 1.0;
        }
        else
        {
            if (tmp <= -1 * max_value)
            {
                tmp = -1 * max_value;
            }
        }
    }

    return ((int32_t)tmp);
}

void app_eq_coeff_peak_filter(double gain, uint32_t fc, double q, uint32_t fs, int32_t *result,
                              double *linear_global_gain)
{
    double t0, tmp0, tmp1, tmp2;
    int32_t i;
    double v = 1, max = 0, abs_val;
    double den[3];
    double num[3];

    tmp0 = PI * fc;
    t0 = 2 * tmp0 / fs;

    if (gain >= 1)
    {
        tmp1 = q * fs;
    }
    else
    {
        tmp1 = gain * q * fs;
    }

    tmp2 = tmp1 + tmp0;

    den[2] = 1 - (2 * tmp0) / tmp2;
    den[1] = (-2 * tmp1 * cos(t0)) / tmp2;
    den[0] = 1;

    num[0] = (tmp1 + tmp0 * gain) / tmp2;
    num[1] = den[1];
    num[2] = -num[0] + den[2] + 1;

    max = fabs(num[0]);
    for (i = 1; i <= 2; i++)
    {
        abs_val = fabs(num[i]);
        if (abs_val > max)
        {
            max = abs_val;
        }
    }

    if (max >= 2)
    {
        v = max;
        for (i = 0; i < 3; i++)
        {
            num[i] = num[i] / v;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_peak2_filter(double gain, uint32_t fc, double q, uint32_t fs, int32_t *result,
                               double *linear_global_gain)
{
    double tmp0, tmp1;
    int32_t i;
    double v = 1, max = 0, abs_val;
    double den[3];
    double num[3];
    double w0 = 2.0f * PI * fc / fs;
    double a = sqrt(gain);
    double alpha = sin(w0) / (2 * q);

    tmp0 = a + alpha;
    tmp1 = alpha * gain;

    den[2] = (a - alpha) / tmp0;
    den[1] = (-2 * cos(w0) * a) / tmp0;
    den[0] = 1;

    num[0] = (a + tmp1) / tmp0;
    num[1] = den[1];
    num[2] = (a - tmp1) / tmp0;

    max = fabs(num[0]);
    for (i = 1; i <= 2; i++)
    {
        abs_val = fabs(num[i]);
        if (abs_val > max)
        {
            max = abs_val;
        }
    }

    if (max >= 2)
    {
        v = max;
        for (i = 0; i < 3; i++)
        {
            num[i] = num[i] / v;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_low_pass_filter(double gain, uint32_t fc, double n, uint32_t fs, int32_t *result,
                                  double *linear_global_gain)
{
    int32_t i;
    double a1;
    double v = 1, max = 0, abs_val;
    double den[3];
    double num[3];
    double t0, tmp0, tmp1;

    if (fc < 20)
    {
        fc = 20;
    }
    else if (fc > 20000)
    {
        fc = 20000;
    }

    if (gain <= -12)
    {
        gain = -12;
    }
    else if (gain >= 6)
    {
        gain = 6;
    }

    t0 = tan(PI * fc / fs);

    tmp0 = t0 * t0;
    tmp1 = 1 + SQRT_2 * t0 + tmp0;

    a1 = (1 - tmp0) / tmp1;

    den[0] = 1;
    den[1] = -2 * a1;
    den[2] = (1 + tmp0 - SQRT_2 * t0) / tmp1;

    num[0] = gain * tmp0 / tmp1;
    num[1] = 2 * num[0];
    num[2] = num[0];

    max = fabs(num[1]);

    if (max >= 2)
    {
        v = max;

        if (num[0] > 0)
        {
            num[0] = 0.5;
            num[1] = 1;
            num[2] = 0.5;
        }
        else
        {
            num[0] = -0.5;
            num[1] = -1;
            num[2] = -0.5;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_high_pass_filter(double gain, uint32_t fc, double n, uint32_t fs, int32_t *result,
                                   double *linear_global_gain)
{
    double tmp0, tmp1;
    int32_t i;
    double v = 1, max = 0, abs_val;
    double den[3];
    double num[3];

    //parameter limited to valid range
    if (fc < 20)
    {
        fc = 20;
    }
    else if (fc > 20000)
    {
        fc = 20000;
    }

    tmp0 = tan(PI * fc / fs);
    tmp1 = 1 + SQRT_2 * tmp0 + tmp0 * tmp0;

    den[0] = 1;
    den[1] = (2 * tmp0 * tmp0 - 2) / tmp1;
    den[2] = (1 + tmp0 * tmp0 - SQRT_2 * tmp0) / tmp1;

    num[0] = gain / tmp1;
    num[1] = -2 * num[0];
    num[2] = num[0];

    max = fabs(num[1]);

    if (max >= 2)
    {
        v = max;

        if (num[0] > 0)
        {
            num[0] = 0.5;
            num[1] = -1;
            num[2] = 0.5;
        }
        else
        {
            num[0] = -0.5;
            num[1] = 1;
            num[2] = -0.5;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_shelving_lp_filter(double gain, uint32_t fc, double s, uint32_t fs,
                                     int32_t *result, double *linear_global_gain)
{
    double t0;
    double alpha;
    double a, b0, b1, b2, a0, a1, a2;
    double v = 1, max = 0, abs_val;
    double cos_t0, sqrt_a;
    double den[3];
    double num[3];
    uint8_t i;
    double a_plus_1, a_minus_1, tmp0;

    t0 = 2 * PI * fc / fs;

    a = sqrt(gain);
    a_plus_1 = a + 1;
    a_minus_1 = a - 1;

    cos_t0 = cos(t0);
    sqrt_a = sqrt(a);

    alpha = sin(t0) / 2 * sqrt((a + 1 / a) * (1 / s - 1) + 2);
    tmp0 = 2 * sqrt_a * alpha;

    b0 = a * (a_plus_1 - a_minus_1 * cos_t0 + tmp0);
    b1 = 2 * a * (a_minus_1 - a_plus_1 * cos_t0);
    b2 = a * (a_plus_1 - a_minus_1 * cos_t0 - tmp0);
    a0 = a_plus_1 + a_minus_1 * cos_t0 + tmp0;
    a1 = -2 * (a_minus_1 + a_plus_1 * cos_t0);
    a2 = a_plus_1 + a_minus_1 * cos_t0 - tmp0;

    den[2] = a2 / a0;
    den[1] = a1 / a0;
    den[0] = 1;
    num[0] = b0 / a0;
    num[1] = b1 / a0;
    num[2] = b2 / a0;

    max = fabs(num[0]);
    for (i = 1; i <= 2; i++)
    {
        abs_val = fabs(num[i]);
        if (abs_val > max)
        {
            max = abs_val;
        }
    }

    if (max >= 2)
    {
        v = max;
        for (i = 0; i < 3; i++)
        {
            num[i] = num[i] / v;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_shelving_hp_filter(double gain, uint32_t fc, double s, uint32_t fs,
                                     int32_t *result, double *linear_global_gain)
{
    double t0;
    double alpha;
    double a, b0, b1, b2, a0, a1, a2;
    double v = 1, max = 0, abs_val;
    double cos_t0, sqrt_a, a_plus_1, a_minus_1;
    double den[3];
    double num[3];
    uint8_t i;
    double tmp0, tmp1, tmp2;

    t0 = 2 * PI * fc / fs;

    a = sqrt(gain);
    cos_t0 = cos(t0);
    sqrt_a = sqrt(a);
    a_plus_1 = a + 1;
    a_minus_1 = a - 1;

    alpha = sin(t0) / 2 * sqrt((a + 1 / a) * (1 / s - 1) + 2);
    tmp0 = 2 * sqrt_a * alpha;
    tmp1 = a_minus_1 * cos_t0;
    tmp2 = a_plus_1 * cos_t0;

    b0 = a * (a_plus_1 + tmp1 + tmp0);
    b1 = -2 * a * (a_minus_1 + tmp2);
    b2 = a * (a_plus_1 + tmp1 - tmp0);
    a0 = a_plus_1 - tmp1 + tmp0;
    a1 = 2 * (a_minus_1 - tmp2);
    a2 = a_plus_1 - tmp1 - tmp0;

    den[2] = a2 / a0;
    den[1] = a1 / a0;
    den[0] = 1;
    num[0] = b0 / a0;
    num[1] = b1 / a0;
    num[2] = b2 / a0;

    max = fabs(num[0]);
    for (i = 1; i <= 2; i++)
    {
        abs_val = fabs(num[i]);
        if (abs_val > max)
        {
            max = abs_val;
        }
    }

    if (max >= 2)
    {
        v = max;
        for (i = 0; i < 3; i++)
        {
            num[i] = num[i] / v;
        }
    }

    *linear_global_gain = *linear_global_gain * v;
    result[0] = app_eq_coeff_signed_quant(num[0], 32, 30);
    result[1] = app_eq_coeff_signed_quant(num[1], 32, 30);
    result[2] = app_eq_coeff_signed_quant(num[2], 32, 30);
    result[3] = app_eq_coeff_signed_quant(den[1], 32, 30);
    result[4] = app_eq_coeff_signed_quant(den[2], 32, 30);
}

void app_eq_coeff_check_para(int32_t stage_num, double *global_gain, uint32_t *freq, double *gain,
                             double *q, T_EQ_FILTER_TYPE *biquad_type)
{
    bool is_change_value = false;

    if (*global_gain > MAX_GAIN)
    {
        *global_gain = MAX_GAIN;
        is_change_value = true;
    }
    else if (*global_gain < MIN_GAIN)
    {
        *global_gain = MIN_GAIN;
        is_change_value = true;
    }

    for (uint8_t i = 0; i < stage_num; i++)
    {
        if (gain[i] > MAX_GAIN)
        {
            gain[i] = MAX_GAIN;
            is_change_value = true;
        }
        else if (gain[i] < MIN_GAIN)
        {
            gain[i] = MIN_GAIN;
            is_change_value = true;
        }

        if (freq[i] > MAX_FREQ)
        {
            freq[i] = MAX_FREQ;
            is_change_value = true;
        }
        else if (freq[i] < MIN_FREQ)
        {
            freq[i] = MIN_FREQ;
            is_change_value = true;
        }

        if ((q[i] != EQ_FILTER_LOW_PASS) && (q[i] != EQ_FILTER_HIGH_PASS))
        {
            if (q[i] > MAX_Q)
            {
                q[i] = MAX_Q;
                is_change_value = true;
            }
            else if (q[i] < MIN_Q)
            {
                q[i] = MIN_Q;
                is_change_value = true;
            }
        }
    }

    if (is_change_value)
    {
        APP_PRINT_TRACE0("app_eq_coeff_check_para: EQ para been modified due to invalid");
    }
}

void app_eq_coeff_calculate(int32_t stage_num, double global_gain, uint32_t fs, uint32_t *freq,
                            double *gain, double *q, T_EQ_FILTER_TYPE *biquad_type, T_EQ_COEFF *result)
{
    int32_t i;

    for (i = 0; i < stage_num; i++)
    {
        switch (biquad_type[i])
        {
        //for different type EQ Filter
        case EQ_FILTER_PEAK:
            app_eq_coeff_peak_filter(gain[i], freq[i], q[i], fs, result->coeff[i], &global_gain);
            break;
        case EQ_FILTER_LOW_PASS:
            app_eq_coeff_low_pass_filter(gain[i], freq[i], 2, fs, result->coeff[i], &global_gain);
            break;
        case EQ_FILTER_HIGH_PASS:
            app_eq_coeff_high_pass_filter(gain[i], freq[i], 2, fs, result->coeff[i], &global_gain);
            break;
        case EQ_FILTER_SHELVING_LP:
            app_eq_coeff_shelving_lp_filter(gain[i], freq[i], q[i], fs, result->coeff[i], &global_gain);
            break;
        case EQ_FILTER_SHELVING_HP:
            app_eq_coeff_shelving_hp_filter(gain[i], freq[i], q[i], fs, result->coeff[i], &global_gain);
            break;
        case EQ_FILTER_PEAK2:
            app_eq_coeff_peak2_filter(gain[i], freq[i], q[i], fs, result->coeff[i], &global_gain);
            break;
        default:
            app_eq_coeff_peak_filter(gain[i], freq[i], q[i], fs, result->coeff[i], &global_gain);
            break;
        }
    }

    memset(&(result->sdk_cmd_hdr), 0, sizeof(T_EQ_MCU_TO_SDK_CMD_HDR));
    result->sdk_cmd_hdr.id = EQ_MCU_TO_SDK_SPK_EQ_PARAM;
    result->gain = app_eq_coeff_signed_quant(global_gain, 24, 12);
    result->guad_bit = 2;
    result->stage = stage_num;
    result->coeff_len = 8 + stage_num * 20;
}
#endif
