#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>>
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

    void addAED(AED* device);

public slots:
    void turnOnIndicator(int index);
    void turnOffIndicator(int index);
    void turnOffAllIndicators();

    // Functions for updating CPR depth compression indicator.
    void setCPRDepth(float depth);
    void setTextMsg(const QString& msg);

    // Updating battery indicator.
    void updateBatteryLevel(int currentLevel);

    // Make UI changes depending on the current state of AED.
    void updateGUI(int state);
    void updatePatientCondition(int condition);
    void updateNumberOfShocks(int shocks);

signals:
    void setPatientHeartCondition(int patientHeartCondition);
    void setShockUntilHealthy(int numberOfShocks);
    void setPadsAttached(bool attached);
    void notifyPadsAttached();
    void setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle);
    void terminate();
    void powerOn();

private slots:
    void on_powerBtn_toggled(bool checked);
    void on_conditionSelector_currentIndexChanged(int index);
    void on_shallowPushButton_clicked();
    void on_deepPushButton_clicked();
    void on_cprPadsAttached_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    void updateElapsedTime();
    void resetElapsedTime();
    void resetStats();

    void toggleBatteryUnitControls(bool enable);

    // Set battery specs on the AED device.
    void setDeviceBatterySpecs();
    void setPatientCondition();
    // Keep a list of the indicators shoing current AED operation step.
    QList<QPushButton*> stepIndicators;

    int currentStep;

    // Used to update the elapsed time.
    QTimer* timeUpdateCounter;

    QTimer* indicatorTimer;

    // Saves elapsed time.
    int elapsedTimeSec;

    AED* device;
    QThread* deviceThread;
};
#endif
