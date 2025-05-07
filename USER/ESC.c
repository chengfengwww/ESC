#include "ESC.h"
#include "OLED.h"

volatile int tim3_counts = 0;        // 计时变量
UnenergizedPhase phase = All;        // 未通电的相
PhaseZeroStatus zerostatus = U_H;    // 过零点时的状态
uint16_t changeable_ccr = 40;        // PWM占空比
uint8_t OpenloopStart_Over_Flag = 0; // 开环启动结束标志
TimeAndSpeedData t_v_Data;           // 时间和速度结构体变量
float expectspeed;                   // 期望速度
uint8_t FirstChange_Flag = 0;        // 第一次换向标志
InterruptStateFlag IT_Flag;
int test[2];

/**
 * @brief 定时中断回调函数，执行变量加一的操作
 */
// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
// {
//     if (htim->Instance == TIM3)
//     {
//         tim3_counts++;
//         ClosedLoopCommutation();
//     }
// }

/**
 * @brief 改变占空比
 * @param ccr
 */
void ChangeCCR(uint16_t ccr)
{
    changeable_ccr = ccr;
}

// void Change_IT_Flag(PhaseZeroStatus status)
// {
//     switch (status)
//     {
//     case U_H:
//         IT_Flag.U_H_Flag = 1;
//         break;
//     case U_L:
//         IT_Flag.U_L_Flag = 1;
//         break;
//     case V_H:
//         IT_Flag.V_H_Flag = 1;
//         break;
//     case V_L:
//         IT_Flag.V_L_Flag = 1;
//         break;
//     case W_H:
//         IT_Flag.W_H_Flag = 1;
//         break;
//     case W_L:
//         IT_Flag.W_L_Flag = 1;
//         break;
//     }
// }

// void ClosedLoopCommutation(void)
// {
//     if (IT_Flag.U_H_Flag == 1 && zerostatus == U_H)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(W_U, changeable_ccr);
//             IT_Flag.U_H_Flag = 0;
//         }
//     }
//     else if (IT_Flag.U_H_Flag == 1)
//     {
//         phase = ChangeStatus(W_U, changeable_ccr);
//         IT_Flag.U_H_Flag = 0;
//     }

//     if (IT_Flag.U_L_Flag == 1 && zerostatus == U_L)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(U_W, changeable_ccr);
//             IT_Flag.U_L_Flag = 0;
//         }
//     }
//     else if (IT_Flag.U_L_Flag == 1)
//     {
//         phase = ChangeStatus(U_W, changeable_ccr);
//         IT_Flag.U_L_Flag = 0;
//     }

//     /*******************************************************************/

//     if (IT_Flag.V_H_Flag == 1 && zerostatus == V_H)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(U_V, changeable_ccr);
//             IT_Flag.V_H_Flag = 0;
//         }
//     }
//     else if (IT_Flag.V_H_Flag == 1)
//     {
//         phase = ChangeStatus(U_V, changeable_ccr);
//         IT_Flag.V_H_Flag = 0;
//     }

//     if (IT_Flag.V_L_Flag == 1 && zerostatus == V_L)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(V_U, changeable_ccr);
//             IT_Flag.V_L_Flag = 0;
//         }
//     }
//     else if (IT_Flag.V_L_Flag == 1)
//     {
//         phase = ChangeStatus(V_U, changeable_ccr);
//         IT_Flag.V_L_Flag = 0;
//     }

//     /******************************************************************/

//     if (IT_Flag.W_H_Flag == 1 && zerostatus == W_H)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(V_W, changeable_ccr); // 更新到V_U状态
//             IT_Flag.W_H_Flag = 0;
//         }
//     }
//     else if (IT_Flag.W_H_Flag == 1)
//     {
//         phase = ChangeStatus(V_W, changeable_ccr);
//         IT_Flag.W_H_Flag = 0;
//     }

//     if (IT_Flag.W_L_Flag == 1 && zerostatus == W_L)
//     {
//         if (tim3_counts >= (t_v_Data.time / 2))
//         {
//             phase = ChangeStatus(W_V, changeable_ccr); // 更新到V_U状态
//             IT_Flag.W_L_Flag = 0;
//         }
//     }
//     else if (IT_Flag.W_L_Flag == 1)
//     {
//         phase = ChangeStatus(W_V, changeable_ccr);
//         IT_Flag.W_L_Flag = 0;
//     }
// }

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

void StatusDetectAndProcess(void)
{
    if (MatchDetect() == 1)
    {
        if (__HAL_TIM_GET_COUNTER(&htim3) >= (t_v_Data.filtertime / 2))
        {
            ChangeBasedOnZerostatus(zerostatus);
        }
    }
    else if (MatchDetect() == 0)
    {
        PhaseZeroStatus tmp;
        if (zerostatus == U_L)
        {
            tmp = W_H;
        }
        else
        {
            tmp = (PhaseZeroStatus)((uint8_t)zerostatus - 1);
        }
        ChangeBasedOnZerostatus(tmp);
    }
}

