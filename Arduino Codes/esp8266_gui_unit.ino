#include <PubSubClient.h>
#include <ESP8266WiFi.h>

void callback(char* topic, byte* payload, unsigned int length);

//Match MQTT and Wifi Setup
const char* MQTT_SERVER = "192.168.43.46";
const char* ssid = "qwert";
const char* password = "qwer1357";

//names of topics
char* sensorTopic = "sensor/#";
char* battTopic = "battery/#";
char* conTopic = "confirm/units";

uint16_t chk = 0, nchk = 0;
uint32_t tWait = 0;
byte confUn = 0;
int chkEn = 16;
int alarm = 14;

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  reconnect();
  delay(2000);
  pinMode(chkEn, INPUT_PULLUP);
  pinMode(alarm, INPUT_PULLUP);
}


void loop() {
  if (!client.connected() && WiFi.status() == 3) {
    reconnect();
  }
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10);

  if ((digitalRead(chkEn)) && (chk == 0))  {
    chk = 1;
    nchk = 1;
    tWait = millis();
    client.publish("check/units", "tempCheck");
  }

  if (chk == 12)  {
    client.publish("check/units", "smokeCheck");
    chk = 2; nchk = 2; tWait = millis();
  }
  else if (chk == 13)  {
    client.publish("check/units", "gasCheck");
    chk = 3; nchk = 3; tWait = millis();
  }
  else if (chk == 14)  {
    client.publish("check/units", "doorCheck");
    chk = 4; nchk = 4; tWait = millis();
  }
  else if (chk == 15) {
    client.publish("check/units", "motion1Check");
    chk = 5; nchk = 5; tWait = millis();
  }
  else if (chk == 16)  {
    client.publish("check/units", "motion2Check");
    chk = 6; nchk = 6; tWait = millis();
  }


  while ((tWait + 20000 < millis()) && (chk != 0)) {
    if (nchk == chk) {
      char arrSend[5] = "ccn";
      arrSend[3] = chk + 0x30;
      arrSend[4] = 0;
      Serial.write(arrSend);
      chk = chk + 11;
      if (chk == 17)chk = 0;
    }
    nchk = chk;
    tWait = millis();
  }

  if (digitalRead(alarm)) {
  client.publish("state/alarm", "on");
  }
  else {
    client.publish("state/alarm", "off");
  }

  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  //convert topic to string to make it easier to work with
  String topicStr = topic;

  //Reduce the strings that will be sent quickly in serial communication.
  char arrSend[20];
  int j;
  if (topicStr == "sensor/smoke") {
    arrSend[0] = 's'; j = 1;
  }
  else if (topicStr == "sensor/gas") {
    arrSend[0] = 'g';  j = 1;
  }
  else if (topicStr == "sensor/motion1") {
    arrSend[0] = 'm';  arrSend[1] = '1'; j = 2;
  }
  else if (topicStr == "sensor/motion2") {
    arrSend[0] = 'm';  arrSend[1] = '2'; j = 2;
  }
  else if (topicStr == "sensor/temp") {
    arrSend[0] = 't';  j = 1;
  }
  else if (topicStr == "sensor/door") {
    arrSend[0] = 'd'; j = 1;
  }

  if (topicStr == "battery/smoke") {
    arrSend[0] = 'b';  arrSend[1] = 's'; j = 2;
  }
  else if (topicStr == "battery/gas") {
    arrSend[0] = 'b';   arrSend[1] = 'g'; j = 2;
  }
  else if (topicStr == "battery/motion1") {
    arrSend[0] = 'b';  arrSend[1] = 'm'; arrSend[2] = '1'; j = 3;
  }
  else if (topicStr == "battery/motion2") {
    arrSend[0] = 'b';  arrSend[1] = 'm'; arrSend[2] = '2'; j = 3;
  }
  else if (topicStr == "battery/temp") {
    arrSend[0] = 'b';  arrSend[1] = 't'; j = 2;
  }
  else if (topicStr == "battery/door") {
    arrSend[0] = 'b';  arrSend[1] = 'd'; j = 2;
  }
  if (topicStr == "confirm/units") {
    arrSend[0] = 'c'; arrSend[1] = 'c';
  }

  char data[20];
  int i = 0;
  for ( ; i != length; i++)
  {
    data[i] = payload[i];
  }
  data[i] = 0;
  String dataStr = (char *)data;

  if ((topicStr == "sensor/motion1") || (topicStr == "sensor/motion2")) {
    if ((dataStr == "motion1 detected") || (dataStr == "motion2 detected")) {
      arrSend[2] = '1'; arrSend[3] = 0;
    }
    if ((dataStr == "motion1 no detect") || (dataStr == "motion2 no detect")) {
      arrSend[2] = '0'; arrSend[3] = 0;
    }
  }
  else if (topicStr == "confirm/units") {
    if (dataStr == "tempWork") {
      arrSend[2] = '1'; arrSend[3] = 0; chk = 12;
    }
    else if (dataStr == "smokeWork") {
      arrSend[2] = '2'; arrSend[3] = 0; chk = 13;
    }
    if (dataStr == "gasWork") {
      arrSend[2] = '3'; arrSend[3] = 0; chk = 14;
    }
    if (dataStr == "doorWork") {
      arrSend[2] = '4'; arrSend[3] = 0; chk = 15;
    }
    if (dataStr == "motion1Work") {
      arrSend[2] = '5'; arrSend[3] = 0; chk = 16;
    }
    if (dataStr == "motion2Work") {
      arrSend[2] = '6'; arrSend[3] = 0; chk = 0;
    }
  }
  else {
    for (int p = j; j < i + p; j++)
    {
      arrSend[j] = data[j - p];
    }
    arrSend[j] = 0;
  }

  Serial.write(arrSend);
}

void reconnect() {
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
      String clientName = "esp8266-guiUnit";

      //if connected, subscribe to the topics we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        client.subscribe(battTopic);
        client.subscribe(sensorTopic);
        client.subscribe(conTopic);
      }
      else {
        abort();
      }
    }
  }
}

