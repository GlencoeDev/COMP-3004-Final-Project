#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QList>
#include "defs.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class AED;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// Implements Power On Self-Test.
    void selfTest();

    void addAED(AED* device);

public slots:
    void turnOnIndicator(int index);
    void turnOffIndicator(int index);

    // Functions for updating CPR depth compression indicator.
    void setCPRDepth(float depth);
    void setTextMsg(const QString& msg);

    // Updating battery indicator.
    void updateBatteryLevel(int currentLevel);
    void setCurrentState(AEDState state);
private slots:
    void on_powerBtn_toggled(bool checked);

    void on_conditionSelector_currentIndexChanged(int index);

    void on_shallowPushButton_clicked();

    void on_deepPushButton_clicked();


private:
    Ui::MainWindow *ui;

    void updateElapsedTime();
    void resetElapsedTime();

    // Set battery specs on the AED device.
    void setDeviceBatterySpecs();
    void setPatientCondition();
    // Keep a list of the indicators shoing current AED operation step.
    QList<QLabel*> stepIndicators;

    // Used to update the elapsed time.
    QTimer* timeUpdateCounter;

    // Saves elapsed time.
    int elapsedTimeSec;

    AED* device;
};
#endif
