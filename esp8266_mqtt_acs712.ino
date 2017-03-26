//code write by Moz for YouTube changel logMaker360, 24-11-2016
//code belongs to this video: https://youtu.be/nAUUdbUkJEI

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
 
// Connect to the WiFi
const char* ssid = "depto14";                           //!!!!!!!!!!!!!!!!!!!!!
const char* password = "qawsed021188!";                //!!!!!!!!!!!!!!!!!!!!!
const char* mqtt_server = "192.168.0.24";                 //!!!!!!!!!!!!!!!!!!!!!

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String localip = "elektronip";
String elektronname = "Elektron";
String currtime = "elektrontime";
String data = "elektrondata";

//Current Sensor ACS712 Variable Set
#define C_SENSOR1 A0
#define relay 2

//For analog read
int r1 = LOW;
int r1_received = LOW;
int incomingByte = 0;   // for incoming serial data
int c_min = 0;
int c_max = 30;

//For analog read
double value;

//Constants to convert ADC divisions into mains current values.
double ADCvoltsperdiv = 0.0048;
double VDoffset = 2.4476; //Initial value (corrected as program runs)

//Equation of the line calibration values
double factorA = 15.35; //factorA = CT reduction factor / rsens
double Ioffset = 0;

//Constants set voltage waveform amplitude.
double SetV = 217.0;

//Counter
int counter=0;

int samplenumber = 4000;

//Used for calculating real, apparent power, Irms and Vrms.
double sumI=0.0;

int sum1i=0;
double sumVadc=0.0;

double Vadc,Vsens,Isens,Imains,sqI,Irms;
double apparentPower;
 
const byte ledPin = D4; // digital pin 4 on a weMos D1 mini is next to ground so easy to stick a LED in.
 
void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  if (receivedChar == '1')
  digitalWrite(ledPin, HIGH);
  if (receivedChar == '0')
   digitalWrite(ledPin, LOW);
  if (receivedChar == '2')
   Serial.println("Retrieve Data Forced");
  func_read_current_sensor();

  char data[80];
  
  String payload = "{\"ip\":\"" + localip + "\",\"time\":\"" + currtime + "\",\"name\":\"" + elektronname + "\",\"data\":\"" + apparentPower + "\"}";
  payload.toCharArray(data, (payload.length() + 1));
  
  client.publish("esp8266status", data);
  }
  Serial.println();
}
 
 
void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 if (client.connect("ESP8266 Client")) {
  Serial.println("connected");
  // ... and subscribe to topic
  client.subscribe("ledStatus");
 } else {
  Serial.print("failed, rc=");
  Serial.print(client.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}
 
void setup()
{
 Serial.begin(9600);
 
 client.setServer(mqtt_server, 1883);
 client.setCallback(callback);
 
 pinMode(ledPin, OUTPUT);
 digitalWrite(ledPin, HIGH);
 client.publish("esp8266status", "Relay OFF");
 delay(5000);
 digitalWrite(ledPin, LOW);
}

void func_read_current_sensor() {
  Serial.println("func_read_current_sensor");

  value = analogRead(C_SENSOR1);

  //Summing counter
  counter++;

  //Voltage at ADC
  Vadc = value * ADCvoltsperdiv;

  //Remove voltage divider offset
  Vsens = Vadc-VDoffset;

  //Current transformer scale to find Imains
  Imains = Vsens;

  //Calculates Voltage divider offset.
  sum1i++; sumVadc = sumVadc + Vadc;
  if (sum1i>=1000) {VDoffset = sumVadc/sum1i; sum1i = 0; sumVadc=0.0;}

  //Root-mean-square method current
  //1) square current values
  sqI = Imains*Imains;
  //2) sum
  sumI=sumI+sqI;

  Serial.println("Reading ACS712 Current Sensor");
  counter=0;
  //Calculation of the root of the mean of the current squared (rms)
  Irms = factorA*sqrt(sumI/samplenumber)+Ioffset;
  if (Irms<0.05) {Irms=0;}

  //Calculation of the root of the mean of the voltage squared (rms)


  /*if (Irms < c_min || Irms > c_max) {
    digitalWrite(RELAY1, HIGH);
    r1 = 0;
  }*/

  apparentPower = Irms * SetV;
  Serial.println("Watios: ");
  Serial.println(apparentPower);
  Serial.println("Voltaje: ");
  Serial.println(SetV);
  Serial.println("Amperios: ");
  Serial.println(Irms);
  Serial.println("status: ");
  Serial.println(r1);
  Serial.println("c_max: ");
  Serial.println(c_max);
  Serial.println();

  //Reset values ready for next sample.
  sumI=0.0;

}
 
void loop()
{
 if (!client.connected()) {
  reconnect();
 }
 IPAddress ip = WiFi.localIP();
 localip = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
 client.loop();
}



