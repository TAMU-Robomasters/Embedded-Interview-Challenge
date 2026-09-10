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
float currRPM = 0.0;
const int KICKER_INTERVAL = 500;
bool kickerRunning = false;
float heat = 0.0;
int currentHeatTime = 0;
int previousHeatTime = 0;
int elapsedHeatTime = 0;
bool loaderRunning = false;
bool unjamming = false;


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
    timer1.restart(0); //kicker interval
    timer2.restart(0); //unjam timer
    timer3.restart(0);
    timer4.restart(0);
    timer5.restart(0);
    timer6.restart(0);

    //System time on initialization, might be useful for heat managment
    initialTime = tap::arch::clock::getTimeMilliseconds();
    previousHeatTime = initialTime;
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
    
    currentHeatTime = tap::arch::clock::getTimeMilliseconds();
    //checking for unjamming
    if(!unjamming){
        //loading and shooting ball
        if(!limitPressed) {
            //load ball if no ball loaded
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
            loaderRunning = true;
            //check for jam
            if(loaderRunning && (currRPM == 0)) {
                unjamming = true;
                timer2.restart(UNJAM_TIMER_MS);
                feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
            }
        } else if(limitPressed) {
            //stop loading ball when ball is there
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
            loaderRunning = false;
        }
    } else {
        //stop unjamming if done
        if(timer2.isExpired()) {
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
            loaderRunning = false;
            unjamming = false;
        }
    }
    if(wantToShoot && limitPressed && (heat <= 100)) {
        //shoot when ball is loaded
        if(!kickerRunning && timer1.isExpired()) {
            //start kicker if kicker is not already on and timer1 is not running
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
            timer1.restart(KICKER_INTERVAL);
            kickerRunning = true;
            //add heat
            heat += 100;
        }
    } else if(!wantToShoot && timer1.isExpired()) {
        feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
        kickerRunning = false;
    }
    //heat management
    elapsedHeatTime = currentHeatTime - previousHeatTime;
    heat -= elapsedHeatTime / 100.0; //divide by 1000 and multiply by 10 = divide by 100
    if(heat < 0) {
        heat = 0.0;
    }
    previousHeatTime = currentHeatTime;
}

/*Declare any helper functions down here (make sure to include in hpp)*/

//void exampleFunc(){
//  do stuff
//};



//Do not touch these functions, they are required for the command to work properly
void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isFinished() const { return false; }

}  
#endif
