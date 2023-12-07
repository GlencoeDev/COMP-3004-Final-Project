#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QThread>
#include <QTimer>
#include "AED.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentStep(-1)
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
    foreach(auto indicator, stepIndicators)
    {
        indicator->setEnabled(false);
    }

    // Disallow clicking on the self-test indicator.
    ui->selftCheckIndicator->setEnabled(false);

    // Set initial CPR depth.
    setCPRDepth(0.0);

    // Time-related code.
    elapsedTimeSec = 0;
    timeUpdateCounter = new QTimer(this);

    // Set default value for patient condition.
    ui->conditionSelector->setCurrentIndex(0);

    // Disable by default since patient is healthy by default.
    ui->numOfRunsSelector->setEnabled(false);

    // Disable the stroke buttons.
    // TODO: Only re-enable stroke buttons.
    ui->shallowPushButton->setEnabled(false);
    ui->deepPushButton->setEnabled(false);
    ui->changeBatteries->setEnabled(false);

    // Set up timer for flashing step indicator.
    indicatorTimer = new QTimer(this);
    connect(indicatorTimer, &QTimer::timeout, this, [this]() {
        if (this->currentStep > -1)
        {
            stepIndicators[this->currentStep]->toggle();
        }
    });
    indicatorTimer->start(500);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addAED(AED* device)
{
    if (device == nullptr) return;

    this->device = device;

    // Conect signals and slots.
    connect(this, SIGNAL(setPatientHeartCondition(int)), device, SLOT(setPatientHeartCondition(int)));
    connect(this, SIGNAL(setShockUntilHealthy(int)), device, SLOT(setShockUntilHealthy(int)));
    connect(this, SIGNAL(setPadsAttached(bool)), device, SLOT(setPadsAttached(bool)));
    connect(this, SIGNAL(notifyPadsAttached()), device, SLOT(notifyPadsAttached()));
    connect(this, SIGNAL(setBatterySpecs(int, int, int)), device, SLOT(setBatterySpecs(int, int, int)));
    connect(this, &MainWindow::powerOn, device, &AED::powerOn);
}

void MainWindow::turnOnIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1) return;

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

void MainWindow::turnOffIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1) return;

    currentStep = -1;

    stepIndicators[index]->setChecked(false);

    QCoreApplication::processEvents();
}

void MainWindow::turnOffAllIndicators()
{
    currentStep = -1;

    foreach(auto indicator, stepIndicators)
    {
        indicator->setChecked(false);
    }

    QCoreApplication::processEvents();
}

void MainWindow::on_powerBtn_toggled(bool checked)
{
    // Reset timer and remove event listeners.
    timeUpdateCounter->stop();
    disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
    disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::resetElapsedTime);

    // Initiate self-test after the start.
    if (checked)
    {
        // Disable all configuration settings.
        ui->conditionSelector->setEnabled(false);
        ui->numOfRunsSelector->setEnabled(false);
        toggleBatteryUnitControls(false);

        ui->padsIndicator->setEnabled(false);

        // Set the battery spec for the device.
        setDeviceBatterySpecs();

        // Set patient heart condition.
        setPatientCondition();

        emit setPadsAttached(ui->cprPadsAttached->isChecked());

        // Start the AED thread.
        emit powerOn();
    }
    else
    {
        // Enable the patient condition selectors.
        ui->conditionSelector->setEnabled(true);
        ui->numOfRunsSelector->setEnabled(true);
        toggleBatteryUnitControls(false);

        ui->padsIndicator->setEnabled(true);

        ui->selftCheckIndicator->setChecked(false);
        ui->powerBtn->setChecked(false);

        // Set up a reset timer.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::resetElapsedTime);
        timeUpdateCounter->start(5000);

        // Terminate the running thread
        emit terminate();
    }
}

void MainWindow::setCPRDepth(float depth)
{
    // Ensure valid range.
    if (depth < 0) return;

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

void MainWindow::toggleBatteryUnitControls(bool enable)
{
    ui->batteryPerShock->setEnabled(enable);
    ui->batteryWhenIdle->setEnabled(enable);
    ui->startingBatteryLevel->setEnabled(enable);
}

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

    QApplication::processEvents();
}

void MainWindow::resetElapsedTime()
{
    elapsedTimeSec = 0;
    ui->elapsedTime->setText("00:00");
    ui->shockCount->setText("00");

    QApplication::processEvents();
}

void MainWindow::on_conditionSelector_currentIndexChanged(int index)
{
    // Disable selector for the number of runs since
    // we can only run it once if the patient is healthy.
    if (index == 0)
    {
        ui->numOfRunsSelector->setValue(1);
        ui->numOfRunsSelector->setEnabled(false);
    }
    else
    {
        ui->numOfRunsSelector->setEnabled(true);
    }
}

void MainWindow::on_deepPushButton_clicked()
{
    setCPRDepth(DEEP_PUSH);
    setTextMsg(QString("GOOD COMPRESSIONS"));
}


