# STM32 Brushless Motor ESC Project

## Preface

Both ADC and comparator methods can be used for zero-crossing detection, each with its own advantages. By constructing a virtual neutral point with resistors, comparing the voltage between the virtual neutral and phase terminals, the zero-crossing point of the back electromotive force (BEMF) can be detected, which is key for sensorless commutation.

$$
\begin{align}
Phase\ Shift\ Angle &= arctan(\frac{2\pi fR_1R_2}{R_1+R_2}) \\
&= arctan(\frac{2\pi \frac{1}{6t} R_1R_2}{R_1+R_2})\ (t: \ interval\ between\ zero\ crossings)
\end{align}
$$
$$Delay\ Time=\frac{ (90 - Phase\ Shift\ Angle ) \times t }{60}$$

## Project Overview

This project is a brushless motor ESC based on the STM32F103C8T6 microcontroller, implementing a complete closed-loop control algorithm for brushless motors. BEMF detection technology is used, leveraging the LM339 comparator for zero-crossing detection, enabling sensorless commutation and speed calculation.

## Main Features

- **Open-loop Start**: Reliable motor startup from standstill.
- **Closed-loop Control**: Precise commutation control based on BEMF zero-crossing detection.
- **Speed Measurement**: Real-time speed measurement and filtering.
- **Stall Protection**: Automatic detection and shutoff upon stall.
- **Dynamic Speed Adjustment**: Supports runtime dynamic PWM duty cycle adjustment.
- **Phase Shift Compensation**: Intelligently compensates for phase delay caused by RC filters.

### Peripheral Configuration

- **TIM1**: PWM output (3 channels for 3-phase bridge); complementary PWM for synchronous rectification in the future.
- **TIM3**: Speed measurement timer.
- **TIM4**: Commutation delay timer.
- **GPIO**: External interrupt (for BEMF zero-crossing detection).
- **GPIO**: Low-side MOSFET control.

### Power Circuit

- Three-phase full bridge driver circuit.
- High-side MOSFETs (PWM drive).
- Low-side MOSFETs (GPIO control).
- RC filter circuit (for BEMF detection).

## Software Architecture

### Core Modules

#### 1. Commutation Control Module

```c
// Six-step commutation state definition
typedef enum {
    U_V = 0,  // U phase PWM, V phase grounded
    W_V,      // W phase PWM, V phase grounded
    W_U,      // W phase PWM, U phase grounded
    V_U,      // V phase PWM, U phase grounded
    V_W,      // V phase PWM, W phase grounded
    U_W       // U phase PWM, W phase grounded
} ReversingStatus;
```

#### 2. Zero-Crossing Detection Module

```c
// BEMF zero-crossing event state
typedef enum {
    U_L = 0,  // U phase falling edge zero-crossing
    V_H,      // V phase rising edge zero-crossing
    W_L,      // W phase falling edge zero-crossing
    U_H,      // U phase rising edge zero-crossing
    V_L,      // V phase falling edge zero-crossing
    W_H       // W phase rising edge zero-crossing
} PhaseZeroStatus;
```

#### 3. State Machine Control

- **Change_Flag**: Core commutation state machine
  - 0: Waiting for the first zero-crossing
  - 1: Zero-crossing captured; start delay timer. If new zero-crossing occurs during delay, flag is set to 2.
  - 2: Wait to perform commutation, then reset flag to 1.

### Key Algorithms

#### 1. Phase Shift Compensation Algorithm

```c
uint16_t timecontrol(uint16_t time)
{
    static float anglelast;
    float angle = 0.95 * anglelast + 0.05 * atan(530 / time) * 180.0 / 3.14159;
    anglelast = angle;
    return (uint16_t)((90.0f - angle) * (float)time / 60.0f);
}
```

#### 2. Speed Detection Algorithm

```c
TimeAndSpeedData SpeedDetection(void)
{
    static TimeAndSpeedData data;
    data.tmp = data.time;
    data.time = __HAL_TIM_GET_COUNTER(&htim3);
    if (data.tmp != 0)
    {
        data.filtertime = 0.95 * data.tmp + 0.05 * data.time;  // Low-pass filter
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

## Usage

### 1. Initialization

```c
int main(void)
{
    // System initialization
    HAL_Init();
    SystemClock_Config();
    
    // ESC initialization
    ESC_Init();
    
    // Open-loop start
    Openloop_Start();
    
    while (1)
    {
        // Main loop
        StatusDetectAndProcess();
        BlockedDetection();
        
        // Optional: Dynamic speed adjustment
        // ChangeCCR(new_ccr_value);
    }
}
```

### 2. Parameter Adjustment

#### PWM Duty Cycle Adjustment

```c
// Set PWM duty cycle (0-1000)
ChangeCCR(400);  // 50% duty cycle
```

### 3. Protection Features

- **Stall Detection**: Automatically detects and stops output on stall.
- **Zero-Crossing Validation**: Verifies correctness of zero-crossing events.
- **Exception Handling**: Handles abnormal zero-crossing events.

## Technical Details

### Timer Configuration

#### TIM1 (PWM Output)
- Frequency: 30kHz
- Resolution: 16-bit
- Channels: 3 (U, V, W phases)

#### TIM3 (Speed Measurement)
- Count frequency: 2MHz
- Purpose: Measure zero-crossing intervals

#### TIM4 (Commutation Delay)
- Count frequency: 2MHz
- Purpose: 90° electrical angle commutation delay

### Interrupt Handling

#### External Interrupts

- Trigger: BEMF zero-crossing detection
- Handler: `HAL_GPIO_EXTI_Callback()`
- Core logic: `EXTI_Process()`

### Filtering Algorithm

#### Speed Filtering

- First-order low-pass filter
- Coefficient: 0.95 (historical) + 0.05 (new)
- Purpose: Smooth speed measurement results

#### Phase Shift Compensation Filtering

- First-order low-pass filter
- Coefficient: 0.95 (historical angle) + 0.05 (new angle)
- Purpose: Smooth phase shift compensation calculations

### FAQ

#### 1. Startup Failure

- Check motor connection
- Adjust startup parameters
- Verify PWM output

#### 2. Unstable Operation

- Adjust phase shift compensation parameters
- Check BEMF detection circuit
- Optimize filter parameters

#### 3. Inaccurate Speed

- Calibrate speed calculation formula
- Adjust timer configuration
- Check zero-crossing detection accuracy

## Project Structure

```
ESC_2/
├── Core/                    # Core code
│   ├── Inc/                # Header files
│   └── Src/                # Source files
├── Drivers/                 # STM32 HAL drivers
├── USER/                    # User code
│   ├── Inc/
│   │   └── ESC.h           # ESC module header
│   └── Src/
│       └── ESC.c           # ESC module implementation
├── CMakeLists.txt          # CMake build configuration
└── README.md               # Project documentation
```

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contribution

Issues and Pull Requests are welcomed to improve this project.

---