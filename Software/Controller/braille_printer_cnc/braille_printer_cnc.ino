
/*
    Braille Printer CNC microcontroller code

    This code is meant to run on an Arduino Mega board
    that drives two stepper motors that position a solenoid
    valve in a 2D plane to punch a hole on a paper.

    Dagim Sisay
    Elias Feleke
    June, 2017


Major problem with EEPROM, since the program is suppose to send
coordinates in mm to the controller, each coordinate will hold
8 bytes of data (4 to each axis) and this puts the extreme maximum
possible data size to almost 40KB which we don't have.
The Mega board has only 4KB of EEPROM.

I propose sending an array of bytes to the controller where each byte
idicates the number of dots to skip relative to the previous one.
The first byte will indicate how much further to go from the zeroth
dot.
For Example
    if the first byte is zero then the first dot is punched
    if the second byte is 2 then skip one do and punch the first
    dot on the next cell.
    if the third byte is 1 then punch the next dot

Even with this method the EEPROM won't be enough but at least it would be
better than the first.
Assuming extreme maximum, there could be 27*30*6 = 4860 dots to be printed
and that is bigger than the available EEPROM space

 */

#include <Stepper.h>
#include <EEPROM.h>

#define motor1_SPR 200 //steps per revolution of motor one (x direction)
#define motor2_SPR 300 //steps per revolution of motor two (y direction)

Stepper motor1(motor1_SPR, A1, A2, A3, A4);
Stepper motor2(motor2_SPR, A5, A6, A7, A8);

//limit buttons pins
#define LIMIT_X 2
#define LIMIT_Y 3

//solenoid valve pin
#define VALVE 4

//State indicating lights pins
#define IDLE_LED 5
#define PRINTING_LED 6

//coordinate data
typedef struct {
    double x;
    double y;
} COORD;


//constants

//States
#define IDLE 0
#define PRINTING 1
uint8_t STATE = IDLE;
bool TRANSITION = true;


//TODO measure DPS of each motor or maybe steps per mm
//distance per step for each motor in mm
#define motor1_DPS 1
#define motor2_DPS 1

//Current position of valve
COORD current_pos;


//functions
void init_pos();
void move_next(COORD);
void punch_dot();



void setup()
{
  // set the speed of motors to 100 and 60 rpm
  motor1.setSpeed(150);
  motor2.setSpeed(130);

  //set pin modes
  pinMode(LIMIT_X, INPUT);
  pinMode(LIMIT_Y, INPUT);
  pinMode(VALVE, OUTPUT);

  // initialize the serial port:
  Serial.begin(9600);

  //move to initial position
  init_pos();
}

void loop()
{
    switch (STATE) {
        case IDLE:
            if (TRANSITION) {
                digitalWrite(PRINTING_LED, LOW);
                digitalWrite(IDLE_LED, HIGH);
                TRANSITION = false;
            }

            if (Serial.available()){
                //accept incoming data and store in EEPROM
                //TODO know exactly how much data the buffer can store
                //if not good then send one coordinate at a time with a
                //terminating token at the end

                //TODO add data length checking and if greater than
                //specified value, stop receiving and reply a notification

                //TODO also reserve the first few bytes of the EEPROM for
                //data size storage. Maybe begin printing at the 5th address
                //and reserve the first 4 bytes for storing the length of the
                //data received
                String data = Serial.readString();
                int address = 0;
                EEPROM.put(address, data);

                //when data is received, move to PRINTING State
                STATE = PRINTING;
                TRANSITION = true;
            }
            break;
        case PRINTING:
            if (TRANSITION) {
                digitalWrite(IDLE_LED, LOW);
                digitalWrite(PRINTING_LED, HIGH);
                TRANSITION = false;
            }

            break;
    }
}


void init_pos()
{
    //TODO fill correct signs for step depending on motor rotation direction
    for (;;){
        digitalWrite(IDLE_LED, HIGH);
        digitalWrite(PRINTING_LED, HIGH);
        if (!digitalRead(LIMIT_X)){
            motor1.step(1);
            digitalWrite(IDLE_LED, LOW);
        }
        if (!digitalRead(LIMIT_Y)){
            motor2.step(1);
            digitalWrite(PRINTING_LED, LOW);
        }
        if (digitalRead(LIMIT_X) && digitalRead(LIMIT_Y))
            break;
    }
    delay(500);
    //after both motors are known to hit their extreme positions,
    //move both to the correct initial position and set current_pos to zero
    motor1.step(-200);
    motor2.step(-200);
    current_pos.x = 0;
    current_pos.y = 0;
}


void move_next(COORD next_pos)
{
    double diff_x = next_pos.x - next_pos.x;
    double diff_y = next_pos.y - next_pos.y;
    double steps_x = diff_x / motor1_DPS;
    double steps_y = diff_y / motor2_DPS;
    motor1.step(steps_x);
    motor2.step(steps_y);
    current_pos.x = next_pos.x;
    current_pos.y = next_pos.y;
}


void punch_dot()
{
    digitalWrite(VALVE, HIGH);
    delay(20); //give valve time to punch the paper TODO how much time???
    digitalWrite(VALVE, LOW);
}
