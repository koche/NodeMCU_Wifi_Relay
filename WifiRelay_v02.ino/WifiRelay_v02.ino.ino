/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RELAY_PIN1   D1
#define RELAY_PIN2   D2

// Update these with values suitable for your network.
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";
/*
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int   mqtt_port = 1883;
const char* userName = "";
const char* userPass = "";
const char* mqttPubTopic = "/wifiRelayPub";
const char* mqttSubTopic = "/wifiRelaySub";
*/

const char* mqtt_server = "iot.cht.com.tw";
const int   mqtt_port = 1883;
const char* userName = "mqtt_user_name";
const char* userPass = "mqtt_user_password";
const char* mqttPubTopic = "publishTopic";
const char* mqttSubTopic = "subscribeTopic";
const char* cht_pub_sensor_ID = "switchStatus";

WiFiClient espClient;
PubSubClient client(espClient);
long startMin, onMin;
long lastMsg = 0;
char msg[256];
int value = 0;
int stat=0, lastStat=1; //1:ON, 0:OFF

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char *p;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  /*
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }*/

  p = strstr((char *)payload, "OPEN_");
  if( p != NULL){
    digitalWrite(RELAY_PIN1, HIGH);
    startMin = millis();
    onMin = atoi(p+5) * 1000 * 60;
    Serial.print(p);
    Serial.print("onMin=");
    Serial.println(onMin);
    stat = 1;
  }else if(strstr((char *)payload, "OFF") != NULL){
    startMin = millis();
    onMin = 0;
    stat = 0;
  }else{
    Serial.println("This is unknown command !");
  }

  /*
  if(strstr((char *)payload, "OPEN_30") != NULL){
    digitalWrite(RELAY_PIN1, HIGH);
    startMin = millis();
    onMin = 30 * 1000 * 60;
  }else if(strstr((char *)payload, "OPEN_15") != NULL){
    digitalWrite(RELAY_PIN1, HIGH);
    startMin = millis();
    onMin = 15 * 1000  * 60;
  }else if(strstr((char *)payload, "OFF") != NULL){
    startMin = millis();
    onMin = 0;
  }else{
    Serial.print("This is unknown command !");
  }
  */
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    randomSeed(millis());
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if (client.connect(clientId.c_str())) {
    if (client.connect(clientId.c_str(), userName, userPass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      if(client.subscribe(mqttSubTopic) == false){
        Serial.println("subscribe fail");
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  
  //set mode
  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  onMin = 0;
  //DynamicJsonDocument doc(1024);

  /* json test */

  
  setup_wifi();
  client.setCallback(callback);
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  stat=0;
  lastStat=1; //1:ON, 0:OFF
}

void loop() {
  float leftMin;
  long clientLoop = 0;
  long now = millis();
  int pubFlag = 0;
  
  if (!client.connected()) {
    reconnect();
  }

  if (now - clientLoop  > 100){
    client.loop();
    clientLoop = now;
  }
  
  //Check if time is up, then turn off sw
  if((now - startMin) > onMin){
    //Need to turn off switch
    if(digitalRead(RELAY_PIN1) == HIGH){  //switch is ON then turn off
      digitalWrite(RELAY_PIN1, LOW);
      pubFlag = 1;
    }
  }

  //When status change (0->1 or 1->0 ), publish message
  if(lastStat != stat){
    pubFlag = 1;
  }
  
  if (now - lastMsg > 30000) { // publish every 30 sec
    lastMsg = now;
    ++value;
    pubFlag = 1;    
  }

  if(pubFlag){
    if(digitalRead(RELAY_PIN1) == HIGH){  //switch is ON
      leftMin = (onMin - (now - startMin))/1000.0/60.0;
      snprintf(msg, 128, "[{\"id\":\"%s\",\"value\":[\"ON 剩 %.1f MIN\"]}]", cht_pub_sensor_ID, leftMin);
      stat = 1;
    }else{
      snprintf(msg, 128, "[{\"id\":\"%s\",\"value\":[\"OFF\"]}]", cht_pub_sensor_ID);
      stat = 0;
    }

    Serial.println("last stat    stat");
    Serial.print(lastStat);Serial.print("            ");Serial.println(stat);
    if( stat == 1 || lastStat != stat)
    {
      //snprintf (msg, 50, "hello world #%ld", value);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish(mqttPubTopic, msg);
    }
    pubFlag = 0;
  }
  lastStat = stat;
  Serial.print("update last stat to = "); Serial.println(lastStat);
  delay(1000);
}
