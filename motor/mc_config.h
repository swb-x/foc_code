#ifndef __MC_CONFIG_H__
#define __MC_CONFIG_H__

#include "main.h"
#include "foc_math.h"   /* observer_state_t —— mc_state_t 依赖它 */

/*==========================================================================
 * 运行模式
 *==========================================================================*/
typedef enum {
    FOC_MODE_IDLE = 0,   /* PWM 关闭，仅遥测 */
    FOC_MODE_OPENLOOP,   /* 开环相位拖转：电流闭环 + 开环相位，观测器并行跟踪 */
    FOC_MODE_SPEED       /* 速度闭环：观测器相位（低速时自动回退开环拖转） */
} foc_mode_t;

/*==========================================================================
 * 电机与控制参数（启动时由 MC_LoadDefaults 填充，电机参数必须按实测修改）
 *==========================================================================*/
typedef struct {
    /* 电机参数 —— 占位值！必须实测或查规格书后修改 */
    float R;                 /* 相电阻 Ω */
    float L;                 /* 相电感 H */
    float lambda;            /* 磁链 Wb */
    int   pole_pairs;        /* 极对数 */

    /* 观测器 / PLL */
    float pll_kp;
    float pll_ki;
    float obs_switch_erpm;   /* 开环 → 观测器 切换阈值（电气 rpm） */

    /* 电流环 PI：kp = L*bw，ki = R*bw（bw 为带宽 rad/s） */
    float cur_kp;
    float cur_ki;

    /* 速度环 PI（输出 iq_set，单位 A） */
    float spd_kp;
    float spd_ki;

    /* 限幅 */
    float i_max;             /* 最大相电流 A */
    float duty_max;          /* 最大占空比（低侧采样需留窗口，≤0.95） */

    /* 采样标定 */
    float i_scale;           /* A/LSB = 3.3/4095/(GAIN=10 × 5mΩ) */
    float vbus_scale;        /* V/LSB = 3.3/4095 × 11（分压 10K/1K） */
    float i_offset[3];       /* 失调校准得到，理论值≈2048 */
} mc_cfg_t;

/*==========================================================================
 * 运行时状态
 * volatile：中断（20kHz FOC）与任务（速度环/串口）共享的字段
 *==========================================================================*/
typedef struct {
    volatile foc_mode_t mode;
    volatile bool       fault;

    /* 采样与变换（ISR 内部） */
    float iu, iv, iw;        /* 三相电流 A */
    float vbus;              /* 母线电压 V */
    float i_alpha, i_beta;
    float id, iq;
    float vd, vq;
    float vd_int, vq_int;    /* 电流环积分项 */
    float v_alpha, v_beta;   /* 本拍指令电压（观测器重构用） */

    /* 观测器 / PLL（ISR 内部） */
    observer_state_t obs;
    float obs_phase;         /* 观测器电角度 rad */
    float pll_phase;
    float pll_speed;         /* 电气角速度 rad/s */

    /* 本拍实际使用的电角度（观测器 or 开环） */
    float phase;

    /* 给定（任务写，ISR 读） */
    volatile float id_set;
    volatile float iq_set;
    volatile float spd_set_rpm;   /* 机械 rpm */
    float spd_int;           /* 速度环积分项 */

    /* 开环拖转 */
    float ol_phase;
    float ol_speed;          /* 电气 rad/s */
    volatile bool use_observer;
} mc_state_t;

extern mc_cfg_t   g_mc_cfg;
extern mc_state_t g_mc_state;

void MC_LoadDefaults(void);
void MC_CalibrateOffsets(void);   /* DRV8353 CAL 失调校准，任务上下文调用 */

#endif
