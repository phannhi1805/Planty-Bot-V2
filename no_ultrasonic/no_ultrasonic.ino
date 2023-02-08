#include <ESP32Encoder.h>
#include <IRremote.h>
#define BIN_1 26
#define BIN_2 25
#define LED_PIN 13
#define IR 15
#define PMP 12
#define POT 14


ESP32Encoder encoder;

//Setup interrupt variables ----------------------------
volatile int count = 0; // encoder count
hw_timer_t * timer2 = NULL;

volatile bool buttonIsPressed = false; 
uint64_t time_val;

IRrecv irrecv(IR);
decode_results results;
unsigned long state = 0;

// setting PWM properties ----------------------------
const int freq = 5000;
const int ledChannel_1 = 1;
const int ledChannel_2 = 2;
const int resolution = 8;
const int MAX_PWM_VOLTAGE = 255;
const int NOM_PWM_VOLTAGE = 150;
int D = 0;

// encoder properties ------------------------------
int v = 0;


//Initialization ------------------------------------
void IRAM_ATTR isr() {  // the function to be called when interrupt is triggered
  buttonIsPressed = true;   
}

//pause = FF02FD
//forawrd = FFC23D
//backward = FF22DD

void setup() {
  pinMode(LED_PIN, OUTPUT); // configures the specified pin to behave either as an input or an output
  pinMode(PMP, OUTPUT);
  pinMode(POT, INPUT); //potentiometer
  digitalWrite(LED_PIN, LOW); // sets the initial state of LED as turned-off

  pinMode(IR, INPUT);  // configures the specified pin to behave either as an input or an output
  irrecv.enableIRIn();
  timer2 = timerBegin(0, 80, true);
  
  Serial.begin(115200);
  ESP32Encoder::useInternalWeakPullResistors = UP; // Enable the weak pull up resistors
  encoder.attachHalfQuad(27, 33); // Attache pins for use as encoder pins
  encoder.setCount(0);  // set starting count value after attaching

  // configure LED PWM functionalitites
  ledcSetup(ledChannel_1, freq, resolution);
  ledcSetup(ledChannel_2, freq, resolution);

  // attach the channel to the GPIO to be controlled 
  ledcAttachPin(BIN_1, ledChannel_1);
  ledcAttachPin(BIN_2, ledChannel_2);
}

void loop() {
      D = map(analogRead(POT), 0, 4095, -NOM_PWM_VOLTAGE, NOM_PWM_VOLTAGE);
  if (D > MAX_PWM_VOLTAGE) {
      D = MAX_PWM_VOLTAGE;
  } else if (D < -MAX_PWM_VOLTAGE) {
      D = -MAX_PWM_VOLTAGE;
  }
  if (irrecv.decode(&results)){
    //Serial.println(results.value, HEX);
    state = results.value;

    switch (state) {
      case 0xFF02FD: //state 0: unarmed state
        Serial.println("Idle state");
        digitalWrite(LED_PIN, LOW);
        ledcWrite(ledChannel_1, LOW);
        ledcWrite(ledChannel_2, LOW);
        //digitalWrite(PMP, HIGH);
        break;
      case 0xFFC23D:
        Serial.println("Going forward");
        digitalWrite(LED_PIN, HIGH);
        ledcWrite(ledChannel_1, 225);
        ledcWrite(ledChannel_2, LOW);
        digitalWrite(PMP, LOW);
        break;
      case 0xFF22DD:
        Serial.println("Going backward");
        digitalWrite(LED_PIN, HIGH);
        ledcWrite(ledChannel_2, 225);
        ledcWrite(ledChannel_1, LOW);
        digitalWrite(PMP, LOW);
        break;
    }
    




    
        irrecv.resume();




  
//  if (val ==1) {
//    ButtonResponse();
//    digitalWrite(PMP, LOW);
//  } else {
//    digitalWrite(LED_PIN, LOW);
//    ledcWrite(ledChannel_2, LOW);
//    digitalWrite(PMP, HIGH);
}
}
