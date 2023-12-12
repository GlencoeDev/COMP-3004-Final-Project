#include "MainWindow.h"

/*
    Function: MainWindow(QWidget *parent)
    Purpose: Constructor.
    Input:
        parent - The parent widget.
    Output:
        None
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), currentStep(-1)
{
    ui->setupUi(this);

    // Set styling for all indicators.
    stepIndicators << ui->responseIndicator
                   << ui->helpIndicator
                   << ui->padsIndicator
                   << ui->contactIndicator
                   << ui->cprIndicator
                   << ui->shockIndicator;

    QPixmap pixmap(":/Icons/indicator_off.png");
    foreach (auto indicator, stepIndicators)
    {
        indicator->setEnabled(false);
    }

    // Disallow clicking on the self-test indicator.
    ui->selfCheckIndicator->setEnabled(false);
    ui->padsIndicator->setEnabled(false);

    // Set initial CPR depth.
    setCPRDepth(0.0);

    // Time-related code.
    elapsedTimeSec = 0;
    timeUpdateCounter = new QTimer(this);

    // Set default value for patient condition.
    ui->conditionSelector->setCurrentIndex(0);

    // Disable by default since patient is healthy by default.
    ui->numOfRunsSelector->setEnabled(false);
    ui->startWithAsystole->setEnabled(false);

    // Disable the stroke buttons.
    // TODO: Only re-enable stroke buttons.
    ui->shallowPushButton->setEnabled(false);
    ui->deepPushButton->setEnabled(false);
    ui->changeBatteries->setEnabled(false);
    ui->reconnectBtn->setEnabled(false);

    // Set up timer for flashing step indicator.
    indicatorTimer = new QTimer(this);
    connect(indicatorTimer, &QTimer::timeout, this, [this]()
            {
        if (this->currentStep > -1)
        {
            stepIndicators[this->currentStep]->toggle();
        } });
    indicatorTimer->start(500);
}

/*
    Function: ~MainWindow()
    Purpose: Destructor.
    Input:
        None
    Output:
        None
*/
MainWindow::~MainWindow()
{
    delete ui;
}

/*
    Function: addAED(AED *device)
    Purpose: Add an AED device to the GUI.
    Input:
        device - A pointer to the AED device.
    Output:
        None
*/
void MainWindow::addAED(AED *device)
{
    if (device == nullptr)
        return;

    this->device = device;

    // Conect signals and slots.
    connect(this, SIGNAL(setPatientHeartCondition(int)), device, SLOT(setPatientHeartCondition(int)));
    connect(this, SIGNAL(setStartWithAsystole(bool)), device, SLOT(setStartWithAsystole(bool)));
    connect(this, SIGNAL(setShockUntilHealthy(int)), device, SLOT(setShockUntilHealthy(int)));
    connect(this, SIGNAL(setPadsAttached(bool)), device, SLOT(setPadsAttached(bool)));
    connect(this, SIGNAL(notifyPadsAttached()), device, SLOT(notifyPadsAttached()));
    connect(this, SIGNAL(setBatterySpecs(int, int, int)), device, SLOT(setBatterySpecs(int, int, int)));
    connect(this, &MainWindow::powerOn, device, &AED::powerOn);
    connect(this, &MainWindow::powerOff, device, &AED::powerOff);
    connect(this, &MainWindow::notifyReconnection, device, &AED::notifyReconnection);
    connect(this, &MainWindow::setLostConnection, device, &AED::setLostConnection);
}

/*
    Function: turnOnIndicator(int index)
    Purpose: Turn on the indicator at the specified index. Here the index
             is the index of the indicator in the stepIndicators list.
    Input:
        index - The index of the indicator to turn on.
    Output:
        None
*/
void MainWindow::turnOnIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1)
        return;

    currentStep = index;

    for (int i = 0; i < stepIndicators.length(); ++i)
    {
        if (index == i)
        {
            if (!stepIndicators[i]->isChecked())
            {
                stepIndicators[i]->setChecked(true);
            }
        }
        else
        {
            if (stepIndicators[i]->isChecked())
            {
                stepIndicators[i]->setChecked(false);
            }
        }
    }

    QCoreApplication::processEvents();
}

/*
    Function: turnOffIndicator(int index)
    Purpose: Turn off the indicator at the specified index. Here the index
             is the index of the indicator in the stepIndicators list.
    Input:
        index - The index of the indicator to turn off.
    Output:
        None
*/
void MainWindow::turnOffIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1)
        return;

    currentStep = -1;

    stepIndicators[index]->setChecked(false);

    QCoreApplication::processEvents();
}

