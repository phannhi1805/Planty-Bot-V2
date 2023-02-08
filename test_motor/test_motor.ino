#define SENSOR_PIN 32 // center pin of the potentiometer
#define RPWM_Output 5 // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM)
#define LPWM_Output 4 // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM)
void setup()
{
 pinMode(RPWM_Output, OUTPUT);
 pinMode(LPWM_Output, OUTPUT);
}
void loop()
{
 int sensorValue = analogRead(SENSOR_PIN);
 // sensor value is in the range 0 to 1023
 // the lower half of it we use for reverse rotation; the upper half for forward
 if (sensorValue < 2048)
 {
 // reverse rotation
 int reversePWM = -(sensorValue - 2047) / 2;
 analogWrite(LPWM_Output, 0);
 analogWrite(RPWM_Output, reversePWM);
 }
 else
 {
 // forward rotation
 int forwardPWM = (sensorValue - 2047) / 2;
 analogWrite(LPWM_Output, forwardPWM);
 analogWrite(RPWM_Output, 0);
 }
}
