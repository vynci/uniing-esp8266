#include <PubSubClient.h>
#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

MDNSResponder mdns;
WiFiServer server(80);

const char* ssid = "uniing-v1";
String st;
char* topic = "device/control";
char* topicPublish = "device/sensor";
//char* broker = "10.1.2.64";
char* broker = "10.1.2.47";
//char* broker = "mqtt://vynci-mac.local";
char message_buff[100];

IPAddress mqttServer(10, 1, 2, 47);

#define BUFFER_SIZE 100

WiFiClient client;
//PubSubClient mqttClient(broker, 1883, callback, client);
PubSubClient mqttClient(client, mqttServer);

void callback(const MQTT::Publish& pub) {
  Serial.print(pub.topic());
  Serial.print(" => ");
  if (pub.has_stream()) {
    uint8_t buf[BUFFER_SIZE];
    int read;
    while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
      Serial.write(buf, read);
    }
    pub.payload_stream()->stop();
    Serial.println("");
  } else{
    Serial.println(pub.payload_string());
    String msgString = String(pub.payload_string());
    if(msgString == "dev1-on"){
      Serial.println("Device 1 on");
      digitalWrite(2, 1);
    } else if(msgString == "dev1-off"){
      Serial.println("Device 1 off");
      digitalWrite(2, 0);    
    } else if(msgString == "dev2-on"){
      Serial.println("Device 2 on");
      digitalWrite(0, 1);
    } else if(msgString == "dev2-off"){
      Serial.println("Device 2 off");    
      digitalWrite(0, 0);
    }
  }
  
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void setup() {
  
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);  
  digitalWrite(0, 0);
  digitalWrite(2, 0);
  
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          Serial.println("launching webtype 2");
          launchWeb(2);
          return;
      }
  }
  setupAP(); 
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { 
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);
      clientName += "-";
      clientName += String(micros() & 0xff, 16);
    
      Serial.print("Connecting to ");
      Serial.print(broker);
      Serial.print(" as ");
      Serial.println(clientName);
      
      return(20); 
  
    } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void launchWeb(int webtype) {
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  
  if (!mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  
  Serial.println("mDNS responder started");
  // Start the server
  server.begin();
  Serial.println("Server started");
  Serial.print("Webtype: ");  
  Serial.println(webtype);  
  
  int b = 20;
  do{
    if(webtype != 1){
      break;
    }
    b = mdns1(webtype);
  } while(b == 20);
  Serial.println("Loop exited!");
}

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
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  WiFi.softAP(ssid);
  Serial.println("softap");
  Serial.println("");
  launchWeb(1);
  Serial.println("over");
}

int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient mdnsClient = server.available();
  if (!mdnsClient) {
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
//  while(mdnsClient.connected() && !mdnsClient.available()){
//    Serial.println("i'm stuck here!");
//    delay(1);
//   }
  
  // Read the first line of HTTP request
  String req = mdnsClient.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  mdnsClient.flush(); 
  String s;
  if ( webtype == 1 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += st;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
        mdnsClient.print(s);
        Serial.println("Done with client");
        return(20);
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        qsid = req.substring(8,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
//        Serial.println("writing eeprom pass:"); 
//        for (int i = 0; i < qpass.length(); ++i)
//          {
//            EEPROM.write(32+i, qpass[i]);
//            Serial.print("Wrote: ");
//            Serial.println(qpass[i]); 
//          }    
        int buffer_size = qpass.length() + 1;
        char pass_encoded[buffer_size];
        char pass_decoded[buffer_size];

        qpass.toCharArray(pass_encoded,buffer_size);
        int decoded_size = urldecode(pass_decoded,pass_encoded);

        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < decoded_size; ++i)
        {
          EEPROM.write(32+i, pass_decoded[i]);
          Serial.print("Wrote: ");
          Serial.println(pass_decoded[i]);
        }
        
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
        mdnsClient.print(s);
        Serial.println("Done with client");
        return(19);
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
        mdnsClient.print(s);
        Serial.println("Done with client");
        return(20);
      }
  } 
  else
  {
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
      mdnsClient.print(s);
      Serial.println("Done with client");
      return(19);      
  }
 
}

int urldecode(char *dst, const char *src)
{
  char a, b;
  int new_size = 0;
  while (*src) {
    if ((*src == '%') &&
      ((a = src[1]) && (b = src[2])) &&
      (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a'-'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a'-'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16*a+b;
      src+=3;
    } 
    else {
      *dst++ = *src++;
    }
    new_size += 1;
  }
  *dst++ = '\0';
  return new_size;
}

void loop() {
  // put your main code here, to run repeatedly  
//  mqttClient.loop();

    mdns1(0);

    if (WiFi.status() != WL_CONNECTED) {
      //abort();
    }

    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) {
        if (mqttClient.connect("uniing-client-0001")) {
      	  mqttClient.set_callback(callback);
          if (mqttClient.subscribe(topic)){
            Serial.println("Successfully subscribed");
          }    
        }
      }
  
      if (mqttClient.connected())
        mqttClient.loop();
    }
  
}
