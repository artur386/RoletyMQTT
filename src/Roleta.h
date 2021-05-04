

#pragma once

#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <PubSubClient.h>

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

class Roleta
{
private:
    Adafruit_MCP23017 *MCP_BANK;
    PubSubClient *mqttClient;

    uint8_t ID;
    uint8_t DIR_RELAY;
    uint8_t EN_RELAY;
    uint8_t STATE, LAST_STATE;

    /***********************************************
 * 
 *   - STATE_REGISTER -
* 
* 
*                          7 6 5 4 3 2 1 0
*                          ---------------
*         [CMD] - UP       - - - - - - - X
*       [CMD] - DOWN       - - - - - - X -
*       [CMD] - STOP       - - - - - - X X               
*     LST [CMD] - UP       - - - - - X - - 
*   LST [CMD] - DOWN       - - - - X - - -
*   LST [CMD] - STOP       - - - - X X - -   
*     SHUTTER_CLOSED       - - - X - - - -                
*    SHUTTER_CLOSING       - - X - - - - -
*       SHUTTER_OPEN       - - X X - - - -
*    SHUTTER_OPENING       - X - - - - - -
*    SHUTTER_STOPPED       - X - X - - - -
*            TRIGGER       X - - - - - - -
*
 * *******************************/

    // #define CMD_UP 0
    // #define CMD_DOWN 1
    // #define CMD_STOP 2
    // #define LST_CMD_UP 3
    // #define LST_CMD_DOWN 4
    // #define LST_CMD_STOP 5
    // #define TRIGGER 7

    // #define CMD_UP 0
    // #define CMD_UP 0

    byte STATE_REGISTER;

    uint16_t TIME_TO_UP;
    uint16_t TIME_TO_DOWN;
    uint16_t *END_TIME;

    unsigned long START_TIME;

    void setDir(uint8_t dir);
    void enOut(uint8_t onOff);
    void SetState(uint8_t state);
    void ChckSTATUS();

public:
    void SetCommand(byte cmd);
    byte GetCommand();

    void SetLastCmd(byte cmd);
    byte GetLastCmd();

    void SetLastSTD(byte cmd);
    byte GetLastSTD();

    void TriggerCmd();
    bool GetTrigger();

    Roleta(uint8_t ID, PubSubClient *mqttClient, Adafruit_MCP23017 *MCP_BANK, uint8_t EN_RELAY, uint8_t DIR_RELAY, uint16_t TIME_UP, uint16_t TIME_DOWN);
    bool CheckState(uint8_t state);
    void PublishSTATE();
    void MoveUP();
    void MoveDown();
    void MoveStop();
    bool Loop();
};
