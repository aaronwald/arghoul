#include <Arduino.h>
#include <FastLED.h>
#include "secrets.hpp"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

// 666 = 45 (15 x 3 )
// ARGHOUL = 97?
// ROAD = 59
#define LED_PIN D2
#define NUM_LEDS 201 // 666 + ARGHOUL + ROAD ==> 201 leds
#define BRIGHTNESS 32
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// websocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setupWifi()
{
  Serial.begin(115200);
  Serial.println();
  // Serial.setDebugOutput(true);

  WiFi.begin(SSID_NAME, SSID_KEY);

  Serial.printf("Connecting to %s", SSID_NAME);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void orangePumpkinPalette()
{
  CRGB orange = CHSV(HUE_ORANGE, 255, 255);
  CRGB pumpkin = CHSV(24, 91, 100);

  currentPalette = CRGBPalette16(orange, orange, orange, orange,
                                 pumpkin, pumpkin, pumpkin, pumpkin,
                                 orange, orange, orange, orange,
                                 pumpkin, pumpkin, pumpkin, pumpkin);
}

void purplePumpkinPalette()
{
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB pumpkin = CHSV(24, 91, 100);

  currentPalette = CRGBPalette16(purple, purple, purple, purple,
                                 pumpkin, pumpkin, pumpkin, pumpkin,
                                 purple, purple, purple, purple,
                                 pumpkin, pumpkin, pumpkin, pumpkin);
}

void setupFastLED()
{
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  currentBlending = LINEARBLEND;

  orangePumpkinPalette();
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    // client disconnected
    os_printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    // error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    // pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    // data packet
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      // the whole message is in a single frame and we got all of it's data
      os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
      if (info->opcode == WS_TEXT)
      {
        data[len] = 0;
        os_printf("%s\n", (char *)data);
      }
      else
      {
        for (size_t i = 0; i < info->len; i++)
        {
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }
      if (info->opcode == WS_TEXT)
      {
        client->text("I got your text message");
        static int i = 0;
        ++i;
        if (i % 2)
        {
          orangePumpkinPalette();
        }
        else
        {
          purplePumpkinPalette();
        }
      }
      else
      {
        client->binary("I got your binary message");
      }
    }
    else
    {
      // message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
      if (info->message_opcode == WS_TEXT)
      {
        data[len] = 0;
        os_printf("%s\n", (char *)data);
      }
      else
      {
        for (size_t i = 0; i < len; i++)
        {
          os_printf("%02x ", data[i]);
        }
        os_printf("\n");
      }

      if ((info->index + len) == info->len)
      {
        os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
void setup()
{
  delay(3000); // power-up safety delay
  setupFastLED();
  FillLEDsFromPaletteColors(0);
  setupWifi();

  // websocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // HTTP Paths
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Arghoul, world"); });
  server.on("/purple", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Purple."); purplePumpkinPalette(); });
  server.on("/orange", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Purple."); orangePumpkinPalette(); });
  server.onNotFound(notFound);

  server.begin();
}

void loop()
{
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}
