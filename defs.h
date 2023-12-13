#ifndef DEFS_H
#define DEFS_H

// Constants
#define CM_PIX_RATIO 15.0
#define SHALLOW_PUSH 5.0
#define DEEP_PUSH 7.0
#define MIN_BATTERY_LEVEL 20
#define MAX_BATTERY_LEVEL 100
#define SUFFICIENT_BATTERY_LEVEL 20

#define SLEEP 3000
#define SHOCKING_TIME 3000
#define CPR_TIME 10000
#define ATTACH_PADS_TIME 100
#define ANALYZING_TIME 3000
#define CHANGE_BATTERIES_TIME 5000
#define CHECK_PADS_TIME 1000
#define RESPONSE_INDICATOR 0
#define HELP_INDICATOR 1
#define PADS_INDICATOR 2
#define CONTACT_INDICATOR 3
#define CPR_INDICATOR 4
#define SHOCK_INDICATOR 5
#define BATTERY_DRAIN_TIME 5000

#define RANDOM_BOUND 1
// Device state.
enum AEDState
{
    OFF,               // Device is off.
    SELF_TEST_SUCCESS, // Self test passed.
    SELF_TEST_FAIL,    // Self test failed.
    CHANGE_BATTERIES,  // Change batteries.
    STAY_CALM,         // Stay calm.
    CHECK_RESPONSE,    // Check for response.
    CALL_HELP,         // Call for help.
    ATTACH_PADS,       // Attach pads.
    ANALYZING,         // Don't touch patient. Analyzing.
    SHOCK_ADVISED,     // Shock advised.
    NO_SHOCK_ADVISED,  // No shock advised.
    STAND_CLEAR,       // Stand clear.
    SHOCKING,          // Shock will be delivered in three, two, one...
    SHOCK_DELIVERED,   // Shock delivered.
    CPR,               // Start CPR.
    STOP_CPR,          // Stop CPR.
    ABORT,             // Turn off if the patient was determined to be healthy.
    LOST_CONNECTION    // Simulate losing connection.
};

// Specifies heart condition of the patient.
enum HeartState
{
    SINUS_RHYTHM,             // Normal sinus rhythm
    VENTRICULAR_FIBRILLATION, // Ventricular fibrillation
    VENTRICULAR_TACHYCARDIA,  // Ventricular tachycardia
};

#endif
