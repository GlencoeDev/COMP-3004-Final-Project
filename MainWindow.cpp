#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , state(OFF)
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

}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::powerOn()
{

}

void MainWindow::powerOff()
{

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
        state = ON;

        QThread::msleep(500);
        selfTest();

        // Set up the time counter.
        connect(timeUpdateCounter, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
        timeUpdateCounter->start(1000);
    }
    else
    {
        state = OFF;

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
    if (state == OFF) return;

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
    if (state == ON) return;

    elapsedTimeSec = 0;
    ui->elapsedTime->setText("00:00");

    QApplication::processEvents();
}
