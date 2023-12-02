#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QThread>
#include <QTimer>
#include "AED.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set styling for all indicators.
    stepIndicators << ui->responseIndicator
                   << ui->helpIndicator
                   << ui->attachIndicator
                   << ui->contactIndicator
                   << ui->cprIndicator
                   << ui->shockIndicator;

    QPixmap pixmap(":/Icons/indicator_off.png");
    foreach(auto indicator, stepIndicators)
    {
        indicator->setStyleSheet("background: transparent;");
        indicator->setPixmap(pixmap);
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



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addAED(AED* device)
{
    if (device == nullptr) return;

    this->device = device;
    //Conect signal and slot
    connect(device, SIGNAL(stateChanged(AEDState)), this, SLOT(setCurrentState(AEDState)));
    connect(device, SIGNAL(batteryChanged(int)), this, SLOT(updateBatteryLevel(int)));
}

void MainWindow::selfTest()
{
    // TODO: Update later.
    // Test indicator highlight functionality.
    QThread::msleep (500);

    for (int i = 0; i < stepIndicators.length(); ++i)
    {
        QThread::msleep(500);
        turnOnIndicator(i);
        QThread::msleep(1000);
        turnOffIndicator(i);
    }
    QThread::msleep(100);
    if(!device -> selfTest()){
        setTextMsg(QString("Self test fail!"));
    }
    setTextMsg(QString("Self test success, ready to use!"));
    ui->selftCheckIndicator->setChecked(true);
}

void MainWindow::turnOnIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1) return;

    QPixmap pixmap(":/Icons/indicator_on.png");
    stepIndicators[index]->setPixmap(pixmap);
    stepIndicators[index]->repaint();
    QCoreApplication::processEvents();
}

void MainWindow::turnOffIndicator(int index)
{
    if (index < 0 || index > stepIndicators.length() - 1) return;

    QPixmap pixmap(":/Icons/indicator_off.png");
    stepIndicators[index]->setPixmap(pixmap);
    stepIndicators[index]->repaint();
    QCoreApplication::processEvents();
}

void MainWindow::on_powerBtn_toggled(bool checked)
{
    if(device == nullptr)
        return;
    // Reset timer and remove event listeners.
    timeUpdateCounter->stop();

    disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
    disconnect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::resetElapsedTime);

    // Initiate self-test after the start.
    if (checked)
    {

        device -> powerOn();
        // Disable the patient condition selectors.
        ui->conditionSelector->setEnabled(false);
        ui->numOfRunsSelector->setEnabled(false);
        ui->batteryUnitLayout->setEnabled(false);
        QThread::msleep(500);



        // Set up the time counter.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
        timeUpdateCounter->start(1000);

        // Set the battery spec for the device
        setDeviceBatterySpecs();

        // Set patient heart condition
        setPatientCondition();

        //Go into self test state
        device -> setState(SELF_TEST);
    }
    else
    {
        device -> powerOff();

        // Enable the patient condition selectors.
        ui->conditionSelector->setEnabled(true);
        ui->numOfRunsSelector->setEnabled(true);
        ui->batteryUnitLayout->setEnabled(true);
        // Turn off the self-test indicator.
        ui->selftCheckIndicator->setChecked(false);

        // Set up a reset timer.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::resetElapsedTime);
        timeUpdateCounter->start(5000);
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

void MainWindow::updateElapsedTime()
{
    // Do not update the timer if the device is off.
    if (device -> getState() == OFF) return;

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
    // This should be triggered only if the device was off for the last five minutes.
    if (device -> getState() != OFF) return;

    elapsedTimeSec = 0;
    ui->elapsedTime->setText("00:00");

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


void MainWindow::on_shallowPushButton_clicked()
{
    setCPRDepth(SHALLOW_PUSH);
    setTextMsg(QString("Push harder"));
}

void MainWindow::updateBatteryLevel(int currentLevel)
{
    ui->batteryIndicator->setValue(currentLevel);
}


void MainWindow::on_deepPushButton_clicked()
{
    setCPRDepth(DEEP_PUSH);
    setTextMsg(QString("Perfect stroke"));
}

void MainWindow::setTextMsg(const QString& msg){
    ui -> textMsg -> setText(msg);
    ui -> audioLabel -> setText(msg);
}

void MainWindow::setDeviceBatterySpecs()
{
    int startingValue = ui -> startingBatteryLevel -> value();
    int batteryPerShock = ui -> batteryPerShock-> value();
    int batteryWhenIdle = ui -> batteryWhenIdle -> value();
    device -> setBatterySpecs(startingValue, batteryPerShock, batteryWhenIdle);
}

void MainWindow::setPatientCondition(){
    int patientHeartCondition = ui -> conditionSelector -> currentIndex();
    int numberOfShock = ui -> numOfRunsSelector -> value();
    device -> setPatientHeartCondition((HeartState)patientHeartCondition);
    device -> setShockUntilHealthy(numberOfShock);
}

void MainWindow::setCurrentState(AEDState state){
    //Always disable CPR button unless in CPR state
    ui -> shallowPushButton -> setDisabled(true);
    ui -> deepPushButton -> setDisabled(true);
    switch (state){
        case OFF:

        break;

        case SELF_TEST:
            selfTest();
        break;

        case STANDBY:
            setTextMsg(QString("Stay away from the patient!"));
        break;

        case ANALYZING:
            setTextMsg(QString("Analyzing the patient heart conditioni"));
        break;

        case CHARGING:
            setTextMsg(QString("Charging for shock"));
        break;

        case CPR:
            ui -> shallowPushButton -> setDisabled(false);
            ui -> deepPushButton -> setDisabled(false);
        break;

        case SHOCKING:
            setTextMsg(QString("Shocking the patient"));
        break;
    }

}
