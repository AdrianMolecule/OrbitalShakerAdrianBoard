
#include <Arduino.h>
#include <Melody.h>

#include "SerialCommunication.h"
#include "Splitter.h"
void setup();
void accelerate();
void loop();
void buzz(int dela, int times);
// void alternate(int pin, int de, int times) ;
void setstepsPerRotation(int newSetstepsPerRotation);
int readPotentiometer();
void setLoudness(int loudness);
void play(Melody melody);
void motor();
int calculateFrequency();

// THIS is working well for the Adrian PCB board without the incorporated Incubator
// TODO add serial commands capability to change the RPM and microstepping.
// music
#define SERIAL_BAUDRATE 115200
#define version 2.1
    const int BUZZ_PIN = 12;  // see the my board esp minipins.png in the Eagle directory

//
const int LED_PIN_PROCESSOR = 2;
// we have 2 io available and we use the one labeled 35
// as in IO35 which is connected to the connector IO pin 6
//(the last pin is 3.3 v and it's in the right top corner when controller key is on the left and board is horizontal - top view
// that is pulled to ground by R29 and serialized by current limiting R28
const int POTENTIOMETER_PIN = 35;
// WORKS WELL FOR 200 but it does not seem to keep track for other speeds
// for drv 8825
enum {
    STEP_PIN = 25,
    DIR_PIN = 32,
    STEPPER_ENABLE_PIN = 26,
    STEPPER_stepsPerRotation_M0 = 16,
    STEPPER_stepsPerRotation_M1 = 4,
    STEPPER_stepsPerRotation_M2 = 15
};
enum {
    STEPS200 = 200,
    STEPS400 = 400,
    STEPS800 = 800,
    STEPS1600 = 1600,
    STEPS3200 = 3200,
    STEPS6400 = 6400
};
int DEMO = false;
int currentStepsPerRotation = (DEMO == true ? STEPS1600 : STEPS200);
double currentRPM = (DEMO == true ? 110 : 300);
const int BUZZ_CHANNEL = 5;  // for the Buzzer
/* Setting motor PWM Properties */
const int MOTOR_PWMChannel = 0;
const int PWMResolution = 10;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
int dutyCycle = MAX_DUTY_CYCLE / 2;
SerialCommunication serialCommunication = SerialCommunication();

void setup() {
    int speed = LOW;  // full power
    Serial.begin(115200);
    Serial.println("=================================starting now============================================");
    // Declare pins as output:
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEPPER_ENABLE_PIN, OUTPUT);
    digitalWrite(STEPPER_ENABLE_PIN, LOW);  // Active on LOW
    pinMode(STEPPER_stepsPerRotation_M0, OUTPUT);
    pinMode(STEPPER_stepsPerRotation_M1, OUTPUT);
    pinMode(STEPPER_stepsPerRotation_M2, OUTPUT);
    pinMode(LED_PIN_PROCESSOR, OUTPUT);
    pinMode(BUZZ_PIN, OUTPUT);
    //
    ledcSetup(BUZZ_CHANNEL, 5000, 8);
    ledcAttachPin(BUZZ_PIN, BUZZ_CHANNEL);
    ledcWrite(BUZZ_CHANNEL, 0);  // duty Cycle  0
    // alternate(LED_PIN_PROCESSOR, 50, 5);
    Melody validChoice(" (cgc*)**---");
    Serial.print("code from OrbitalShakerAdrianBoard under diy espProjects in Platformio version: ");
    Serial.println(version);
    play(validChoice);
    Serial.println("stepper target speed at RPM :");
    Serial.println(currentRPM);
    Serial.println("stepper set desired steps per rotation at:");
    Serial.println(currentStepsPerRotation);
    // Melody frereJacques("(cdec)x2   (efgr)x2   ((gagf)-ec)x2     (c g_ c+)x2");
    Melody scaleLouder("c>>> d>> e>f g< a<< b<<< c*<<<<", 480);
    Melody auClairDeLaLune("cccde+dr  ceddc+.r");
    play(scaleLouder);
    // motor
    // alternate(LED_PIN_PROCESSOR, 100, 10);
    motor();
    // play(auClairDeLaLune);
    // stepper.setMinPulseWidth(3);//https://forum.arduino.cc/t/how-to-decide-acceleration-in-accelstepper-stepper-library/466769/7
}

void loop() {
    // alternate(LED_PIN_PROCESSOR, 100, 1);
    String command = serialCommunication.checkForCommand();
    if (!command.equals("")) {
        // Serial.print("lookup command ");
        // Serial.println(command);
        // if (command.indexOf("d") != -1) {
        //     Serial.print("found command d: ");
        //     File dir = SD.open("/");
        //     printDirectory(dir, 0);
        // }
        if (command.indexOf("rpm") != -1) {
            Splitter splitter = Splitter(command);
            String newRPMAsString = splitter.getItemAtIndex(1);
            if (newRPMAsString.isEmpty()) {
                Serial.print("Please enter the new desired RPM after rpm. Like rpm 80");
            }
            currentRPM = newRPMAsString.toDouble();
            int f = calculateFrequency();
            Serial.println("new RPM set to: " + newRPMAsString);
            Serial.println("new frequency set to: " + f);
            motor();
        }
        else if (command.indexOf("steps") != -1) {
            Splitter splitter = Splitter(command);
            String newStepsAsString = splitter.getItemAtIndex(1);
            if (newStepsAsString.isEmpty()) {
                Serial.print("Please enter the new desired steps per rotation after steps. Like steps 200");
            }
            currentStepsPerRotation = newStepsAsString.toDouble();
            Serial.println("new stepsPerRotation set to: " + currentStepsPerRotation);
            motor();
        }
        else if (command.indexOf("i") != -1) {
            Serial.print("currentStepsPerRotation: ") ;  Serial.println( currentStepsPerRotation);
            Serial.print("currentRPM: ") ; Serial.println(currentRPM);
            Serial.print("frequency: ") ; Serial.println(calculateFrequency());

        } else if (command.indexOf("?") != -1) {
            Serial.println("Example commands: rpm 300\nsteps 200");
        } else {
            Serial.println("unknown command, try ? for list of commands");
        }
    }
}

