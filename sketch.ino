#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===========================
// WOKWI WIFI
// ===========================
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ===========================
// HIVEMQ CLOUD
// ===========================
const char* mqtt_server = "9ca03c36523a4fbaa8ffdfb128beef7b.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "GlobalSolutionESPS";
const char* mqtt_pass   = "Global@Solution1";

const char* telemetryTopic = "spacex/dragon01/telemetry";
const char* alertTopic     = "spacex/dragon01/alerts";

// ===========================
// PINOS
// ===========================
#define DHTPIN      15
#define DHTTYPE     DHT22
#define PIN_PRESSAO 34   // potenciômetro de pressão
#define PIN_OXIGENIO 35  // potenciômetro de oxigênio
#define LED_ALERTA   2
#define BUZZER       4

DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure espClient;
PubSubClient     client(espClient);

// ===========================
// WIFI
// ===========================
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

// ===========================
// MQTT RECONNECT
// ===========================
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

// ===========================
// SETUP
// ===========================
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

// ===========================
// LOOP
// ===========================
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // ------------------------------------------
  // TEMPERATURA — lida do DHT22
  // Se retornar NaN (comum no Wokwi), usa 23°C fixo
  // ------------------------------------------
  float temperatura = dht.readTemperature();
  if (isnan(temperatura)) {
    temperatura = 23.0;
  }

  // ------------------------------------------
  // PRESSÃO — potenciômetro no pino 34
  //
  // O ADC vai de 0 a 4095.
  // Dividimos em duas metades:
  //   0    a 2047 → faixa NOMINAL  (94 a 106 kPa)
  //   2048 a 4095 → faixa ANOMALIA (115 a 125 kPa)
  //
  // Esquerda do pot = 0    → nominal mínimo  (94 kPa)
  // Centro do pot   = 2047 → nominal máximo  (106 kPa)
  // Centro do pot   = 2048 → anomalia mínima (115 kPa)
  // Direita do pot  = 4095 → anomalia máxima (125 kPa)
  // ------------------------------------------
  int leituraPressao = analogRead(PIN_PRESSAO);
  float pressao;

  if (leituraPressao <= 2047) {
    // metade esquerda → valores nominais
    pressao = map(leituraPressao, 0, 2047, 94, 106);
  } else {
    // metade direita → valores de anomalia
    pressao = map(leituraPressao, 2048, 4095, 115, 125);
  }

  // ------------------------------------------
  // OXIGÊNIO — potenciômetro no pino 35
  //
  //   0    a 2047 → faixa NOMINAL  (19.5 a 22.5 %)
  //   2048 a 4095 → faixa ANOMALIA (14.0 a 17.0 %)
  //
  // Esquerda do pot = 0    → nominal mínimo  (19.5%)
  // Centro do pot   = 2047 → nominal máximo  (22.5%)
  // Centro do pot   = 2048 → anomalia mínima (14.0%)
  // Direita do pot  = 4095 → anomalia máxima (17.0%)
  // ------------------------------------------
  int leituraOxigenio = analogRead(PIN_OXIGENIO);
  float oxigenio;

  if (leituraOxigenio <= 2047) {
    // metade esquerda → valores nominais
    // map() trabalha com inteiros, então multiplicamos por 10
    // e depois dividimos para ter uma casa decimal
    oxigenio = map(leituraOxigenio, 0, 2047, 195, 225) / 10.0;
  } else {
    // metade direita → valores de anomalia
    oxigenio = map(leituraOxigenio, 2048, 4095, 140, 170) / 10.0;
  }

  // ------------------------------------------
  // VERIFICAÇÃO DE ALERTAS
  // ------------------------------------------
  bool alerta = false;

  if (temperatura < 18.0 || temperatura > 28.0) alerta = true;
  if (pressao     < 90.0 || pressao     > 110.0) alerta = true;
  if (oxigenio    < 19.0 || oxigenio    > 23.0)  alerta = true;

  // ------------------------------------------
  // ATUADORES
  // ------------------------------------------
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

  // ------------------------------------------
  // MONTA E PUBLICA PAYLOAD JSON
  // ------------------------------------------
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
