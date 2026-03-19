#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>


const char* ssid = "V-de-virus";
const char* password = "cerveja2025";

const char* mqtt_server = "18.210.29.178";


WiFiClient espClient;
PubSubClient client(espClient);


#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17

#define POT_PIN 34
#define LED_ALERTA 1   // LED de nível crítico

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int x = 50;
int y = 15;
int largura = 40;
int altura = 90;

// Função conectar WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando ao WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Função reconectar MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");

      // Se quiser receber comandos
      client.subscribe("controle");

    } else {
      Serial.print("Erro: ");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s...");
      delay(5000);
    }
  }
}

// Receber mensagens MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;

  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Mensagem recebida: ");
  Serial.println(mensagem);

  // Exemplo: ligar/desligar LED
  if (mensagem == "ON") {
    digitalWrite(1, HIGH);
  } else if (mensagem == "OFF") {
    digitalWrite(1, LOW);
  }
}


void desenharGota(int gx, int gy){
  
  // parte redonda
  tft.fillCircle(gx, gy+8, 8, ST77XX_CYAN);

  // ponta da gota
  tft.fillTriangle(
    gx-8, gy+6,
    gx+8, gy+6,
    gx, gy-10,
    ST77XX_CYAN
  );
}

void setup() {

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  SPI.begin(18, -1, 23, 5);

  tft.init(135, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  pinMode(LED_ALERTA, OUTPUT);
  digitalWrite(LED_ALERTA, LOW);

  // contorno do reservatório
  tft.drawRect(x, y, largura, altura, ST77XX_WHITE);

  // indicadores MIN e MAX
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(x + largura + 5, y - 2);
  tft.print("MAX");

  tft.setCursor(x + largura + 5, y + altura - 5);
  tft.print("MIN");

  // gota
  desenharGota(x - 20, y + altura/2);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int valorPot = analogRead(POT_PIN);

  int nivel = map(valorPot, 0, 4095, 0, altura);
  int porcentagem = map(valorPot, 0, 4095, 0, 100);

  // limpa interior da barra
  tft.fillRect(x+1, y+1, largura-2, altura-2, ST77XX_BLACK);

  // desenha nível
  tft.fillRect(x+1, y + altura - nivel, largura-2, nivel, ST77XX_BLUE);

  // limpa área de texto
  tft.fillRect(0, 120, 135, 100, ST77XX_BLACK);

  String texto;
  uint16_t cor;

  if (porcentagem <= 20) {

    texto = "CRITICO";
    cor = ST77XX_RED;

    digitalWrite(LED_ALERTA, HIGH);   // LIGA LED

  }
  else if (porcentagem < 80) {

    texto = "BOM";
    cor = ST77XX_BLUE;

  }
  else {

    texto = "CHEIO";
    cor = ST77XX_GREEN;

    digitalWrite(LED_ALERTA, LOW);    // DESLIGA LED

  }

  // texto estado
  tft.setTextSize(2);
  tft.setTextColor(cor);
  tft.setCursor(20, 125);
  tft.print(texto);

  // porcentagem
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(35, 170);
  tft.print(porcentagem);
  tft.print("%");



  // Converter para %
  //int nivel = map(valor, 0, 4095, 0, 100);

  Serial.print("Nivel: ");
  Serial.println(nivel);

  Serial.begin(115200);
  Serial.print(client.state());

  // Publicar no MQTT
String msg = String(porcentagem);
client.publish("sensor/nivel", msg.c_str());

  delay(2000);
}