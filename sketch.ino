#include <ThingerESP32.h>
#include <WiFi.h>
#include <math.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define username "user"
#define thinger_id "id"
#define credentials "creden"
ThingerESP32 thing(username, thinger_id, credentials);
//Bot Telegram
#define BOT_TOKEN "your bot token "
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long lastTelegramCheck=0;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

String telegramID = "";

#define red 19
#define green 18
#define blue 5
#define buzzer 4
#define NTC_PIN 34
#define POT_PIN 35

#define TEMP_BUFFER 5
#define BPM_BUFFER 5

#define RED_LED setLED(255, 0, 0)
#define GREEN_LED setLED(0, 255, 0)
#define BLUE_LED setLED(0, 0, 255)
#define OFF_LED setLED(0, 0, 0)

const float BETA = 3950.0;

float tempBuffer[TEMP_BUFFER] = { 0 };
int bpmBuffer[BPM_BUFFER] = { 0 };

int tempCount = 0;
int bpmCount = 0;

float currentTemp = 0;
int currentBPM = 0;

int tempAlert = 0;
int bpmAlert = 0;
int lowTempTime = 0;
int lowBPMTime = 0;

void setLED(int r, int g, int b) {
  analogWrite(red, r);
  analogWrite(green, g);
  analogWrite(blue, b);
}

void addTemperature(float value) {
  for (int i = 0; i < TEMP_BUFFER - 1; i++)
    tempBuffer[i] = tempBuffer[i + 1];
  tempBuffer[TEMP_BUFFER - 1] = value;
  tempCount++;
}

float averageTemp() {
  int count = min(tempCount, TEMP_BUFFER);
  if (count == 0) return 0;
  float sum = 0;
  for (int i = 0; i < count; i++)
    sum += tempBuffer[i];
  return sum / count;
}

float tempTrend() {
  int count = min(tempCount, TEMP_BUFFER);
  if (count < 2) return 0;
  return tempBuffer[count - 1] - tempBuffer[0];
}

void addBPM(int value) {
  for (int i = 0; i < BPM_BUFFER - 1; i++)
    bpmBuffer[i] = bpmBuffer[i + 1];
  bpmBuffer[BPM_BUFFER - 1] = value;
  bpmCount++;
}

float averageBPM() {
  int count = min(bpmCount, BPM_BUFFER);
  if (count == 0) return 0;
  float sum = 0;
  for (int i = 0; i < count; i++)
    sum += bpmBuffer[i];
  return sum / count;
}

int bpmTrend() {
  int count = min(bpmCount, BPM_BUFFER);
  if (count < 2) return 0;
  return bpmBuffer[count - 1] - bpmBuffer[0];
}

float readTemperature() {
  int adc = analogRead(NTC_PIN);
  float value = adc * (1023.0 / 4095.0);
  return 1.0 / ((log(1.0 / (1023.0 / value - 1.0)) / BETA) + (1.0 / 298.15)) - 273.15;
}

int readBPM() {
  return map(analogRead(POT_PIN), 0, 4095, 40, 200);
}
void beep() {
  tone(buzzer, 262, 250);
  delay(500);
  tone(buzzer, 0, 230);
  delay(200);
}
String healthStatus() {

  if (currentTemp > 38.5 && currentBPM > 120)
    return "CRITICAL HEALTH";

  else if (currentTemp > 38.5)
    return "HIGH TEMPERATURE";

  else if (currentTemp < 35)
    return "LOW TEMPERATURE";

  else if (currentBPM > 135)
    return "HIGH HEART RATE";

  else if (currentBPM < 60)
    return "LOW HEART RATE";

  else
    return "NORMAL";
}
void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {

    bool result = bot.sendMessage(
      telegramID,
      message,
      ""
    );

    if (result)
      Serial.println("Message sent");
    else
      Serial.println("Send failed");

  } else {
    Serial.println("WiFi disconnected");
  }
}
void checkTelegram() {
  int messages = bot.getUpdates(bot.last_message_received + 1);

  while (messages) {
    for (int i = 0; i < messages; i++) {

      String text = bot.messages[i].text;
      String chat_id = bot.messages[i].chat_id;
      String msg = "";

      if (text == "/status") {

        msg = "🩺 *HEALIX HEALTH REPORT*\n\n";
        msg += "🌡 Temperature: ";
        msg += String(currentTemp, 1);
        msg += " °C\n";

        msg += "❤️ Heart Rate: ";
        msg += String(currentBPM);
        msg += " BPM\n\n";

        msg += "📌 Status: *";
        msg += healthStatus();
        msg += "*";

      }

      else if (text == "/temp") {

        msg = "🌡 *TEMPERATURE STATUS*\n\n";
        msg += "Temperature: ";
        msg += String(currentTemp, 1);
        msg += " °C";

      }

      else if (text == "/bpm") {

        msg = "❤️ *HEART RATE STATUS*\n\n";
        msg += "Heart Rate: ";
        msg += String(currentBPM);
        msg += " BPM";

      }

      else if (text == "/help") {

        msg = "🩺 *HEALIX COMMANDS*\n\n";
        msg += "📊 /status - Full health report\n";
        msg += "🌡 /temp - Temperature\n";
        msg += "❤️ /bpm - Heart rate";
      }

      if (msg != "")
        bot.sendMessage(chat_id, msg, "Markdown");
    }

    messages = bot.getUpdates(bot.last_message_received + 1);
  }
}
void setup() {
  Serial.begin(115200);
  client.setInsecure();

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println("Enter Telegram Chat ID:");

  while (!Serial.available()) {
    delay(100);
  }

  telegramID = Serial.readStringUntil('\n');
  telegramID.trim();
  Serial.print("ID: ");
  Serial.println(telegramID);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(buzzer, OUTPUT);

  thing["temp"] >> [](pson &out) {
    out = currentTemp;
  };

  thing["BPM"] >> [](pson &out) {
    out = currentBPM;
  };
  sendTelegramMessage("ESP32 connected successfully!");
}