/*
    Function: turnOffAllIndicators()
    Purpose: Turn off all indicators.
    Input:
        None
    Output:
        None
*/
void MainWindow::turnOffAllIndicators()
{
    currentStep = -1;

    foreach (auto indicator, stepIndicators)
    {
        indicator->setChecked(false);
    }

    QCoreApplication::processEvents();
}

/*
    Function: MainWindow::on_powerBtn_toggled(bool checked)
    Purpose: Turn on/off the AED device.
    Input:
        checked - Whether the button is checked.
    Output:
        None
*/
void MainWindow::on_powerBtn_toggled(bool checked)
{
    // Initiate self-test after the start.
    if (checked)
    {
        // Disable all configuration settings.
        ui->conditionSelector->setEnabled(false);
        ui->numOfRunsSelector->setEnabled(false);
        toggleBatteryUnitControls(false);

        // Disable loss connection selector.
        ui->connectionLoss->setEnabled(false);

        // Disable pads selector.
        ui->padsSelector->setEnabled(false);

        // Set the battery spec for the device.
        setDeviceBatterySpecs();

        // Set patient heart condition.
        setPatientCondition();

        // Set other conditions prior to running the device.
        emit setPadsAttached(ui->cprPadsAttached->isChecked());
        emit setLostConnection(ui->connectionLoss->isChecked());

        // Start the AED thread.
        emit powerOn();

        ui->powerBtn->setChecked(true);

        // Reset timer and remove event listeners.
        disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
        timeUpdateCounter->start();
    }
    else
    {
        if (device->getState() == OFF)
        {
            return;
        }
        else
        {
            // Stop the thread.
            // Ensure that the AED thread is running.
            emit device->powerOff();
            QThread::msleep(100);
        }

        // Reset timer and remove event listeners.
        timeUpdateCounter->stop();
        disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);

        turnOffAllIndicators();
        setTextMsg("");

        // Enable the patient condition selectors.
        ui->conditionSelector->setEnabled(true);
        ui->numOfRunsSelector->setEnabled(true);
        toggleBatteryUnitControls(true);

        ui->cprPadsAttached->setChecked(false);
        ui->cprPadsAttached->setEnabled(true);
        ui->padsIndicator->setChecked(false);

        ui->selfCheckIndicator->setChecked(false);

        // Disable pads selector.
        ui->padsSelector->setEnabled(true);

        // Re-enable loss connection selector.
        ui->connectionLoss->setEnabled(true);

        ui->powerBtn->setChecked(false);

        device->setState(OFF);

        // Set up a reset timer.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::resetElapsedTime);
        timeUpdateCounter->start(5000);
    }
}

/*
    Function: setCPRDepth(float depth)
    Purpose: Set the CPR depth.
    Input:
        depth - The depth of the CPR.
    Output:
        None
*/
void MainWindow::setCPRDepth(float depth)
{
    // Ensure valid range.
    if (depth < 0)
        return;

    // No CPR is taking place.
    if (depth == 0)
    {
        ui->cprDepth0cm->setFixedHeight(0);
        ui->cprDepth5cm->setFixedHeight(0);
        ui->cprDepth6cm->setFixedHeight(0);

        ui->cprDepthMark5cm->setVisible(false);
        ui->cprDepthMark6cm->setVisible(false);
    }

    // Fill the first bar.
    if (ui->cprDepth0cm != nullptr)
    {
        ui->cprDepth0cm->setFixedHeight((depth > 5.0 ? 5.0 : depth) * CM_PIX_RATIO);
    }
    // Fill the second bar.
    if (ui->cprDepth5cm != nullptr)
    {
        float diff = depth - 5.0;
        ui->cprDepth5cm->setFixedHeight((diff <= 0 ? 0 : (diff >= 1.0 ? 1.0 : diff)) * CM_PIX_RATIO);
    }
    // Fill the third bar.
    if (ui->cprDepth6cm != nullptr)
    {
        float diff = depth - 6.0;
        ui->cprDepth6cm->setFixedHeight((diff <= 0 ? 0 : (diff >= 1.0 ? 1.0 : diff)) * CM_PIX_RATIO);
    }

    // Set up the depth marks.
    ui->cprDepthMark5cm->setVisible(depth >= 5.0);
    ui->cprDepthMark6cm->setVisible(depth >= 6.0);

    QApplication::processEvents();
}

