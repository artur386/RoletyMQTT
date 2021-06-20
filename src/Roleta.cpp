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
    this->triger = false;
    this->LAST_STATE_REGISTER = this->GetEEprom(STATUS);
    this->LAST_COMMAND_REGISTER = this->GetEEprom(COMMAND);
    this->STATE_REGISTER = this->LAST_STATE_REGISTER;
    this->COMMAND_REGISTER = this->LAST_COMMAND_REGISTER;
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
    delay(200);
    this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    this->MoveIsOn = true;
}
void Roleta::RelayHALT(void)
{
    this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
    if (LAST_STATE_REGISTER == STD_CLOSING)
    {
        delay(200);
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
    if (LAST_COMMAND_REGISTER != CMD_OPEN)
    {
        COMMAND_REGISTER = CMD_OPEN;
        cmdChange = true;
    }
}
void Roleta::SetDOWN()
{
    if (LAST_COMMAND_REGISTER != CMD_CLOSE)
    {
        COMMAND_REGISTER = CMD_CLOSE;
        cmdChange = true;
    }
}
void Roleta::SetHALT()
{
    if (LAST_COMMAND_REGISTER != CMD_STOP)
    {
        COMMAND_REGISTER = CMD_STOP;
        cmdChange = true;
    }
}
boolean Roleta::PublishSTATE(byte state)
{
    char *topicP = (char *)malloc(sizeof(char) * 16);
    char *messageP = (char *)malloc(sizeof(char) * 8);
    char *messageOut = (char *)malloc(sizeof(char) * 60);
    bool pubOK;

    switch (state)
    {
    case STD_CLOSED:
        snprintf_P(messageP, 7, PSTR("closed"));
        break;

    case STD_CLOSING:
        snprintf_P(messageP, 8, PSTR("closing"));
        break;

    case STD_OPEN:
        snprintf_P(messageP, 5, PSTR("open"));
        break;

    case STD_OPENING:
        snprintf_P(messageP, 8, PSTR("opening"));
        break;

    case STD_STOPPED:
        snprintf_P(messageP, 8, PSTR("stopped"));
        break;

    default:
        break;
    }
    LAST_STATE_REGISTER = STATE_REGISTER;
    snprintf_P(topicP, 20, PSTR("roleta/%i/state"), this->ID);
    if (this->mqttClient->connected())
    {
        pubOK = this->mqttClient->publish(topicP, messageP);
    }
    free(topicP);
    free(messageP);
    free(messageOut);

    return pubOK;
}

byte Roleta::GetEEprom(byte regName)
{
    switch (regName)
    {
    case STATUS:
        return EEPROM.read((this->ID * 2) + 0);
        break;

    case COMMAND:
        return EEPROM.read((this->ID * 2) + 1);
        break;

    default:
        break;
    }

    return 0;
}

void Roleta::UpdateEEprom(byte state, byte command)
{
    EEPROM.update((this->ID * 2) + 0, state);
    EEPROM.update((this->ID * 2) + 1, command);
}

void Roleta::Trigger(void)
{
    this->triger = true;
}

boolean Roleta::CheckTrigger(void)
{
    return this->triger;
}

void Roleta::Loop()
{
    if (this->cmdChange)
    {
        if (this->triger)
        {
            this->triger = false;
            cmdChange = false;

            switch (COMMAND_REGISTER)
            {
            case CMD_OPEN:
                STATE_REGISTER = STD_OPENING;
                if (LAST_STATE_REGISTER == STD_CLOSING)
                    this->RelayHALT();
                this->UpdateEEprom(STATE_REGISTER, COMMAND_REGISTER);
                this->PublishSTATE(STATE_REGISTER);
                this->MoveUP();
                break;

            case CMD_CLOSE:
                STATE_REGISTER = STD_CLOSING;
                if (LAST_STATE_REGISTER == STD_OPENING)
                    this->RelayHALT();
                this->UpdateEEprom(STATE_REGISTER, COMMAND_REGISTER);
                this->PublishSTATE(STATE_REGISTER);
                this->MoveDown();
                break;

            case CMD_STOP:
                STATE_REGISTER = STD_STOPPED;
                this->UpdateEEprom(STATE_REGISTER, COMMAND_REGISTER);
                this->PublishSTATE(STATE_REGISTER);
                this->RelayHALT();
                break;

            default:
                break;
            }
            LAST_COMMAND_REGISTER = COMMAND_REGISTER;
        }
    }

    if (this->MoveIsOn)
    {
        if (millis() - this->START_TIME > *this->END_TIME)
        {
            if (LAST_STATE_REGISTER == STD_OPENING)
                STATE_REGISTER = STD_OPEN;
            if (LAST_STATE_REGISTER == STD_CLOSING)
                STATE_REGISTER = STD_CLOSED;
            this->UpdateEEprom(STATE_REGISTER, COMMAND_REGISTER);
            this->PublishSTATE(STATE_REGISTER);
            this->RelayHALT();
        }
    }
}
