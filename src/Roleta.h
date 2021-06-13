

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#define DEBUG_ 1
//#define DEBUG_SW_PORT swserial(4,5)

#ifdef DEBUG_
#ifdef DEBUG_SW_PORT
#define DEBUG_PORT swserial
#else
#define DEBUG_PORT Serial
#endif
#define DEBUG_BEGIN(baud) DEBUG_PORT.begin(baud)
#define DEBUG_WRITE(...) DEBUG_PORT.write(__VA_ARGS__)
#define DEBUG_PRINT(...) DEBUG_PORT.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DEBUG_PORT.println(__VA_ARGS__)
#else
#define DEBUG_BEGIN(baud)
#define DEBUG_WRITE(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif //

#define DIR_DOWN 0
#define DIR_UP 1

#define CMD_OPEN 0
#define CMD_CLOSE 1
#define CMD_STOP 2

#define STD_CLOSED 0
#define STD_CLOSING 1
#define STD_OPEN 2
#define STD_OPENING 3
#define STD_STOPPED 4

#define STATUS 0
#define COMMAND 1

#define CMD 0x03
#define STD 0x1C
#define TRIG 0x20
#define ONOFF 0x40

class Roleta
{
private:
    Adafruit_MCP23017 *MCP_BANK;
    PubSubClient *mqttClient;

    uint8_t ID;
    uint8_t DIR_RELAY;
    uint8_t EN_RELAY;

    byte STATE_REGISTER, LAST_STATE_REGISTER;
    byte COMMAND_REGISTER, LAST_COMMAND_REGISTER;
    boolean triger;

    uint16_t TIME_TO_UP;
    uint16_t TIME_TO_DOWN;
    uint16_t *END_TIME;

    unsigned long START_TIME;

    void RelayUP(void);
    void RelayDOWN(void);
    void RelayHALT(void);

public:
    bool MoveIsOn;
    bool cmdChange;

    void UpdateEEprom(byte adress, byte data);
    byte GetEEprom(byte adress);

    void SetRegister(byte &reg, byte data, byte val);
    byte GetRegister(byte reg, byte data);
    void ClearRegister(byte &reg, byte data);
    byte ShiftCount(byte mask);

    void MoveUP();
    void MoveDown();

    Roleta(uint8_t ID, PubSubClient *mqttClient, Adafruit_MCP23017 *MCP_BANK, uint8_t EN_RELAY, uint8_t DIR_RELAY, uint16_t TIME_UP, uint16_t TIME_DOWN);
    // bool CheckState(uint8_t state);
    boolean PublishSTATE(byte state);
    void Loop();

    void Trigger(void);
    bool CheckTrigger(void);

    void SetUP();
    void SetDOWN();
    void SetHALT();
};