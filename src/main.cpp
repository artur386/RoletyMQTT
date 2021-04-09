
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
#include <MemoryFree.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include "Roleta.h"
#include "PinButton.h"
#include <PubSubClient.h>

// #include <EtherCard.h>
#define STATIC 1

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress ip(172, 16, 0, 100);
IPAddress server(172, 16, 0, 2);
// char const page[] PROGMEM =

bool ledState = true;
unsigned long lastLedTime = 0;
unsigned long blinkTime = 1000;
unsigned long lastrlTime = 0;
unsigned long timeToNextRL = 200;
uint16_t BreakTimeBeetwen = 800;

#define MAX_RELAY_ON_TIME 4
#define TOTAL_WINDOW_NUMB 9

bool lastPower = true;
bool bAllAutoUp = false;
bool bAllAutoDown = false;
uint8_t inProgressNow = 0;
unsigned long LastSwitchOnTime;
int lastSwitchedRelay = -1;

#define PIN_00 0
#define PIN_01 1
#define PIN_04 4
#define PIN_05 5
#define PIN_06 6
#define PIN_07 7
#define PIN_08 8
#define PIN_09 9
#define PIN_10 10
#define PIN_11 11
#define PIN_12 12
#define PIN_13 A0
#define PIN_14 A1
#define PIN_15 A2
#define PIN_16 A3
#define PIN_17 A6
#define PIN_18 A7

Adafruit_MCP23017 mcp1; // PIERWSZY MCP23017
Adafruit_MCP23017 mcp2; // DRUGI MCP23017
Roleta roleta[11] = {
    Roleta(1, &mcp1, 9, 1),    //    SYPIALNIA_PARTER_1"
    Roleta(2, &mcp1, 10, 2),   //    SYPIALNIA_PARTER_2"
    Roleta(3, &mcp2, 5, 10),   //    LAZIENKA"
    Roleta(4, &mcp1, 14, 6),   //    KUCHNIA_1"
    Roleta(5, &mcp1, 15, 7),   //    KUCHNIA_2"
    Roleta(6, &mcp1, 12, 4),   //    SALON_FRONT"
    Roleta(7, &mcp1, 13, 5),   //    SALON_BALKON"
    Roleta(8, &mcp1, 11, 3),   //    WSCHOD_PIETRO"
    Roleta(9, &mcp2, 6, 9),    //    SYPIALNIA_PIETRO"
    Roleta(10, &mcp2, 4, 11),  //    ZACHOD"
    Roleta(11, &mcp2, 0, 15)}; //    WC "

uint8_t menuChoice = 0;

//   *************************************************
//  *  PROTOTYPES
// *************************************************

void callback(char *topic, byte *payload, unsigned int length);
void menub(int tt);
void menu();
void goToUp(uint8_t _roleta);
void goToDown(uint8_t _roleta);
void rialEvent();
// void screen(uint8_t);
void chckBT_event();
void blinkLed();
void allForceStop();  //all stop after manual release        -state 0
void allAutoUp();     //single click                        -state 1
void allAutoDown();   //single click                        -state 2
void allHalfUp();     //double click                        -state 3
void allHalfDown();   //double click                        -state 4
void allManualUp();   //long click                          -state 5
void allManualDown(); //long click                          -state 6
void checkHandle();

uint8_t handleState = 0;
uint8_t lastHandleState = 99;
#define clearHandleState() handleState &= 0x00

#define BTN_UP 2
#define BTN_DOWN 3

PinButton bt_up(BTN_UP);
PinButton bt_down(BTN_DOWN);

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;

