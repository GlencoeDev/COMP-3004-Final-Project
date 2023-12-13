#include <string>
#include <QDebug>
#include <iostream>
#include <QTextStream>
#include <QTimer>
#include <QObject>
#include <QWaitCondition>
#include <QMutexLocker>
#include <QMutex>
#include <QThread>
#include <QRandomGenerator>

#ifndef AED_H
#define AED_H

#include "defs.h"

class MainWindow;

class AED : public QObject
{
    Q_OBJECT
public:
    explicit AED();
    ~AED();

    AED *getInstance();

    // Getters
    HeartState getPatientHeartCondition() const;
    AEDState getState() const;
    bool getPadsAttached() const;
    int getBatteryLevel() const;

    // Setters
    void setGUI(MainWindow *mainWindow);

public slots:
    void powerOn();
    void powerOff();

    // Set configs prior to simulation.
    void setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle);
    void setPatientHeartCondition(int patientHeartCondition);
    void setShockUntilHealthy(int shockUntilHealthy);
    void setPadsAttached(bool padsAttached);
    void notifyPadsAttached();
    void setStartWithAsystole(bool checked);
    void setLostConnection(bool simulateConnectionLoss);
    void setBatteryLevel(int level);
    void notifyReconnection();
    void setState(int state);
private slots:
    void cleanUp();
signals:
    // For updating UI state.
    void updateGUI(int state);
    void batteryChanged(int level);
    void updateShockCount(int count);
    void updatePatientCondition(int condition);

private:
    bool nextStep(AEDState state, unsigned long sleepTime, int batteryUsed);
    bool shockable() const;
    void run();
    bool checkPadsAttached();
    void checkConnection();

    HeartState patientHeartCondition;
    bool startWithAsystole;
    AEDState state;
    bool padsAttached;
    int batteryLevel;
    int shockCount;
    bool loseConnection;

    // For simulation purpose.
    int shockUntilHealthy;

    // Indicate battery discharge for each operation.
    int batteryUnitsPerShock = 5;
    int batteryUnitsWhenIdle = 1;

    QMutex padsAttachedMutex;
    QWaitCondition waitForPadsAttachement;

    QMutex restoreConnectionMutex;
    QWaitCondition waitForConnection;


    MainWindow *gui;
    std::unique_ptr<QThread> m_thread;
};

#endif
