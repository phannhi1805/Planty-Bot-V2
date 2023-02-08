#include <ESP32Encoder.h>
#include <IRremote.h>
#define BIN_1 26
#define BIN_2 25
#define LED_PIN 13
#define IR 15
#define PMP 12
#define TRIG 5
#define ECHO 18

ESP32Encoder encoder;

//Setup timer ----------------------------
hw_timer_t * timer = NULL;
uint64_t time_val;

//Setup IR receiver ----------------------------
IRrecv irrecv(IR);
decode_results results;
unsigned long last_signal = 0;


// Setting PWM properties ----------------------------
const int freq = 5000;
const int ledChannel_1 = 1;
const int ledChannel_2 = 2;
const int resolution = 8;
int MAX_PWM_VOLTAGE = 255;

const int max_dist = -5000; //counts by encoder, might want to switch wire to make this positive. When hit this point, turn around
const int min_dist = 0; //when hit this point, stops/move forward

//Setup ultrasonic sensor ----------------------------
const float sound_speed = 0.034;
long duration = 0;
float distance = 0; //cm

//Setup states ----------------------------
int state = 0;
int first_dist = 0; //first found value
int last_dist = 0; //last found value
int dist = 0; //in the middle, where to water
volatile bool found_first_val = false;
volatile bool found_last_val = true;
volatile bool movefront = false;
volatile bool moveback = false;
volatile bool water = false;

//Initialization ------------------------------------
void setup() {
  pinMode(IR, INPUT); //IR receiver
  pinMode(ECHO, INPUT); //Ultrasonic
  
  pinMode(LED_PIN, OUTPUT); //built in LED
  pinMode(PMP, OUTPUT); //water pump
  pinMode(TRIG, OUTPUT); //ultrasonic
  
  digitalWrite(LED_PIN, LOW); // sets the initial state of LED as turned-off
  irrecv.enableIRIn(); //configure IR
  timer = timerBegin(0, 80, true); //timer for water pump
  
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
//0xFF02FD
//0xFFC23D
//0xFF22DD

void loop() {
  if (irrecv.decode(&results)){
    Serial.print("Signal: ");
    Serial.println(results.value, HEX);
    if (newinput(results.value)) { //new value != old value, need to store old value somewhere, also check to only acept 3 inputs and ignore the rest
      analyze_input(results.value);
    }
    Serial.println("State: ");
    Serial.println(state);
    switch (state) {
      case 0: //unarmed state
        light_off();
        motors_off();
        Serial.println("Idle");
        break;
      case 1: //moving forward 
        light_on();
        Serial.println(movefront);
        if (movefront ==true) { //adjusting after watering
          Serial.println("Moving forward after watering");
          if (encoder.getCount() >= last_dist) {
            Serial.println(encoder.getCount());
            move_forward();
          } else {
            movefront =false;
            state = 3;
            motors_off();
          }
          
        } else { //regular moving forward
          Serial.println("Move forward");
          if (encoder.getCount() > max_dist) { //since max_dist is a negative number
            Serial.println(encoder.getCount());
            move_forward();
            check_sensor(); //if hit desired value, trigger state 3
            
          } else {
            state = 2; //turn around
          }
        }
        break;
      case 2: //moving backward
        light_on();
        
        if (moveback ==true) { //adjusting for watering
          Serial.println("Moving backward for watering");
          if (encoder.getCount() < dist) {
            Serial.println(encoder.getCount());
            move_backward();
          } else {
            moveback = false;
            water = true;
            state = 3;
            motors_off();
          }
          
        } else { //regular moving backward, to the base
          Serial.println("Moving back to the base");
          if (encoder.getCount() < 0) {
            Serial.println(encoder.getCount());
            move_backward();
          } else {
            state = 0;
          }
        }
        break;
      case 3: //readjusting and watering
        light_on();
          if (moveback ==true) {
            state = 2;
          } else if (water ==true) {
            countDownWater();
          } else if (movefront ==true) {
            state = 1;
          } else {
            state = 1;
          }
        break;  
      }
        
      irrecv.resume();  
    }
}
    
bool newinput(unsigned long value) {
  if (value != last_signal) {
    if ((value == 0xFF02FD) or (value == 0xFFC23D) or (value == 0xFF22DD)) {
      last_signal = value;
      return true;
    } else {
      return false;
    }
  }
}

void analyze_input(unsigned long value) {
  switch (value) {
    case 0xFF02FD:
      Serial.println("state 0");
      state = 0; 
      break;
    case 0xFFC23D:
      state = 1;
      Serial.println("state 1");
      break;
    case 0xFF22DD:
      state = 2;
      Serial.println("state 2");
      break;
  }
}

void light_off() {
  digitalWrite(LED_PIN, LOW);
}

void light_on() {
  digitalWrite(LED_PIN, HIGH);
}

void motors_off() {
  ledcWrite(ledChannel_1, LOW);
  ledcWrite(ledChannel_2, LOW);
  digitalWrite(PMP, LOW);
}

void move_forward() {
  ledcWrite(ledChannel_1, MAX_PWM_VOLTAGE);
  ledcWrite(ledChannel_2, LOW);
  digitalWrite(PMP, LOW);
}

void move_backward() {
  ledcWrite(ledChannel_1, LOW);
  ledcWrite(ledChannel_2, MAX_PWM_VOLTAGE);
  digitalWrite(PMP, LOW);
}

void check_sensor() {
  Serial.println("Checking sensor");
  if ((current_dist() != 0) and (found_first_val ==false)) {
    first_dist = encoder.getCount();
    found_first_val =true;
    found_last_val = false;
    Serial.print("first_dist: ");
    Serial.println(first_dist);
  }else if ((current_dist() ==0) and (found_last_val ==false)) {
    last_dist = encoder.getCount();
    dist = (last_dist - first_dist) / 2; //round to nearest int
    moveback = true;
    state = 3;
    found_first_val = false;
    found_last_val = true;
    Serial.print("last_dist");
    Serial.println(last_dist);
    
  }  
}

int current_dist() {
  digitalWrite(TRIG, LOW); //set trigger signal low for 2us
  delayMicroseconds(2);

   /*send 10 microsecond pulse to trigger pin of HC-SR04 */
  digitalWrite(TRIG, HIGH);  // make trigger pin active high
  delayMicroseconds(10);            // wait for 10 microseconds
  digitalWrite(TRIG, LOW);   // make trigger pin active low

  /*Measure the Echo output signal duration or pulss width */
  duration = pulseIn(ECHO, HIGH); // save time duration value in "duration variable
  distance= duration*0.034/2; //Convert pulse duration into distance
  if (distance > 20) { //limit sensor range to 5cm so not mistaken other objects
    distance = 0;
  }
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}

void countDownWater() {
  if (!timerStarted(timer)) {
    timerStart(timer);
    digitalWrite(PMP, HIGH);
    Serial.println("Starting watering");
  } else {
    time_val = timerReadSeconds(timer);
    Serial.print("watering time");
    Serial.println(time_val);
    if (time_val > 5) {
      timerStop(timer);
      timerRestart(timer);
      state = 3;
      water =false;
      movefront =true;
      motors_off();
      Serial.println("Finished water" );
    }
  }
}











    
      
