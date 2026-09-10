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
bool limitPressed = false; //if ball is present
bool wantToShoot = false; 
float currRPM = 0.0; //rpm of feeder
//variables i added
//All potential states of action
bool Loading = false;
bool Kicking = false;
bool Jammed = false;
float heatLevel = 0.0;
int lastHeatUpdate = 0;
// all arbitrary timings based on video in github
const int loadTime = 1000;
const int kickTime = 1200 ;
const int desRPM = 12;

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

    //I added this
    //establishes initial states of action
    Loading = false;
    Kicking = false;
    Jammed = false;
    heatLevel = 0.0;
    lastHeatUpdate = initialTime;
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
    
    //Overheat Mechanic
    int currentTime = tap::arch::clock::getTimeMilliseconds();
    int elapsed = currentTime - lastHeatUpdate;
    //lower heat bound
    if(heatLevel < 0) heatLevel = 0;
    //heat depletion 10/1s = 10/1000ms = 1/100ms
    if(elapsed > 0){
        float decrement = elapsed/100.0f;
        heatLevel -= decrement;
        lastHeatUpdate = currentTime;
    }
    //Overheat state
    bool overheat = (heatLevel >= 200);
    //heat continues in shoot mechanic

    //Shooting Mechanic
    if(wantToShoot && limitPressed && !Loading && !Kicking && !Jammed && !overheat){
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
        timer1.restart(loadTime);
        timer4.restart(0);
        Loading = true;
    }
    //checking if loading done
    if(Loading && timer1.isExpired() && limitPressed){
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
        Loading = false;
        //start kicking once loading done
        feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
        timer2.restart(kickTime);
        Kicking = true;
    }
    //checking if kicking done
    if(Kicking && timer2.isExpired()){
        feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
        Kicking = false;
        //increments heat once shot is shot
        heatLevel += 100;
        //upper heat bound
        if(heatLevel > 200) heatLevel = 200;
    }
    
    //jamming mechanic
    //if loading is looked slow we start jam counteraction
    if(Loading && limitPressed && currRPM < desRPM && timer4.get()>100){
        Jammed = true;
        Loading = false;
        //stop loading
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
        //start unjamming
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
        timer3.restart(UNJAM_TIMER_MS);
    }
    //checks if unjamming is finished
    if(Jammed && timer3.isExpired()){
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
        Jammed = false;
    }
    
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
