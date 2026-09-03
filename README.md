# Embedded-Interview-Challenge

You will not need to modify the .hpp, only the .cpp file.

Logic should be written under the execute function. Variables should be initialized near the top of the file. Feel free to write any helper functions (DEFINE THEM IN THE .hpp FILE!).
Please read the comments before attempting, we define commands, timers, variables, and other useful things throughout the files.


## Helpful Commands


### Motor Control
To activate motors, call
**feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::activateFeederMotor);**

To deactivate motors, call
**feeder->ForFeederMotorGroup(ALL, &FeederSubsystem::deactivateFeederMotor);**

To call a specific motor replace ALL with LOADER or KICKER, these are described in the video.


### Timers
Units for the timer are in milliseconds, call **.restart(x)** to activate the timer for x ms. 

To check if a timer has expired, call **timer.isExpired()**, it will return true if timer is not running/is done.

Call **getTimeMilliseconds()** to get current system time.


## Challenges
- Challenge 1 **(Required)**
    - Hero is our hard hitting robot with a semi-automatic turret, shooting a 42mm projectile (aka a golf ball). The robot must first load the shot using the LOADER motor. Once loaded, you must shoot the shot using the KICKER motor. Using the variables and functions that you create and are given, create the logic behind our Hero's feeder subsystem using C++ to load and shoot. 

<<<<<<< HEAD
- Challenge 2 **(Required)**
    - Barrel Heat is a mechanic that limits how much a robot is able to shoot in a certain amount of time and if it overheats, the robot will lock. Hero's max heat is 200, each 42mm shot adds 100 amount of heat, and dissapates at 10 heat/second. Implement into your load and shooting logic a heat management system to handle overheat. 

- Challege 3 **(Required if Only Embedded)**
    - Hero, in the past, would jam due to a 42mm shot getting stuck inbetween the ammo reserve and the loader. To first determine a jam, check if the LOADER motor is moving when you command it to. If it isn't moving, you have a jam! Reverse the LOADER motor for UNJAM_TIMER_MS to relieve the pressure within the system, then try to load again. Implement the jam management system into your load and shooting logic.

## NOTE:
 You will not be able to compile or run the code given until the interview! Do not worry, you will be given 5 minutes to run and debug any issues with the code, be it syntax or logic, if you get to the interview phase. Additionally, you will not be rejected if the robot doesn't work, we mainly want to see your problem solving, understanding of logic, and general coding skills. From that last point, if you do not know C++, do not be afraid to use Google for syntax. However, we do not condone the use of AI for this challenge because we are looking for your skills. Therefore, be ready to walk us through your code during the interview! 