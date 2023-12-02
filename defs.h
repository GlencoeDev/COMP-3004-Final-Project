#ifndef DEFS_H
#define DEFS_H

// Constants
#define CM_PIX_RATIO 15.0
#define SHALLOW_PUSH 5.0
#define DEEP_PUSH 7.0
#define MIN_BATTERY_LEVEL 20
#define MAX_BATTERY_LEVEL 100
#define SUFFICIENT_BATTERY_LEVEL 20

// Device state.
enum AEDState
{
    OFF, // Off state
    SELF_TEST, // Self test state
    STANDBY, // Standby state
    ANALYZING, // Analyzing state
    CHARGING, // Charging state
    CPR, // CPR state
    SHOCKING, // Shocking state
};

// Specifies heart condition of the patient.
enum HeartState
{
    NORMAL, // Normal sinus rhythm
    VENTRICULAR_FIBRILLATION, // Ventricular fibrillation
    VENTRICULAR_TACHYCARDIA // Ventricular tachycardia
};

#endif
