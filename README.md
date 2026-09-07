
# Embedded-Interview-Challenge

### **HEADS UP!** 
Logic should be written under the execute function. Variables should be initialized near the top of the file. Feel free to write any helper functions (DEFINE THEM IN THE .hpp FILE!).
Please read the comments before attempting, we define commands, timers, variables, and other useful things throughout the files.

# Start Here 
Welcome to the Embedded Interview Challenge! Your task is to create the control systems we have in place for our heavy hitter, Hero. Below are some videos showing some of the parts moving to help give some context for you.

<img width="800" height="449" alt="20260907_172301-ezgif com-video-to-gif-converter" src="https://github.com/user-attachments/assets/b9f7f3fe-40e3-463b-9d77-454074e4d3fc" />

Here is the LOADER motor in action to load balls into the ball path, pushing a ball into the KICKER motor. The unjam challenge will have you reverse this.

<img width="800" height="449" alt="20260907_172358-ezgif com-video-to-gif-converter" src="https://github.com/user-attachments/assets/0704b086-f4fe-41ff-a9ab-187230b01cdf" />

Here is the KICKER motor in action to "kick" the shot into the flywheel to shoot. It is the small, black motor that spins for a small interval into the constantly spinning flywheel.

<img width="542" height="360" alt="20260907_172442-ezgif com-video-to-gif-converter (1)" src="https://github.com/user-attachments/assets/6958d924-f97b-4544-b129-b24e1f95f2ed" />

Here is the heat display increasing and decreasing over time. You do not need to make the display work, just the internal logic.

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

- Challenge 2 **(Required)**
    - Barrel Heat is a mechanic that limits how much a robot is able to shoot in a certain amount of time and if it overheats, the robot will lock. Hero's max heat is 200, each 42mm shot adds 100 amount of heat, and dissapates at 10 heat/second. Implement into your load and shooting logic a heat management system to handle overheat. 

- Challege 3 **(Required if Only Embedded)**
    - Hero, in the past, would jam due to a 42mm shot getting stuck inbetween the ammo reserve and the loader. To first determine a jam, check if the LOADER motor is moving when you command it to. If it isn't moving, you have a jam! Reverse the LOADER motor for UNJAM_TIMER_MS to relieve the pressure within the system, then try to load again. Implement the jam management system into your load and shooting logic.

## NOTE:
 You will not be able to compile or run the code given until the interview! Do not worry, you will be given 5 minutes to run and debug any issues with the code, be it syntax or logic, if you get to the interview phase. Additionally, you will not be rejected if the robot doesn't work, we mainly want to see your problem solving, understanding of logic, and general coding skills. From that last point, if you do not know C++, do not be afraid to use Google for syntax. However, we do not condone the use of AI for this challenge because we are looking for your skills. Therefore, be ready to walk us through your code during the interview! 