// int8_t LevelDetection(GPIO_TypeDef *GPIO_Port,uint16_t GPIO_Pin)
// {
//     uint8_t counts = 0;
//     for(int i = 0; i < 9; i ++)
//     {
//         counts += (uint8_t)HAL_GPIO_ReadPin(GPIO_Port, GPIO_Pin);
//     }
//     if(counts == 9)
//     {
//         return 1;
//     }
//     else if(counts == 0)
//     {
//         return 0;
//     }
//     return -1;
// }

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     if (GPIO_Pin == UOUT_Pin && (phase == U || phase == W || phase == All) && OpenloopStart_Over_Flag == 1)
//     {
//         if (FirstChange_Flag == 1)
//         {
//             FirstChange_Flag = 0;
//             __HAL_TIM_SET_COUNTER(&htim3, 0);
//             if (LevelDetection(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = U_H;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = U_L;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//         }
//         else
//         {
//             t_v_Data = SpeedDetection();
//             if (LevelDetection(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = U_H;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = U_L;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             test[0] = t_v_Data.time;
//             test[1] = (int)zerostatus;
//         }
//     }
//     if (GPIO_Pin == VOUT_Pin && (phase == V || phase == U || phase == All) && OpenloopStart_Over_Flag == 1)
//     {
//         if (FirstChange_Flag == 1)
//         {
//             FirstChange_Flag = 0;
//             __HAL_TIM_SET_COUNTER(&htim3, 0);
//             if (LevelDetection(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = V_H;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = V_L;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//         }
//         else
//         {
//             t_v_Data = SpeedDetection();
//             if (LevelDetection(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = V_H;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = V_L;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             test[0] = t_v_Data.time;
//             test[1] = (int)zerostatus;
//         }
//     }
//     if (GPIO_Pin == WOUT_Pin && (phase == W || phase == V || phase == All) && OpenloopStart_Over_Flag == 1)
//     {
//         if (FirstChange_Flag == 1)
//         {
//             FirstChange_Flag = 0;
//             __HAL_TIM_SET_COUNTER(&htim3, 0);
//             if (LevelDetection(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = W_H;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = W_L;
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//         }
//         else
//         {
//             t_v_Data = SpeedDetection();
//             if (LevelDetection(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
//             {
//                 zerostatus = W_H;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             else
//             {
//                 zerostatus = W_L;
//                 while(__HAL_TIM_GET_COUNTER(&htim3) < (t_v_Data.time / 2));
//                 ChangeBasedOnZerostatus(zerostatus);
//             }
//             test[0] = t_v_Data.time;
//             test[1] = (int)zerostatus;
//         }
//     }
// }

int PeakClean(PhaseZeroStatus statuslast, PhaseZeroStatus statusnow)
{
    PhaseZeroStatus statusopposite;
    if ((uint8_t)statuslast - 3 >= 0)
    {
        statusopposite = (PhaseZeroStatus)((uint8_t)statuslast - 3);
    }
    else
    {
        statusopposite = (PhaseZeroStatus)((uint8_t)statuslast + 3);
    }
    if (statusopposite == statusnow)
    {
        if(__HAL_TIM_GET_COUNTER(&htim3) < 100)
        {
            int time = 0;
            time = t_v_Data.time;
            time += __HAL_TIM_GET_COUNTER(&htim3);
            __HAL_TIM_SET_COUNTER(&htim3, time);
            t_v_Data.time = t_v_Data.tmp;
            zerostatus = (PhaseZeroStatus)((uint8_t)statuslast - 1 >= 0 ? (uint8_t)statuslast - 1 : 5);
            return -1;
        }
        else
        {
            return -1;
        }
    }
    else if((uint8_t)statusnow - (uint8_t)statuslast == 1 || (uint8_t)statusnow - (uint8_t)statuslast == -5)
    {
        return 0;
    }
}