/*
    Function: toggleBatteryUnitControls(bool enable)
    Purpose: Enable/disable battery unit controls.
    Input:
        enable - Whether to enable/disable the controls.
    Output:
        None
*/
void MainWindow::toggleBatteryUnitControls(bool enable)
{
    ui->batteryPerShock->setEnabled(enable);
    ui->batteryWhenIdle->setEnabled(enable);
    ui->startingBatteryLevel->setEnabled(enable);
}

/*
    Function: updateElapsedTime()
    Purpose: Update the elapsed time.
    Input:
        None
    Output:
        None
*/
void MainWindow::updateElapsedTime()
{
    elapsedTimeSec++;

    // Find seconds and timer;
    int seconds = elapsedTimeSec % 60;
    int minutes = elapsedTimeSec / 60;

    // Reset the timer if needed.
    if (seconds == 59 && minutes == 59)
    {
        elapsedTimeSec = 0;
        seconds = 0;
        minutes = 0;
    }

    // Set the timer label.
    QString timerStr = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    ui->elapsedTime->setText(timerStr);

    // Decrease battery.
    int currentBatteryLevel = ui->startingBatteryLevel->value();
    int batteryWhenIdle = ui->batteryWhenIdle->value();
    currentBatteryLevel -= batteryWhenIdle;
    device->setBatteryLevel(currentBatteryLevel);
    updateBatteryLevel(currentBatteryLevel);

    QApplication::processEvents();
}

/*
    Function: resetElapsedTime()
    Purpose: Reset the elapsed time.
    Input:
        None
    Output:
        None
*/
void MainWindow::resetElapsedTime()
{
    elapsedTimeSec = 0;
    ui->elapsedTime->setText("00:00");
    ui->shockCount->setText("SHOCKS: 00");

    QApplication::processEvents();
}

/*
    Function: on_conditionSelector_currentIndexChanged(int index)
    Purpose: Update the number of runs selector depending on the patient condition.
    Input:
        index - The index of the patient condition.
    Output:
        None
*/
void MainWindow::on_conditionSelector_currentIndexChanged(int index)
{
    // Disable selector for the number of runs since
    // we can only run it once if the patient is healthy.
    if (index == 0)
    {
        ui->numOfRunsSelector->setValue(1);
        ui->numOfRunsSelector->setEnabled(false);
        ui->startWithAsystole->setEnabled(false);
    }
    else
    {
        ui->numOfRunsSelector->setEnabled(true);
        ui->startWithAsystole->setEnabled(true);
    }
}

/*
    Function: on_shallowPushButton_clicked()
    Purpose: Set the CPR depth to shallow.
    Input:
        None
    Output:
        None
*/
void MainWindow::on_deepPushButton_clicked()
{
    setCPRDepth(DEEP_PUSH);
    setTextMsg(QString("GOOD COMPRESSIONS"));
}

/*
    Function: on_shallowPushButton_clicked()
    Purpose: Set the CPR depth to deep push.
    Input:
        None
    Output:
        None
*/
void MainWindow::on_shallowPushButton_clicked()
{
    setCPRDepth(SHALLOW_PUSH);
    setTextMsg(QString("PUSH HARDER"));
}

/*
    Function: updateBatteryLevel(int currentLevel)
    Purpose: Update the battery level.
    Input:
        currentLevel - The current battery level.
    Output:
        None
*/
void MainWindow::updateBatteryLevel(int currentLevel)
{
    ui->batteryIndicator->setValue(currentLevel);
    // Starting will always be set to the battery level with which the device finished operation.
    ui->startingBatteryLevel->setValue(currentLevel);
}

/*
    Function: setTextMsg(const QString& msg)
    Purpose: Set the text message.
    Input:
        msg - The message to display.
    Output:
        None
*/
void MainWindow::setTextMsg(const QString &msg)
{
    QString displayedMsg = msg;
    QFont labelFont = QApplication::font();

    // Check the length of the message and decrease the font if necessary.
    if (msg.length() > 10)
    {
        labelFont.setPointSize(10);
        ui->textMsg->setWordWrap(true);
        ui->audioLabel->setWordWrap(true);
    }
    else
    {
        labelFont.setPointSize(12);
    }

    ui->textMsg->setFont(labelFont);
    ui->audioLabel->setFont(labelFont);

    ui->textMsg->setText(msg);
    ui->audioLabel->setText(msg);
}

/*
    Function: setDeviceBatterySpecs()
    Purpose: Set the battery specs for the AED device.
    Input:
        None
    Output:
        None
*/
void MainWindow::setDeviceBatterySpecs()
{
    int startingValue = ui->startingBatteryLevel->value();
    int batteryPerShock = ui->batteryPerShock->value();
    int batteryWhenIdle = ui->batteryWhenIdle->value();

    // Update battery indicator.
    ui->batteryIndicator->setValue(startingValue);

    // Emit a singal to the AED to set battery specs.
    emit setBatterySpecs(startingValue, batteryPerShock, batteryWhenIdle);
}

