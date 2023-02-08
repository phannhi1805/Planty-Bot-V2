#include <ESP32Encoder.h>
#include <IRremote.h>
#include <ezButton.h>

#define LED_PIN 13//built in LED pin
#define IR 15     //IR receiver pin
#define PMP 12    //water pump pin
#define TRIG 18  //ultrasonic transmitter pin
#define ECHO 19   //ultrasonic receiver pin
#define POT 32   //potentiometer
#define RPWM 4 // define pin 3 for RPWM pin (output)
#define R_EN 26
#define LPWM 16 // define pin 6 for LPWM pin (output)
#define L_EN 25
ezButton SW1(27);
ezButton SW2(33);


ESP32Encoder encoder;

//Setup timer to measure watering time ----------------------------
hw_timer_t * timer = NULL; 
hw_timer_t * timer1 = NULL; 
uint64_t time_val;
int start_time_delay = 0;
int start_time_disable = 0;

//Setup IR receiver ----------------------------
IRrecv irrecv(IR);
decode_results results;
unsigned long last_signal = 0;


// Setting PWM properties ----------------------------
const int freq = 5000;
const int ledChannel_1 = 1;
const int ledChannel_2 = 2;
const int resolution = 8;
const int MAX_PWM_VOLTAGE = 255;
const int NOM_PWM_VOLTAGE = 150;
int D = 0;

//Setup ultrasonic sensor ----------------------------
const float sound_speed = 0.034;
long duration = 0;
float distance = 0; //cm
int count = 0;

//Setup states ----------------------------
int state = 0;
volatile bool finished_watering = false;
volatile bool disable_sensor = false;
volatile bool delay_sensor = false;

//Initialization ------------------------------------
void setup() {
  pinMode(IR, INPUT); //IR receiver
  pinMode(ECHO, INPUT); //Ultrasonic
  pinMode(POT, INPUT); //potentiometer
  
  pinMode(LED_PIN, OUTPUT); //built in LED
  pinMode(PMP, OUTPUT); //water pump
  pinMode(TRIG, OUTPUT); //ultrasonic
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  
  digitalWrite(LED_PIN, LOW); // sets the initial state of LED as turned-off
  irrecv.enableIRIn(); //configure IR
  timer = timerBegin(0, 80, true); //timer for water pump
  timer1 = timerBegin(0, 80, true);
  SW1.setDebounceTime(50); // set debounce time to 50 milliseconds
  SW2.setDebounceTime(50); // set debounce time to 50 milliseconds
  
  Serial.begin(115200);
}


void loop() {
  SW1.loop(); // MUST call the loop() function first
  SW2.loop();
  int front_end = SW1.getState();
  int back_end = SW2.getState();
  //Serial.println(gate);
  
  //Process potentiometer input to get motor pwm
  D = map(analogRead(POT), 0, 4095, 0, 100); //in percent

  //Process IR receiver input to change state if applicable
  if (irrecv.decode(&results)){
    Serial.print("Signal: ");
    Serial.println(results.value, HEX);
    if (newinput(results.value)) { 
      analyze_input(results.value);
    }
    irrecv.resume();  
  }

  //State machine
  switch (state) {
    case 0: //unarmed state
      light_off();
      stop_motor();
      timerStop(timer);
      timerRestart(timer);
      Serial.println("State 0: Idle");
      break;
    case 1: //moving forward 
      light_on();
      timerStop(timer);
      timerRestart(timer);
      finished_watering =false;
      Serial.println(back_end);
      if (back_end == HIGH) { 
        //Serial.println("Moving forward");
        //move_forward(D);
        rotate(D, 0);
        if (disable_sensor == true) {
          temp_disable_ultrasonic(); //if hit an object, trigger state 3
        } else {
          check_sensor();
          Serial.println("Using sensor");
        }
      } else {
        Serial.println("SW2 triggered");
        stop_motor();
        state = 2; //turn around, moving backward 
      }
      break;
      
    case 2: //moving backward
      light_on();
      stop_motor();
      timerStop(timer);
      timerRestart(timer);
      Serial.println("Moving back");
      if (front_end == HIGH) {
        //move_backward(D);
        rotate(D, 1);
      } else {
        state = 0; //finished cycle
      }    
      break;
    case 3: //Watering when encounter the object
      light_on();
      if (delay_sensor == true) {
         temp_delay_sensor();
      } else {
        countDownWater();
        if (finished_watering ==true) {
          Serial.println("Transision to state 1");
          disable_sensor = true;
          state = 1;
        }
      } 
      break; 
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
      Serial.println("Changed to state 0 from remote");
      state = 0; 
      break;
    case 0xFFC23D:
      state = 1;
      Serial.println("Changed to state 1 from remote");
      break;
    case 0xFF22DD:
      state = 2;
      Serial.println("Changed to state 2 from remote");
      break;
  }
}