/**
 * @brief EXTI外部中断回调函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    
    // U相过零点，并且U相未通电
    if (GPIO_Pin == UOUT_Pin && (phase == U || phase == W || phase == All) && __HAL_TIM_GET_COUNTER(&htim3) > 100)
    {
        // U相为高电平
        if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET)
        {
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    zerostatus = U_H;
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(U_W, changeable_ccr);
                }
                else if (PeakClean(zerostatus, U_H) == 0)
                {
                    zerostatus = U_H;
                    t_v_Data = SpeedDetection(); // 获取时间和速度数据
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // phase = V;
                    //  Change_IT_Flag(zerostatus);
                    //  while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
            }
        }
        // U相为低电平
        else if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_RESET)
        {
            
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(W_U, changeable_ccr); // 更新到U_V状态
                }
                else if(PeakClean(zerostatus,U_L) == 0) 
                {
                    zerostatus = U_L;
                    t_v_Data = SpeedDetection(); // 获取时间和速度数据
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // Change_IT_Flag(zerostatus);
                    // phase = V;
                    // while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(UOUT_Pin);
    }

    else if (GPIO_Pin == VOUT_Pin && (phase == V || phase == U || phase == All) && __HAL_TIM_GET_COUNTER(&htim3) > 100)
    {
        if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET)
        {
            
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(V_U, changeable_ccr);
                }
                else if(PeakClean(zerostatus,V_H) == 0)
                {
                    zerostatus = V_H;
                    t_v_Data = SpeedDetection();
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // Change_IT_Flag(zerostatus);
                    // phase = W;
                    // while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
            }
        }
        else if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_RESET)
        {
            
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(U_V, changeable_ccr);
                }
                else if(PeakClean(zerostatus,V_L) == 0)
                {
                    zerostatus = V_L;
                    t_v_Data = SpeedDetection();
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // Change_IT_Flag(zerostatus);
                    // phase = W;
                    // while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(VOUT_Pin);
    }
    else if (GPIO_Pin == WOUT_Pin && (phase == W || phase == V || phase == All) && __HAL_TIM_GET_COUNTER(&htim3) > 100)
    {
        if (HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
        { 
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(W_V, changeable_ccr);
                }
                else if(PeakClean(zerostatus,W_H) == 0)
                {
                    zerostatus = W_H;
                    t_v_Data = SpeedDetection();
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // Change_IT_Flag(zerostatus);
                    // phase = U;
                    // while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
            }
        }
        else if (HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_RESET)
        {
            if (OpenloopStart_Over_Flag == 1)
            {
                if (FirstChange_Flag == 1)
                {
                    FirstChange_Flag = 0;
                    __HAL_TIM_SET_COUNTER(&htim3, 0);
                    phase = ChangeStatus(V_W, changeable_ccr);
                }
                else if(PeakClean(zerostatus,W_L) == 0)
                {
                    zerostatus = W_L;
                    t_v_Data = SpeedDetection();
                    test[0] = t_v_Data.time;
                    test[1] = (int)zerostatus;
                    // Change_IT_Flag(zerostatus);
                    // phase = U;
                    // while(tim3_counts < (t_v_Data.time / 2));//延后30度换向
                }
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

    phase = ChangeStatus(U_W, 20);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000)
        ;

    phase = ChangeStatus(W_V, 20);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000)
        ;

    phase = ChangeStatus(U_V, 20);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000)
        ;

    Initialization();
    HAL_Delay(500);

    for (int i = 0; i < 10; i++)
    {
        phase = ChangeStatus((ReversingStatus)(Openloopstatus % 6), 40);
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        while (__HAL_TIM_GET_COUNTER(&htim3) < time)
            ;
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
    // TimeAndSpeedData data;
    // static int timelast,timenow;
    // if(state == TIME_RESET)
    // {
    //     timelast = t_v_Data.tmp;
    //     timenow = t_v_Data.tmp;
    //     t_v_Data.time = t_v_Data.tmp;
    // }
    // else
    // {
    //     timenow =  __HAL_TIM_GET_COUNTER(&htim3);
    //     if(timelast == 0)
    //     {
    //         data.time = timenow;
    //     }
    //     else
    //     {
    //         data.time = 0.3 * timenow + 0.7 * timelast;
    //     }
    //     data.speed = 60.0 / 7.0 / 360.0 / (data.time * 0.00001);
    //     data.tmp = timelast;
    //     timelast = timenow;
    //     __HAL_TIM_SET_COUNTER(&htim3, 0);
    //     return data;
    // }
    TimeAndSpeedData data;
    data.tmp = data.time;
    data.time = __HAL_TIM_GET_COUNTER(&htim3);
    if(data.tmp != 0)
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
    if ((__HAL_TIM_GET_COUNTER(&htim3) - t_v_Data.time > (t_v_Data.time / 10)) && OpenloopStart_Over_Flag == 1)
    {
        Initialization();
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    }
}

int SpeedControl(void)
{
    float Kp = 0;
    float Ki = 0;
    static float erroraccumulate = 0;
    int output = Kp * (expectspeed - t_v_Data.speed) + Ki * erroraccumulate;
    erroraccumulate += expectspeed - t_v_Data.speed;
    return output;
}

void ESC_Init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    Initialization();
}
