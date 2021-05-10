

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

enum STATE
{
    SHUTTER_CLOSED,
    SHUTTER_CLOSING,
    SHUTTER_OPEN,
    SHUTTER_OPENING,
    SHUTTER_STOPPED
};

enum DIR
{
    DIR_DOWN,
    DIR_UP
};

enum REG_DATA
{
    CMD,
    STD,
    TRIG,
    ONOFF
};

#define CMD_NOPE 0
#define CMD_OPEN 1
#define CMD_CLOSE 2
#define CMD_STOP 3
#define STD_CLOSED 0
#define STD_CLOSING 1
#define STD_OPEN 2
#define STD_OPENING 3
#define STD_STOPPED 4

const byte RegOrder[4][2] = {
    {0, 2},
    {2, 3},
    {5, 1},
    {6, 1}};

class Roleta
{
private:
    Adafruit_MCP23017 *MCP_BANK;
    PubSubClient *mqttClient;

    uint8_t ID;
    uint8_t DIR_RELAY;
    uint8_t EN_RELAY;

    /***********************************************
 * 
 *   - STATE_REGISTER -
* 
*                          76543210
* ===================      ========
*     [CMD] - UP           -------+
*     [CMD] - DOWN         ------+-
*     [CMD] - STOP         ------++               
* ===================      ========
* [LST_CMD] - UP           -----+-- 
* [LST_CMD] - DOWN         ----+---
* [LST_CMD] - STOP         ----++--   
* ===================      ========
*   [STATE] - CLOSED       ---+----                
*   [STATE] - CLOSING      --+-----
*   [STATE] - OPEN         --++----
*   [STATE] - OPENING      -+------
*   [STATE] - STOPPED      -+-+----
* ===================      ========
*  [TRIGER] - TRIGGER      +-------
* ===================      ========
*
*
 * *******************************/

    byte STATE_REGISTER, LAST_STATE_REGISTER;

    uint16_t TIME_TO_UP;
    uint16_t TIME_TO_DOWN;
    uint16_t *END_TIME;

    unsigned long START_TIME;

    void SetState(uint8_t state);
    void ChckSTATUS();

public:
    void SetCommand(byte cmd);
    byte GetCommand();

    void SetLastCmd(byte cmd);
    byte GetLastCmd();

    void SetState(byte std);
    byte GetState();

    void SetEEprom();
    void GetEEprom();

    void SetRegister(byte &reg, byte data, byte val);
    byte GetRegister(byte &reg, byte data);
    void ClearRegister(byte &reg, byte data);
    void SetRegData(byte &nbit, byte &lbit, byte data);

    void SetTrigger();
    bool CheckTrigger();

    Roleta(uint8_t ID, PubSubClient *mqttClient, Adafruit_MCP23017 *MCP_BANK, uint8_t EN_RELAY, uint8_t DIR_RELAY, uint16_t TIME_UP, uint16_t TIME_DOWN);
    bool CheckState(uint8_t state);
    void PublishSTATE();
    void MoveUP();
    void MoveDown();
    void MoveStop();
    bool Loop();
    void Trigger();
};