/*
    Function: setPatientCondition()
    Purpose: Set the patient condition.
    Input:
        None
    Output:
        None
*/
void MainWindow::setPatientCondition()
{
    int patientHeartCondition = ui->conditionSelector->currentIndex();
    int numberOfShock = ui->numOfRunsSelector->value();

    // Emit signals to the AED device to set patient condtions.
    emit setPatientHeartCondition(patientHeartCondition);
    emit setShockUntilHealthy(numberOfShock);
    emit setStartWithAsystole(ui->startWithAsystole->isChecked());
}

/*
    Function: updateECGDisplay(const QString &image)
    Purpose: Update the ECG display.
    Input:
        image - The image to display.
    Output:
        None
*/
void MainWindow::updateECGDisplay(const QString &image)
{
    QPixmap pixmap;
    pixmap.load(image);
    ui->ecgDisplay->setPixmap(pixmap);
}

/*
    Function: updateECGDisplay(HeartState state)
    Purpose: Update the ECG display depending on the heart condition of the patient.
    Input:
        state - The heart condition of the patient.
    Output:
        None
*/
void MainWindow::updateECGDisplay(HeartState state)
{
    switch (state)
    {
    case SINUS_RHYTHM:
        updateECGDisplay(QString("://Icons/ECG_SINUS.png"));
        break;

    case VENTRICULAR_FIBRILLATION:
        updateECGDisplay(QString("://Icons/ventricullar_fibrillation.png"));
        break;

    case VENTRICULAR_TACHYCARDIA:
        updateECGDisplay(QString("://Icons/ventricular_tachycardia.png"));
        break;
    }
}

/*
    Function: updateGUI(int state)
    Purpose: Update the GUI depending on the state of the AED device.
    Input:
        state - The state of the AED device.
    Output:
        None
*/
void MainWindow::updateGUI(int state)
{
    AEDState theState = (AEDState)state;
    switch (theState)
    {
    case OFF:
        setTextMsg("");
        ui->selfCheckIndicator->setChecked(false);
        ui->powerBtn->setChecked(false);
        currentStep = -1;
        break;

    case SELF_TEST_FAIL:
        setTextMsg("UNIT FAILED");
        ui->selfCheckIndicator->setChecked(false);
        QTimer::singleShot(2000, this, [this]()
                           { this->ui->powerBtn->setChecked(false); });

        break;

    case SELF_TEST_SUCCESS:
        setTextMsg("UNIT OK");
        ui->selfCheckIndicator->setChecked(true);
        ui->powerBtn->setChecked(true);

        // Set up the time counter.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
        timeUpdateCounter->start(1000);

        break;

    case CHANGE_BATTERIES:
        setTextMsg("CHANGE BATTERIES");
        ui->selfCheckIndicator->setEnabled(false);

        // Block all UI elements until the change batteries.
        toggleBatteryUnitControls(false);
        ui->patientInfoBox->setEnabled(false);

        // Enable the button for switching batteries;
        ui->changeBatteries->setEnabled(true);
        break;

    case STAY_CALM:
        setTextMsg("STAY CALM");
        break;

    case CHECK_RESPONSE:
        setTextMsg("CHECK RESPONSIVENESS");
        turnOnIndicator(RESPONSE_INDICATOR);
        break;

    case CALL_HELP:
        setTextMsg("CALL HELP");
        turnOnIndicator(HELP_INDICATOR);
        break;

    case ATTACH_PADS:
        turnOnIndicator(PADS_INDICATOR);
        // Check the UI whether the pads button is checked.
        if (!ui->cprPadsAttached->isChecked())
        {
            setTextMsg("ATTACH DEFIB PADS");

            // Allow user to select pads.
            ui->padsSelector->setEnabled(true);

            // Prompt the user to attach the pads if necessary.
            ui->cprPadsAttached->setEnabled(true);
        }
        else
        {
            setTextMsg("");
            ui->cprPadsAttached->setEnabled(false);
        }
        break;

    case ANALYZING:
        turnOnIndicator(CONTACT_INDICATOR);
        setTextMsg("ANALYZING");
        break;

    case LOST_CONNECTION:
        setTextMsg("PLUG IN CABLE");
        ui->reconnectBtn->setEnabled(true);
        break;

    case NO_SHOCK_ADVISED:
        turnOnIndicator(CONTACT_INDICATOR);
        setTextMsg("NO SHOCK ADVISED");

        if (ui->startWithAsystole->isChecked() &&
            device->getPatientHeartCondition() != SINUS_RHYTHM)
        {
            updateECGDisplay("://Icons/asystole.png");
        }
        else
        {
            updateECGDisplay("://Icons/ECG_SINUS.png");
        }
        break;

    case SHOCK_ADVISED:
        turnOnIndicator(CONTACT_INDICATOR);
        setTextMsg("SHOCK ADVISED");

        // TODO: Update ECG waveform.
        updateECGDisplay(device->getPatientHeartCondition());
        break;

    case STAND_CLEAR:
        turnOnIndicator(SHOCK_INDICATOR);
        setTextMsg("STAND CLEAR");
        break;

    case SHOCKING:
        turnOnIndicator(SHOCK_INDICATOR);
        setTextMsg("SHOCK WILL BE DELIVERED IN THREE, TWO, ONE...");
        break;

    case SHOCK_DELIVERED:
        turnOnIndicator(SHOCK_INDICATOR);
        setTextMsg("SHOCK DELIVERED");
        break;

    case CPR:
        turnOnIndicator(CPR_INDICATOR);
        setTextMsg("START CPR");
        ui->shallowPushButton->setEnabled(true);
        ui->deepPushButton->setEnabled(true);
        break;

    case STOP_CPR:
        setTextMsg("STOP CPR");
        // Disable CPR button
        ui->shallowPushButton->setEnabled(false);
        ui->deepPushButton->setEnabled(false);
        setCPRDepth(0.0);
        break;

    case ABORT:
        setTextMsg("");

        // Turn off the device.
        ui->powerBtn->toggle();
        ui->selfCheckIndicator->setEnabled(false);

        // Turn off all indicators
        turnOffAllIndicators();

        // Remove ECG waveforms.
        ui->ecgDisplay->clear();
        break;

    default:
        setTextMsg("");
        break;
    }

    QApplication::processEvents();
}

