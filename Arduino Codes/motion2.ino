#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//motion sensor on ESP8266 GPIO16
#define motionSen 16

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
  batt = analogRead(A0);
  batt = (batt - 754.0) / 176.0 * 100.0;
  intToAscii(batt);
  client.publish("battery/motion2", arr);
  time_delay = millis();
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
  pinMode(motionSen, INPUT_PULLUP);
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

  if (digitalRead(motionSen) == HIGH)
    client.publish("sensor/motion2", "motion2 detected");
  else
    client.publish("sensor/motion2", "motion2 no detect");

  if ((time_delay + 900000 <= millis()) && (batt > 20))
    battPerPub();
  else if ((time_delay + 300000 <= millis()) && (batt < 20))
    battPerPub();

  delay(1000);
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

  if (aaa == "motion2Check") {
    client.publish("confirm/units", "motion2Work");
  }
}

void reconnect() {
  //attempt to connect to the wifi if connection is lost
  if (WiFi.status() != WL_CONNECTED) {
    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if (WiFi.status() == WL_CONNECTED) {
    // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      String clientName = "esp8266-motion2";
      //if connected, subscribe to the topic we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        client.subscribe(checkTopic);
      }
      else {
        abort();
      }
    }
  }
}
