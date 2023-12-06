#include <string>
#include <QDebug>
#include <iostream>
#include <QTextStream>
#include <QTimer>
#include <QObject>

#ifndef AED_H
#define AED_H

#include "defs.h"
#include <QThread>

class MainWindow;

class AED : public QObject{
    Q_OBJECT
public:
    explicit AED();
    AED* getInstance();
    // Getters
    HeartState getPatientHeartCondition() const;
    AEDState getState() const;
    bool getPadsAttached() const;
    int getBatteryLevel() const;

    // Setters
    void setBatteryLevel(int level);
    void setGUI(MainWindow* mainWindow);


public slots:
    void powerOn();
    void run();
    // Set configs prior to simulation.
    void setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle);
    void setPatientHeartCondition(HeartState patientHeartCondition);
    void setShockUntilHealthy(int shockUntilHealthy);
    void setPadsAttached(bool padsAttached);

signals:
    // For updating UI state.
    void updateGUI(AEDState state);
    void batteryChanged(int level);
    void updateShockCount(int count);
    void updatePatientCondition(HeartState condition);

private:
    void nextStep(AEDState state, unsigned long sleepTime, int batteryUsed);
    bool shockable() const;

    HeartState patientHeartCondition;
    AEDState state;
    bool padsAttached;
    int batteryLevel;
    int shockCount;

    // For simulation purpose.
    int shockUntilHealthy;

    // Indicate battery discharge for each operation.
    int batteryUnitsPerShock = 5;
    int batteryUnitsWhenIdle = 1;

    MainWindow* gui;
};

#endif
