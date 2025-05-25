#include "ESC.h"
#include "OLED.h"

UnenergizedPhase phase = All;        // 未通电的相
PhaseZeroStatus zerostatus = U_H;    // 过零点时的状态
uint16_t changeable_ccr = 100;       // PWM占空比
uint8_t OpenloopStart_Over_Flag = 0; // 开环启动结束标志
TimeAndSpeedData t_v_Data;           // 时间和速度结构体变量
float expectspeed;                   // 期望速度
uint8_t FirstChange_Flag = 0;        // 第一次换向标志
uint8_t Change_Flag = 0;             // 换向标志

/**
 * @brief 改变占空比
 * @param ccr
 */
void ChangeCCR(uint16_t ccr)
{
    changeable_ccr = ccr;
}

void ChangeBasedOnZerostatus(PhaseZeroStatus status)
{
    switch (status)
    {
    case U_H:
        phase = ChangeStatus(U_W, changeable_ccr);
        break;
    case U_L:
        phase = ChangeStatus(W_U, changeable_ccr);
        break;
    case V_H:
        phase = ChangeStatus(V_U, changeable_ccr);
        break;
    case V_L:
        phase = ChangeStatus(U_V, changeable_ccr);
        break;
    case W_H:
        phase = ChangeStatus(W_V, changeable_ccr);
        break;
    case W_L:
        phase = ChangeStatus(V_W, changeable_ccr);
        break;
    }
}

