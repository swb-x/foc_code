#include "foc_math.h"

/*==========================================================================
 * 工具函数
 *==========================================================================*/
float foc_norm_angle(float angle)
{
    float a = fmodf(angle, FOC_2PI);
    if (a < 0.0f) {
        a += FOC_2PI;
    }
    return a;
}

float foc_norm_angle_pi(float angle)
{
    float a = foc_norm_angle(angle);
    if (a > FOC_PI) {
        a -= FOC_2PI;
    }
    return a;
}

float foc_clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void foc_truncate_number_abs(float *v, float max_abs)
{
    if (*v > max_abs)       *v = max_abs;
    else if (*v < -max_abs) *v = -max_abs;
}

void foc_truncate_number_int(int32_t *v, int32_t lo, int32_t hi)
{
    if (*v < lo)      *v = lo;
    else if (*v > hi) *v = hi;
}

/*==========================================================================
 * 坐标变换
 *==========================================================================*/

/* Clarke：iu + iv + iw = 0 前提下的等幅值变换 */
void foc_clarke(float iu, float iv, float *i_alpha, float *i_beta)
{
    *i_alpha = iu;
    *i_beta  = ONE_BY_SQRT3 * (iu + 2.0f * iv);
}

void foc_park(float i_alpha, float i_beta, float phase, float *id, float *iq)
{
    float s = sinf(phase);
    float c = cosf(phase);
    *id =  i_alpha * c + i_beta * s;
    *iq = -i_alpha * s + i_beta * c;
}

void foc_inv_park(float vd, float vq, float phase, float *v_alpha, float *v_beta)
{
    float s = sinf(phase);
    float c = cosf(phase);
    *v_alpha = vd * c - vq * s;
    *v_beta  = vd * s + vq * c;
}

/*==========================================================================
 * 磁链观测器（VESC MxLemming 基础版）
 * 对 (v - R*i) 积分并扣掉 L*di，得到磁链；幅值钳到 lambda 提供收敛性。
 * 输入电压用上一拍实际施加的指令电压重构。
 *==========================================================================*/
void foc_observer_update(float v_alpha, float v_beta, float i_alpha, float i_beta,
                         float R, float L, float lambda, float dt,
                         observer_state_t *state, float *phase)
{
    state->x1 += (v_alpha - R * i_alpha) * dt - L * (i_alpha - state->i_alpha_last);
    state->x2 += (v_beta  - R * i_beta)  * dt - L * (i_beta  - state->i_beta_last);

    state->i_alpha_last = i_alpha;
    state->i_beta_last  = i_beta;

    UTILS_NAN_ZERO(state->x1);
    UTILS_NAN_ZERO(state->x2);

    /* 磁链幅值不应超过 lambda，越界就拉回（观测器的收敛机制） */
    foc_truncate_number_abs(&state->x1, lambda);
    foc_truncate_number_abs(&state->x2, lambda);

    /* 幅值太低时角度会发散，轻微放大保持数值健康 */
    float mag = NORM2_f(state->x1, state->x2);
    if (mag < (lambda * 0.5f)) {
        state->x1 *= 1.1f;
        state->x2 *= 1.1f;
    }

    if (phase) {
        *phase = foc_norm_angle(atan2f(state->x2, state->x1));
    }
}

/*==========================================================================
 * PLL：跟踪观测器相位，输出平滑的电气角速度
 *==========================================================================*/
void foc_pll_run(float phase, float kp, float ki, float dt,
                 float *pll_phase, float *pll_speed)
{
    UTILS_NAN_ZERO(*pll_phase);
    float delta_theta = foc_norm_angle_pi(phase - *pll_phase);
    UTILS_NAN_ZERO(*pll_speed);
    *pll_phase += (*pll_speed + kp * delta_theta) * dt;
    *pll_phase = foc_norm_angle(*pll_phase);
    *pll_speed += ki * delta_theta * dt;
}

/*==========================================================================
 * SVM：min-max 零序注入法，与 VESC 扇区法等价但更短
 * v_alpha/v_beta 单位伏特；输出 CCR 值（0 ~ arr+1）
 *==========================================================================*/
void foc_svm(float v_alpha, float v_beta, float vbus, float duty_max,
             uint32_t arr, uint32_t *du, uint32_t *dv, uint32_t *dw)
{
    /* 反 Clarke 得到三相相电压 */
    float vu = v_alpha;
    float vv = -0.5f * v_alpha + SQRT3_BY_2 * v_beta;
    float vw = -0.5f * v_alpha - SQRT3_BY_2 * v_beta;

    /* 零序注入：把三相整体平移，扩大线性调制范围到电压圆 vbus/√3 */
    float vmax = fmaxf(fmaxf(vu, vv), vw);
    float vmin = fminf(fminf(vu, vv), vw);
    float vcom = -0.5f * (vmax + vmin);

    float duty_min = 1.0f - duty_max;   /* 低侧采样需要最小导通窗口 */
    float duty_u = foc_clamp(0.5f + (vu + vcom) / vbus, duty_min, duty_max);
    float duty_v = foc_clamp(0.5f + (vv + vcom) / vbus, duty_min, duty_max);
    float duty_w = foc_clamp(0.5f + (vw + vcom) / vbus, duty_min, duty_max);

    *du = (uint32_t)(duty_u * (float)(arr + 1U));
    *dv = (uint32_t)(duty_v * (float)(arr + 1U));
    *dw = (uint32_t)(duty_w * (float)(arr + 1U));
}
