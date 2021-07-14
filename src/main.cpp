#include "WiFi.h"
#include "config.h"
#include "time.h"
#include <Arduino.h>

#define uS_TO_S_FACTOR 1000000
#define INTERVAL_SECONDS 30

struct Reading
{
  float value;
  unsigned long timestamp;
};

RTC_DATA_ATTR unsigned long timestamp = 0;
RTC_DATA_ATTR int8_t i = 0;
RTC_DATA_ATTR int8_t toSend = 0;
RTC_DATA_ATTR Reading readings[20];

unsigned long getTimestamp()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return (0);
  }
  time(&now);
  return now;
}

void setup_wifi()
{
  int n = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
  }

  Serial.println("");
  Serial.print("WiFi connected in ");
  Serial.print(millis() - n);
  Serial.print(" ms\n");
}

void setup_sensor()
{
  pinMode(4, OUTPUT);
  digitalWrite(4, 1);
  pinMode(2, INPUT);
}

int read_sensor()
{
  return analogRead(2);
}

void setup()
{
  Serial.begin(9600);

  if (timestamp == 0)
  {
    setup_wifi();
    configTime(0, 0, "pool.ntp.org");
    timestamp = getTimestamp();
    // doesn't work that reliably, better to soft reboot
    // with deep sleep
    // WiFi.disconnect(true);
    timestamp += INTERVAL_SECONDS;
    esp_sleep_enable_timer_wakeup(INTERVAL_SECONDS * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }
  else
  {
    setup_sensor();

    Reading r;
    r.timestamp = timestamp;
    r.value = read_sensor();

    readings[i] = r;
    Serial.print("Reading: " + String(r.value) + ", timestamp: " + String(r.timestamp) + "\n");
    ++i;
    if (toSend < 20)
      ++toSend;
  }
  bool gotTimestamp = false;
  if (i == 5 || i == 10 || i == 15 || i == 20)
  {
    setup_wifi();

    WiFiClient client;
    if (client.connect(host, port))
    {
      String request;
      request += "POST /readings HTTP/1.1\r\n";
      request += "Host: " + String(host) + "\r\n";
      request += "Connection: close\r\n";
      String body;
      body.reserve(25 * toSend + 80);
      body = "{\"deviceName\":\"" + String(deviceName) + "\",\"schoolName\":\"" + schoolName + "\",\"values\":[";
      for (int j = 0; j < toSend; j++)
      {
        body += "{\"value\":" + String(readings[j].value) + ",\"timestamp\":" + String(readings[j].timestamp) + "}";
        if (j != toSend - 1)
          body += ",";
      }
      body += "]}";
      request += "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.114 Safari/537.36\r\n";
      request += "Content-Length: " + String(body.length()) + "\r\n";
      request += "Content-Type: application/json\r\n";
      request += "\r\n";

      request += body;
      // Serial.println(request);
      client.println(request);

      Serial.print("Sent " + String(toSend) + " readings successfully\n");
      toSend = 0;
      i = 0;
    }
    client.stop();

    if (i == 20)
      i = 0;
    configTime(0, 0, "pool.ntp.org");
    // int tTimestamp = timestamp + round(millis() / 1000.0);
    timestamp = getTimestamp();
    gotTimestamp = true;
    // Serial.print("Error is: " + String(int(tTimestamp - timestamp)));
    // Serial.print("\nI thought timestamp is " + String(tTimestamp) + "\n");
    // Serial.print("Timestamp is " + String(timestamp) + "\n");
  }

  {
    int timeToSleep = INTERVAL_SECONDS * uS_TO_S_FACTOR - millis() * 1000;
    if (gotTimestamp)
      timestamp += round(timeToSleep / 1000000.0);
    else
      timestamp += INTERVAL_SECONDS;
    esp_sleep_enable_timer_wakeup(timeToSleep);
    esp_deep_sleep_start();
  }
}

void loop()
{
}