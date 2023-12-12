// IMPORTS
#include "AED.h"
/*
    Function: AED()
    Purpose: Constructor for AED class. Initializes the AED device.
    Inputs:
        None
    Outputs:
        None
*/
AED::AED()
    : QObject(nullptr), patientHeartCondition(SINUS_RHYTHM), padsAttached(false), batteryLevel(100), shockCount(0), loseConnection(false)
{
    m_thread.reset(new QThread);
    moveToThread(m_thread.get());
    m_thread->start();
}

/*
    Function: ~AED()
    Purpose: Destructor for AED class. Cleans up the AED device.
    Inputs:
        None
    Outputs:
        None
*/
AED::~AED()
{
    QMetaObject::invokeMethod(this, "cleanup");
    m_thread->wait();
}

/*
    Function: cleanUp()
    Purpose: Cleans up the AED device.
    Inputs:
        None
    Outputs:
       None
*/
void AED::cleanUp()
{

    m_thread->quit();
}

/*
    Function: powerOn()
    Purpose: Powers on the AED device.
    Inputs:
        None
    Outputs:
        None
*/
void AED::powerOn()
{
    // Abort if there is not GUI connected.
    if (gui == nullptr)
        return;

    run();
}

/*
    Function: powerOff()
    Purpose: Powers off the AED device.
    Inputs:
        None
    Outputs:
        None
*/
void AED::powerOff()
{
    if (state != OFF && state != ABORT && state != SHOCKING)
    {
        state = ABORT;

        // Release all locks.
        QMutexLocker padsLocker(&padsAttachedMutex);
        waitForPadsAttachement.wakeOne();
        QMutexLocker reconnectionlocker(&restoreConnectionMutex);
        waitForConnection.wakeOne();
    }
}

bool AED::checkPadsAttached()
{
    if (!padsAttached)
    {
        // Cycle through the stages if the pads have not been attached.
        if (!nextStep(STAY_CALM, SLEEP, 0)) return false;
        if (!nextStep(CHECK_RESPONSE, SLEEP, 0)) return false;
        if (!nextStep(CALL_HELP, SLEEP, 0)) return false;
        // Ask the user to attach the pads.
        if (!nextStep(ATTACH_PADS, ATTACH_PADS_TIME, 0)) return false;

        if (padsAttached)
        {
            // Keep the pads indicator message for some time.
            QThread::msleep(1000);
            return true;
        }

        QMutexLocker locker(&padsAttachedMutex);
        waitForPadsAttachement.wait(&padsAttachedMutex);

        // Keep the pads indicator message for some time.
        QThread::msleep(1000);

        padsAttached = true;
    }
    else
    {
        QThread::msleep(CHECK_PADS_TIME);
    }

    return true;
}

/*
    Function: setLostConnection()
    Purpose: Sets the connection status of the AED device.
    Inputs:
        bool simulateConnectionLoss: True if the connection is lost, false otherwise.
    Outputs:
        None

*/
void AED::setLostConnection(bool simulateConnectionLoss)
{
    this->loseConnection = simulateConnectionLoss;
}

/*
    Function: checkConnection()
    Purpose: Checks if the connection is lost.
    Inputs:
        None
    Outputs:
        None
*/
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

/*
    Function: run()
    Purpose: Runs the AED device.
    Inputs:
        None
    Outputs:
        None
*/
void AED::run()
{
    // Start self test procedure, only checking for battery in this case
    QThread::msleep(SLEEP);

    // Randomly determine whether the self-test should fail.
    int random = QRandomGenerator::global()->bounded(100);
    if (random >= 90)
    {
        if (!nextStep(SELF_TEST_FAIL, 0, 0)) return;
    }
    else if (batteryLevel < SUFFICIENT_BATTERY_LEVEL)
    {
        if (!nextStep(CHANGE_BATTERIES, 0, 0)) return;
    }
    else
    {
        if (!nextStep(SELF_TEST_SUCCESS, SLEEP, 0)) return;
    }

    // Start reducing the battery.
    if (!checkPadsAttached()) return;

    // One extra round of CPR before delivering all shocks.
    if (startWithAsystole)
    {
        shockUntilHealthy++;
    }

    for (int i = 0; i <= shockUntilHealthy; ++i)
    {
        if (!nextStep(ANALYZING, ANALYZING_TIME, 0)) return;


        // Change patient to healthy once all shocks have been delivered.
        if (shockable() && i == shockUntilHealthy)
        {
            // Update patient heart condition to healthy since all shocks have been delivered.
            patientHeartCondition = SINUS_RHYTHM;
            emit updatePatientCondition(SINUS_RHYTHM);
        }

        bool shockNeeded = shockable() && (i > 0 || !startWithAsystole);
        if (!nextStep(shockNeeded ? SHOCK_ADVISED : NO_SHOCK_ADVISED, SLEEP, 0)) return;

        // Normal rhythm. Turn off the device.
        if (!shockNeeded && patientHeartCondition == SINUS_RHYTHM)
        {
           emit updateGUI(ABORT);
           return;
        }

        // Simulating connection lost.
        checkConnection();

        if (shockNeeded)
        {
            // Check if we have enough battery.
            if (batteryLevel - batteryUnitsPerShock < SUFFICIENT_BATTERY_LEVEL)
            {
                // Indicate the user to change battery.
                if (!nextStep(CHANGE_BATTERIES, CHANGE_BATTERIES_TIME, 0)) return;
            }

            if (!nextStep(STAND_CLEAR, SLEEP, 0)) return;
            if (!nextStep(SHOCKING, SHOCKING_TIME, 0)) return;
            if (!nextStep(SHOCK_DELIVERED, SLEEP, batteryUnitsPerShock)) return;
        }

        if (!nextStep(CPR, CPR_TIME, 0)) return;
        if (!nextStep(STOP_CPR, SLEEP, 0)) return;
    }
}

