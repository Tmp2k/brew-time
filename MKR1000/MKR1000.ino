#include <WiFi101.h>
#include <MQTTClient.h>
#include "arduino_secrets.h" 


#define DISPLAY_VOLT  800  //This correlates ot the voltage read at the pin. 750 = LED ON   1000 = LED OFF
#define ON_DELAY      6000 //This is the delay in ms before another turnOn command will be executed as multiple quick button presses set the temperature
#define OFF_DELAY     1000 //This is the minimum time after turning on that you can turn back off.
#define PIN_DISPLAY   A0   //The pin that reads the display backlight LED
#define PIN_HEAT      8    //The pin that reads the heating element control signal
#define PIN_HEAT_BTN  11    //The pin that simulates pressing the heat button on the kettle
#define PIN_POWER_BTN 12    //The pin that simulates pressing the power button on the kettle
#define PIN_LED       6    //The pin connected to the LED to show the device status

//loaded from arduino_secrets.h
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;


long          lastOn = -ON_DELAY;   //this is set to a -ve number so that it does not cause issues when the arduino is first powered on.
unsigned long lastMessage = 0;
bool          lastStatus = false;

//used to create the topic strings
char ACTUAL_TOPIC[80];
char DESIRED_TOPIC[80];

//create the clients
WiFiClient net;
MQTTClient client;


void setup() {
  //create the topic strings with the correct device ID
  sprintf(ACTUAL_TOPIC,"/%s/actual",MQTT_DEVICE_ID);
  sprintf(DESIRED_TOPIC,"/%s/desired",MQTT_DEVICE_ID);

  //set the I/O pins
  pinMode(PIN_LED,OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_HEAT_BTN,OUTPUT);
  digitalWrite(PIN_HEAT_BTN,HIGH);
  pinMode(PIN_POWER_BTN,OUTPUT);
  digitalWrite(PIN_POWER_BTN,HIGH);
  pinMode(PIN_HEAT,INPUT);
  pinMode(PIN_DISPLAY,INPUT);
  
  //start serial, WiFi and MQTT
  WiFi.begin(ssid, pass);
  client.begin("broker.shiftr.io", net);
  client.onMessage(messageReceived);
  Serial.begin(9600);

  //connect to WiFi and MQTT server
  connect();
}

void connect() {
 

  //wait for WiFi to connect
  Serial.print("Checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));  //flash LED
    Serial.print(".");
    delay(500);
  }
  
  //connect MQTT
  Serial.print("done\nConnecting to MQTT broker...");
  while (!client.connect(MQTT_DEVICE_ID, MQTT_KEY, MQTT_SECRET)) {
    digitalWrite(PIN_LED,!digitalRead(PIN_LED)); //flash LED
    delay(250);
  }
  Serial.println("done");
  digitalWrite(PIN_LED,LOW);
  sendStatus(isOn()); //update the LED and broker with current status
  
  client.subscribe(DESIRED_TOPIC); //subscribe to incomming command topic

}

void loop() {
  client.loop();  //do the MQTT client stuff
  
  if (!client.connected()) { //if not connected then re-connect
    connect();
  }

  // if status has changed, then publish it
  bool thisStatus = isOn();
  if(thisStatus != lastStatus) {
    lastMessage = 0;
    lastStatus = thisStatus;
  }
  
  // publish a message roughly every 10 seconds, just so we know the device is responding.
  
  if (millis() - lastMessage > 10000) {
    sendStatus(thisStatus);
    if(thisStatus) {
      Serial.print("Status: ON  ");
    } else {
      Serial.print("Status: OFF ");
    }
    if(isPowerOn()) {
      Serial.print("Power: ON  ");
    } else {
      Serial.print("Power: OFF ");
    }
    if(isHeatOn()) {
      Serial.print("Heat: ON  ");
    } else {
      Serial.print("Heat: OFF ");
    }
    Serial.println(" ");
  }

  
  
  delay(10);  //give things time to settle down.
  
}

//this function sends the status to the MQTT broker, updates the status LED and prints debug info to the serial port
void sendStatus(bool statusIsOn) {
  lastMessage = millis();
  
  if (statusIsOn) {
      client.publish(ACTUAL_TOPIC, "ON");
      digitalWrite(PIN_LED,HIGH);
    } else {
      client.publish(ACTUAL_TOPIC, "OFF");
      digitalWrite(PIN_LED,LOW);
    }

 
}


//when a message is recieved this fuction fires and executes the requested command.
void messageReceived(String &topic, String &payload) {
  
  if (payload == "OFF") {
    turnOff();
  }
  if (payload == "ON") {
    turnOn();
  }
  
}

//returns true/false to describe if the kettle is "on" i.e. in the process of boiling the water
bool isOn() {
  return millis() - lastOn < ON_DELAY || isHeatOn(); //either a request to boil the kettle has just been sent, or it's alreay heating the water.
}


//boil the kettle, once the kettle has boiled it will remain on - displaying the temperature
bool turnOn() {
  if(isOn()) return false; //if it's already on - do nothing
  lastOn = millis(); //store the current time so we know the last time the kettle was turned on
  if(!isPowerOn()) { 
    //simulate pressing the button for 100ms
    digitalWrite(PIN_POWER_BTN,LOW);
    delay(100);
    digitalWrite(PIN_POWER_BTN,HIGH);
  }
  if(!isHeatOn()) {
    //simulate pressing the button for 100ms
    digitalWrite(PIN_HEAT_BTN,LOW);
    delay(100);
    digitalWrite(PIN_HEAT_BTN,HIGH);
  }

  return true;
  
}

//stop boiling the kettle and also turn it fully off
bool turnOff() {
  if(millis() - lastOn < OFF_DELAY) return false; //if it's only just been turned on then do nothing
  lastOn = -ON_DELAY; //reset the last time the kettle was last turned on
  if(isPowerOn()) {   
    //simulate pressing the button for 100ms 
    digitalWrite(PIN_POWER_BTN,LOW);
    delay(100);
    digitalWrite(PIN_POWER_BTN,HIGH);
  }

  return true;
}

//is the kettle powered on? 
bool isPowerOn() {
  int sensorValue = analogRead(PIN_DISPLAY);
  return (sensorValue < DISPLAY_VOLT);
}

//is the kettle heating the water?
bool isHeatOn() {
  int sensorValue = digitalRead(PIN_HEAT);
  return (sensorValue == HIGH);
}
