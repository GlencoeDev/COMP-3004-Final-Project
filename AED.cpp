#include "AED.h"


AED::AED()
    :
      QObject(nullptr)
    , patientHeartCondition(NORMAL)
    , padsAttached(false)
    , batteryLevel(100)
    , shockCount(0)
{ }

// bool AED::selfTest();
void AED::startProcedure()
{
    //Start self test procedure, only checking for battery in this case
    if(this -> batteryLevel < SUFFICIENT_BATTERY_LEVEL)
    {
        emit stateChanged(SELF_TEST_FAIL);
        QThread::msleep(SLEEP);
        emit stateChanged(CHANGE_BATTERIES);
        return;
    }
    nextStep(SELF_TEST_SUCCESS, SLEEP, batteryUnitsWhenIdle);
    nextStep(STAY_CALM, SLEEP, batteryUnitsWhenIdle);
    nextStep(CHECK_RESPONSE, SLEEP, batteryUnitsWhenIdle);
    nextStep(CALL_HELP, SLEEP, batteryUnitsWhenIdle);

    //Ask the user to attach the pads
    while(!padsAttached){
        nextStep(ATTACH_PADS, ATTACH_PADS_TIME, 0);
    }

    for(int i = 0; i < shockUntilHealthy; ++i){
        nextStep(ANALYZING, ANALYZING_TIME, batteryUnitsWhenIdle);
        if(!shockable()){
            nextStep(ABORT, 0, 0);
        }else{
    //        emit stateChanged(SHOCK_ADVISED);
    //        QThread::msleep(SLEEP);

            //Check for battery
            int shockJoule = shockCount >= 3 ? 3 : shockCount;
            int batteryUnits = shockJoule * batteryUnitsPerShock;
            if(batteryLevel - batteryUnits < SUFFICIENT_BATTERY_LEVEL){
                nextStep(CHANGE_BATTERIES, 0, 0);
                return;
            }
            nextStep(STAND_CLEAR, SLEEP, batteryUnitsWhenIdle);

            nextStep(SHOCKING, SHOCKING_TIME, batteryUnitsWhenIdle);

            nextStep(SHOCK_DELIVERED, SLEEP, batteryUnits);

            nextStep(CPR, CPR_TIME, batteryUnitsWhenIdle * (CPR_TIME/SLEEP));

            nextStep(STOP_CPR, SLEEP, batteryUnitsWhenIdle);
        };
    }

}

//Going to the next step,
void AED::nextStep(AEDState state, unsigned long sleepTime, int batteryUsed){
    emit stateChanged(state);
    batteryLevel -= batteryUsed;
    emit batteryChanged(batteryLevel);
    if(sleepTime != 0){
        QThread::msleep(sleepTime);
    }
}
bool AED::shockable()
{
    // if patiernt is on shockable rythm
    if(patientHeartCondition == VENTRICULAR_FIBRILLATION
    || patientHeartCondition == VENTRICULAR_TACHYCARDIA)
        return true;
    return false;
}

//void AED::chargeBattery()
//{
//    // Q Timer delay for 11 seconds
//    emit sendTextMsg(QString("Charge AED!"));
//    QThread::msleep(SLEEP);
//    //QTimer for 11s
//    this -> batteryLevel = MAX_BATTERY_LEVEL;
//}

//void AED::shock()
//{
//    // 1st shock 1%, 2 shock 2%, subsequent shocks 3%
//    if (this->batteryLevel < MIN_BATTERY_LEVEL)
//    {
//        // Battery level is too low to shock.
//        emit sendTextMsg(QString("Not enough battery!"));

//    }
    
//    else
//    {
//        setState(SHOCKING);
//        // Q Timer delay for 11 seconds
//    }
    
//}


//void AED::moveToCPR()
//{
//    // Start q time (10s) Log 2 minutes
//    //
//    //


//}


//void AED::powerOn()
//{
//    setState(STANDBY);
//}

//void AED::powerOff()
//{
//    setState(OFF);
//}

//void AED::cancelShock()
//{
//    if (this->state == SHOCKING)
//    {
//        setState(STANDBY);
//    }
//}


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
    emit stateChanged(state);
}

void AED::setPadsAttached(bool padsAttached)
{
    this->padsAttached = padsAttached;
}

void AED::setBatteryLevel(int level){
    batteryLevel = level;
    emit batteryChanged(level);

}

void AED::setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle)
{
    batteryLevel = startingLevel;
    batteryUnitsPerShock = unitsPerShock;
    batteryUnitsWhenIdle = unitsWhenIdle;
    emit batteryChanged(startingLevel);
}

void AED::setShockUntilHealthy(int shockUntilHealthy){
    this -> shockUntilHealthy = shockUntilHealthy;
}