uint8_t MatchDetect(void)
{
    if ((uint8_t)zerostatus % 3 == (uint8_t)phase)
    {
        return 1;
    }
    else if ((uint8_t)zerostatus % 3 == (((uint8_t)phase == 0 ? 2 : (uint8_t)phase - 1)))
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

int timecontrol(int time)
{
    float angle = atan(246.09142 / time) * 180.0 / 3.14159;
    if (angle < 30)
    {
        return (30 - angle) * time / 60.0;
    }
    else
    {
        return 0;
    }
}

void StatusDetectAndProcess(void)
{
    if (__HAL_TIM_GET_COUNTER(&htim3) >= timecontrol(t_v_Data.filtertime))
    {
        if (Change_Flag == 1)
        {
            ChangeBasedOnZerostatus(zerostatus);
            Change_Flag = 0;
        }
    }
}

PhaseZeroStatus zerostatuscheck(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == UOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
        {
            return U_L;
        }
        else if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_RESET)
        {
            return U_H;
        }
    }
    else if (GPIO_Pin == VOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_RESET)
        {
            return V_L;
        }
        else if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
        {
            return V_H;
        }
    }
    else if (GPIO_Pin == WOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET)
        {
            return W_L;
        }
        else if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_RESET)
        {
            return W_H;
        }
    }
    else
    {
        zerostatuscheck(GPIO_Pin);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == UOUT_Pin && OpenloopStart_Over_Flag == 1)
    {
        if (FirstChange_Flag == 1)
        {
            zerostatus = zerostatuscheck(GPIO_Pin);
            FirstChange_Flag = 0;
            __HAL_TIM_SET_COUNTER(&htim3, 0);
            ChangeBasedOnZerostatus(zerostatus);
        }
        else
        {
            PhaseZeroStatus zerostatuslast = zerostatus;
            zerostatus = zerostatuscheck(GPIO_Pin);
            if ((uint8_t)zerostatus - (uint8_t)zerostatuslast == 1 || (uint8_t)zerostatus - (uint8_t)zerostatuslast == -5)
            {
                t_v_Data = SpeedDetection(); // 获取时间和速度数据
                if (Change_Flag = 1)
                {
                    ChangeBasedOnZerostatus(zerostatuslast);
                }
                else
                {
                    Change_Flag = 1;
                }
            }
            else if (zerostatus != zerostatuslast)
            {
                __HAL_TIM_SET_COUNTER(&htim3, 0);
                ChangeBasedOnZerostatus(zerostatus);
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(UOUT_Pin);
    }
    else if (GPIO_Pin == VOUT_Pin && OpenloopStart_Over_Flag == 1)
    {
        if (FirstChange_Flag == 1)
        {
            zerostatus = zerostatuscheck(GPIO_Pin);
            FirstChange_Flag = 0;
            __HAL_TIM_SET_COUNTER(&htim3, 0);
            ChangeBasedOnZerostatus(zerostatus);
        }
        else
        {
            PhaseZeroStatus zerostatuslast = zerostatus;
            zerostatus = zerostatuscheck(GPIO_Pin);
            if ((uint8_t)zerostatus - (uint8_t)zerostatuslast == 1 || (uint8_t)zerostatus - (uint8_t)zerostatuslast == -5)
            {
                t_v_Data = SpeedDetection(); // 获取时间和速度数据
                if (Change_Flag = 1)
                {
                    ChangeBasedOnZerostatus(zerostatuslast);
                }
                else
                {
                    Change_Flag = 1;
                }
            }
            else if (zerostatus != zerostatuslast)
            {
                __HAL_TIM_SET_COUNTER(&htim3, 0);
                ChangeBasedOnZerostatus(zerostatus);
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(VOUT_Pin);
    }
    else if (GPIO_Pin == WOUT_Pin && OpenloopStart_Over_Flag == 1)
    {
        if (FirstChange_Flag == 1)
        {
            zerostatus = zerostatuscheck(GPIO_Pin);
            FirstChange_Flag = 0;
            __HAL_TIM_SET_COUNTER(&htim3, 0);
            ChangeBasedOnZerostatus(zerostatus);
        }
        else
        {
            PhaseZeroStatus zerostatuslast = zerostatus;
            zerostatus = zerostatuscheck(GPIO_Pin);
            if ((uint8_t)zerostatus - (uint8_t)zerostatuslast == 1 || (uint8_t)zerostatus - (uint8_t)zerostatuslast == -5)
            {
                t_v_Data = SpeedDetection(); // 获取时间和速度数据
                if (Change_Flag = 1)
                {
                    ChangeBasedOnZerostatus(zerostatuslast);
                }
                else
                {
                    Change_Flag = 1;
                }
            }
            else if (zerostatus != zerostatuslast)
            {
                __HAL_TIM_SET_COUNTER(&htim3, 0);
                ChangeBasedOnZerostatus(zerostatus);
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(WOUT_Pin);
    }
}

/**
 * @brief 占空比置零，低mos管关闭
 */
void Initialization(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
    HAL_GPIO_WritePin(LIN_U_GPIO_Port, LIN_U_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIN_V_GPIO_Port, LIN_V_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIN_W_GPIO_Port, LIN_W_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 更新状态
 * @param status 换向状态
 * @param ccr PWM占空比
 * @return 未通电的相
 */
UnenergizedPhase ChangeStatus(ReversingStatus status, uint16_t ccr)
{
    switch (status)
    {
    case U_V:
        Initialization();                                            // 初始化
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);           // 设置PWM占空比
        HAL_GPIO_WritePin(LIN_V_GPIO_Port, LIN_V_Pin, GPIO_PIN_SET); // 打开下NMOS管
        return W;                                                    // 返回未通电的相
    case U_W:
        Initialization();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
        HAL_GPIO_WritePin(LIN_W_GPIO_Port, LIN_W_Pin, GPIO_PIN_SET);
        return V;
    case V_W:
        Initialization();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, ccr);
        HAL_GPIO_WritePin(LIN_W_GPIO_Port, LIN_W_Pin, GPIO_PIN_SET);
        return U;
    case V_U:
        Initialization();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, ccr);
        HAL_GPIO_WritePin(LIN_U_GPIO_Port, LIN_U_Pin, GPIO_PIN_SET);
        return W;
    case W_U:
        Initialization();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr);
        HAL_GPIO_WritePin(LIN_U_GPIO_Port, LIN_U_Pin, GPIO_PIN_SET);
        return V;
    case W_V:
        Initialization();
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr);
        HAL_GPIO_WritePin(LIN_V_GPIO_Port, LIN_V_Pin, GPIO_PIN_SET);
        return U;
    }
}

/**
 * @brief 反电动势检测，检测到反电动势稳定时，退出开环启动
 * @param zerostatus 过零点状态
 * @return 0---反电动势稳定   1---反电动势不稳定
 */
uint8_t BackEMFdetection(PhaseZeroStatus zerostatus)
{
    static uint8_t num = 0;
    static PhaseZeroStatus status_last = U_H;
    static PhaseZeroStatus status_now = U_H;
    status_now = zerostatus;

    if ((uint8_t)status_now - (uint8_t)status_last == 1 || (uint8_t)status_now - (uint8_t)status_last == -5 || (uint8_t)status_now - (uint8_t)status_last == -1 || (uint8_t)status_now - (uint8_t)status_last == 5)
    {
        num++;
    }
    else if ((uint8_t)status_now - (uint8_t)status_last != 0)
    {
        num = 0;
    }

    status_last = status_now;
    OLED_ShowNum(1, 1, num, 2);
    OLED_ShowNum(1, 8, (uint8_t)status_now, 2);

    if (num > 6)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief 开环启动
 */
void Openloop_Start(void)
{
    int Openloopstatus = 0;
    int time = 6000;

    phase = ChangeStatus(U_W, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000);

    phase = ChangeStatus(W_V, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000);

    phase = ChangeStatus(U_V, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000);

    Initialization();
    HAL_Delay(500);

    for (int i = 0; i < 10; i++)
    {
        phase = ChangeStatus((ReversingStatus)(Openloopstatus % 6), 60);
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        while (__HAL_TIM_GET_COUNTER(&htim3) < time);
        Openloopstatus++;
        if (time > 800)
        {
            time -= 600;
        }
    }
    Initialization();
    OpenloopStart_Over_Flag = 1;
    FirstChange_Flag = 1;
    phase = All;
    HAL_TIM_Base_Stop(&htim3);
    htim3.Init.Prescaler = 71;
    HAL_TIM_Base_Init(&htim3);
    HAL_TIM_Base_Start(&htim3);
}

TimeAndSpeedData SpeedDetection(void)
{
    TimeAndSpeedData data;
    data.tmp = data.time;
    data.time = __HAL_TIM_GET_COUNTER(&htim3);
    if (data.tmp != 0)
    {
        data.filtertime = 0.7 * data.tmp + 0.3 * data.time;
    }
    else
    {
        data.filtertime = data.time;
    }
    data.speed = 60.0 / 7.0 / 360.0 / (data.filtertime * 0.000001);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    return data;
}

void BlockedDetection(void)
{
    if ((__HAL_TIM_GET_COUNTER(&htim3) > 10000) && OpenloopStart_Over_Flag == 1)
    {
        Initialization();
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
        HAL_TIM_Base_Stop(&htim3);
    }
}

void ESC_Init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    Initialization();
}
