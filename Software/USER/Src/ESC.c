#include "ESC.h"

// =====================================================================================================================
// 全局变量定义
// =====================================================================================================================

UnenergizedPhase phase = All;           ///< 当前未通电的相，用于BEMF检测
PhaseZeroStatus zerostatus = U_H;       ///< 当前的过零状态
uint16_t changeable_ccr = 80;           ///< 可变的PWM占空比
uint8_t OpenloopStart_Over_Flag = 0;    ///< 开环启动结束标志, 1:结束
TimeAndSpeedData t_v_Data;              ///< 存储速度和时间相关的数据
uint16_t FirstChange_Flag = 0;           ///< 第一类换向标志, 过零即换向

/**
 * @brief 核心换向状态机标志 (State Machine Flag for Commutation)
 * @note 这是整个闭环控制算法的核心。它使用一个简单的整数来管理复杂的换向逻辑：
 * - 0: IDLE / 等待首次过零点。
 * - 1: DELAY_ARMED / 已捕获过零点，启动延迟计时。在执行换向前，下一个过零点到来时，状态会变为2。
 * - 2: PENDING\COMMUTATION / 等待执行换向，换向后标志置1。
 */
uint8_t Change_Flag = 0;                 ///< 第二类换向标志，用于延迟90度换向


/**
 * @brief  动态改变PWM占空比
 * @param  ccr: 新的占空比值
 * @retval None
 */
void ChangeCCR(uint16_t ccr)
{
    changeable_ccr = ccr;
}

/**
 * @brief  根据当前的过零状态执行正确的换向
 * @param  status: 当前的过零状态 (PhaseZeroStatus)，例如 U_H, V_L 等
 * @retval None
 * @note   该函数是过零点事件到电机六步换向的具体映射。
 *         例如，当检测到U相高电平过零(U_H)时，下一步应该是U相输出PWM，W相接地（U_W）。
 */
void ChangeBasedOnZerostatus(PhaseZeroStatus status)
{
    switch (status)
    {
        case U_H: // U相高电平过零
            phase = ChangeStatus(U_W, changeable_ccr);
            break;
        case U_L: // U相低电平过零
            phase = ChangeStatus(W_U, changeable_ccr);
            break;
        case V_H: // V相高电平过零
            phase = ChangeStatus(V_U, changeable_ccr);
            break;
        case V_L: // V相低电平过零
            phase = ChangeStatus(U_V, changeable_ccr);
            break;
        case W_H: // W相高电平过零
            phase = ChangeStatus(W_V, changeable_ccr);
            break;
        case W_L: // W相低电平过零
            phase = ChangeStatus(V_W, changeable_ccr);
            break;
    }
}


/**
 * @brief  相移补偿函数
 * @param  time: 两次过零点之间的时间间隔 (定时器计数)
 * @retval 延迟的时间
 * @note   根据过零点之间的时间间隔计算出相移补偿时间，延迟 90-相移角 换向
 */
uint16_t timecontrol(uint16_t time)
{
    static float anglelast;
    //参数确定方法：根据rc滤波电路的相移初步计算，然后根据实际情况调整
    float angle = 0.95 * anglelast + 0.05 * atan(530 / time) * 180.0 / 3.14159;
    anglelast = angle;
    return (uint16_t)((90.0f - angle) * (float)time / 60.0f);
}

/**
 * @brief  状态检测和处理
 * @note   此函数由主循环调用，用于处理基于时间的换相逻辑。
 *         它检查一个专用定时器（htim4），当达到`timecontrol`函数计算的延迟时间后，
 *         会根据`Change_Flag`状态机的状态来执行换相。
 *         `Change_Flag`的状态转换由过零中断(`EXTI_Process`)和此函数共同管理，
 *         以实现精确的闭环换相控制。
 */
void StatusDetectAndProcess(void)
{

    if (__HAL_TIM_GET_COUNTER(&htim4) >= timecontrol(t_v_Data.filtertime))
    {
        if (Change_Flag == 2)
        {
            ChangeBasedOnZerostatus(zerostatus);
            Change_Flag = 1;
            __HAL_TIM_SET_COUNTER(&htim4, __HAL_TIM_GET_COUNTER(&htim3));
        }
        else if (Change_Flag == 1)
        {
            ChangeBasedOnZerostatus((PhaseZeroStatus)((uint8_t)zerostatus == 5 ? 0 : (uint8_t)zerostatus + 1));
            Change_Flag = 0;
        }
    }
}

/**
 * @brief  检查并确定当前的过零状态
 * @param  GPIO_Pin: 触发中断的GPIO引脚
 * @retval PhaseZeroStatus: 检测到的过零状态 (e.g., U_H, U_L)
 * @note   当一个相的BEMF过零时，会触发外部中断。此函数在中断服务程序中被调用。
 *         它通过读取另外两个通电相的电平状态，来判断是哪个相发生了过零，以及是上升沿还是下降沿过零。
 *         例如，如果U相引脚触发了中断，并且V相为高电平、W相为低电平，则可以确定是U相的BEMF从低到高过零（U_H）。
 */
