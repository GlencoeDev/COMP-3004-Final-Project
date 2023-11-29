#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QThread>
#include <QTimer>
#include "defs.h"

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
    // Initiate self-test after the start.
    if (checked)
    {
        QTimer::singleShot(500, this, &MainWindow::selfTest);
    }
    else
    {
        // Turn off the self-test indicator.
        ui->selftCheckIndicator->setChecked(false);
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


