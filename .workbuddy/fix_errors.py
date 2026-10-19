# -*- coding: utf-8 -*-
"""修复 Keil 3 个编译错误（保持 GBK 编码，保持原文件行尾风格）。
   行尾无关：先归一成 LF 做替换，再还原为原主导行尾。"""
import re

FOC   = r"C:\Users\a2025625\Desktop\Foc\motor\mcpwm_foc.c"
AFR   = r"C:\Users\a2025625\Desktop\Foc\Core\Src\app_freertos.c"
AADCH = r"C:\Users\a2025625\Desktop\Foc\Core\Inc\app_adc.h"
AADCC = r"C:\Users\a2025625\Desktop\Foc\Core\Src\app_adc.c"


def load(p):
    raw = open(p, "rb").read()
    txt = raw.decode("gbk")
    crlf = raw.count(b"\r\n")
    lf = raw.count(b"\n") - crlf
    eol = "\r\n" if crlf > lf else "\n"
    return txt.replace("\r\n", "\n"), eol


def save(p, txt, eol):
    open(p, "wb").write(txt.replace("\n", eol).encode("gbk"))


# ---------- 1) app_adc.h: 补 ADC1_Current_Start 原型 ----------
t, eol = load(AADCH)
old = "void App_ADC2_ConvCplt(void);"
new = ("void App_ADC2_ConvCplt(void);\n\n"
       "/* ADC1 电流采样启动：校准 + 使能 + DMA1_Ch1 循环搬运 + TIM1 触发\n"
       "   buf 需为 4 个半字（U, V, W, Vbus），任务上下文调用，只调一次 */\n"
       "void ADC1_Current_Start(uint16_t *buf, uint32_t len);")
assert t.count(old) == 1, "app_adc.h anchor count=%d" % t.count(old)
save(AADCH, t.replace(old, new), eol)
print("[ok] app_adc.h: added ADC1_Current_Start prototype")

# ---------- 2) mcpwm_foc.c: include app_adc.h ----------
t, eol = load(FOC)
old = '#include "mcpwm_foc.h"\n'
new = '#include "mcpwm_foc.h"\n#include "app_adc.h"   /* ADC1_Current_Start */\n'
assert t.count(old) == 1, "mcpwm_foc.c anchor count=%d" % t.count(old)
save(FOC, t.replace(old, new, 1), eol)
print("[ok] mcpwm_foc.c: include app_adc.h")

# ---------- 3) app_freertos.c: 补回 MotorTask 声明 ----------
t, eol = load(AFR)
old = "};\n/* USER CODE END FunctionPrototypes */"
new = ("};\n"
       "osThreadId_t MotorTaskHandle;\n"
       "const osThreadAttr_t MotorTask_attributes = {\n"
       "  .name = \"MotorTask\",\n"
       "  .priority = (osPriority_t) osPriorityAboveNormal,\n"
       "  .stack_size = 256 * 4\n"
       "};\n"
       "/* USER CODE END FunctionPrototypes */")
assert t.count(old) == 1, "app_freertos.c anchor count=%d" % t.count(old)
save(AFR, t.replace(old, new, 1), eol)
print("[ok] app_freertos.c: restored MotorTaskHandle / MotorTask_attributes")

# ---------- 4) app_adc.c: 补 DMA TC 中断使能 ----------
t, eol = load(AADCC)
old = "    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);"
new = ("    /* 使能传输完成中断：FOC_ControlLoop() 靠它每 50us 触发一次，\n"
       "       不开的话 NVIC 永远不进，电流环整个停摆 */\n"
       "    LL_DMA_ClearFlag_TC1(DMA1);\n"
       "    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);\n"
       "    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);")
assert t.count(old) == 1, "app_adc.c anchor count=%d" % t.count(old)
save(AADCC, t.replace(old, new, 1), eol)
print("[ok] app_adc.c: enabled DMA1_Ch1 TC interrupt")