/*
    Function: setGUI()
    Purpose: Sets the GUI for the AED device.
    Inputs:
        MainWindow *mainWindow: Pointer to the GUI.
    Outputs:
        None
*/
void AED::setGUI(MainWindow *mainWindow)
{
    this->gui = mainWindow;

    connect(this, SIGNAL(updateGUI(int)), mainWindow, SLOT(updateGUI(int)));
    connect(this, SIGNAL(batteryChanged(int)), gui, SLOT(updateBatteryLevel(int)));
    connect(this, SIGNAL(updateShockCount(int)), gui, SLOT(updateNumberOfShocks(int)));
    connect(this, SIGNAL(updatePatientCondition(int)), gui, SLOT(updatePatientCondition(int)));
}

/*
    Function: nextStep()
    Purpose: Updates the AED device to the next step.
    Inputs:
        AEDState state: The next state of the AED device.
        unsigned long sleepTime: The time to sleep before the next step.
        int batteryUsed: The amount of battery used for the next step.
    Outputs:
        A boolean indicating whether the device has successfully proceeded 
        to the final step. 
*/
bool AED::nextStep(AEDState state, unsigned long sleepTime, int batteryUsed)
{
    if (this->state == ABORT)
    {
        emit updateGUI(ABORT);
        return false;
    };

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

    if (state == SELF_TEST_FAIL || state == CHANGE_BATTERIES)
    {
        return false;
    }

    if (sleepTime != 0)
    {
        QThread::msleep(sleepTime);
    }

    return true;
}

/*
    Function: shockable()
    Purpose: Checks if the patient is on shockable rhythm.
    Inputs:
        None
    Outputs:
        True if the patient is on shockable rhythm, false otherwise.
*/
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

/*
    Function: getPatientHeartCondition()
    Purpose: Gets the patient's heart condition.
    Inputs:
        None
    Outputs:
        The patient's heart condition.
*/
HeartState AED::getPatientHeartCondition() const
{
    return this->patientHeartCondition;
}

/*
    Function: getState()
    Purpose: Gets the state of the AED device.
    Inputs:
        None
    Outputs:
        The state of the AED device.
*/
AEDState AED::getState() const
{
    return this->state;
}

/*
    Function: setState()
    Purpose: Sets the state of the AED device.
    Inputs:
        int state: The state of the AED device.
    Outputs:
        None
*/
void AED::setState(int state)
{
    this->state = (AEDState)state;
}

/*
    Function: getBatteryLevel()
    Purpose: Gets the battery level of the AED device.
    Inputs:
        None
    Outputs:
        The battery level of the AED device.
*/
int AED::getBatteryLevel() const
{
    return this->batteryLevel;
}

/*
    Function: setPatientHeartCondition()
    Purpose: Sets the patient's heart condition.
    Inputs:
        int patientHeartCondition: The patient's heart condition. This represents the index of the heart condition in the HeartState enum.
    Outputs:
        None
*/
void AED::setPatientHeartCondition(int patientHeartCondition)
{
    this->patientHeartCondition = (HeartState)patientHeartCondition;
}

/*
    Function: setPadsAttached()
    Purpose: Sets the status of the pads.
    Inputs:
        bool padsAttached: True if the pads are attached, false otherwise.
    Outputs:
        None
*/
void AED::setPadsAttached(bool padsAttached)
{
    this->padsAttached = padsAttached;
}

/*
    Function: notifyPadsAttached()
    Purpose: Notifies the AED device that the pads are attached.
    Inputs:
        None
    Outputs:
        None
*/
void AED::notifyPadsAttached()
{
    padsAttached = true;
    QMutexLocker locker(&padsAttachedMutex);
    waitForPadsAttachement.wakeOne();
}

/*
    Function: notifyReconnection()
    Purpose: Notifies the AED device that the connection is restored.
    Inputs:
        None
    Outputs:
        None
*/
void AED::notifyReconnection()
{
    QMutexLocker locker(&restoreConnectionMutex);
    waitForConnection.wakeOne();
}

/*
    Function: setBatteryLevel()
    Purpose: Sets the battery level of the AED device.
    Inputs:
        int level: The battery level of the AED device.
    Outputs:
        None
*/
void AED::setBatteryLevel(int level)
{
    batteryLevel = level;
}

/*
    Function: setBatterySpecs()
    Purpose: Sets the battery specs of the AED device.
    Inputs:
        int startingLevel: The starting battery level of the AED device.
        int unitsPerShock: The amount of battery used per shock.
        int unitsWhenIdle: The amount of battery used when the AED device is idle.
    Outputs:
        None
*/
void AED::setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle)
{
    batteryLevel = startingLevel;
    batteryUnitsPerShock = unitsPerShock;
    batteryUnitsWhenIdle = unitsWhenIdle;
}

/*
    Function: setShockUntilHealthy()
    Purpose: Sets the number of shocks to deliver until the patient is healthy.
    Inputs:
        int shockUntilHealthy: The number of shocks to deliver until the patient is healthy.
    Outputs:
        None
*/
void AED::setShockUntilHealthy(int shockUntilHealthy)
{
    this->shockUntilHealthy = shockUntilHealthy;
}

/*
    Function: setStartWithAsystole()
    Purpose: Sets whether the patient should start with asystole.
    Inputs:
        bool checked: True if the patient should start with asystole, false otherwise.
    Outputs:
        None
*/
void AED::setStartWithAsystole(bool checked)
{
    this->startWithAsystole = checked;
}
