#include "feeder_limit_command.hpp"

#include "tap/communication/gpio/leds.hpp"
#include "tap/control/command.hpp"

#include "subsystems/feeder/control/feeder.hpp"
#include "utils/tools/common_types.hpp"
#include "utils/ref_system/ref_helper_turreted.hpp"

#include "drivers.hpp"

/*Included these two libraries to use sleep and get system time
#include <iostream>

#include <windows.h>
 */
#ifdef FEEDER_COMPATIBLE

namespace src::Feeder {

/*
Define any variables you need here! 
We have provided some here to give you sensor data and other useful things
*/
bool limitPressed = false;
bool wantToShoot = false;
float currRPM = 0.0;
//Initialized variable to hold temp
float BarrelTemp = 0.0;


//Subsystem declarations, do not touch these
FeederLimitCommand::FeederLimitCommand(
    src::Drivers* drivers,
    FeederSubsystem* feeder,
    src::Utils::RefereeHelperTurreted* refHelper,
    int UNJAM_TIMER_MS)
    : drivers(drivers),
      feeder(feeder),
      refHelper(refHelper),
      UNJAM_TIMER_MS(UNJAM_TIMER_MS) {
    addSubsystemRequirement(dynamic_cast<tap::control::Subsystem*>(feeder));
}

void FeederLimitCommand::initialize() {
    //Makes sure robot doesn't shoot when turned on
    feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor);

    //Initialized 6 timers, you can use as many or as few as you like
    timer1.restart(0);
    timer2.restart(0);
    timer3.restart(0);
    timer4.restart(0);
    timer5.restart(0);
    timer6.restart(0); 

    //System time on initialization, might be useful for heat managment
    initialTime = tap::arch::clock::getTimeMilliseconds();
}

void FeederLimitCommand::execute() {
    /*
    Update calls to get sensor data and input, dont touch but do use these variables
    to check input states and sensor data
    */

    //Updates if the limit switch detects a ball 
    //True = ball detected, False means no ball detected
    limitPressed = feeder->getPressed();

    //Gets if the driver wants to shoot a ball
    //Will hold true for a little bit before dropping back down to false
    wantToShoot = (drivers->remote.getSwitch(Remote::Switch::RIGHT_SWITCH) == Remote::SwitchState::UP || drivers->remote.getMouseL()==true || drivers->cvCommunicator.shouldFire());

    //Gets current rpm of the base feeder motor, might be useful for unjam
    currRPM = feeder->getCurrentRPM(0);


    /*To activate motors, call feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::activateFeederMotor);

    To deactivate motors, call feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor);

    specifically for unjam, call feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor); 

    To call a specific motor replace ALL with LOADER or KICKER, these are described in the video.

    Timers
    Units for the timer are in milliseconds, call .restart(x) to activate the timer for x ms.

    To check if a timer has expired, call timer.isExpired(), it will return true if timer is not running/is done.

    Call getTimeMilliseconds() to get current system time.
    */

    //Write Here for Challenge 1, 2, 3!
    if wantToShoot {
        timer2.restart(500);
        if tempAvail(BarrelTemp) && limitPressed {
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
            while timer2.isExpired() == false {}
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
            BarrelTemp += 100;
        }
        else if tempAvail(BarrelTemp) && !limitPressed {
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
            if currRPM == 0 {
                unjamer();
                feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
                timer2.restart(500);
            }
            while timer2.isExpired() == false {}
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
            timer2.restart(500);
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
            while timer2.isExpired() == false {}
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
            BarrelTemp += 100;
        }
    }
}

    timer1.restart(1000);
    if timer1.isExpired() && BarrelTemp >= 10 {
        BarrelTemp -= 10;
        timer1.restart(1000);
    }
    else if timer1.isExpired() && BarrelTemp == 0 {
        timer1.restart(1000);
    }
}

/*Declare any helper functions down here (make sure to include in hpp)*/
bool tempAvail(float temp) {
    if temp > 100 {
        return false;
    } 
    else {
        return true;
    }
}

void unjamer() {
    feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
    timer3.restart(UNJAM_TIMER_MS);
    while timer3.isExpired() == false {
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
    }
    feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
    if currRPM != 0 {
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
        return;
    }
    else {
        unjamer();
    }
}



//void exampleFunc(){
//  do stuff
//};



//Do not touch these functions, they are required for the command to work properly
void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isFinished() const { return false; }

}  
#endif
