
/*
SYPIALNIA_PARTER_1 0
SYPIALNIA_PARTER_2 1
WSCHOD_PIETRO 2
SALON_FRONT 3
SALON_BALKON 4
KUCHNIA_1 5
KUCHNIA_2 6
SYPIALNIA_PIETRO 7
LAZIENKA 8
ZACHOD 9
WC 10
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include "Roleta.h"
#include "PinButton.h"
#include <SPI.h>
#include <Ethernet3.h>    // instead Ethernet.h
#include <EthernetUdp3.h> // instead EthernetUdp.h for UDP functionality
#include <PubSubClient.h>
#include <avr/wdt.h>

#define CLIENT_ID "NANO"
#define CLIENT_USER "HAmQTT"
#define CLIENT_PASS "barbapapa1"
#define PORT_MQTT 1883
#define PUBLISH_DELAY 2000

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress nanoIP(192, 168, 1, 21);
IPAddress mqttServer(192, 168, 1, 19);

#define BREAK_TIME_BETWEEN_NEXT_SHUTTER 600
#define MAX_RELAY_ON_TIME 10
#define TOTAL_WINDOW_NUMB 10
#define RECONNECT_TIME 2000
#define RECONNECT_TRY_NUMB 10

uint8_t StartupShutter;
uint8_t reconnectNumb = 0;

bool lastPower = false;
unsigned long LastSwitchOnTime;
unsigned long LastOnlinePublish;

Adafruit_MCP23017 mcp1; // PIERWSZY MCP23017
Adafruit_MCP23017 mcp2; // DRUGI MCP23017

//   *************************************************
//  *  PROTOTYPES
// *************************************************

void callback(char *topic, byte *payload, unsigned int length);
bool reconnect();
String getValue(String data, char separator, int index);
void Avaible();
void SetAllShutter(byte CMDd);
byte ShutterInProgress(void);
void switchPower(void);

void chckBT_event();

unsigned long lastReconnectAttempt = 0;

#define BTN_UP 2
#define BTN_DOWN 3

PinButton bt_up(BTN_UP);
PinButton bt_down(BTN_DOWN);

EthernetClient ethClient;
PubSubClient client(ethClient);

Roleta roleta[TOTAL_WINDOW_NUMB] = {
    Roleta(0, &client, &mcp1, 9, 1, 25000, 22000),   //    SYPIALNIA_PARTER_1"
    Roleta(1, &client, &mcp1, 10, 2, 25000, 22000),  //    SYPIALNIA_PARTER_2"
    Roleta(2, &client, &mcp2, 5, 10, 25000, 22000),  //    LAZIENKA"
    Roleta(3, &client, &mcp1, 14, 6, 25000, 22000),  //    KUCHNIA_1"
    Roleta(4, &client, &mcp1, 15, 7, 25000, 22000),  //    KUCHNIA_2"
    Roleta(5, &client, &mcp2, 3, 12, 30000, 27000),  //    SALON_FRONT"
    Roleta(6, &client, &mcp1, 13, 5, 25000, 22000),  //    SALON_BALKON"
    Roleta(7, &client, &mcp1, 11, 3, 25000, 22000),  //    WSCHOD_PIETRO"
    Roleta(8, &client, &mcp2, 6, 9, 25000, 22000),   //    SYPIALNIA_PIETRO"
    Roleta(9, &client, &mcp2, 4, 11, 25000, 22000)}; //    ZACHOD"
                                                     // Roleta(10, &client, &mcp2, 0, 15, 22000, 16000)}; //    WC "

boolean reconnect()
{
  if (client.connect(CLIENT_ID, CLIENT_USER, CLIENT_PASS))
  {
    for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
    {
      char *command_topic = (char *)malloc(sizeof(char) * 14);
      snprintf_P(command_topic, 14, PSTR("roleta/%i/cmd"), i);
      client.subscribe(command_topic, 1);
      free(command_topic);
    }
    char *command_topic = (char *)malloc(sizeof(char) * 20);
    snprintf_P(command_topic, 20, PSTR("roleta/all/cmd"));
    client.subscribe(command_topic, 1);
    free(command_topic);
  }
  return client.connected();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void callback(char *topic, byte *payload, unsigned int length)
{
  wdt_reset();
  bool all;
  uint8_t roleta_nb;
  String topicMsg = getValue(topic, '/', 1);
  if (topicMsg.equals("all"))
  {
    all = true;
  }
  else
  {
    all = false;
    roleta_nb = topicMsg.toInt();
  }

  char *ch_payload = (char *)calloc(length + 1, sizeof(char));

  memcpy(ch_payload, payload, length);

  if (strcmp_P(ch_payload, PSTR("OPEN")) == 0)
  {
    if (all)
    {
      SetAllShutter(CMD_OPEN);
    }
    else
    {
      roleta[roleta_nb].SetUP();
    }
  }
  else if (strcmp_P(ch_payload, PSTR("CLOSE")) == 0)
  {
    if (all)
    {
      SetAllShutter(CMD_CLOSE);
    }
    else
    {
      roleta[roleta_nb].SetDOWN();
    }
  }
  else if (strcmp_P(ch_payload, PSTR("STOP")) == 0)
  {
    if (all)
    {
      SetAllShutter(CMD_STOP);
    }
    else
    {
      roleta[roleta_nb].SetHALT();
    }
  }

  free(ch_payload);
}

void SetAllShutter(byte CMDd)
{
  for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    switch (CMDd)
    {
    case CMD_OPEN:
      roleta[i].SetUP();
      break;

    case CMD_CLOSE:
      roleta[i].SetDOWN();
      break;

    case CMD_STOP:
      roleta[i].SetHALT();
      roleta[i].Trigger();
      break;

    default:
      break;
    }
  }
}

void Avaible()
{
  if (client.connected())
  {
    if (millis() - LastOnlinePublish > 60000 || LastOnlinePublish == 0)
    {
      LastOnlinePublish = millis();

      for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
      {
        char *availability_topic = (char *)malloc(sizeof(char) * 23);
        snprintf_P(availability_topic, 23, PSTR("roleta/%i/availability"), i);
        client.publish(availability_topic, "online");
        free(availability_topic);
      }
      char *availability_topic = (char *)malloc(sizeof(char) * 25);
      snprintf_P(availability_topic, 25, PSTR("roleta/all/availability"));
      client.publish(availability_topic, "online");
      free(availability_topic);
    }
  }
}

void setup()
{
  wdt_enable(WDTO_8S);
  Serial.begin(9600);

  //ustawienie mcp
  mcp1.begin();  // MCP - brak adres 0x20
  mcp2.begin(1); // MCP - brak adres 0x21

  for (byte j = 0; j <= 15; j++)
  {
    mcp1.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp1.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
    mcp2.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp2.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
  }
  client.setServer(mqttServer, PORT_MQTT);
  client.setCallback(callback);

  Ethernet.init(10);
  Ethernet.begin(mac, nanoIP);

  lastReconnectAttempt = 0;
  LastOnlinePublish = 0;
  wdt_reset();
}

void switchPower(void)
{
  bool onOff = !!ShutterInProgress();
  if (lastPower != onOff)
  {
    lastPower = onOff;
    mcp2.digitalWrite(14, !lastPower);
  }
}

byte ShutterInProgress(void)
{
  byte inProgress = 0;
  for (int i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    if (roleta[i].MoveIsOn)
    {
      inProgress++;
    }
  }
  return inProgress;
}

void roletyLOOP()
{
  for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    roleta[i].Loop();
    if (roleta[i].cmdChange)
    {
      if (ShutterInProgress() < MAX_RELAY_ON_TIME)
      {
        if (millis() - LastSwitchOnTime >= BREAK_TIME_BETWEEN_NEXT_SHUTTER)
        {
          roleta[i].Trigger();
          roleta[i].Loop();
          LastSwitchOnTime = millis();
        }
        else
        {
          break;
        }
      }
    }
    switchPower();
  }
}

void chckBT_event()
{
  bt_down.update();
  bt_up.update();
  if (!!ShutterInProgress())
  {
    if (bt_up.isSingleClick() || bt_down.isSingleClick())
    {
      SetAllShutter(CMD_STOP);
    }
  }
  else
  {
    if (bt_up.isSingleClick())
    {
      SetAllShutter(CMD_OPEN);
    }
    else if (bt_down.isSingleClick())
    {
      SetAllShutter(CMD_CLOSE);
    }
  }
}

void loop()
{

  if (!client.connected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > RECONNECT_TIME)
    {
      // Serial.println(F("not Connected"));
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        // Serial.println(F("Connected"));
        wdt_reset();
        reconnectNumb = 0;
      }
      else
      {
        wdt_reset();
        reconnectNumb++;
      }
      if (reconnectNumb == RECONNECT_TRY_NUMB)
      {
        delay(10000);
      }
    }
  }
  else
  {
    // Client connected
    wdt_reset();
    Avaible();
    client.loop();
  }

  chckBT_event();
  roletyLOOP();
  wdt_reset();
}