/*
    Function: updatePatientCondition(int condition)
    Purpose: Update the patient condition.
    Input:
        condition - The condition of the patient.
    Output:
        None
*/
void MainWindow::updatePatientCondition(int condition)
{
    ui->conditionSelector->setCurrentIndex(condition);
}

/*
    Function: updateNumberOfShocks(int shocks)
    Purpose: Update the number of shocks.
    Input:
        shocks - The number of shocks.
    Output:

*/
void MainWindow::updateNumberOfShocks(int shocks)
{
    // Set number of shocks
    ui->shockCount->setText(QString("SHOCKS: %1").arg(shocks, 2, 10, QChar('0')));
}

/*
    Function: on_cprPadsAttached_clicked(bool checked)
    Purpose: Attach the pads to the patient.
    Input:
        checked - Whether the pads are attached.
    Output:
        None
*/
void MainWindow::on_cprPadsAttached_clicked(bool checked)
{
    ui->cprPadsAttached->setChecked(checked);
    ui->padsAttachedIndicator->setChecked(checked);

    if (device->getState() <= ATTACH_PADS && checked)
    {
        if (device->getState() > OFF)
        {
            // Disable selector once the pads were attached.
            ui->cprPadsAttached->setEnabled(false);
        }

        if (device->getState() == ATTACH_PADS)
        {
            // Display the kind of pads that were attached.
            bool adultPads = ui->padsSelector->currentIndex() == 0;
            setTextMsg(QString("%1 PADS").arg(adultPads ? "ADULT" : "PEDIATRIC"));

            // Operator is attaching the pads to the patient.
            device->notifyPadsAttached();
        }
        else
        {
            device->notifyPadsAttached();
        }
    }
}

/*
    Function: on_changeBatteries_clicked()
    Purpose: Change the batteries.
    Input:
        None
    Output:
        None
*/
void MainWindow::on_changeBatteries_clicked()
{
    // Reset the battery to max battery level.
    ui->startingBatteryLevel->setValue(MAX_BATTERY_LEVEL);
    device->setBatteryLevel(MAX_BATTERY_LEVEL);
}

/*
    Function: on_reconnectBtn_clicked()
    Purpose: Reconnect the device.
    Input:
        None
    Output:
        None
*/
void MainWindow::on_reconnectBtn_clicked()
{
    // Reset connection setting.
    ui->connectionLoss->setChecked(false);

    // Disable reconnectBtn.
    ui->reconnectBtn->setEnabled(false);
    emit setLostConnection(true);
    device->notifyReconnection();
}