void loop() {
  float temp = readTemperature();
  int bpm = readBPM();

  addTemperature(temp);
  addBPM(bpm);

  float avgTemp = averageTemp();
  float avgBPM = averageBPM();

  float tTrend = tempTrend();
  int bTrend = bpmTrend();

  currentTemp = avgTemp;
  currentBPM = avgBPM;

  Serial.print("TEMP:");
  Serial.print(avgTemp);
  Serial.print(" | BPM:");
  Serial.print(avgBPM);
  Serial.print(" | T Trend:");
  Serial.print(tTrend);
  Serial.print(" | BPM Trend:");
  Serial.println(bTrend);

  if (avgTemp > 38.5 && avgBPM > 120) {
    RED_LED;
    Serial.println("CRITICAL HEALTH!");
    beep();

    if (tempAlert == 0) {
      sendTelegramMessage(
      "🚨 *CRITICAL HEALTH ALERT* 🚨\n\n"
      "🌡 Temperature: "+String(avgTemp,1)+" °C\n"
      "❤️ Heart Rate: "+String(avgBPM)+" BPM\n\n"
      "⚠️ Both parameters are above normal.\n"
      "Please check immediately."
      );
      tempAlert = 1;
    }
  }
  else if (avgTemp > 38.5) {
    RED_LED;
    Serial.println("HIGH TEMP");
    if (tempAlert == 0) {
      for (int i = 0; i <= 20; i++)
        beep();
      tempAlert = 1;
    } else if (tempAlert == 1) {
      sendTelegramMessage(
      "🌡 *HIGH TEMPERATURE WARNING*\n\n"
      "Temperature: "+String(avgTemp,1)+" °C\n\n"
      "⚠️ Fever detected.\n"
      "Please follow up."
      );
      tempAlert = 2;
    }
  }
  else if (avgTemp < 35) {
    BLUE_LED;
    Serial.println("LOW TEMP");

    lowTempTime++;

    if (lowTempTime > 7) {
      sendTelegramMessage(
      "❄️ *LOW TEMPERATURE WARNING*\n\n"
      "Temperature: "+String(avgTemp,1)+" °C\n\n"
      "⚠️ Body temperature is below normal."
      );
      lowTempTime = 0;
    }
  }
  else if (avgBPM > 135) {
    RED_LED;
    Serial.println("HIGH BPM");

    if (bpmAlert == 0) {
      for (int i = 0; i <= 20; i++)
        beep();
      bpmAlert = 1;
    } else if (bpmAlert == 1) {
      sendTelegramMessage(
      "❤️ *HIGH HEART RATE WARNING*\n\n"
      "Heart Rate: "+String(avgBPM)+" BPM\n\n"
      "⚠️ Heart rate is higher than normal.\n"
      "Please follow up."
      );
      bpmAlert = 2;
    }
  }
  else if (avgBPM < 60) {
    BLUE_LED;
    Serial.println("LOW BPM");

    lowBPMTime++;

    if (lowBPMTime > 7) {
      sendTelegramMessage(
      "💙 *LOW HEART RATE WARNING*\n\n"
      "Heart Rate: "+String(avgBPM)+" BPM\n\n"
      "⚠️ Heart rate is below normal."
      );
      lowBPMTime = 0;
    }
  }
  else {
    GREEN_LED;
    tempAlert = 0;
    bpmAlert = 0;
  }
  if (avgTemp > 36.1 && tTrend > 0.3) {
    Serial.println("SMS: Possible Fever Soon");
  }
  if (avgBPM > 90 && bTrend > 5) {
    Serial.println("SMS: The heart rate will soon increase excessively.");
  }
  if(millis()-lastTelegramCheck>2000){
    checkTelegram();
    lastTelegramCheck=millis();
  }

  thing.handle();
  delay(1000);
}
