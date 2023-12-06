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
    ~AED();
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

    // Set configs prior to simulation.
    void setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle);
    void setPatientHeartCondition(int patientHeartCondition);
    void setShockUntilHealthy(int shockUntilHealthy);
    void setPadsAttached(bool padsAttached);

private slots:
    void cleanUp();
signals:
    // For updating UI state.
    void updateGUI(int state);
    void batteryChanged(int level);
    void updateShockCount(int count);
    void updatePatientCondition(int condition);

private:
    void nextStep(AEDState state, unsigned long sleepTime, int batteryUsed);
    bool shockable() const;
    void run();
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
    std::unique_ptr<QThread> m_thread;
};

#endif
