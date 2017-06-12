#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>

char* ssid = "test";
char* password = "12345678";
String st;
String content;
int statusCode;
int auxwebtype;

String qurl;
String qsid;

String equrl;
String esid;

String apiurl;
String mqtt_server;                 //!!!!!!!!!!!!!!!!!!!!!


ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String localip = "elektronip";
String elektronname = "Elektron";
String currtime = "elektrontime";
float data = 1;

//Current Sensor ACS712 Variable Set
#define C_SENSOR1 A0
const byte ledPin = D4; // digital pin 4 on a weMos D1 mini is next to ground so easy to stick a LED in.

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
int i = 0;

//Used for calculating real, apparent power, Irms and Vrms.
double sumI=0.0;

int sum1i=0;
double sumVadc=0.0;

double Vadc,Vsens,Isens,Imains,sqI,Irms;
double apparentPower;

//NODEMCU ESP8266-12 VALUES MAPPING!!!//
int val;

 
void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.println(topic);
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
 
int interrupt = 0;
int wait = 5;

void setup() {
  Serial.begin(115200);

   pinMode(ledPin, OUTPUT);
   digitalWrite(ledPin, HIGH);
   delay(5000);
   digitalWrite(ledPin, LOW);

  EEPROM.begin(512);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.println("Startup");

  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  esid = "";
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);

  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 64; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);

  Serial.println("Reading EEPROM Server URL");
  mqtt_server = "";
  for (int i = 64; i < 128; ++i)
  {
    //equrl += char(EEPROM.read(i));
    mqtt_server += char(EEPROM.read(i));
  }
  Serial.print("URL: ");
  Serial.println(mqtt_server);


  // If SSID is set then start web mode 0 (web server mode)
  if ( esid.length() > 1 ) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      launchWeb(0);
      return;
    }
  }
  //If not connects to AP set up its own, start web mode 1

  setupAP();
}

//test wifi to connect configured SSID AP
bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  IPAddress ip = WiFi.localIP();
  localip = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  if (localip != "0.0.0.0") {
    Serial.println("Local IP is NOT 0.0.0.0 so show main page: web mode 0 (connecting to configured SSID AP and start web server)");
    createWebServer(0);
    mqtt_start();
  } else {
    Serial.println("Local IP is 0.0.0.0 so show SSID, PASS & URL configuration web: web mode 1");
    func_cleareeprom();
    createWebServer(1);
    
  }

  // Start the server
  server.begin();
  Serial.println("Server started");

}

void mqtt_start(){
   Serial.println("Starting MQTT On Device Client");
   Serial.print("MQTT BROKER IP (STRING): ");
   Serial.println(mqtt_server);
   char* mqtt_server_char; 
   mqtt_server.toCharArray(mqtt_server_char, 50);
   Serial.print("MQTT BROKER IP (CHAR): ");
   Serial.println(mqtt_server_char);
   client.setServer(mqtt_server_char, 1883);
   client.setCallback(callback);
   
   client.publish("esp8266status", "Relay OFF");
   if (!client.connected()) {
    reconnect();
   }
   IPAddress ip = WiFi.localIP();
   localip = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
   client.loop();
}



//function to start AP mode
void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, password, 6);
  Serial.println("softap");

  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  launchWeb(1);
  Serial.println("over");
}

//function to create web server
void createWebServer(int webtype)
{
  Serial.println("If Webtype is 1 Configure SSID & PASS if is 0 show local IP: Webtype is:");
  Serial.println(webtype);
  auxwebtype = webtype;
  delay(5000);
  if ( webtype == 1 ) {
    Serial.println("CONFIGURING SSID CONNECTION");
    server.on("/", []() {
      if ( auxwebtype == 1 ) {
        Serial.println("CONFIGURING SSID CONNECTION STARTING");
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>PASS: </label><input name='pass' length=64><label>SERVER URL: </label><input name='url' length=64><input type='submit'></form>";
        content += "</html>";
        Serial.println("Sending configuration data");
        server.send(200, "text/html", content);
        content = "";
      } else if ( auxwebtype == 0 ) {
        IPAddress ip = WiFi.localIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        server.send(200, "text/html", "Second Stage {\"IP\":\"" + ipStr + "\"}");
      }
    });
    server.on("/setting", []() {
      qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      qurl = server.arg("url");

      if (qsid.length() > 0 && qpass.length() > 0 && qurl.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 128; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println(qurl);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        Serial.println("writing eeprom url:");
        for (int i = 0; i < qurl.length(); ++i)
        {
          EEPROM.write(64 + i, qurl[i]);
          Serial.print("Wrote: ");
          Serial.println(qurl[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        Serial.println("stoping AP mode . . .");
        WiFi.mode(WIFI_STA);
        Serial.println("AP mode Stopped");
        setup();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {

    Serial.println("stoping AP mode . . .");
    WiFi.mode(WIFI_STA);
    Serial.println("AP mode Stopped");

    char connected_ssid[80];
    esid.toCharArray(connected_ssid, 80);

    Serial.println("CONNECTED TO SSID: ");
    Serial.println(connected_ssid);


    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "text/html", "{\"IP\":\"" + ipStr + "\"}");
    });

    server.on("/cleareeprom", []() {

       func_cleareeprom();

    });
    server.on("/disconnect", []() {

      func_disconnect();

    });

    }
}




void func_disconnect() {
      //Disconnect from configured SSID AP and setup owns AP again web mode 1
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Disconecting</p></html>";
      server.send(200, "text/html", content);
      Serial.println("disconecting . . .");
      WiFi.disconnect();
      Serial.println("Disconected");
      //setup owns AP again web mode 1
      setupAP();
}

void func_cleareeprom() {
      //Clear EEPROM to restart ESP default values
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 128; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.println("eeprom cleared...");
}


void func_configuration_mode() {
      Serial.println("CONFIGURING SSID CONNECTION");
      Serial.println("CONFIGURING SSID CONNECTION STARTING");
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>PASS: </label><input name='pass' length=64><label>SERVER URL: </label><input name='url' length=64><input type='submit'></form>";
      content += "</html>";
      Serial.println("Sending configuration data");
      server.send(200, "text/html", content);
      content = "";
      return;
}

void func_read_current_sensor() {
  Serial.println("func_read_current_sensor");
  i = 0;
  for (int x = 0; x < samplenumber + 1; x++){
    value = analogRead(C_SENSOR1);    
    
    val = map(value, 0, 767, 0, 512);
    value = val;
    
  
    //Summing counter
    i++;
  
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
  
    if (i>=samplenumber)
    {
      i=0;
      //Calculation of the root of the mean of the current squared (rms)
      Irms = factorA*sqrt(sumI/samplenumber)+Ioffset;
      if (Irms<0.05) {Irms=0;}
  
      //Calculation of the root of the mean of the voltage squared (rms)
  
      apparentPower = Irms * SetV;
      Serial.print(" A0: ");
      Serial.print(value);
      Serial.print(" Watios: ");
      Serial.print(apparentPower);
      Serial.print(" Voltaje: ");
      Serial.print(SetV);
      Serial.print(" Amperios: ");
      Serial.print(Irms);
      Serial.print(" status: ");
      Serial.print(r1);
      Serial.print(" c_max: ");
      Serial.print(c_max);
      Serial.println();
  
      //Reset values ready for next sample.
      sumI=0.0;
    }
  }
}

void loop() {
  server.handleClient();
}