#include <string>
#include <QDebug>
#include <iostream>
#include <QTextStream>
#include <QTimer>
#include <QObject>

#ifndef AED_H
#define AED_H

#include "defs.h"

class AED : public QObject{
    Q_OBJECT
    public:
        explicit AED();
        bool selfTest();
        void startProcedure();
        void chargeBattery();
        void moveToCPR();
        void analyzeHeartRhythm();
        void powerOn();
        void powerOff();
        void shock();
        void cancelShock();
        void startHeartRhythmAnalysis();
        // Getters
        HeartState getPatientHeartCondition() const;
        AEDState getState() const;
        bool getPadsAttached() const;
        int getBatteryLevel() const;

        // Setters
        void setPatientHeartCondition(HeartState patientHeartCondition);
        void setState(AEDState state);
        void setPadsAttached(bool padsAttached);
        void setShockUntilHealthy(int shockUntilHealthy);
        void setBatteryLevel(int level);
        // Set bettery config prior to simulation.
        void setBatterySpecs(int startingLevel, int unitsPerShock, int unitsWhenIdle);
    
    private:
        HeartState patientHeartCondition;
        AEDState state;
        bool padsAttached;
        int batteryLevel;
        int shockCount;
        //for simulation purpose.
        int shockUntilHealthy;
        // Indicate battery discharge for each operation.
        int batteryUnitsPerShock = 5;
        int batteryUnitsWhenIdle = 1;
    signals:
        void stateChanged(AEDState state);
        void batteryChanged(int level);
};

#endif

