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
struct Heat{
    int heat_x100; // x100 so that variable decreases by 1/ms
    int lastUpdate_ms;
    static const int MAX_HEAT_X100 = 20000;
    Heat(int heat, int currentTime_ms) 
    : heat_x100{heat*100}, lastUpdate_ms{currentTime_ms} {};
    void updateHeat(const int& currTime_ms){
        heat_x100 += (lastUpdate_ms - currTime_ms); // Decrease heat by 1/ms
        lastUpdate_ms = currTime_ms;
        if(heat_x100 < 0) heat_x100 = 0;
    };
    bool overHeated(){
        return heat_x100 > MAX_HEAT_X100;
    };
    bool canShoot(const int& heatGenerated){
        return (heat_x100 + heatGenerated*100) <= MAX_HEAT_X100;
    }
    void addHeat(const int& heat){
        heat_x100 += heat * 100;
    }
}
bool limitPressed = false;
bool wantToShoot = false;
float currRPM = 0.0;
int kickerRunDuration_ms = 500;
Heat heat(0, 0);
bool loaderActivated = false;

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
    // Stop Kicker motor after kickerRunDuration_ms
    if(timer1.isExpired()){
        feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
    }
    
    // Check if shooting is possible and shoot input is pressed, if so then shoot.
    if(wantToShoot && canShoot(&heat, 100, getTimeMilliseconds(), limitPressed)){
        feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
        timer1.restart(kickerRunDuration_ms);
        heat.addHeat(100);
    }
    // Check if jammed, if jammed, unjam and start timer (timer is only started if not already started)
    if(isJammed(currRPM, 1.0, loaderActivated)){
        feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
        if(timer2.isExpired()) timer2.restart(UNJAM_TIMER_MS);
        loaderActivated = false;
    }
    // If not unjamming, run the loader if the ball is not in place, deactivate otherwise
    if (timer2.isExpired()) {
        if(limitPressed){
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
            loaderActivated = false;
        } else {
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
            loaderActivated = true;
        }
    }
    
    
}

/*Declare any helper functions down here (make sure to include in hpp)*/

//void exampleFunc(){
//  do stuff
//};
bool FeederLimitCommand::canShoot(Heat* heat, const int& shotHeat, const int& currTime_ms, const bool& hasBall){
    heat->updateHeat(currTime_ms);
    return hasBall && heat->canShoot(shotHeat);
};
bool FeederLimitCommand::isJammed(const float& currRPM, const float& thresholdRPM, const bool& motorActivated){
    return motorActivated && (currRPM < thresholdRPM);
};



//Do not touch these functions, they are required for the command to work properly
void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isFinished() const { return false; }

}  
#endif
