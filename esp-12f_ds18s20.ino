#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WiFi
const char* ssid =  "<YOUR_WIFI_SSID>";
const char* password =  "<YUOR_WIFI_PASSWORD>";

// Thingspeak
const char* server = "api.thingspeak.com";
String apiKey = "<YOUR_THINGSPEAK_API_KEY>";

// Telegram
String tServer = "https://api.telegram.org/";
String tApiKey = "<YOUR_TELEGRAM_BOT_API_KEY>";
String chatID = "<YUOR_CHAT_ID(Saved messages ID from web telegram)>";

float temp;
float alarmThreshold = 2.0f;  // Alarm threshold for telegram notification

#define Pin D1
#define LED 2
OneWire ourWire(Pin);
DallasTemperature sensors(&ourWire);
DeviceAddress Thermometer;
int deviceCount = 0;
#define TEMPERATURE_PRECISION 12

WiFiClient client;

void setup(void)
{
  Serial.begin(9600);
  delay(10);

  pinMode(D0, WAKEUP_PULLUP);
  delay(1000);
  pinMode(LED, OUTPUT);

  sensors.begin();
  sensors.setResolution(12);

  Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  Serial.println("Printing addresses...");
  for (int i = 0;  i < deviceCount;  i++)
  {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);
    printAddress(Thermometer);
  }

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop(void)
{
  sensors.begin();
  sensors.requestTemperatures();
  sensors.setResolution(12);
  temp = sensors.getTempCByIndex(0);
  Serial.println(temp);

  ThingSpeak(String(temp));

  delay(1000);
  if(temp < alarmThreshold){
    TelegramPrint(String(temp));
  }
  ESP.deepSleep(30e6);

  delay(30e6);
  client.stop();
  client.flush();
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}

void ThingSpeak(String message) {

  if (client.connect(server, 80))
  {
    String sendData = apiKey + "&field1=" + message + "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: " + String(server) + "\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(sendData.length());
    client.print("\n\n");
    client.print(sendData);
  }
}

void TelegramPrint(String message) {
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  Serial.println("T-Bot");
  if (https.begin(*client, tServer + tApiKey + "/")) {
    https.addHeader("Content-Type", "application/json");
    https.POST("{\"method\":\"sendMessage\",\"chat_id\":" + chatID + ",\"text\":\"" + message + "\"}");
    https.end();
    Serial.println("T-Bot send succesfull");
  }
}
