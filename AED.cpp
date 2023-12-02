#include "AED.h"

const int AED::MAX_BATTERY_LEVEL = 100;
const int AED::SUFFICIENT_BATTERY_LEVEL = 20;

AED::AED()
{
    this->state = OFF;
    this->batteryLevel = 100;
    this->padsAttached = false;
    this->patientHeartCondition = NORMAL;
    this->shockCount = 0;
}

AED::~AED()
{
    // Nothing to do here.
}

bool AED::selfTest()
{
    setState(SELF_TEST);

    if (this->batteryLevel < SUFFICIENT_BATTERY_LEVEL || !this->padsAttached )
    {
        return false;
    }
    
    else
    {
        // Q Timer delay for 11 seconds
        setState(STANDBY);
    }
}




void AED::startProcedure()
{
// if electrodesare attached, go staright to analyzing stage

// else go to the stay calm, check responsiveness, call for help , attach defib pads
    if(padsAttached) analyzeHeartRhythm();
    // else ....*
}

void AED::chargeBattery()
{
    // Q Timer delay for 11 seconds 
    setState(CHARGING);
    //QTimer for 11s
    setBatteryLevel(MAX_BATTERY_LEVEL);
}

void AED::shock()
{
    // 1st shock 1%, 2 shock 2%, subsequent shocks 3%
    if (this->batteryLevel < MIN_BATTERY_LEVEL)
    {
        // Battery level is too low to shock.
    }
    
    else
    {
        setState(SHOCKING);
        // Q Timer delay for 11 seconds 
        setState(STANDBY);
    }
    
}

void AED::moveToCPR()
{
    // Start q time (10s) Log 2 minutes
    // 
    // 

}

void AED::analyzeHeartRhythm()
{

    // if patiernt is on shockable rythm 


}

void AED::powerOn()
{
    setState(STANDBY);
}

void AED::powerOff()
{
    setState(OFF);
}

void AED::cancelShock()
{
    if (this->state == SHOCKING)
    {
        setState(STANDBY);
    }
}

void AED::startHeartRhythmAnalysis()
{}

HeartState AED::getPatientHeartCondition() const
{
    return this->patientHeartCondition;
}

AEDState AED::getState() const
{
    return this->state;
}

bool AED::getPadsAttached() const
{
    return this->padsAttached;
}

int AED::getBatteryLevel() const
{
    return this->batteryLevel;
}

void AED::setPatientHeartCondition(HeartState patientHeartCondition)
{
    this->patientHeartCondition = patientHeartCondition;
}

void AED::setState(AEDState state)
{
    this->state = state;
}

void AED::setPadsAttached(bool padsAttached)
{
    this->padsAttached = padsAttached;
}

void AED::setBatteryLevel(int batteryLevel)
{
    if(batteryLevel > MAX_BATTERY_LEVEL) this->batteryLevel = MAX_BATTERY_LEVEL;
    if(batteryLevel < 0) this->batteryLevel = 0;
    this->batteryLevel = batteryLevel;
}
