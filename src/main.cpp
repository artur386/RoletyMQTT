
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
// #include <UIPEthernet.h>
#include <SPI.h>
#include <Ethernet3.h>    // instead Ethernet.h
#include <EthernetUdp3.h> // instead EthernetUdp.h for UDP functionality
#include <PubSubClient.h>
#include <EEPROM.h>
// #include <EtherCard.h>

#define CLIENT_ID "NANO"
#define CLIENT_USER "HAmQTT"
#define CLIENT_PASS "barbapapa1"
#define PORT_MQTT 1883
#define PUBLISH_DELAY 2000

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress nanoIP(192, 168, 1, 21);
IPAddress mqttServer(192, 168, 1, 19);

enum MANUAL_SWITCH
{
  MANUAL_NOPE,
  MANUAL_UP,
  MANUAL_DOWN,
  MANUAL_OFF,
  MANUAL_IN_PROGRESS
};

enum CMD
{
  UP_CMD,
  DOWN_CMD,
  STOP_CMD
};
// #define STATIC 1

// char const page[] PROGMEM =

// bool ledState = true;
// unsigned long lastLedTime = 0;
// #define blinkTime 1000
#define TIME_TO_NEXT_RL 300
#define BREAK_TIME_BETWEEN_NEXT_RL 1100

#define MAX_RELAY_ON_TIME 1
#define TOTAL_WINDOW_NUMB 10
#define TOTAL_UNCONNECT 10
#define RECONNECT_TIME 2000
#define RESET_PIN 4

uint8_t StartupShutter;
uint8_t reconnectNumb = 0;
byte saveCMD[TOTAL_WINDOW_NUMB] = {
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD,
    STOP_CMD};

#define INIT_EEPROM_VAR 1

bool lastPower = false;
bool bAllAutoUp = false;
bool bAllAutoDown = false;
uint8_t inProgressNow = 0;
unsigned long LastSwitchOnTime;
unsigned long LastOnlinePublish;
unsigned long LastEEpromWriteTime;
int lastSwitchedRelay = 0;

Adafruit_MCP23017 mcp1; // PIERWSZY MCP23017
Adafruit_MCP23017 mcp2; // DRUGI MCP23017

// uint8_t menuChoice = 0;

//   *************************************************
//  *  PROTOTYPES
// *************************************************

// void (*resetFunc)(void) = 0; //declare reset function @ address 0
void resetFunc(void);
void callback(char *topic, byte *payload, unsigned int length);
bool reconnect();
String getValue(String data, char separator, int index);
void checkManualState();
void Avaible();
// void menub(int tt);
// void menu();
// void goToUp(uint8_t _roleta);
// void goToDown(uint8_t _roleta);
// void rialEvent();
// void screen(uint8_t);
void chckBT_event();
void allForceStop(); //all stop after manual release        -state 0

uint8_t manualState = 0, LastManualState = 99;
unsigned long lastReconnectAttempt = 0;

#define BTN_UP 2
#define BTN_DOWN 3

PinButton bt_up(BTN_UP);
PinButton bt_down(BTN_DOWN);

EthernetClient ethClient;
PubSubClient client(ethClient);

Roleta roleta[TOTAL_WINDOW_NUMB] = {
    Roleta(0, &client, &mcp1, 9, 1, 22000, 16000),   //    SYPIALNIA_PARTER_1"
    Roleta(1, &client, &mcp1, 10, 2, 22000, 16000),  //    SYPIALNIA_PARTER_2"
    Roleta(2, &client, &mcp2, 5, 10, 22000, 16000),  //    LAZIENKA"
    Roleta(3, &client, &mcp1, 14, 6, 22000, 16000),  //    KUCHNIA_1"
    Roleta(4, &client, &mcp1, 15, 7, 22000, 16000),  //    KUCHNIA_2"
    Roleta(5, &client, &mcp1, 12, 4, 22000, 16000),  //    SALON_FRONT"
    Roleta(6, &client, &mcp1, 13, 5, 22000, 16000),  //    SALON_BALKON"
    Roleta(7, &client, &mcp1, 11, 3, 22000, 16000),  //    WSCHOD_PIETRO"
    Roleta(8, &client, &mcp2, 6, 9, 22000, 16000),   //    SYPIALNIA_PIETRO"
    Roleta(9, &client, &mcp2, 4, 11, 22000, 16000)}; //    ZACHOD"
                                                     // Roleta(10, &client, &mcp2, 0, 15, 22000, 16000)}; //    WC "

void GetEEpromData()
{
  byte flag = EEPROM.read(0);

  if (!flag)
  {
    EEPROM.get(1, saveCMD);
  }
  else
  {
    EEPROM.write(0, 1);
  }
}

void PutEEpromData(uint16_t lTime)
{
  if (millis() - LastEEpromWriteTime > lTime)
  {
    EEPROM.put(1, saveCMD);
  }
}

boolean reconnect()
{
  if (client.connect(CLIENT_ID, CLIENT_USER, CLIENT_PASS))
  {
    StartupShutter = TOTAL_WINDOW_NUMB + 1;
    for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
    {
      char *command_topic = (char *)malloc(sizeof(char) * 14);
      snprintf_P(command_topic, 14, PSTR("roleta/%i/cmd"), i);
      client.subscribe(command_topic, 1);
      Serial.print("Subscribe: ");
      Serial.println(command_topic);
      free(command_topic);
    }
    char *command_topic = (char *)malloc(sizeof(char) * 20);
    snprintf_P(command_topic, 20, PSTR("roleta/all/cmd"));
    client.subscribe(command_topic, 1);
    Serial.print("Subscribe: ");
    Serial.println(command_topic);
    free(command_topic);
  }
  return client.connected();
}

