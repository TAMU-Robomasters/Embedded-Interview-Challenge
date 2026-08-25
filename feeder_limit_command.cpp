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
Define any variables you need here, we have provided some here to give you sensor data and other useful things
*/
bool limitPressed = false;
bool wantToShoot = false;

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
    feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor);
    startupThreshold.restart(500);  // delay to wait before attempting unjam
    unjamTimer.restart(0);
    prevTime = tap::arch::clock::getTimeMilliseconds();
}

void FeederLimitCommand::execute() {
    /*
    Update calls to get sensor data and input, dont touch but do use these variables
    to check input states and sensor data
    */

    //updates if the limit switch detects a ball, true = ball detected, false means no ball detected
    limitPressed = feeder->getPressed();
    //gets if the driver wants to shoot a ball, will hold true for a little bit before dropping back down to false
    wantToShoot = (drivers->remote.getSwitch(Remote::Switch::RIGHT_SWITCH) == Remote::SwitchState::UP || drivers->remote.getMouseL()==true || drivers->cvCommunicator.shouldFire());
  
    }
}

void FeederLimitCommand::updateBarrelHeat(){
    uint32_t currTime =  tap::arch::clock::getTimeMilliseconds();
    timeDis = currTime;
    uint32_t timeDiff = currTime - prevTime;
    prevTime = currTime;
    double heatLoss = (24.0/1000.0)*timeDiff;
    heatRegenDis = heatLoss;
    barrelHeat += heatLoss;
    if(barrelHeat > 200){barrelHeat = 200;}
};

void FeederLimitCommand::registerShot(){
    barrelHeat -= 100; 
};

void FeederLimitCommand::end(bool) { feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor); }

bool FeederLimitCommand::isReady() { return true; }

bool FeederLimitCommand::isFinished() const { return false; }

}  // namespace src::Feeder
#endif
