#ifndef __ESC_H_
#define __ESC_H_

#include "main.h"
#include "tim.h"
#include "math.h"



typedef enum
{
    U_V = 0,
    W_V,
    W_U,
    V_U,
    V_W,
    U_W
}ReversingStatus;

typedef enum
{
    U = 0,
    V,
    W,
    All
}UnenergizedPhase;

typedef enum
{
    U_L = 0,
    V_H,
    W_L,
    U_H,
    V_L,
    W_H
}PhaseZeroStatus;

typedef struct
{
    int time;
    int filtertime;
    int tmp;
    float speed;
}TimeAndSpeedData;

typedef struct
{
    uint8_t U_H_Flag;
    uint8_t U_L_Flag;
    uint8_t V_H_Flag;
    uint8_t V_L_Flag;
    uint8_t W_H_Flag;
    uint8_t W_L_Flag;

}InterruptStateFlag;


UnenergizedPhase ChangeStatus(ReversingStatus status, uint16_t ccr);
void BlockedDetection(void);
void Openloop_Start(void);
TimeAndSpeedData SpeedDetection(void);
void ESC_Init(void);
void StatusDetectAndProcess(void);
//void ClosedLoopCommutation(void);

#endif