boolean reconnect()
{
  if (client.connect("arduinoClient"))
  {
    // Once connected, publish an announcement...
    client.publish("outTopic", "hello world");
    // ... and resubscribe
    client.subscribe("inTopic");
  }
  return client.connected();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void blinkLed()
{
  if (millis() - lastLedTime > blinkTime)
  {
    ledState = !ledState;
    digitalWrite(13, ledState);
    lastLedTime = millis();
    // Serial.print("freeMemory()=");
    // Serial.println(freeMemory());
  }
}
void setup()
{
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  delay(1500);

  //   Serial.println("\n[backSoon]");
  //   if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
  //     Serial.println("Failed to access Ethernet controller");
  // #if STATIC
  //   ether.staticSetup(myip, gwip);
  // #else
  //   if (!ether.dhcpSetup())
  //     Serial.println("DHCP failed");
  // #endif
  //   ether.printIp("IP:  ", ether.myip);
  //   ether.printIp("GW:  ", ether.gwip);
  //   ether.printIp("DNS: ", ether.dnsip);

  delay(1000);

  pinMode(PIN_00, INPUT_PULLUP);
  pinMode(PIN_01, INPUT_PULLUP);
  pinMode(PIN_04, INPUT_PULLUP);
  pinMode(PIN_05, INPUT_PULLUP);
  pinMode(PIN_06, INPUT_PULLUP);
  pinMode(PIN_07, INPUT_PULLUP);
  pinMode(PIN_08, INPUT_PULLUP);
  pinMode(PIN_09, INPUT_PULLUP);
  pinMode(PIN_10, INPUT_PULLUP);
  pinMode(PIN_11, INPUT_PULLUP);
  pinMode(PIN_12, INPUT_PULLUP);
  pinMode(PIN_13, INPUT_PULLUP);
  pinMode(PIN_14, INPUT_PULLUP);
  pinMode(PIN_15, INPUT_PULLUP);
  pinMode(PIN_16, INPUT_PULLUP);
  pinMode(PIN_17, INPUT_PULLUP);
  pinMode(PIN_18, INPUT_PULLUP);

  // screen(1);
  mcp1.begin();  // MCP - brak adres 0x20
  mcp2.begin(1); // MCP - brak adres 0x21

  //aby nie wpisywać 16 razy dla każdego wyjścia mcp.pinMode(0, OUTPUT) dodamy pętle
  for (byte j = 0; j <= 15; j++)
  {
    mcp1.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp1.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
    mcp2.pinMode(j, OUTPUT);    // ustaw pin jako wyjście
    mcp2.digitalWrite(j, HIGH); // ustaw pin na stan wysoki
  }
}

// void screen(uint8_t side)
// {
//   for (size_t i = 0; i < 10; i++)
//   {
//     Serial.println();
//   }

//   if (side == 1)
//   {
//     Serial.println("Wybierz kierunek:");
//     Serial.println("[1] - góra");
//     Serial.println("[2] - dół");
//   }
//   else if (side == 2)
//   {
//     Serial.println("Wybierz okno:");
//     Serial.println(" [0] - SYPIALNIA_PARTER_1");
//     Serial.println(" [1] - SYPIALNIA_PARTER_2");
//     Serial.println(" [2] - WSCHOD_PIETRO");
//     Serial.println(" [3] - SALON_FRONT");
//     Serial.println(" [4] - SALON_BALKON");
//     Serial.println(" [5] - KUCHNIA_1");
//     Serial.println(" [6] - KUCHNIA_2");
//     Serial.println(" [7] - SYPIALNIA_PIETRO");
//     Serial.println(" [8] - LAZIENKA");
//     Serial.println(" [9] - ZACHOD");
//     Serial.println(" [10] - WC ");
//   }
// }

// void menu(int input)
// {
//   if (!(bool)menuChoice)
//   {
//     if (input == 1 || input == 2)
//     {
//       menuChoice = input;
//       screen(2);
//     }
//   }
//   else
//   {
//     if (input > 0 && input < 11)
//     {
//       if (menuChoice == 1)
//       {
//         roleta[input].AutoUp();
//       }
//       else if (menuChoice == 2)
//       {
//         roleta[input].AutoDown();
//       }
//     }
//     menuChoice = 0;
//     screen(1);
//   }
// }
// void serialEvent()
// {
//   if (Serial.available())
//   {
//     String s = Serial.readString();
//     int i = s.toInt();
//     menu(i);
//   }
// }
void switchPower(bool onOff)
{
  if (lastPower != !onOff)
  {
    lastPower = !onOff;
    mcp2.digitalWrite(14, lastPower);
    delay(100);
  }
}

void chckRolety()
{
  inProgressNow = 0;
  for (int i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    roleta[i].Update();
    if (roleta[i].isRunning())
    {
      inProgressNow++;
    }
  }
  switchPower((bool)inProgressNow);
}

void chckBT_event()
{
  if (bt_up.isSingleClick())
  {
    if (handleState > 0)
    {
      allForceStop();
    }
    else
    {
      allAutoUp();
      handleState = 1;
    }
  }
  if (bt_down.isSingleClick())
  {
    if (handleState > 0)
    {
      allForceStop();
    }
    else
    {
      allAutoDown();
      handleState = 2;
    }
  }

#ifdef POTEM

  if (bt_up.isDoubleClick())
  {
    if (handleState > 0)
    {

      allForceStop();
    }
    else
    {
      allHalfUp();
      handleState = 3;
    }
  }
  if (bt_down.isDoubleClick())
  {
    if (handleState > 0)
    {
      allForceStop();
    }
    else
    {
      allHalfUp();
      handleState = 4;
    }
  }
  if (bt_up.isReleased())
  {
    if (handleState == 5)
    {
      allForceStop();
    }
  }
  if (bt_down.isReleased())
  {
    if (handleState == 6)
    {
      allForceStop();
    }
  }
  if (bt_up.isLongClick())
  {
    if (handleState > 0)
    {
      allForceStop();
    }
    else
    {
      allManualUp();
      handleState = 5;
    }
  }
  if (bt_down.isLongClick())
  {
    if (handleState > 0)
    {
      allForceStop();
    }
    else
    {
      allManualUp();
      handleState = 6;
    }
  }
#endif // DEBUG
}

void allForceStop()
{
  for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  {
    roleta[i].ForceStop();
  }
  bAllAutoUp = false;
  bAllAutoDown = false;
  handleState = 0;
}
void allAutoUp()
{

  if (!bAllAutoUp)
  {
    bAllAutoUp = true;
  }
}
void allAutoDown()
{
  if (!bAllAutoDown)
  {
    bAllAutoDown = true;
  }
  // for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  // {
  //   roleta[i].AutoDown();
  // }
}
void allHalfUp()
{
  // for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  // {
  //   roleta[i].HalfUp();
  // }
}
void allHalfDown()
{
  // for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  // {
  //   roleta[i].HalfDown();
  // }
}
void allManualUp()
{
  // for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  // {
  //   roleta[i].ManualUp();
  // }
}
void allManualDown()
{
  // for (size_t i = 0; i < TOTAL_WINDOW_NUMB; i++)
  // {
  //   roleta[i].ManualDown();
  // }
}
void checkState()
{
  if (lastHandleState != handleState)
  {
    lastHandleState = handleState;
    switch (handleState)
    {
    case 0:
      blinkTime = 2000;
      break;
    case 1:
      blinkTime = 800;
      break;
    case 2:
      blinkTime = 800;
      break;
    case 3:
      blinkTime = 350;
      break;
    case 4:
      blinkTime = 350;
      break;
    case 5:
      blinkTime = 100;
      break;
    case 6:
      blinkTime = 70;
      break;

    default:
      break;
    }
  }
}

void checkAllAutoUp()
{
  if (bAllAutoUp)
  {
    if (inProgressNow < MAX_RELAY_ON_TIME)
    {
      if (lastSwitchedRelay < TOTAL_WINDOW_NUMB)
      {
        if (millis() - LastSwitchOnTime > BreakTimeBeetwen)
        {
          lastSwitchedRelay++;
          roleta[lastSwitchedRelay].AutoUp();
          LastSwitchOnTime = millis();
        }
      }
      else
      {
        lastSwitchedRelay = -1;
        bAllAutoUp = false;
      }
    }
  }
}

void checkAllAutoDown()
{
  if (bAllAutoDown)
  {
    if (inProgressNow < MAX_RELAY_ON_TIME)
    {
      if (lastSwitchedRelay < TOTAL_WINDOW_NUMB)
      {
        if (millis() - LastSwitchOnTime > BreakTimeBeetwen)
        {
          lastSwitchedRelay++;
          roleta[lastSwitchedRelay].AutoDown();
          LastSwitchOnTime = millis();
        }
      }
      else
      {
        lastSwitchedRelay = -1;
        bAllAutoDown = false;
      }
    }
  }
}

void loop()
{

  // if (ether.packetLoop(ether.pack))
  // {
  // Serial.println("et loop");
  // memcpy_P(ether.tcpOffset(), page, sizeof page);
  // ether.httpServerReply(sizeof page - 1);
  // delay(5000);
  // }
  // IF LED10=ON turn it ON
  // if (strstr((char *)Ethernet::buffer + pos, "GET /?relay1up") != 0)
  // {
  //   Serial.println("Received ON command");
  //   digitalWrite(13, HIGH);
  // }

  // // IF LED10=OFF turn it OFF
  // if (strstr((char *)Ethernet::buffer + pos, "GET /?relay1down") != 0)
  // {
  //   Serial.println("Received OFF command");
  //   digitalWrite(13, LOW);
  // }

  // // show some data to the user
  // memcpy_P(ether.tcpOffset(), page, sizeof page);
  // ether.httpServerReply(sizeof page - 1);

  checkAllAutoDown();
  checkAllAutoUp();
  checkState();
  // Serial.println();
  bt_down.update();
  bt_up.update();
  chckBT_event();
  // checkHandle();
  // serialEvent();
  chckRolety();
  blinkLed();
}

// // Print a message for each type of click.
// // Note that "click" is also generated for the
// // first click of a double-click, but "single"
// // is only generated if it definitely is not
// // going to be a double click.
