#include "AED.h"
#include "MainWindow.h"
#include <QRandomGenerator>




AED::AED()
    : QObject(nullptr)
    , patientHeartCondition(SINUS_RHYTHM)
    , padsAttached(false)
    , batteryLevel(100)
    , shockCount(0)
    , loseConnection(false)
{
    m_thread.reset(new QThread);
    moveToThread(m_thread.get());
    m_thread->start();
}

AED::~AED()
{
    QMetaObject::invokeMethod(this, "cleanup");
    m_thread->wait();
}

void AED::cleanUp(){

    m_thread->quit();
}

void AED::powerOn()
{
    // Abort if there is not GUI connected.
    if (gui == nullptr) return;

    run();
}

void AED::powerOff()
{
    if (state != OFF && state != ABORT && state != SHOCKING)
    {
        state = ABORT;
    }
}

void AED::checkPadsAttached()
{
    if (!padsAttached)
    {
        if (state == ABORT) return;
        // Cycle through the stages if the pads have not been attached.
        nextStep(STAY_CALM, SLEEP, 0);

        if (state == ABORT) return;
        nextStep(CHECK_RESPONSE, SLEEP, 0);


        if (state == ABORT) return;
        nextStep(CALL_HELP, SLEEP, 0);


        if (state == ABORT) return;
        // Ask the user to attach the pads.
        nextStep(ATTACH_PADS, ATTACH_PADS_TIME, 0);

        QMutexLocker locker(&padsAttachedMutex);
        waitForPadsAttachement.wait(&padsAttachedMutex);
        padsAttached = true;
    }
    else
    {
        QThread::msleep(CHECK_PADS_TIME);
    }
}

void AED::setLostConnection(bool simulateConnectionLoss)
{
    this->loseConnection = simulateConnectionLoss;
}


void AED::checkConnection()
{
    // Simulate connection loss if such testing requirement was selected with ~33% probability.
    int random = QRandomGenerator::global()->bounded(RANDOM_BOUND);
    if (loseConnection && random == 0)
    {
        nextStep(LOST_CONNECTION, 0, 0);
        QMutexLocker locker(&restoreConnectionMutex);
        waitForConnection.wait(&restoreConnectionMutex);
    }
}

void AED::run()
{
    if (state == ABORT) return;

    // Start self test procedure, only checking for battery in this case
    QThread::msleep(SLEEP);

    // Randomly determine whether the self-test should fail.
    int random = QRandomGenerator::global()->bounded(100);
    if (random >= 90)
    {
        state = SELF_TEST_FAIL;
        emit updateGUI(SELF_TEST_FAIL);
        return;
    }
    else if (batteryLevel < SUFFICIENT_BATTERY_LEVEL)
    {
        state = CHANGE_BATTERIES;
        emit updateGUI(CHANGE_BATTERIES);
        return;
    }
    else
    {
        nextStep(SELF_TEST_SUCCESS, SLEEP, 0);
    }

    // Start reducing the battery.

    checkPadsAttached();

    // One extra round of CPR before delivering all shocks.
    if (startWithAsystole)
    {
        shockUntilHealthy++;
    }

    for (int i = 0; i <= shockUntilHealthy; ++i)
    {
        if (state == ABORT) return;
        nextStep(ANALYZING, ANALYZING_TIME, 0);

        if (state == ABORT) return;
        // Change patient to healthy once all shocks have been delivered.
        if (shockable() && i == shockUntilHealthy)
        {
           // Update patient heart condition to healthy since all shocks have been delivered.
           patientHeartCondition = SINUS_RHYTHM;
           emit updatePatientCondition(SINUS_RHYTHM);
        }

        if (state == ABORT) return;
        bool shockNeeded = shockable() && (i > 0 || !startWithAsystole);
        emit updateGUI(shockNeeded ? SHOCK_ADVISED : NO_SHOCK_ADVISED);
        QThread::msleep(SLEEP);

        // Normal rhythm. Turn off the device.
        if (!shockNeeded && patientHeartCondition == SINUS_RHYTHM)
        {
            nextStep(ABORT, 0, 0);
            return;
        }

        if (state == ABORT) return;
        // Simulating connection lost.
        checkConnection();

        if (shockNeeded)
        {
            if (state == ABORT) return;

            // Check if we have enough battery.
            if (batteryLevel - batteryUnitsPerShock < SUFFICIENT_BATTERY_LEVEL)
            {
                //Indicate the user to change battery
                nextStep(CHANGE_BATTERIES, CHANGE_BATTERIES_TIME, 0);
                //Then abort
                nextStep(ABORT, 0, 0);
                return;
            }

            if (state == ABORT) return;
            nextStep(STAND_CLEAR, SLEEP, 0);
            nextStep(SHOCKING, SHOCKING_TIME, 0);
            nextStep(SHOCK_DELIVERED, SLEEP, batteryUnitsPerShock);
        }

        if (state == ABORT) return;
        nextStep(CPR, CPR_TIME, 0);
        nextStep(STOP_CPR, SLEEP, 0);
    }
}

void AED::setGUI(MainWindow* mainWindow)
{
    this->gui = mainWindow;

    connect(this, SIGNAL(updateGUI(int)), mainWindow, SLOT(updateGUI(int)));
    connect(this, SIGNAL(batteryChanged(int)), gui, SLOT(updateBatteryLevel(int)));
    connect(this, SIGNAL(updateShockCount(int)), gui, SLOT(updateNumberOfShocks(int)));
    connect(this, SIGNAL(updatePatientCondition(int)), gui, SLOT(updatePatientCondition(int)));
}

// Going to the next step.
void AED::nextStep(AEDState state, unsigned long sleepTime, int batteryUsed)
{
    this->state = state;
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
bool AED::shockable() const
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


void AED::setState(int state)
{
    this->state = (AEDState)state;
}


int AED::getBatteryLevel() const
{
    return this->batteryLevel;
}

void AED::setPatientHeartCondition(int patientHeartCondition)
{
    this->patientHeartCondition = (HeartState) patientHeartCondition;
}

void AED::setPadsAttached(bool padsAttached)
{
    this->padsAttached = padsAttached;
}

void AED::notifyPadsAttached()
{
    padsAttached = true;
    QMutexLocker locker(&padsAttachedMutex);
    waitForPadsAttachement.wakeOne();
}

void AED::notifyReconnection()
{
    QMutexLocker locker(&restoreConnectionMutex);
    waitForConnection.wakeOne();

}

void AED::setBatteryLevel(int level)
{
    batteryLevel = level;
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

void AED::setStartWithAsystole(bool checked)
{
    this->startWithAsystole = checked;
}
