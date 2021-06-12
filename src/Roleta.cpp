#include "Roleta.h"

Roleta::Roleta(uint8_t ID, PubSubClient *mqttClient, Adafruit_MCP23017 *MCP_BANK, uint8_t EN_RELAY, uint8_t DIR_RELAY, uint16_t TIME_UP, uint16_t TIME_DOWN)
{
    this->ID = ID;
    this->mqttClient = mqttClient;
    this->MCP_BANK = MCP_BANK;
    this->EN_RELAY = EN_RELAY;
    this->DIR_RELAY = DIR_RELAY;
    this->TIME_TO_UP = TIME_UP;
    this->TIME_TO_DOWN = TIME_DOWN;
    this->END_TIME = 0;
    this->MoveIsOn = false;
    this->cmdChange = false;
    this->LAST_STATE_REGISTER = 0x00;
    this->STATE_REGISTER = 0x00;
    byte tempReg = GetEEprom(this->ID);
    this->SetRegister(this->LAST_STATE_REGISTER, STD, GetRegister(tempReg, STD));
    this->SetRegister(this->LAST_STATE_REGISTER, CMD, GetRegister(tempReg, CMD));
    STATE_REGISTER = LAST_STATE_REGISTER;
    this->UpdateEEprom(this->ID, this->LAST_STATE_REGISTER);
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
    delay(100);
    this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    this->MoveIsOn = true;
}
void Roleta::RelayHALT(void)
{
    this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
    if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_CLOSING)
    {
        delay(100);
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
    if (this->GetRegister(this->LAST_STATE_REGISTER, CMD) != CMD_OPEN)
    {
        this->SetRegister(this->STATE_REGISTER, CMD, CMD_OPEN);
        cmdChange = true;
    }
}
void Roleta::SetDOWN()
{
    if (this->GetRegister(this->LAST_STATE_REGISTER, CMD) != CMD_CLOSE)
    {
        this->SetRegister(this->STATE_REGISTER, CMD, CMD_CLOSE);
        cmdChange = true;
    }
}
void Roleta::SetHALT()
{
    if (this->GetRegister(this->LAST_STATE_REGISTER, CMD) != CMD_STOP)
    {
        this->SetRegister(this->STATE_REGISTER, CMD, CMD_STOP);
        // this->Trigger();
        cmdChange = true;
    }
}
boolean Roleta::PublishSTATE(byte state)
{
    char *topicP = (char *)malloc(sizeof(char) * 16);
    char *messageP = (char *)malloc(sizeof(char) * 8);
    char *messageOut = (char *)malloc(sizeof(char) * 60);
    bool pubOK;

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
        pubOK = this->mqttClient->publish(topicP, messageP);
    }
    free(topicP);
    free(messageP);
    free(messageOut);

    return pubOK;
}

void Roleta::SetRegister(byte &reg, byte mask, byte val)
{
    ClearRegister(reg, mask);
    reg |= (val << ShiftCount(mask));
}

byte Roleta::GetRegister(byte reg, byte mask)
{
    return (reg & mask) >> ShiftCount(mask);
}

void Roleta::ClearRegister(byte &reg, byte mask)
{
    reg &= ~(mask);
}

byte Roleta::ShiftCount(byte mask)
{
    byte count = 0;
    while ((~(mask) >> count) & 0x01)
    {
        count++;
    }
    return count;
}

byte Roleta::GetEEprom(byte adress)
{
    return EEPROM.read(adress);
}

void Roleta::UpdateEEprom(byte adress, byte data)
{
    EEPROM.update(adress, data);
}
void Roleta::Trigger(void)
{
    this->SetRegister(STATE_REGISTER, TRIG, 1);
}
boolean Roleta::CheckTrigger(void)
{
    return !!this->GetRegister(STATE_REGISTER, TRIG);
}
void Roleta::Loop()
{
    if (cmdChange)
    {
        if (this->CheckTrigger())
        {
            this->ClearRegister(STATE_REGISTER, TRIG);
            cmdChange = false;

            switch (this->GetRegister(STATE_REGISTER, CMD))
            {
            case CMD_OPEN:
                if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_CLOSING)
                {
                    this->RelayHALT();
                    delay(100);
                }
                this->SetRegister(STATE_REGISTER, STD, STD_OPENING);
                this->MoveUP();
                break;

            case CMD_CLOSE:
                if (this->GetRegister(LAST_STATE_REGISTER, STD) == STD_OPENING)
                {
                    this->RelayHALT();
                    delay(100);
                }
                this->SetRegister(STATE_REGISTER, STD, STD_CLOSING);
                this->MoveDown();
                break;

            case CMD_STOP:
                this->SetRegister(STATE_REGISTER, STD, STD_STOPPED);
                this->RelayHALT();
                break;

            default:
                break;
            }

            this->SetRegister(LAST_STATE_REGISTER, CMD, this->GetRegister(STATE_REGISTER, CMD));
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
        if (this->PublishSTATE(this->GetRegister(LAST_STATE_REGISTER, STD)))
        {
            this->SetRegister(LAST_STATE_REGISTER, STD, this->GetRegister(STATE_REGISTER, STD));
            this->UpdateEEprom(this->ID, LAST_STATE_REGISTER);
            delay(5);
        }
    }
}
