## 项目简介

本项目基于 STM32F103 微控制器，采用无刷直流电机（BLDC）方波驱动方案，实现了开环启动、反电动势检测、堵转检测等功能。

## 主要功能

- BLDC 电机六步换向控制
- 开环启动与闭环运行自动切换
- 反电动势过零点检测
- 堵转检测与保护
- PWM 占空比调节

## 目录结构

```
.
├── Core/           # STM32 HAL 库生成的核心代码
│   ├── Inc/
│   └── Src/
├── Drivers/        # STM32 HAL/CMSIS 驱动
├── USER/           # 用户自定义代码（ESC.c、OLED.c等）
├── MDK-ARM/        # Keil 工程文件
├── .vscode/        # VSCode 配置
└── ...
```

## 主要文件说明

- ESC.c：电机控制主逻辑，包括状态检测、换向、开环启动等
- main.c：主函数入口，初始化外设并启动主循环
- tim.c：定时器初始化与配置
- gpio.c：GPIO 初始化
- usart.c：串口初始化

## 编译与烧录

1. 使用 Keil MDK-ARM 打开 ESC.uvprojx 工程文件。
2. 选择合适的仿真器/下载器（如 ST-Link）。
3. 编译并下载程序到 STM32F103 芯片。

或使用 VSCode + PlatformIO/STM32CubeIDE 进行开发。

## 外设连接

- 电机三相输出：U、V、W

## 关键函数说明

- `ChangeBasedOnZerostatus`：根据过零点状态切换电机相位
- `zerostatuscheck`：检测当前过零点状态
- `Openloop_Start`：开环启动流程
- `SpeedDetection`：速度与周期滤波计算
- `BlockedDetection`：堵转检测

## 注意事项

- 请根据实际硬件连接修改 `main.h`、`gpio.h` 中的引脚定义。
- 电机驱动部分请确保硬件保护，避免误操作损坏器件。
- 代码默认使用 TIM2 作为 PWM，TIM3 作为定时/速度测量。

## 许可证

- STM32 HAL/CMSIS 驱动遵循 [BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause) 和 [Apache-2.0](https://opensource.org/licenses/Apache-2.0)。
- 用户代码可自由修改与分发。