void resetFunc()
{
  PutEEpromData(60000);
  digitalWrite(RESET_PIN, LOW);
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

  if (StartupShutter == 0)
  {
    // Serial.println(length);
    // Serial.println((byte)payload[length - 1]);

    char *ch_payload = (char *)calloc(length + 1, sizeof(char));

    memcpy(ch_payload, payload, length);

    // ch_payload[strlen(ch_payload) - 1] = 0x00;
    Serial.println(topic);
    Serial.println(ch_payload);

    // Serial.println(strcmp(ch_payload, "OPEN"));
    // Serial.println((byte)ch_payload[4]);
    // Serial.println(topic);

    if (strcmp_P(ch_payload, PSTR("OPEN")) == 0)
    {
      if (all)
      {
        if (manualState == MANUAL_NOPE)
        {
          manualState = MANUAL_UP;
        }
      }
      else
      {
        roleta[roleta_nb].MoveUP();
      }
    }
    else if (strcmp_P(ch_payload, PSTR("CLOSE")) == 0)
    {
      if (all)
      {
        if (manualState == MANUAL_NOPE)
        {
          manualState = MANUAL_DOWN;
        }
      }
      else
      {
        roleta[roleta_nb].MoveDown();
      }
    }
    else if (strcmp_P(ch_payload, PSTR("STOP")) == 0)
    {
      if (all)
      {
        manualState = MANUAL_OFF;
      }
      else
      {
        roleta[roleta_nb].MoveStop();
      }
    }

    free(ch_payload);
  }
  else
  {
    StartupShutter--;
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
  Serial.begin(9600);

  client.setServer(mqttServer, PORT_MQTT);
  client.setCallback(callback);

  Serial.println(F("Connect to network"));
  Ethernet.init(10);
  Ethernet.begin(mac, nanoIP);
  Serial.println(Ethernet.localIP());
  // delay(800);

  mcp1.begin();  // MCP - brak adres 0x20
  mcp2.begin(1); // MCP - brak adres 0x21

  //ustawienie mcp
  for (byte j = 0; j <= 15; j++)
  {
    mcp1.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp1.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
    mcp2.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp2.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
  }
  lastReconnectAttempt = 0;
  LastOnlinePublish = 0;
}

void switchPower(bool onOff)
{
  if (lastPower != !onOff)
  {
    lastPower = !onOff;
    mcp2.digitalWrite(14, lastPower);
  }
}

void roletyLOOP()
{
  inProgressNow = 0;
  for (int i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    if (roleta[i].Loop())
    {
      inProgressNow++;
    }
  }
  switchPower((bool)inProgressNow);
}

void chckBT_event()
{
  if ((manualState != MANUAL_NOPE) && (bt_up.isSingleClick() || bt_down.isSingleClick()))
  {
    manualState = MANUAL_OFF;
  }
  else if (manualState == MANUAL_NOPE)
  {
    if (bt_up.isSingleClick())
    {
      manualState = MANUAL_UP;
    }
    else if (bt_down.isSingleClick())
    {
      manualState = MANUAL_DOWN;
    }
  }
}

void checkManualState()
{
  switch (manualState)
  {
  case MANUAL_UP:
    // Serial.println(F("MANUAL_UP"));
    // if (inProgressNow <= MAX_RELAY_ON_TIME)
    // {
    if (millis() - LastSwitchOnTime > BREAK_TIME_BETWEEN_NEXT_RL)
    {
      roleta[lastSwitchedRelay].MoveUP();
      LastSwitchOnTime = millis();
      lastSwitchedRelay++;
    }
    if (lastSwitchedRelay == TOTAL_WINDOW_NUMB)
    {
      manualState = MANUAL_IN_PROGRESS;
    }
    break;

  case MANUAL_DOWN:
    // Serial.println(F("MANUAL_DOWN"));
    // if (inProgressNow <= MAX_RELAY_ON_TIME)
    // {
    if (millis() - LastSwitchOnTime > BREAK_TIME_BETWEEN_NEXT_RL)
    {
      roleta[lastSwitchedRelay].MoveDown();
      LastSwitchOnTime = millis();
      lastSwitchedRelay++;
    }
    if (lastSwitchedRelay == TOTAL_WINDOW_NUMB)
    {
      manualState = MANUAL_IN_PROGRESS;
    }
    break;

  case MANUAL_OFF:
    Serial.println(F("MANUAL_OFF"));
    for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
    {
      roleta[i].MoveStop();
    }
    manualState = MANUAL_IN_PROGRESS;
    break;

  case MANUAL_IN_PROGRESS:
    Serial.println(F("MANUAL_IN_PROGRESS"));
    if (inProgressNow == 0)
    {
      manualState = MANUAL_NOPE;
    }
    lastSwitchedRelay = 0;
    break;

  case MANUAL_NOPE:
    Serial.println(F("MANUAL_NOPE"));
    lastSwitchedRelay = 0;
    break;

  default:
    break;
  }
}

void loop()
{
  Serial.print(F("inProgressNow: "));
  Serial.println(inProgressNow);
  Serial.println();
  Serial.print(F("lastSwitchedRelay: "));
  Serial.println(lastSwitchedRelay);
  Serial.println();
  // delay(250);
  if (!client.connected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > RECONNECT_TIME)
    {
      Serial.println(F("not Connected"));
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        Serial.println(F("Connected"));
        lastReconnectAttempt = 0;
        reconnectNumb = 0;
      }
      else
      {
        reconnectNumb++;
      }
      if (reconnectNumb == TOTAL_UNCONNECT)
      {
        Serial.println(F("REBOOT"));
        resetFunc();
      }
    }
  }
  else
  {
    // Client connected
    Avaible();
    client.loop();
  }

  bt_down.update();
  bt_up.update();
  chckBT_event();
  roletyLOOP();
  checkManualState();
}
