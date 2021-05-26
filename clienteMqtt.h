#include <PubSubClient.h>

void inicializarMqttClient();
void reconnect();
void loopClientMqtt();
void publicarMqtt(String topic, String mensaje);
