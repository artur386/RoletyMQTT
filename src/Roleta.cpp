#include "Roleta.h"
#include <MemoryFree.h>

Roleta::Roleta(uint8_t ID, Adafruit_MCP23017 *MCP_BANK, int EN_RELAY, int DIR_RELAY)
{
    this->DIR_RELAY = DIR_RELAY;
    this->EN_RELAY = EN_RELAY;
    this->ID = ID;
    this->MCP_BANK = MCP_BANK;
    this->TIME_AUTO_UP = 22000;
    this->TIME_AUTO_DOWN = 16000;
    this->TIME_HALF_UP = 5000;
    this->TIME_HALF_DOWN = 4500;
    this->TIME_MANUAL_UP = 60000;
    this->TIME_MANUAL_DOWN = 60000;
}

void Roleta::SetTimeAutoUp(uint16_t time)
{
    this->TIME_AUTO_UP = time;
}
void Roleta::SetTimeAutoDown(uint16_t time)
{
    this->TIME_AUTO_DOWN = time;
}
void Roleta::SetTimeHalfUp(uint16_t time)
{
    this->TIME_HALF_UP = time;
}
void Roleta::SetTimeHalfDown(uint16_t time)
{
    this->TIME_HALF_DOWN = time;
}

void Roleta::commonUp(uint16_t time)
{
    if (this->CheckState(ST_RUN) && !this->CheckState(ST_STOP))
    {
        this->ForceStop();
    }
    else
    {
        this->setDir(UP);
        this->enOut(eSTART);
        this->ClearState(ST_STOP);
        this->SetState(ST_RUN);
        this->SetState(ST_MOVE_UP);

        this->START_TIME = millis();
        this->END_TIME = time;
    }
}
void Roleta::commonDown(uint16_t time)
{
    if (this->CheckState(ST_RUN) && !this->CheckState(ST_STOP))
    {
        this->ForceStop();
    }
    else
    {
        this->setDir(DOWN);
        this->enOut(eSTART);
        this->ClearState(ST_STOP);
        this->SetState(ST_RUN);
        this->SetState(ST_MOVE_DOWN);
        this->SetState(ST_MOVE_FULL);
        this->START_TIME = millis();
        this->END_TIME = time;
    }
}

void Roleta::AutoUp()
{
    this->commonUp(this->TIME_AUTO_UP);
    this->SetState(ST_MOVE_FULL);
}
void Roleta::AutoDown()
{
    this->commonDown(this->TIME_AUTO_DOWN);
    this->SetState(ST_MOVE_FULL);
}
void Roleta::HalfUp()
{
    this->commonUp(this->TIME_HALF_UP);
}
void Roleta::HalfDown()
{
    this->commonDown(this->TIME_HALF_DOWN);
}
void Roleta::ManualUp()
{
    this->commonUp(this->TIME_MANUAL_UP);
}
void Roleta::ManualDown()
{
    this->commonDown(this->TIME_MANUAL_DOWN);
}

void Roleta::setDir(uint8_t dir)
{
    if (dir == UP)
    {
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
    }
    else if (dir == DOWN)
    {
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, LOW);
    }
    delay(500);
}
void Roleta::enOut(uint8_t onOff)
{
    if (onOff == eSTART)
    {
        this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    }
    else if (onOff == eSTOP)
    {
        this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
    }
    delay(500);
}
void Roleta::ForceStop()
{
    this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH); //enable rl
    delay(500);
    this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH); //direction rl
    delay(500);

    this->ClearState(ST_MOVE_UP);
    this->ClearState(ST_MOVE_DOWN);
    this->ClearState(ST_RUN);
    this->SetState(ST_STOP);
}
bool Roleta::CheckState(uint8_t _state)
{
    return bitRead(this->STATE, _state);
}
bool Roleta::isRunning()
{
    return CheckState(ST_RUN);
}
void Roleta::SetState(uint8_t _state)
{
    bitWrite(this->STATE, _state, HIGH);
}
void Roleta::ClearState(uint8_t _state)
{
    bitWrite(this->STATE, _state, LOW);
}
bool Roleta::Update()
{
    if (CheckState(ST_RUN))
    {
        if (millis() - this->START_TIME > this->END_TIME)
        {
            // Serial.println("force stop");
            this->ForceStop();
        }
        return true;
    }
    else
    {
        return false;
    }
}