void MainWindow::on_shallowPushButton_clicked()
{
    setCPRDepth(SHALLOW_PUSH);
    setTextMsg(QString("PUSH HARDER"));
}

void MainWindow::updateBatteryLevel(int currentLevel)
{
    ui->batteryIndicator->setValue(currentLevel);
    // Starting will always be set to the battery level with which the device finished operation.
    ui->startingBatteryLevel->setValue(currentLevel);
}

void MainWindow::setTextMsg(const QString& msg)
{
    QString displayedMsg = msg;
    QFont labelFont = QApplication::font();

    // Check the length of the message and decrease the font if necessary.
    if (msg.length() > 10)
    {
        auto strs = displayedMsg.split(" ", Qt::SkipEmptyParts);
        displayedMsg.clear();
        for (int i = 0; i < strs.length(); ++i)
        {
            if (displayedMsg.length() == 10)
            {
                displayedMsg += "/n";
            }
            else if (i != strs.length() - 1)
            {
                displayedMsg += " ";
            }

            displayedMsg += strs[i];
        }

       labelFont.setPointSize(8);
    }
    else
    {
        labelFont.setPointSize(11);
    }

    ui->textMsg->setFont(labelFont);
    ui->audioLabel->setFont(labelFont);

    ui->textMsg->setText(msg);
    ui->audioLabel->setText(msg);
}

void MainWindow::setDeviceBatterySpecs()
{
    int startingValue = ui->startingBatteryLevel->value();
    int batteryPerShock = ui->batteryPerShock-> value();
    int batteryWhenIdle = ui->batteryWhenIdle->value();

    // Update battery indicator.
    ui->batteryIndicator->setValue(startingValue);

    // Emit a singal to the AED to set battery specs.
    emit setBatterySpecs(startingValue, batteryPerShock, batteryWhenIdle);
}

void MainWindow::setPatientCondition()
{
    int patientHeartCondition = ui->conditionSelector->currentIndex();
    int numberOfShock = ui->numOfRunsSelector->value();

    // Emit signals to the AED device to set patient condtions.
    emit setPatientHeartCondition(patientHeartCondition);
    emit setShockUntilHealthy(numberOfShock);
}

void MainWindow::updateGUI(int state)
{
    AEDState theState = (AEDState) state;
    switch (theState)
    {
    case OFF:
        setTextMsg("");
        ui->selftCheckIndicator->setChecked(false);
        ui->powerBtn->setChecked(false);
        currentStep = -1;
        break;
    case SELF_TEST_FAIL:
        setTextMsg("UNIT FAILED.");
        ui->selftCheckIndicator->setChecked(false);
        ui->powerBtn->setChecked(false);
        break;
    case SELF_TEST_SUCCESS:
        setTextMsg("UNIT OK.");
        ui->selftCheckIndicator->setChecked(true);
        ui->powerBtn->setChecked(true);

        // Set up the time counter.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
        timeUpdateCounter->start(1000);

        break;
    case CHANGE_BATTERIES:
        setTextMsg("CHANGE BATTERIES");
        ui->selftCheckIndicator->setEnabled(false);

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
        setTextMsg("ANALYZING.");

        // TODO: Update ECG waveform.
    break;
    case NO_SHOCK_ADVISED:
        turnOnIndicator(CONTACT_INDICATOR);
        setTextMsg("NO SHOCK ADVISED");
    break;
    case SHOCK_ADVISED:
        turnOnIndicator(CONTACT_INDICATOR);
        setTextMsg("SHOCK ADVISED");
    break;
    case STAND_CLEAR:
        turnOnIndicator(SHOCK_INDICATOR);
        setTextMsg("STAND CLEAR");
    break;
    case SHOCKING:
        turnOnIndicator(SHOCK_INDICATOR);
        setTextMsg("SHOCK WILL BE DELIVERED IN 1, 2, 3...");
    break;
    case CPR:
        turnOnIndicator(CPR_INDICATOR);
        setTextMsg("START CPR");
        ui->shallowPushButton->setEnabled(true);
        ui->deepPushButton->setEnabled(true);
    break;
    default:
        setTextMsg("");
    break;
    }

    QApplication::processEvents();
}

void MainWindow::updatePatientCondition(int condition)
{
    ui->conditionSelector->setCurrentIndex(condition);
}

void MainWindow::updateNumberOfShocks(int shocks)
{
    ui->shockCount->setText(QString("%1").arg(shocks, 2, 10, QChar('0')));
}

void MainWindow::on_cprPadsAttached_clicked(bool checked)
{
    ui->cprPadsAttached->setChecked(checked);
    ui->padsAttachedIndicator->setChecked(checked);


    if (device->getState() == ATTACH_PADS && checked)
    {
        // Operator is attaching the pads to the patient.
        ui->cprPadsAttached->setEnabled(false);
        device->notifyPadsAttached();
    }
}

