#include "feeder_limit_command.hpp"

#include "tap/communication/gpio/leds.hpp"
#include "tap/control/command.hpp"

#include "subsystems/feeder/control/feeder.hpp"
#include "utils/tools/common_types.hpp"
#include "utils/ref_system/ref_helper_turreted.hpp"

#include "drivers.hpp"
 
#ifdef FEEDER_COMPATIBLE

namespace src::Feeder {

/*
Define any variables you need here! 
We have provided some here to give you sensor data and other useful things
*/
bool limitPressed = false;
bool wantToShoot = false;
bool loaded = false;
float currRPM = 0.0;
int currentHeat = 0;
const int maxHeat = 200;
const int heat_dis = 10;


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

    //Write Here for Challenge 1, 2, 3!

    void Loaded_Kicker();
}

/*Declare any helper functions down here (make sure to include in hpp)*/

//void exampleFunc(){
//  do stuff
//};

void heat_dissapation {
    //Should dissapate at a rate of 10 heat per second; 1 second = 1000 milliseconds
    //Check if disspation runs all times globally
    for (timer1.restart(0); timer1.getTimeMilliseconds() == 1000; ) {
        currentHeat -= 10;
    }
}

void confirm_loaded {
    currRPM = feeder->getCurrentRPM(0);
    if (feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor && currRPM > 0 && limitPressed)) {
        loaded = true;
    }
    else {
        loaded = false;
        while (loaded == false) {
            //Should give time for unjam to occur, then try to load again; if it fails again, repeat until successful
            unjam_timer.restart(0);
            while (unjam_timer.getTimeMilliseconds() < UNJAM_TIMER_MS) {
                feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
            }
            feeder ->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
            currRPM = feeder->getCurrentRPM(0);
            if (feeder ->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor && currRPM > 0 && limitPressed)) {
                loaded = true;
            }
            else {loaded = false;}
            limitPressed = feeder->getPressed();
        }
    }
}

void Loaded_Kicker() {
    if (currentHeat > maxHeat) {
        currentHeat = maxHeat;
        wantToShoot = false;
        while (currentHeat > maxHeat) {
            heat_dissapation();
        }
    }
    else if (currentHeat < 0) {
        currentHeat = 0;
    }
    else {
        //Only Loads
        if (!limitPressed) {   
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor)
            limitPressed = feeder->getPressed();
            confirm_loaded();
        }
        else if (limitPressed && loaded) {
            if (wantToShoot) {
                //Shoots then adds heat to the system; so load then fire immediately after
                feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
                currentHeat +=100;
            }
        }
        else {
            feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor);
        }
        heat_dissapation();
    }  

}

//Do not touch these functions, they are required for the command to work properly
void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isFinished() const { return false; }

}  
#endif