PhaseZeroStatus zerostatuscheck(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == UOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin)
            == GPIO_PIN_SET)
        {
            return U_L;
        }
        else if (HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(
                     WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_RESET)
        {
            return U_H;
        }
    }
    else if (GPIO_Pin == VOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(WOUT_GPIO_Port, WOUT_Pin) ==
            GPIO_PIN_RESET)
        {
            return V_L;
        }
        else if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(
                     WOUT_GPIO_Port, WOUT_Pin) == GPIO_PIN_SET)
        {
            return V_H;
        }
    }
    else if (GPIO_Pin == WOUT_Pin)
    {
        if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_RESET && HAL_GPIO_ReadPin(VOUT_GPIO_Port, VOUT_Pin)
            == GPIO_PIN_SET)
        {
            return W_L;
        }
        else if (HAL_GPIO_ReadPin(UOUT_GPIO_Port, UOUT_Pin) == GPIO_PIN_SET && HAL_GPIO_ReadPin(
                     VOUT_GPIO_Port, VOUT_Pin) == GPIO_PIN_RESET)
        {
            return W_H;
        }
    }
    return zerostatus;
}

/**
 * @brief  处理外部中断（过零检测）
 * @param  GPIO_Pin: 触发中断的GPIO引脚
 * @retval None
 * @note   这是过零检测中断的核心处理函数。
 *         - 在从开环到闭环的切换阶段 (`FirstChange_Flag > 0`), 它会立即根据检测到的过零点进行换相，以帮助电机同步。
 *         - 在正常的闭环运行阶段, 它会:
 *           1. 验证过零事件的有效性（是否按预期的顺序发生）。
 *           2. 调用 `SpeedDetection` 来更新电机速度。
 *           3. 管理 `Change_Flag` 状态机，为定时换相(`StatusDetectAndProcess`)做准备。
 *           4. 处理异常的过零事件，例如通过立即换相来纠正错误。
 */
void EXTI_Process(uint16_t GPIO_Pin)
{
    if (FirstChange_Flag > 0)
    {
        zerostatus = zerostatuscheck(GPIO_Pin);
        ChangeBasedOnZerostatus(zerostatus);
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        __HAL_TIM_SET_COUNTER(&htim4, 0);
        if (FirstChange_Flag == 1)
        {
            Change_Flag = 1;
        }
        FirstChange_Flag -= 1;
    }
    else
    {
        PhaseZeroStatus zerostatuslast = zerostatus;
        zerostatus = zerostatuscheck(GPIO_Pin);
        if ((uint8_t) zerostatus - (uint8_t) zerostatuslast == 1 || (uint8_t) zerostatus - (uint8_t) zerostatuslast == -5)
        {
            t_v_Data = SpeedDetection();
            if (Change_Flag == 0)
            {
                __HAL_TIM_SET_COUNTER(&htim4, 0);
                Change_Flag = 1;
            }
            else if (Change_Flag == 1)
            {
                Change_Flag = 2;
            }
            else if (Change_Flag == 2)
            {
                ChangeBasedOnZerostatus(zerostatuslast);
                __HAL_TIM_SET_COUNTER(&htim4, t_v_Data.time);
            }
        }
        else if (zerostatus != zerostatuslast)
        {
            ChangeBasedOnZerostatus(zerostatus);
            __HAL_TIM_SET_COUNTER(&htim3, 0);
            __HAL_TIM_SET_COUNTER(&htim4, 0);
            if(Change_Flag == 0)
            {
                Change_Flag = 1;
            }
        }
    }
}

/**
 * @brief  GPIO外部中断回调函数
 * @param  GPIO_Pin: 触发中断的引脚
 * @retval None
 * @note   这是由STM32 HAL库在检测到外部中断时调用的标准回调函数。
 *         此函数仅在开环启动完成后(`OpenloopStart_Over_Flag == 1`)才处理中断。
 *         它调用 `EXTI_Process` 来执行主要的过零检测逻辑，并清除相应的中断标志位，防止重复触发。
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (OpenloopStart_Over_Flag == 1)
    {
        EXTI_Process(GPIO_Pin);
        
        // 清除中断标志
        if (GPIO_Pin == UOUT_Pin)
            __HAL_GPIO_EXTI_CLEAR_IT(UOUT_Pin);
        else if (GPIO_Pin == VOUT_Pin)
            __HAL_GPIO_EXTI_CLEAR_IT(VOUT_Pin);
        else if (GPIO_Pin == WOUT_Pin)
            __HAL_GPIO_EXTI_CLEAR_IT(WOUT_Pin);
    }
}

/**
 * @brief 占空比置零，低mos管关闭
 */
