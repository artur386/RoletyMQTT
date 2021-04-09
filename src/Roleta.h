

#ifndef ROLETA_H_
#define ROLETA_H_

#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_MCP23017.h>

// #define BT_SHORT_up() bitWrite(REG_button, 0, HIGH)
// #define BT_LONG_up() bitWrite(REG_button, 1, HIGH)
// #define BT_DOUBLE_up() bitWrite(REG_button, 2, HIGH)
// #define BT_SHORT_down() bitWrite(REG_button, 3, HIGH)
// #define BT_LONG_down() bitWrite(REG_button, 4, HIGH)
// #define BT_DOUBLE_down() bitWrite(REG_button, 5, HIGH)

// #define BT_RESET_SHORT_up() bitWrite(REG_button, 0, LOW)
// #define BT_RESET_LONG_up() bitWrite(REG_button, 1, LOW)
// #define BT_RESET_DOUBLE_up() bitWrite(REG_button, 2, LOW)
// #define BT_RESET_SHORT_down() bitWrite(REG_button, 3, LOW)
// #define BT_RESET_LONG_down() bitWrite(REG_button, 4, LOW)
// #define BT_RESET_DOUBLE_down() bitWrite(REG_button, 5, LOW)
// #define BT_RESET_ALL() REG_button &= 0x00

// #define BT_CHECK_SHORT_up() bitRead(REG_button, 0)
// #define BT_CHECK_LONG_up() bitRead(REG_button, 1)
// #define BT_CHECK_DOUBLE_up() bitRead(REG_button, 2)
// #define BT_CHECK_SHORT_down() bitRead(REG_button, 3)
// #define BT_CHECK_LONG_down() bitRead(REG_button, 4)
// #define BT_CHECK_DOUBLE_down() bitRead(REG_button, 5)
// #define BT_CHECK_ANY()  \
//     if (REG_button > 0) \
//     ? true : false

enum STATE
{
    ST_STOP = 0,
    ST_RUN,
    ST_MOVE_UP,
    ST_MOVE_DOWN,
    ST_FULL_OPEN,
    ST_FULL_CLOSE,
    ST_MID_OPEN,
    ST_MOVE_FULL
};

enum DIR
{
    UP,
    DOWN
};
enum EN
{
    eSTART,
    eSTOP
};

class Roleta
{
private:
    Adafruit_MCP23017 *MCP_BANK;
    uint8_t ID;
    uint8_t DIR_RELAY;
    uint8_t EN_RELAY;

    uint16_t TIME_AUTO_UP;
    uint16_t TIME_AUTO_DOWN;
    uint16_t TIME_HALF_UP;
    uint16_t TIME_HALF_DOWN;
    uint16_t TIME_MANUAL_UP;
    uint16_t TIME_MANUAL_DOWN;

    unsigned long START_TIME;
    unsigned long END_TIME;

    uint8_t STATE;
    uint8_t REG_button;

    void setDir(uint8_t dir);
    void enOut(uint8_t onOff);
    void SetState(uint8_t state);
    bool CheckState(uint8_t state);
    void commonUp(uint16_t time);
    void commonDown(uint16_t time);

public:
    Roleta(uint8_t ID, Adafruit_MCP23017 *MCP_BANK, int EN_RELAY, int DIR_RELAY);
    void SetTimeAutoUp(uint16_t time);
    void SetTimeAutoDown(uint16_t time);
    void SetTimeHalfUp(uint16_t time);
    void SetTimeHalfDown(uint16_t time);
    void AutoUp();
    void AutoDown();
    void HalfUp();
    void HalfDown();
    void ManualUp();
    void ManualDown();
    void ForceStop();
    bool Update();
    void ClearState(uint8_t _state);
    bool isRunning();
};

#endif // !ROLETA_H_
