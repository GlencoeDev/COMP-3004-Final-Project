#include <string>
#include <QDebug>
#include <iostream>
#include <QTextStream>
#include <QTimer>

#ifndef AED_H
#define AED_H

#include "HeartState.h"
#include "AEDState.h"

class AED{
    public:
        AED();
        AED(AEDState state, int batteryLevel, bool padsAttached, HeartState patientHeartCondition);
        ~AED();
        void selfTest();
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
    
    private:
        HeartState patientHeartCondition;
        AEDState state;
        bool padsAttached;
        int batteryLevel;
        
}

#endif

