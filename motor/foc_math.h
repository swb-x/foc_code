#ifndef __FOC_MATH_H__
#define __FOC_MATH_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*==========================================================================
 * 常量与宏（从 VESC 精简）
 *==========================================================================*/
#define FOC_PI              3.14159265358979f
#define FOC_2PI             6.28318530717959f
#define ONE_BY_SQRT3        0.57735026919f
#define TWO_BY_SQRT3        1.15470053838f
#define SQRT3_BY_2          0.86602540378f

#define SQ(x)               ((x) * (x))
#define NORM2_f(x, y)       (sqrtf(SQ(x) + SQ(y)))

/* NaN 保护：变量为 NaN 时清零（VESC 防御性写法，观测器必需） */
#define UTILS_NAN_ZERO(x)   do { if (isnan(x)) { (x) = 0.0f; } } while (0)

/*==========================================================================
 * 观测器状态（VESC MxLemming 观测器）
 *==========================================================================*/
typedef struct {
    float x1;              /* alpha 轴磁链估计 */
    float x2;              /* beta 轴磁链估计 */
    float i_alpha_last;    /* 上一拍电流（微分项用） */
    float i_beta_last;
} observer_state_t;

/*==========================================================================
 * 工具函数
 *==========================================================================*/
float foc_norm_angle(float angle);        /* 归一化到 [0, 2π) */
float foc_norm_angle_pi(float angle);     /* 归一化到 (-π, π] */
float foc_clamp(float v, float lo, float hi);
void  foc_truncate_number_abs(float *v, float max_abs);
void  foc_truncate_number_int(int32_t *v, int32_t lo, int32_t hi);

/*==========================================================================
 * 坐标变换
 *==========================================================================*/
void  foc_clarke(float iu, float iv, float *i_alpha, float *i_beta);
void  foc_park(float i_alpha, float i_beta, float phase, float *id, float *iq);
void  foc_inv_park(float vd, float vq, float phase, float *v_alpha, float *v_beta);

/*==========================================================================
 * FOC 核心算法（移植自 VESC，去掉温度补偿/凸极/lambda 补偿等非必要分支）
 *==========================================================================*/

/* 磁链观测器：输入指令电压 vα/vβ 和实测电流 iα/iβ，输出电角度 phase([0,2π)) */
void  foc_observer_update(float v_alpha, float v_beta, float i_alpha, float i_beta,
                          float R, float L, float lambda, float dt,
                          observer_state_t *state, float *phase);

/* PLL：从观测器相位提取平滑转速（电气 rad/s） */
void  foc_pll_run(float phase, float kp, float ki, float dt,
                  float *pll_phase, float *pll_speed);

/* SVM：输入 vα/vβ（伏特）和母线电压，输出三路 CCR 值 */
void  foc_svm(float v_alpha, float v_beta, float vbus, float duty_max,
              uint32_t arr, uint32_t *du, uint32_t *dv, uint32_t *dw);

#endif
