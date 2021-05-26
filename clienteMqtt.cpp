#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#define reconnectButton D1
boolean presionoReconnectButton = false;

//Pines bomba de agua
#define pinBombaAguaIN1 D5 //pin direccion motor, nunca poner en HIGH ambos
#define pinBombaAguaIN2 D6 //pin direccion motor, nunca poner en HIGH 
#define pinBombaAguaENA D7 //Pin controla la velocidad
int velocidadMotor = 255;


void controlBombaAgua(String valor) {
  if (valor == "1") {
    Serial.println("Encender Bomba de agua");
    analogWrite(pinBombaAguaENA, 255);
    digitalWrite(pinBombaAguaIN1, HIGH);
    digitalWrite(pinBombaAguaIN2, LOW);
    analogWrite(pinBombaAguaENA, velocidadMotor);
  } else if (valor == "0") {
    Serial.println("Apagar Bomba de agua");
    digitalWrite(pinBombaAguaIN1, LOW);
    digitalWrite(pinBombaAguaIN2, LOW);
  }
}

// void callback(char *topic, byte *payload, unsigned int length1);

void callback(char *topic, byte *payload, unsigned int length1)
{
  Serial.print("Mensaje del topic: [");
  Serial.print(topic);
  Serial.println("]");
  String myString = "";
  for (int i = 0; i < length1; i++)
  {
    //        Serial.print(payload[i]);
    myString += (char)payload[i];
  }

  Serial.println("Datos recibidos: " + myString);
  controlBombaAgua(myString);


}


char mqtt_server[50] = "XXXX";
char mqtt_topic[50] = "XXXX";
char mqtt_device[50] = "XXXX";
char mqtt_client_id[30] = "";

WiFiClient espclient;
PubSubClient client(mqtt_server, 1883, callback, espclient);

#define OTHER_PIN 0

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // ESP32 DOES NOT DEFINE LED_BUILTIN
#endif

//for LED status
#include <Ticker.h>
Ticker ticker;

int LED = LED_BUILTIN;

// Validar si hay valores guardados en la epprom
bool shouldSaveConfig = false;

void tick()
{
  //toggle state
  digitalWrite(LED, !digitalRead(LED)); // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  shouldSaveConfig = true;
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void reconnect()
{
  Serial.println("Mqtt server en reconnect(): " + (String)mqtt_server);

  //    while (WiFi.status() != WL_CONNECTED)
  //    {
  //        delay(500);
  //        Serial.print(".");
  //    }
  while (!client.connected())
  {
    if (client.connect(mqtt_client_id))
    {
      Serial.println("Conectado al MQTT server: " + (String)mqtt_server);

      client.subscribe(mqtt_topic);
      //      client.subscribe("mike/5625/luzsalacasamama");
      //            client.subscribe("testTopic");
    }
    else
    {
      Serial.print("failed,rc=");
      Serial.println(client.state());
      delay(500);
    }
  }
}


void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  //  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}


String generarId(int tamanio) {
  String id = "";
  for (int i = 0; i <= tamanio; i++) {
    id += (String)random(0, 9);
  }
  return id;
}

void conectarWifi() {
  int reconnectButtonVal = digitalRead(reconnectButton);
  if (!presionoReconnectButton && reconnectButtonVal == LOW) {
    presionoReconnectButton = true;
  }
  if (presionoReconnectButton && reconnectButtonVal == HIGH) {
    presionoReconnectButton = false;
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("OnDemandAP");
    Serial.println("connected...yeey :)");

  }
}

void inicializarMqttClient()
{
  Serial.begin(115200);
  EEPROM.begin(512);

  //pines Bomba de agua
  pinMode(pinBombaAguaIN1, OUTPUT);
  pinMode(pinBombaAguaIN2, OUTPUT);
  pinMode(pinBombaAguaENA, OUTPUT);


  pinMode(reconnectButton, INPUT_PULLUP);


  WiFi.mode(WIFI_STA);
  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_server("server", "Mqtt Server", mqtt_server, 50);
  WiFiManagerParameter custom_mqtt_topic("topic", "Mqtt topic", mqtt_topic, 50);
  WiFiManagerParameter custom_mqtt_device("device", "Mqtt device", mqtt_device, 50);

  //WiFiManager
  WiFiManager wm;
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_device);

  //Funcion para buscar otro wifi sino encuentra el que esta registrado
  //No usar junto a startConfigPortal()

  if (!wm.autoConnect())
  {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }


  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(mqtt_device, custom_mqtt_device.getValue());

  Serial.println("EL MQTT SERVER: " + (String)mqtt_server);


  if (shouldSaveConfig) {
    Serial.println("SE CAMBIO MQTT SERVER: " + (String)mqtt_server);
    Serial.println("SE CAMBIO MQTT TOPIC: " + (String)mqtt_topic);
    Serial.println("SE CAMBIO MQTT TOPIC: " + (String)mqtt_device);
    writeString(250, (String)mqtt_server);
    writeString(310, (String)mqtt_topic);
    writeString(370, (String)mqtt_device);
    EEPROM.commit();
  } else {
    strcpy(mqtt_server, read_String(250).c_str());
    strcpy(mqtt_topic, read_String(310).c_str());
    strcpy(mqtt_device, read_String(370).c_str());
    //    Serial.println("prueba mqtt server en eeprom: " + read_String(350));
    //    Serial.println("prueba mqtt topic en eeprom: " + read_String(400));

  }

  Serial.println("EL MQTT SERVER: " + (String)mqtt_server);
  Serial.println("EL MQTT TOPIC: " + (String)mqtt_topic);
  Serial.println("EL MQTT DEVICE: " + (String)mqtt_device);

  String generateClientId = "Esp8266Client" + generarId(10);
  strcpy(mqtt_client_id, generateClientId.c_str());
  Serial.println("EL MQTT CLIENT ID: " + (String)mqtt_client_id);

  ticker.detach();
  //keep LED on
  digitalWrite(LED, LOW);

  reconnect();
}

void loopClientMqtt()
{

  //Funcion para conectar a un nuevo wifi presionando el boton
  conectarWifi();

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}

void publicarMqtt(String topic, String mensaje)
{
  String unido = String(mqtt_topic) + "/" + String(mqtt_device) + "/" + topic;
  bool enviado = client.publish(String(unido).c_str(), String(mensaje).c_str());
  String aviso  = "";
  if (enviado) {
    aviso = "si";
  } else {
    aviso = "no";
  }
  Serial.println("Publish: topic = " + unido + "; data = " + mensaje + "; enviado = " + aviso);
}