void light_off() {
  digitalWrite(LED_PIN, LOW);
}

void light_on() {
  digitalWrite(LED_PIN, HIGH);
}

void move_forward(int D) {
  analogWrite(LPWM, 0);
  analogWrite(RPWM, D);
}

void move_backward(int D) {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 255);
}

void check_sensor() {
  
  if (current_dist() != 0) {
    stop_motor();
    state = 3;
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
  if (distance > 10) { //limit sensor range to 20cm so not mistaken other objects
    distance = 0;
  }  
  if (distance != 0) {
    if (count < 900) {
      count = count + 1;
      Serial.print("Count: ");
      Serial.print(count);
      return 0;
    } else {
      count = 0;
    }
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
    Serial.print("Watering time (s): ");
    Serial.println(time_val);
    if (time_val > 5) {
      timerStop(timer);
      timerRestart(timer);
      finished_watering =true;
      stop_motor();
      count = 0;
      Serial.println("Finished watering" );
    }
  }
}


void rotate(int value, int dir) {

  digitalWrite(L_EN, HIGH);
  digitalWrite(R_EN, HIGH);
  int pwm1Pin, pwm2Pin;
  if(dir == 1){
    analogWrite(LPWM, toPWM(value));
    analogWrite(RPWM, 0);
  }else{
    analogWrite(LPWM, 0);
    analogWrite(RPWM, toPWM(value));  
  }
  digitalWrite(pwm1Pin, LOW);
  Serial.print("PWM1: ");
  Serial.print(pwm1Pin);
  Serial.print(" PWM2: ");
  Serial.print(pwm2Pin);
  Serial.print(" PWM value: ");
  Serial.print(value);
  Serial.println("%");    
     
  if(value >=0 && value <=100 ){
    analogWrite(pwm2Pin, toPWM(value));
  }
}

void stop_motor(){
  digitalWrite(L_EN, LOW);
  digitalWrite(R_EN, LOW);
  digitalWrite(PMP, LOW);
  //Serial.println("Motor stopped");
}

int toPWM(int v){
  return map(v, 0, 100,0,255);
}

void temp_disable_ultrasonic() {
  int current_time = millis();
  if (start_time_disable == 0) {
    start_time_disable = current_time;
  } else {
    int time_diff = current_time - start_time_disable;
    Serial.println("Pausing time: ");
    Serial.print(time_diff);
    if (time_diff >= 12000) {
      check_sensor();
      start_time_disable = 0;
      disable_sensor = false;
      Serial.println("Stop pausing");
      count == 0;
    }
  }
}

void temp_delay_sensor() {
  int current_time = millis();
  if (start_time_delay == 0) {
    start_time_delay = current_time;
  } else {
    int time_diff = current_time - start_time_delay;
    Serial.println("Delaying time: ");
    Serial.print(time_diff);
    if (time_diff >= 5000) {
      check_sensor();
      start_time_delay = 0;
      delay_sensor = false;
      Serial.println("Stop delaying");
    }
  }
}








    
      
