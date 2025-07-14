#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>

OneWire  ds(2);
void callback(char* topic, byte* payload, unsigned int length);

//Match MQTT and Wifi Setup
const char* MQTT_SERVER = "192.168.43.46";
const char* ssid = "qwert";
const char* password = "qwer1357";

char* checkTopic = "check/units";

float batt;
uint32_t time_delay;
char arr[4];

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void battPerPub ()
{
  batt = (analogRead(A0) - 754.0) / 176.0 * 100.0;
  intToAscii(batt);
  client.publish("battery/temp", arr);
  time_delay = millis();
}

void senPerPub ()
{
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  float celsius;

  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);
  //ds.depower() here, but the reset will take care of it.

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad


  for ( i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];

  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time

  celsius = (float)raw / 16.0;
  intToAscii (celsius);
  client.publish("sensor/temp", arr);
}

void intToAscii (uint16_t data) {
  if (data >= 100) {
    arr[0] = '1';
    arr[1] = '0';
    arr[2] = '0';
    arr[3] = 0;
  }
  else if ((data < 100) && (data > 9)) {
    arr[0] = data / 10 + 48;
    arr[1] = data % 10 + 48;
    arr[2] = 0;
  }
  else if ( data < 10) {
    arr[0] = data + 48;
    arr[1] = 0;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  reconnect();
  delay(2000);
  battPerPub ();
}

void loop() {
  if (!client.connected() && WiFi.status() == 3) {
    reconnect();
  }
  client.loop();
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10);

  senPerPub ();

  if ((time_delay + 900000 <= millis()) && (batt > 20))
    battPerPub();
  else if ((time_delay + 300000 <= millis()) && (batt < 20))
    battPerPub();
}

void callback(char* topic, byte* payload, unsigned int length) {
  //convert topic to string to make it easier to work with
  String topicStr = topic;

  //publish a confirmation message to the MQTT server that the unit is working
  char aa[20];
  int i = 0;
  for ( ; i != length; i++)
  {
    aa[i] = payload[i];
  }
  aa[i] = 0;
  String aaa = (char *)aa;

  if (aaa == "tempCheck") {
    client.publish("confirm/units", "tempWork");
  }
}

void reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if (WiFi.status() == WL_CONNECTED) {
    // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      String clientName = "esp8266-tempUnit";
      
      //if connected, subscribe to the topic we want to be notified about
      if (client.connect((char*) clientName.c_str()))
        client.subscribe(checkTopic);
      else
        abort();
    }
  }
}