//?? not accurate but an idea. we move up  to half of RPM in x2 stepsPerRotation and then full in no stepsPerRotation
void motor() {
    // if(desiredStepsPerRotation==STEPS200){
    Serial.print("start motor with stepsPerRotation:");
    Serial.println(currentStepsPerRotation);
    Serial.print("start motor with rpm:");
    Serial.print(currentRPM);
    int targetFreq = calculateFrequency();
    Serial.print("   start motor with targetFreq:");
    Serial.println(targetFreq);
    setstepsPerRotation(currentStepsPerRotation);
    // }else{
    // 	setstepsPerRotation(desiredStepsPerRotation/2);	// start with more force
    // }
    for (int f = targetFreq * .9; f <= targetFreq; f += 5) {
        ledcSetup(MOTOR_PWMChannel, f, PWMResolution);
        delay(10);
        ledcAttachPin(STEP_PIN, MOTOR_PWMChannel); /* Attach                                        the STEP_PIN PWM Channel to the GPIO Pin */
        delay(10);
        ledcWrite(MOTOR_PWMChannel, dutyCycle);
        delay(10);
    }
    Serial.println("============:");
}

int calculateFrequency() {
    int freq = (int)(currentRPM / 60 * currentStepsPerRotation);
    return freq;
}
// void buzz(int dela, int times) {
// 	alternate(BUZZ_PIN, dela, times);
// }

// void alternate(int pin, int de, int times) {
// 	Serial.println("alternate on pin:");
// 	Serial.println(pin);
// 	for (int var = 0; var < times; ++var) {
// 		digitalWrite(pin, HIGH);
// 		delay(de);
// 		digitalWrite(pin, LOW);
// 		delay(de);
// 	}
// }

void setstepsPerRotation(int newStepsPerRotation) {
    Serial.println("set micro-stepping to:");
    Serial.println(newStepsPerRotation);
    switch (newStepsPerRotation) {
        case STEPS200:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            Serial.println("set micro-stepping to 200");
            break;
        case STEPS400:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            Serial.println("set micro-stepping to 400");
            break;
        case STEPS800:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            Serial.println("set micro-stepping to 800");
            break;
        case STEPS1600:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            Serial.println("set micro-stepping to 1600");
            break;
        case STEPS3200:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, HIGH);
            Serial.println("set micro-stepping to 3200");
            break;
        case 6400:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, HIGH);
            Serial.println("set micro-stepping to 6400");
            break;
        default:
            Serial.println("BAD micro-stepping");
    }
    // m0,m1,m2
    // l,l,l 		200
    // h,l,l 		400
    // lhl 		800
    // hhl 		1600
    // llh 		3200
    // hlh		6400
    // lhh 		6400
    // hhh 		6400
    // https://www.ti.com/lit/ds/symlink/drv8825.pdf?ts=1645298914215&ref_url=https%253A%252F%252Fwww.google.com%252F
}
// Function for reading the Potentiometer
int readPotentiometer() {
    int customDelay = analogRead(POTENTIOMETER_PIN);  // Reads the potentiometer
    int newRPM = map(customDelay, 0, 1023, 0, 300);   // read values of the potentiometer from 0 to 1023 into  d0->300
    return 300;
}

void setLoudness(int loudness) {
    // Loudness could be use with a mapping function, according to your buzzer or sound-producing hardware
#define MIN_HARDWARE_LOUDNESS 0
#define MAX_HARDWARE_LOUDNESS 16
    ledcWrite(BUZZ_CHANNEL, map(loudness, -4, 4, MIN_HARDWARE_LOUDNESS, MAX_HARDWARE_LOUDNESS));
}

void play(Melody melody) {
    melody.restart();         // The melody iterator is restarted at the beginning.
    while (melody.hasNext())  // While there is a next note to play.
    {
        melody.next();                                   // Move the melody note iterator to the next one.
        unsigned int frequency = melody.getFrequency();  // Get the frequency in Hz of the curent note.
        unsigned long duration = melody.getDuration();   // Get the duration in ms of the curent note.
        unsigned int loudness = melody.getLoudness();    // Get the loudness of the curent note (in a subjective relative scale from -3 to +3).
                                                         // Common interpretation will be -3 is really soft (ppp), and 3 really loud (fff).
        if (frequency > 0) {
            ledcWriteTone(BUZZ_CHANNEL, frequency);
            setLoudness(loudness);
        } else {
            ledcWrite(BUZZ_CHANNEL, 0);
        }

        delay(duration);

        // This 1 ms delay with no tone is added to let a "breathing" time between each note.
        // Without it, identical consecutives notes will sound like just one long note.
        ledcWrite(BUZZ_CHANNEL, 0);
        delay(1);
    }
    ledcWrite(BUZZ_CHANNEL, 0);
    delay(1000);
}

// Invalid choice kind-of sound
Melody invalidChoice(" (cg_)__");
// Frere Jacques

// Au Clair de la Lune
