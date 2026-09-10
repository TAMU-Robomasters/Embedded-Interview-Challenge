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

    //reset state and heat stuff
    state = FeederState::IDLE;
    heat = 0.0f;
    lastHeatUpdateTime = initialTime;
    canShoot = true;
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

    //this is for challenge 2, it will decay heat over time and figure out if we're allowed to fire
    updateHeat();

    //if the state is idle, nothing will happen until the driver wants to shoot and it CAN shoot
    if(state == FeederState::IDLE) {
        if(wantToShoot && canShoot) {
            if(limitPressed) {
                //if a ball is detected, just fire
                startKicking();
            } else{
                //no ball is detected so it needs to load one
                startLoading();
            }
        }
    }

    else if(state == FeederState::LOADING) {
        if (limitPressed) {
            //the ball is there so we can shoot it
            startKicking();
        } else if(timer1.isExpired() && currRPM < JAM_RPM_THRESHOLD) {
            //this is a jam, so I'm using the unjam code you guys provided
            feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::unjamFeederMotor);
            //once unjammed, I need to restart the unjam timer and go to the unjam state
            timer2.restart(UNJAM_TIMER_MS);
            state = FeederState::UNJAMMING;
        }
    } 
    else if(state == FeederState::UNJAMMING) {
        if (timer2.isExpired()) {
            //the pressure is relieved, and it will try loading again
            startLoading();
        }
    } 
    else if(state == FeederState::KICKING) {
        if (timer3.isExpired()) {
            //the ball is fired so kicker is deactivated and the state is idle
            feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::deactivateFeederMotor);
            state = FeederState::IDLE;
        }
    }
}

/*Declare any helper functions down here (make sure to include in hpp)*/

void FeederLimitCommand::updateHeat(){
    uint32_t now = tap::arch::clock::getTimeMilliseconds();
    uint32_t elapsed = now - lastHeatUpdateTime;
    lastHeatUpdateTime = now;

    //this will dissipate heat based on however much time passed since the last tick
    heat -= (elapsed / 1000.0f) * HEAT_DISSIPATION_PER_SEC;
    if (heat < 0.0f) {
        heat = 0.0f;
    }

    //can't allow a shot that will push us over the max
    canShoot = (heat + HEAT_PER_SHOT) <= MAX_HEAT;
}

void FeederLimitCommand::startLoading() {
    //given code
    feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::activateFeederMotor);
    timer1.restart(JAM_CHECK_DELAY_MS);
    state = FeederState::LOADING;
}

void FeederLimitCommand::startKicking() {
    //given code
    feeder->ForFeederMotorGroup(LOADER, &FeederSubsystem::deactivateFeederMotor);
    feeder->ForFeederMotorGroup(KICKER, &FeederSubsystem::activateFeederMotor);
    heat += HEAT_PER_SHOT;
    timer3.restart(KICK_DURATION_MS);
    state = FeederState::KICKING;
}



//Do not touch these functions, they are required for the command to work properly
void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isFinished() const { return false; }

}  
#endif
