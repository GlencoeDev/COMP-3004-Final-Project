#include "AED.h"
#include "MainWindow.h"
#include <QRandomGenerator>


AED::AED()
    : QObject(nullptr)
    , patientHeartCondition(SINUS_RHYTHM)
    , padsAttached(false)
    , batteryLevel(100)
    , shockCount(0)
{ }

void AED::powerOn()
{
    // Abort if there is not GUI connected.
    if (gui == nullptr) return;

    run();
}

void AED::run()
{
    // Abort if the device is already running.
    if (state != OFF) return;

    // Start self test procedure, only checking for battery in this case
    QThread::msleep(SLEEP);

    // Randomly determine whether the self-test should fail.
    int random = QRandomGenerator::global()->bounded(100);
    if (random >= 90)
    {
        emit updateGUI(SELF_TEST_FAIL);
        return;
    }
    else if (batteryLevel < SUFFICIENT_BATTERY_LEVEL)
    {
        emit updateGUI(CHANGE_BATTERIES);
        return;
    }
    else
    {
         nextStep(SELF_TEST_SUCCESS, SLEEP, batteryUnitsWhenIdle);
    }

    nextStep(STAY_CALM, SLEEP, batteryUnitsWhenIdle);
    nextStep(CHECK_RESPONSE, SLEEP, batteryUnitsWhenIdle);
    nextStep(CALL_HELP, SLEEP, batteryUnitsWhenIdle);

    // Ask the user to attach the pads.
    nextStep(ATTACH_PADS, ATTACH_PADS_TIME, batteryUnitsWhenIdle);

    // Keep spinning while the pads are not attached.
    while (!padsAttached) {}

    for (int i = 0; i < shockUntilHealthy; ++i)
    {
        nextStep(ANALYZING, ANALYZING_TIME, batteryUnitsWhenIdle);

        emit updateGUI(shockable() ? SHOCK_ADVISED : NO_SHOCK_ADVISED);
        QThread::msleep(SLEEP);

        // Normal rhythm. Turn off the device.
        if (!shockable() && patientHeartCondition == SINUS_RHYTHM)
        {
            nextStep(ABORT, 0, 0);
            return;
        }
            // Check for .
            int shockJoule = shockCount >= 3 ? 3 : shockCount;
            int batteryUnits = shockJoule * batteryUnitsPerShock;

            if (batteryLevel - batteryUnits < SUFFICIENT_BATTERY_LEVEL)
            {
                nextStep(CHANGE_BATTERIES, 0, 0);
                return;
            }

            nextStep(STAND_CLEAR, SLEEP, batteryUnitsWhenIdle);
            nextStep(SHOCKING, SHOCKING_TIME, batteryUnitsWhenIdle);
            nextStep(SHOCK_DELIVERED, SLEEP, batteryUnits);
            nextStep(CPR, CPR_TIME, batteryUnitsWhenIdle * (CPR_TIME/SLEEP));
            nextStep(STOP_CPR, SLEEP, batteryUnitsWhenIdle);
    }
}

void AED::setGUI(MainWindow* mainWindow)
{
    this->gui = mainWindow;

    connect(this, SIGNAL(updateGUI(AEDState)), mainWindow, SLOT(updateState(AEDState)));
    connect(this, SIGNAL(batteryChanged(int)), gui, SLOT(updateBatteryLevel(int)));
    connect(this, SIGNAL(updateShockCount(int)), gui, SLOT(updateNumberOfShocks(int)));
    connect(this, SIGNAL(updatePatientCondition(int)), gui, SLOT(updatePatientCondition(int)));
}

// Going to the next step.
void AED::nextStep(AEDState state, unsigned long sleepTime, int batteryUsed)
{
    emit updateGUI(state);

    if (batteryUsed > 0)
    {
        batteryLevel -= batteryUsed;
        emit batteryChanged(batteryLevel);
    }

    if (state == SHOCK_DELIVERED)
    {
        shockCount++;
        emit updateShockCount(shockCount);
    }

    if (sleepTime != 0)
    {
        QThread::msleep(sleepTime);
    }
}
bool AED::shockable()
{
    // if patiernt is on shockable rhythm
    if (patientHeartCondition == VENTRICULAR_FIBRILLATION ||
        patientHeartCondition == VENTRICULAR_TACHYCARDIA)
    {
        return true;
    }

    return false;
}

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
}

void AED::setShockUntilHealthy(int shockUntilHealthy){
    this->shockUntilHealthy = shockUntilHealthy;
}
