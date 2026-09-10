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

enum FeederState{
    IDLE, 
    LOADING, 
    READY_TO_SHOOT, 
    SHOOTING, 
    UNJAMMING, 
    OVERHEATED
}; 

FeederState state = IDLE; 

//These define the variables for Challenge 1 = Feeder Logic
bool loadCheck = false; 
bool loading = false; 

//These define the variables/constants for Challenge 2 - Heat Checks
float heat = 0.0f; 

//Heat constants
const float MAX_HEAT = 200.0f; 
const float SHOOT_HEAT = 100.0f; 
const float HEAT_DISSIPATION = 10.0f; 
uint32_t heatUpdate = 0; 

//These help define the variables for Challenge 3 - Unjamming
int kickTimeMS = 100; 
int loadStartMS = 100; 
float loaderThreshold = 1.0f; 


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


    //This sets everything to the default values before running the program
    state = IDLE;
    loadCheck = false;
    loading = false;

    heat = 0.0f;
    heatUpdate = initialTime;
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

   
    /**
     * Heat Dissipation - Challenge 2
     * If the machine is overheating, the following code cools down the feeder over a period of time
     */
    uint32_t currentTime =
    tap::arch::clock::getTimeMilliseconds();

    float elapsedSeconds =
        (currentTime - heatUpdate) / 1000.0f;

    heat -= elapsedSeconds * HEAT_DISSIPATION;

    if (heat < 0.0f) {
        heat = 0.0f;
    }

    heatUpdate = currentTime;

    /**
     * Check Overheating - Challenge 2
     * This compares the current heat of the robot to the shooting heat to prevent overheating of the robot system
     * 
     */
    if (state == OVERHEATED){
        feeder->ForFeederMotorGroup(ALL,
            &FeederSubsystem::deactivateFeederMotor); 

        if (heat < SHOOT_HEAT){
            state = IDLE; 
        }

        return; 
    }

    //Loading and Shooting Mechanism (State Machine) - Challenge 1/Challenge 2 
    switch(state){
        //This is the initial state for the feeder mechanism (default state)
        case IDLE: {
            feeder->ForFeederMotorGroup(
                ALL,
                &FeederSubsystem::deactivateFeederMotor); 

                loading = false; 

                //This checks if the loading mechanism is overheated before the mechanism load
                if (wantToShoot){
                    if (heat + SHOOT_HEAT > MAX_HEAT){
                        state = OVERHEATED; 
                    }
                    else{
                        state = LOADING; 
                        loadCheck = false; 
                    }
                }
            break; 
        }
         //This is the actual loading state for the feeder mechanism
        case LOADING:{
            // This runs the loader forward until it hits the limit switch
            feeder->ForFeederMotorGroup(
                LOADER,
                &FeederSubsystem::activateFeederMotor);

            //The startup timer begins when the mechanosm starts loading
            if(!loading){
                timer2.restart(loadStartMS);
                loading = true; 
            }

            // When the limit switch is pressed, this the mechanism stops loading
            if(limitPressed){
                feeder->ForFeederMotorGroup(
                    LOADER,
                    &FeederSubsystem::deactivateFeederMotor); 

                loading = false; 
                loadCheck = true; 
                state = READY_TO_SHOOT; 

                break; 
            }

            //Jam Detection - Challenge 3
            //If the loader is suppposed to move but its RPM is zero, it's movement will be reversed for the unjam mechanism
            if (timer2.isExpired()
                && currRPM < loaderThreshold) {
                feeder->ForFeederMotorGroup(
                    LOADER,
                    &FeederSubsystem::unjamFeederMotor);


                timer3.restart(UNJAM_TIMER_MS);

                loading = false;
                state = UNJAMMING;
            }
            break; 
        }

        //This is the state that checks/confims if shooting will occur
        case READY_TO_SHOOT: {
            //When the ball is loaded, the loading mechanism stops
            feeder->ForFeederMotorGroup(
                LOADER,
                &FeederSubsystem::deactivateFeederMotor); 
            
            //If the heat limit is exceeded the mechanism will not shoot
            if(heat + SHOOT_HEAT > MAX_HEAT){
                state = OVERHEATED; 
                break; 
            }

            //The kicker motor fires the ball
            if(wantToShoot && loadCheck){
                feeder->ForFeederMotorGroup(
                    KICKER,
                    &FeederSubsystem::activateFeederMotor);

                timer1.restart(kickTimeMS); 

                state = SHOOTING; 
            }

            break; 

        }

        //This is the case that commands the actual shooting of the ball
        case SHOOTING: {
            //This keeps the kicker running for a set amount of time
            feeder->ForFeederMotorGroup(
                KICKER,
                &FeederSubsystem::activateFeederMotor);

            if(timer1.isExpired()){
                feeder->ForFeederMotorGroup(
                KICKER,
                &FeederSubsystem::deactivateFeederMotor);
                
                //After the shooting is successful, heat will be added
                heat += SHOOT_HEAT; 
                if(heat >= MAX_HEAT){
                    heat = MAX_HEAT; 
                    state = OVERHEATED; 
                }
                else{
                    loadCheck = false; 
                    state = IDLE;  
                }
            }

            break; 
        }

        //This occurs when the mechanism is jammed
        case UNJAMMING: {
            //This reverses the motor until the unjam timer expires
            feeder->ForFeederMotorGroup(
                LOADER,
                &FeederSubsystem::unjamFeederMotor);

             if (timer3.isExpired()) {
                feeder->ForFeederMotorGroup(
                    LOADER,
                    &FeederSubsystem::deactivateFeederMotor);

                loading = false;
                state = LOADING;
            }

            break;
        }

        //This is included to supplement the case above
        case OVERHEATED: {
            feeder->ForFeederMotorGroup(
                ALL,
                &FeederSubsystem::deactivateFeederMotor);

            break;
        }
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
