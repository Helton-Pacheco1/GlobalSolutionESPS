
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>


const char* ssid     = "Wokwi-GUEST";
const char* password = "";


const char* mqtt_server = "9ca03c36523a4fbaa8ffdfb128beef7b.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "GlobalSolutionESPS";
const char* mqtt_pass   = "Global@Solution1";

const char* telemetryTopic = "spacex/dragon01/telemetry";
const char* alertTopic     = "spacex/dragon01/alerts";


#define DHTPIN      15
#define DHTTYPE     DHT22
#define PIN_PRESSAO 34   // potenciômetro de pressão
#define PIN_OXIGENIO 35  // potenciômetro de oxigênio
#define LED_ALERTA   2
#define BUZZER       4

DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure espClient;
PubSubClient     client(espClient);


void setup_wifi() {
  Serial.println();
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao HiveMQ... ");
    String clientId = "Dragon-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("OK");
    } else {
      Serial.print("Erro MQTT: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(LED_ALERTA, OUTPUT);
  pinMode(BUZZER,     OUTPUT);
  digitalWrite(LED_ALERTA, LOW);
  digitalWrite(BUZZER,     LOW);

  dht.begin();
  setup_wifi();

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Sistema Dragon iniciado.");
  Serial.println("Potenciometro ESQUERDA = nominal | DIREITA = anomalia");
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  float temperatura = dht.readTemperature();
  if (isnan(temperatura)) {
    temperatura = 23.0;
  }

  int leituraPressao = analogRead(PIN_PRESSAO);
  float pressao;

  if (leituraPressao <= 2047) {

    pressao = map(leituraPressao, 0, 2047, 94, 106);
  } else {
 
    pressao = map(leituraPressao, 2048, 4095, 115, 125);
  }


  int leituraOxigenio = analogRead(PIN_OXIGENIO);
  float oxigenio;

  if (leituraOxigenio <= 2047) {

    oxigenio = map(leituraOxigenio, 0, 2047, 195, 225) / 10.0;
  } else {
    // metade direita → valores de anomalia
    oxigenio = map(leituraOxigenio, 2048, 4095, 140, 170) / 10.0;
  }

  bool alerta = false;

  if (temperatura < 18.0 || temperatura > 28.0) alerta = true;
  if (pressao     < 90.0 || pressao     > 110.0) alerta = true;
  if (oxigenio    < 19.0 || oxigenio    > 23.0)  alerta = true;


  if (alerta) {
    digitalWrite(LED_ALERTA, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);

    String alertMsg = "{\"status\":\"ALERTA\"}";
    client.publish(alertTopic, alertMsg.c_str());

  } else {
    digitalWrite(LED_ALERTA, LOW);
  }


  String payload = "{";
  payload += "\"temperatura\":" + String(temperatura, 1) + ",";
  payload += "\"pressao\":"     + String(pressao,     1) + ",";
  payload += "\"oxigenio\":"    + String(oxigenio,    1) + ",";
  payload += "\"alerta\":"      + String(alerta ? "true" : "false");
  payload += "}";

  client.publish(telemetryTopic, payload.c_str());

  Serial.println();
  Serial.println("===== TELEMETRIA DRAGON =====");
  Serial.print("Leitura pressao (ADC):  "); Serial.println(leituraPressao);
  Serial.print("Leitura oxigenio (ADC): "); Serial.println(leituraOxigenio);
  Serial.println(payload);
  Serial.println("=============================");

  delay(5000);
}
