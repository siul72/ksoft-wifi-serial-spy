#include <Arduino.h>
#include <SoftwareSerial.h>
//#include <Ticker.h>
#include <TaskScheduler.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <AsyncMqttClient.h>

#define  LED_WATCHDOG 2
#define RE 5 //D1 on WeMos board
#define rxPin 4 //D2 on WeMos board
#define txPin 16 //D0 on WeMos board

#define WIFI_SSID "dragino-1ad834"
#define WIFI_PASSWORD "dragino-dragino"

#define MQTT_HOST IPAddress(10, 130, 1, 3)
#define MQTT_PORT 1883

Scheduler runner;

AsyncMqttClient mqttClient;
//mqttReconnectTimer.once(2, connectToMqtt);
void connectToMqtt();
Task mqttReconnectTimer(2000, TASK_ONCE, &connectToMqtt);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
void connectToWifi();
Task wifiReconnectTimer(2000, TASK_ONCE, &connectToWifi);

//delay(40);
//serialThread();
void serialThread();
Task readSerialPort(40, TASK_FOREVER, &serialThread);
SoftwareSerial mySerial(rxPin, txPin);

void listWifiAPS(){

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();

  delay(5000);

}

void testPing(){

if(Ping.ping(MQTT_HOST)) {
  Serial.println("Ping to mqtt server ok!!");
} else {
  Serial.println("Ping to mqtt server nok!!");
}
}

void connectToWifi() {

  Serial.print("Connecting to Wi-Fi...");
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  //WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  testPing();
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  mqttClient.connect();
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {


  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  Serial.println("... Connected to Wi-Fi.");
  Serial.print("Status: "); Serial.println(WiFi.status());    // Network parameters
  Serial.print("IP: ");     Serial.println(WiFi.localIP());
  Serial.print("Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("SSID: "); Serial.println(WiFi.SSID());
  Serial.print("Signal: "); Serial.println(WiFi.RSSI());

  //connectToMqtt();
  wifiReconnectTimer.disable();
  runner.deleteTask(wifiReconnectTimer);

  runner.addTask(mqttReconnectTimer);
  mqttReconnectTimer.setIterations(TASK_ONCE);
  mqttReconnectTimer.enable();
  Serial.println("start mqttReconnectTimer");
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  Serial.println("Disconnected from Wi-Fi.");
  //mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  //wifiReconnectTimer.once(2, connectToWifi);

  mqttReconnectTimer.disable();
  runner.deleteTask(mqttReconnectTimer);
  runner.addTask(wifiReconnectTimer);
  wifiReconnectTimer.enable();

}



void onMqttConnect(bool sessionPresent) {
  mqttReconnectTimer.disable();
  runner.deleteTask(mqttReconnectTimer);
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  Serial.println("Connected to MQTT.");
  //Serial.print("Session present: ");
  //Serial.println(sessionPresent);

  runner.addTask(readSerialPort);
  readSerialPort.enable();
  readSerialPort.setIterations(TASK_FOREVER);
  //disable rx
  digitalWrite(RE, LOW);
  Serial.println("added and enable readSerialPort Task");

}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));
  Serial.printf("Disconnected from MQTT. %d\n", (int)reason);

  if (WiFi.isConnected()) {
    //mqttReconnectTimer.once(2, connectToMqtt);
    runner.addTask(mqttReconnectTimer);
    mqttReconnectTimer.setIterations(TASK_ONCE);
    mqttReconnectTimer.enable();
    Serial.println("mqttReconnectTimer");
  }

  if(readSerialPort.isEnabled()){
    readSerialPort.disable();
    runner.deleteTask(readSerialPort);
    //disable rx
    digitalWrite(RE, HIGH);
    Serial.println("remove readSerialPort Task");
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void mqttPublish(String payload) {
  if(!mqttClient.connected()){
    return;
  }

  mqttClient.publish("rs485/frame", 0, true, payload.c_str());


}

void setup() {

  pinMode(LED_WATCHDOG, OUTPUT);
  pinMode(RE, OUTPUT);
  pinMode(rxPin, INPUT);
  //disable rx
  digitalWrite(RE, HIGH);

  Serial.begin(115200);
  mySerial.begin(19200);           // only for debug
  delay(5000);
  Serial.println("start logging");
  listWifiAPS();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  runner.init();
  Serial.println("Initialized scheduler");

  runner.addTask(wifiReconnectTimer);
  wifiReconnectTimer.enable();
  Serial.println("added and enable wifiReconnectTimer");
  Serial.println("RS485toMQTT 1.0 started");


}

void timeToString(char* string, size_t size) {
  unsigned long nowMillis = millis();
  unsigned long seconds = nowMillis / 1000;
  uint32 mil = nowMillis % 1000;
  int days = seconds / 86400;
  seconds %= 86400;
  byte hours = seconds / 3600;
  seconds %= 3600;
  byte minutes = seconds / 60;
  seconds %= 60;
  snprintf(string, size, "%04d:%02d:%02d:%02d.%03d", days, hours, minutes, (uint)seconds, mil);
}

void serialThread() {

  bool rcv_done = false;

  String frame = "";
  while(mySerial.available()>0){
    if (!rcv_done){
      char str[18] = "";
      timeToString(str, sizeof(str));
      //Serial.print(str);
      frame = frame +"<frame timestamp=\""+str+"\">";
    }
    byte b=mySerial.read();
    //delay(1);
    char tmp[4];
    sprintf(tmp, "%02x", b);
    frame = frame + tmp + ':';
    rcv_done = true;
  }

  if (rcv_done) {
    //Serial.println("new frame on queue");
    frame = frame +"</frame>";
    //rxQueue.push(frame);
    //Serial.println(frame);
    mqttPublish(frame.c_str());
    digitalWrite(LED_WATCHDOG, !digitalRead(LED_WATCHDOG));

  }

}

void publishSomething(){
    String frame = "";
    char str[18] = "";
    timeToString(str, sizeof(str));
    frame = frame + str + " hello!";
    mqttPublish(frame.c_str());
}


void loop() {
 
  runner.execute();

}