void Initialization(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
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
            Initialization(); // 初始化
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr); // 设置PWM占空比
            HAL_GPIO_WritePin(LIN_V_GPIO_Port, LIN_V_Pin, GPIO_PIN_SET); // 打开下NMOS管
            return W; // 返回未通电的相
        case U_W:
            Initialization();
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
            HAL_GPIO_WritePin(LIN_W_GPIO_Port, LIN_W_Pin, GPIO_PIN_SET);
            return V;
        case V_W:
            Initialization();
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);
            HAL_GPIO_WritePin(LIN_W_GPIO_Port, LIN_W_Pin, GPIO_PIN_SET);
            return U;
        case V_U:
            Initialization();
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);
            HAL_GPIO_WritePin(LIN_U_GPIO_Port, LIN_U_Pin, GPIO_PIN_SET);
            return W;
        case W_U:
            Initialization();
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr);
            HAL_GPIO_WritePin(LIN_U_GPIO_Port, LIN_U_Pin, GPIO_PIN_SET);
            return V;
        case W_V:
            Initialization();
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr);
            HAL_GPIO_WritePin(LIN_V_GPIO_Port, LIN_V_Pin, GPIO_PIN_SET);
            return U;
        default:
            return All;//非要让我return
    }   
}


/**
 * @brief 开环启动
 */
void Openloop_Start(void)
{
    int Openloopstatus = 0;
    uint32_t time = 6000;

    phase = ChangeStatus(U_W, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000) {};

    phase = ChangeStatus(W_V, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000) {};

    phase = ChangeStatus(U_V, 60);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    while (__HAL_TIM_GET_COUNTER(&htim3) < 10000) {};

    Initialization();
    HAL_Delay(300);

    for (int i = 0; i < 10; i++)
    {
        phase = ChangeStatus((ReversingStatus) (Openloopstatus % 6), 60);
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        while (__HAL_TIM_GET_COUNTER(&htim3) < time) {};
        Openloopstatus++;
        if (time > 800)
        {
            time -= 600;
        }
    }
    Initialization();
    OpenloopStart_Over_Flag = 1;
    FirstChange_Flag = 1000;
    phase = All;
    HAL_TIM_Base_Stop(&htim3);
    htim3.Init.Prescaler = 35;
    HAL_TIM_Base_Init(&htim3);
    HAL_TIM_Base_Start(&htim3);
}

/**
 * @brief  电机速度检测
 * @retval TimeAndSpeedData: 包含时间间隔和速度信息的数据结构
 * @note   该函数在每次有效的过零事件时被调用。
 *         它使用定时器htim3来测量两次连续过零事件之间的时间间隔。
 *         为了得到更稳定的速度值，它对测量到的时间间隔进行了一阶低通滤波。
 *         最后，根据滤波后的时间值计算出电机的转速(r/s)。
 *         计算完毕后，复位htim3计数器，为下一次测量做准备。
 */
TimeAndSpeedData SpeedDetection(void)
{
    static TimeAndSpeedData data;
    data.tmp = data.time;
    data.time = __HAL_TIM_GET_COUNTER(&htim3);
    if (data.tmp != 0)
    {
        data.filtertime = 0.95 * data.tmp + 0.05 * data.time;
    }
    else
    {
        data.filtertime = data.time;
    }
    data.speed = 60.0 / 7.0 / 360.0 / (data.filtertime * 0.0000005);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    return data;
}

/**
 * @brief  电机堵转检测
 * @retval None
 * @note   此函数用于检测电机是否堵转或停转。
 *         它检查htim3的计数值。如果在闭环模式下，该计数值超过一个阈值(10000)，
 *         意味着长时间没有检测到过零信号，可判定为电机堵转。
 *         检测到堵转后，它会关闭所有PWM输出和MOS管，停止电机，以保护硬件。
 */
void BlockedDetection(void)
{
    if ((__HAL_TIM_GET_COUNTER(&htim3) > 10000) && OpenloopStart_Over_Flag == 1)
    {
        Initialization();
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
        HAL_TIM_Base_Stop(&htim3);
    }
}

/**
 * @brief  ESC硬件初始化
 * @retval None
 * @note   此函数负责初始化所有驱动无刷电机所需的硬件。
 *         - 启动用于PWM生成的TIM1定时器。
 *         - 启动用于测量过零间隔的TIM3定时器。
 *         - 启动用于换相延迟的TIM4定时器。
 *         - 调用 `Initialization()` 函数确保电机在启动时处于安全状态 (所有MOS管关闭)。
 */
void ESC_Init(void)
{
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_Base_Start(&htim4);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    Initialization();
}
