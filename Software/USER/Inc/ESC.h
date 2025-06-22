#ifndef __ESC_H_
#define __ESC_H_

#include "main.h"
#include "tim.h"
#include "math.h"

/**
 * @brief 电机六步换向状态
 * @note 定义了电机驱动的六个基本步骤。
 *       例如, U_V 表示U相输出PWM，V相接地。
 */
typedef enum
{
    U_V = 0,
    W_V,
    W_U,
    V_U,
    V_W,
    U_W
}ReversingStatus;

/**
 * @brief 未通电的相
 * @note 用于标识当前哪个相是悬空的，以便在该相上检测反电动势（BEMF）。
 *       All 表示所有相都未通电（电机停转或初始化状态）。
 */
typedef enum
{
    U = 0,
    V,
    W,
    All
}UnenergizedPhase;

/**
 * @brief BEMF过零事件状态
 * @note 描述了在哪个相上检测到了哪种类型的过零事件。
 *       例如, U_L 表示在U相上检测到下降沿过零；V_H 表示在V相上检测到上升沿过零。
 */
typedef enum
{
    U_L = 0,
    V_H,
    W_L,
    U_H,
    V_L,
    W_H
}PhaseZeroStatus;

/**
 * @brief 存储电机速度和时间相关的数据结构
 */
typedef struct
{
    int time;           ///< 两次过零事件之间的原始时间间隔（定时器计数值）
    int filtertime;     ///< 经过滤波处理的平滑时间间隔
    int tmp;            ///< 用于滤波计算的临时变量，存储上一次的时间
    float speed;        ///< 计算出的电机转速 (r/s)
}TimeAndSpeedData;


UnenergizedPhase ChangeStatus(ReversingStatus status, uint16_t ccr);
void BlockedDetection(void);
void Openloop_Start(void);
TimeAndSpeedData SpeedDetection(void);
void ESC_Init(void);
void StatusDetectAndProcess(void);
void ChangeCCR(uint16_t ccr);
PhaseZeroStatus zerostatuscheck(uint16_t GPIO_Pin);

#endif
