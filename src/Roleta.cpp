#include "Roleta.h"

Roleta::Roleta(uint8_t ID, PubSubClient *mqttClient, Adafruit_MCP23017 *MCP_BANK, uint8_t EN_RELAY, uint8_t DIR_RELAY, uint16_t TIME_UP, uint16_t TIME_DOWN)
{
    this->mqttClient = mqttClient;
    this->DIR_RELAY = DIR_RELAY;
    this->EN_RELAY = EN_RELAY;
    this->ID = ID;
    this->MCP_BANK = MCP_BANK;
    this->TIME_TO_UP = TIME_UP;
    this->TIME_TO_DOWN = TIME_DOWN;
    this->END_TIME = 0;
    this->MoveIsOn = false;
    this->cmdChange = false;
    this->GetEEprom();
}

void Roleta::RelayUP(void)
{
    this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
    this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    this->MoveIsOn = true;
}
void Roleta::RelayDOWN(void)
{
    this->MCP_BANK->digitalWrite(this->DIR_RELAY, LOW);
    delay(20);
    this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    this->MoveIsOn = true;
}
void Roleta::RelayHALT(void)
{
    this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
    if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_CLOSING)
    {
        delay(20);
    }
    this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
    this->MoveIsOn = false;
}

void Roleta::MoveUP()
{

    this->RelayUP();
    this->END_TIME = &this->TIME_TO_UP;
    this->START_TIME = millis();
}
void Roleta::MoveDown()
{

    this->RelayDOWN();
    this->END_TIME = &this->TIME_TO_DOWN;
    this->START_TIME = millis();
}

void Roleta::SetUP()
{
    this->SetRegister(STATE_REGISTER, CMD, CMD_OPEN);
}
void Roleta::SetDOWN()
{
    this->SetRegister(STATE_REGISTER, CMD, CMD_CLOSE);
}
void Roleta::SetHALT()
{
    this->SetRegister(STATE_REGISTER, CMD, CMD_STOP);
}
void Roleta::PublishSTATE(byte state)
{
    char *topicP = (char *)malloc(sizeof(char) * 16);
    char *messageP = (char *)malloc(sizeof(char) * 8);
    char *messageOut = (char *)malloc(sizeof(char) * 60);

    if (state == STD_CLOSED)
    {
        snprintf_P(messageP, 7, PSTR("closed"));
    }
    else if (state == STD_CLOSING)
    {
        snprintf_P(messageP, 8, PSTR("closing"));
    }
    else if (state == STD_OPEN)
    {
        snprintf_P(messageP, 5, PSTR("open"));
    }
    else if (state == STD_OPENING)
    {
        snprintf_P(messageP, 8, PSTR("opening"));
    }
    else if (state == STD_STOPPED)
    {
        snprintf_P(messageP, 8, PSTR("stopped"));
    }

    snprintf_P(topicP, 16, PSTR("roleta/%i/state"), this->ID);
    if (this->mqttClient->connected())
    {
        this->mqttClient->publish(topicP, messageP);
    }
    free(topicP);
    free(messageP);
    free(messageOut);
}

void Roleta::SetRegister(byte &reg, byte mask, byte val)
{
    ClearRegister(reg, mask);
    reg |= (val << ShistCount(mask));
}

byte Roleta::GetRegister(byte &reg, byte mask)
{
    return (reg & mask) >> ShistCount(mask);
}

void Roleta::ClearRegister(byte &reg, byte mask)
{
    reg &= ~(mask);
}

byte Roleta::ShistCount(byte mask)
{
    byte count = 0;
    while ((~(mask) >> count) & 0x01)
    {
        count++;
    }
    return count;
}

void Roleta::GetEEprom()
{
    EEPROM.get(this->ID + 1, LAST_STATE_REGISTER);
}

void Roleta::SetEEprom()
{
    EEPROM.put(this->ID + 1, LAST_STATE_REGISTER);
}
void Roleta::Trigger(void)
{
    this->SetRegister(STATE_REGISTER, TRIG, 1);
}
bool Roleta::CheckTrigger(void)
{
    return !!this->GetRegister(STATE_REGISTER, TRIG);
}
void Roleta::Loop()
{

    if (this->GetRegister(STATE_REGISTER, CMD) != this->GetRegister(LAST_STATE_REGISTER, CMD))
    {
        cmdChange = true;
    }

    if (this->CheckTrigger())
    {
        byte CMDb = this->GetRegister(STATE_REGISTER, CMD);
        this->ClearRegister(STATE_REGISTER, TRIG);
        cmdChange = false;

        if (CMDb != this->GetRegister(LAST_STATE_REGISTER, CMD))
        {
            this->SetRegister(LAST_STATE_REGISTER, CMD, CMDb);

            if (CMDb == CMD_OPEN)
            {
                if (!this->MoveIsOn)
                {
                    this->SetRegister(STATE_REGISTER, STD, STD_OPENING);
                    this->MoveUP();
                }
            }
            else if (CMDb == CMD_CLOSE)
            {
                if (!this->MoveIsOn)
                {
                    this->SetRegister(STATE_REGISTER, STD, STD_CLOSING);
                    this->MoveDown();
                }
            }
            else if (CMDb == CMD_STOP)
            {
                if (this->MoveIsOn)
                {
                    this->SetRegister(STATE_REGISTER, STD, STD_STOPPED);
                    this->RelayHALT();
                }
            }
        }
    }
    if (this->MoveIsOn)
    {
        if (millis() - this->START_TIME > *this->END_TIME)
        {
            if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_OPENING)
            {
                this->SetRegister(STATE_REGISTER, STD, STD_OPEN);
            }
            else if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_CLOSING)
            {
                this->SetRegister(STATE_REGISTER, STD, STD_CLOSED);
            }

            this->RelayHALT();
        }
    }
    if (this->GetRegister(STATE_REGISTER, STD) != this->GetRegister(LAST_STATE_REGISTER, STD))
    {
        this->SetRegister(LAST_STATE_REGISTER, STD, this->GetRegister(STATE_REGISTER, STD));
        this->PublishSTATE(this->GetRegister(LAST_STATE_REGISTER, STD));
        this->SetEEprom();
    }
}
