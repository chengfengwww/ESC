# STM32 无刷电机电调 (ESC) 项目

## 前言
ADC法和比较器法检测过零都是可以的，各有优点。利用电阻构建出虚拟中性点，比较虚拟中性点电压和端电压得出反电动势的过零点，反电动势过零则代表着电机转到特定的位置，根据这个即可换向。但是，只通过电阻构建的中性点，得到的反电动势波形是非常杂乱的，比较器无法输出正常的波形。ADC法可以通过软件滤波规避这个问题，所以很多采用ADC方案的商用电调的反电动势检测电路是非常简单的。如果想要用比较器来检测过零点，则必须在反电动势检测电路加入电容，加入电容则会引起相移。针对相移，有延迟 **30度-相移角** 和 **90度-相移角** 等方案，对于KV值比较高的无刷电机，第一种方案对于延迟的补偿是有限的，所以我采用了延迟 **90度-相移角** 的方案，实践表明，这种方案的效果和延迟 **30度-相移角** 的差距很小，并且有不需要反复调试反电动势检测电路的电容值的优点。

$$
\begin{align}
相移角&=arctan(\frac{2\pi fR_1R_2}{R_1+R_2}) \\
&=arctan(\frac{2\pi \frac{1}{6t} R_1R_2}{R_1+R_2})(t为过零点的间隔时间)
\end{align}
$$
$$延迟时间=\frac{（90 - 相移角）\times t}{60} $$


## 项目概述

这是一个基于STM32F103C8T6微控制器的无刷电机电调(ESC)项目，实现了完整的无刷电机闭环控制算法。项目采用反电动势(BEMF)检测技术，利用LM339比较器检测过零，通过过零检测实现精确的六步换向控制。

## 主要特性

- **开环启动**: 实现电机从静止到旋转的可靠启动
- **闭环控制**: 基于BEMF过零检测的精确换向控制
- **速度检测**: 实时测量和滤波处理电机转速
- **堵转保护**: 自动检测电机堵转并停止输出
- **动态调速**: 支持运行时动态调整PWM占空比
- **相移补偿**: 智能补偿RC滤波电路带来的相位延迟

### 外设配置
- **TIM1**: PWM输出 (3通道，用于驱动三相桥)，后续会利用互补PWM实现同步整流
- **TIM3**: 速度测量定时器
- **TIM4**: 换相延迟定时器
- **GPIO**: 外部中断 (用于BEMF过零检测)
- **GPIO**: 低端MOS管控制

### 功率电路
- 三相全桥驱动电路
- 高端MOS管 (PWM驱动)
- 低端MOS管 (GPIO控制)
- RC滤波电路 (用于BEMF检测)

## 软件架构

### 核心模块

#### 1. 换向控制模块
```c
// 六步换向状态定义
typedef enum {
    U_V = 0,  // U相PWM，V相接地
    W_V,      // W相PWM，V相接地
    W_U,      // W相PWM，U相接地
    V_U,      // V相PWM，U相接地
    V_W,      // V相PWM，W相接地
    U_W       // U相PWM，W相接地
} ReversingStatus;
```

#### 2. 过零检测模块
```c
// BEMF过零事件状态
typedef enum {
    U_L = 0,  // U相下降沿过零
    V_H,      // V相上升沿过零
    W_L,      // W相下降沿过零
    U_H,      // U相上升沿过零
    V_L,      // V相下降沿过零
    W_H       // W相上升沿过零
} PhaseZeroStatus;
```

#### 3. 状态机控制
- **Change_Flag**: 核心换向状态机
  - 0: 等待首次过零点
  - 1: 已捕获过零点，启动延迟计时，若延迟途中有新的过零点，标志置2
  - 2: 等待执行换向，换向后标志置1

### 关键算法

#### 1. 相移补偿算法
```c
uint16_t timecontrol(uint16_t time)
{
    static float anglelast;
    float angle = 0.95 * anglelast + 0.05 * atan(530 / time) * 180.0 / 3.14159;
    anglelast = angle;
    return (uint16_t)((90.0f - angle) * (float)time / 60.0f);
}
```

#### 2. 速度检测算法
```c
TimeAndSpeedData SpeedDetection(void)
{
    static TimeAndSpeedData data;
    data.tmp = data.time;
    data.time = __HAL_TIM_GET_COUNTER(&htim3);
    if (data.tmp != 0)
    {
        data.filtertime = 0.95 * data.tmp + 0.05 * data.time;  // 低通滤波
    }
    else
    {
        data.filtertime = data.time;
    }
    data.speed = 60.0 / 7.0 / 360.0 / (data.filtertime * 0.0000005);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    return data;
}
```

## 使用方法

### 1. 初始化
```c
int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    
    // ESC初始化
    ESC_Init();
    
    // 开环启动
    Openloop_Start();
    
    while (1)
    {
        // 主循环
        StatusDetectAndProcess();
        BlockedDetection();
        
        // 可选：动态调整速度
        // ChangeCCR(new_ccr_value);
    }
}
```

### 2. 参数调整

#### PWM占空比调整
```c
// 设置PWM占空比 (0-1000)
ChangeCCR(400);  // 50%占空比
```

### 3. 保护功能
- **堵转检测**: 自动检测电机堵转并停止输出
- **过零验证**: 验证过零事件的正确性
- **异常处理**: 处理异常的过零事件

## 技术细节

### 定时器配置

#### TIM1 (PWM输出)
- 频率: 30kHz
- 分辨率: 16位
- 通道: 3个 (U, V, W相)

#### TIM3 (速度测量)
- 计数频率: 2MHz
- 用途: 测量过零间隔

#### TIM4 (换相延迟)
- 计数频率: 2MHz
- 用途: 实现90度电气角延迟换相

### 中断处理

#### 外部中断
- 触发源: BEMF过零检测
- 处理函数: `HAL_GPIO_EXTI_Callback()`
- 核心逻辑: `EXTI_Process()`


### 滤波算法

#### 速度滤波
- 一阶低通滤波
- 系数: 0.95 (历史值) + 0.05 (新值)
- 目的: 平滑速度测量结果

#### 相移补偿滤波
- 一阶低通滤波
- 系数: 0.95 (历史角度) + 0.05 (新角度)
- 目的: 平滑相移补偿计算


### 常见问题

#### 1. 启动失败
- 检查电机连接
- 调整启动参数
- 验证PWM输出

#### 2. 运行不稳定
- 调整相移补偿参数
- 检查BEMF检测电路
- 优化滤波参数

#### 3. 速度不准确
- 校准速度计算公式
- 调整定时器配置
- 检查过零检测精度

## 项目结构

```
ESC_2/
├── Core/                    # 核心代码
│   ├── Inc/                # 头文件
│   └── Src/                # 源文件
├── Drivers/                 # STM32 HAL驱动
├── USER/                    # 用户代码
│   ├── Inc/
│   │   └── ESC.h           # ESC模块头文件
│   └── Src/
│       └── ESC.c           # ESC模块实现
├── CMakeLists.txt          # CMake构建配置
└── README.md               # 项目文档
```

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

---