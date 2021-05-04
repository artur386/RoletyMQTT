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
    this->STATE = SHUTTER_STOPPED;
    this->LAST_STATE = 255;
}

void Roleta::MoveUP()
{
    if (this->CheckState(SHUTTER_STOPPED) || this->CheckState(SHUTTER_CLOSED))
    {
        DEBUG_PRINTLN(F("MOVE UP"));
        this->END_TIME = &this->TIME_TO_UP;
        this->SetState(SHUTTER_OPENING);
        this->START_TIME = millis();
    }
}
void Roleta::MoveDown()
{
    if (this->CheckState(SHUTTER_STOPPED) || this->CheckState(SHUTTER_OPEN))
    {
        Serial.println(F("MOVE DOWN"));
        this->END_TIME = &this->TIME_TO_DOWN;
        this->SetState(SHUTTER_CLOSING);
        this->START_TIME = millis();
    }
}

void Roleta::MoveStop()
{
    if (this->CheckState(SHUTTER_CLOSING) || this->CheckState(SHUTTER_OPENING))
    {
        Serial.println(F("MOVE STOP"));
        this->SetState(SHUTTER_STOPPED);
    }
}

void Roleta::PublishSTATE()
{
    char *topicP = (char *)malloc(sizeof(char) * 16);
    char *messageP = (char *)malloc(sizeof(char) * 8);
    char *messageOut = (char *)malloc(sizeof(char) * 60);

    if ((this->STATE >> SHUTTER_CLOSED) & 1)
    {
        snprintf_P(messageP, 7, PSTR("closed"));
        this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
        delay(100);
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
        // delay(100);
    }
    else if ((this->STATE >> SHUTTER_CLOSING) & 1)
    {
        snprintf_P(messageP, 8, PSTR("closing"));
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, LOW);
        delay(100);
        this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    }
    else if ((this->STATE >> SHUTTER_OPEN) & 1)
    {
        snprintf_P(messageP, 5, PSTR("open"));
        this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
        delay(100);
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
        // delay(100);
    }
    else if ((this->STATE >> SHUTTER_OPENING) & 1)
    {
        snprintf_P(messageP, 8, PSTR("opening"));
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
        delay(100);
        this->MCP_BANK->digitalWrite(this->EN_RELAY, LOW);
    }
    else if ((this->STATE >> SHUTTER_STOPPED) & 1)
    {
        snprintf_P(messageP, 8, PSTR("stopped"));
        this->MCP_BANK->digitalWrite(this->EN_RELAY, HIGH);
        delay(100);
        this->MCP_BANK->digitalWrite(this->DIR_RELAY, HIGH);
        // delay(100);
    }

    snprintf_P(topicP, 16, PSTR("roleta/%i/state"), this->ID);
    if (this->mqttClient->connected())
    {
        snprintf_P(messageOut, 60, PSTR("PUBLISH ON TOPIC [%s] MESSAGE: [%s]"), topicP, messageP);
        Serial.println(messageOut);
        this->mqttClient->publish(topicP, messageP);
    }
    free(topicP);
    free(messageP);
    free(messageOut);
}

// void Roleta::ChckSTATUS()
// {
//     if (this->LAST_STATE != this->STATE)
//     {
//         this->LAST_STATE = this->STATE;
//         this->PublishSTATE();
//     }
// }

void Roleta::SetCommand(byte cmd)
{
    STATE_REGISTER = STATE_REGISTER |= cmd;
}
byte Roleta::GetCommand()
{
    return (STATE_REGISTER &= 0x00000011);
}
void Roleta::SetLastCmd(byte cmd)
{
    STATE_REGISTER = STATE_REGISTER |= (cmd << 2);
}
byte Roleta::GetLastCmd()
{
    return ((STATE_REGISTER &= 0x00001100) >> 2);
}
void Roleta::SetLastSTD(byte cmd)
{
    STATE_REGISTER = STATE_REGISTER |= (cmd << 4);
}
byte Roleta::GetLastSTD()
{
    return ((STATE_REGISTER &= 0x00001100) >> 4);
}
void Roleta::TriggerCmd()
{
    STATE_REGISTER = (STATE_REGISTER |= (1 << 7));
}

bool Roleta::GetTrigger()
{
    return (STATE_REGISTER &= 0x10000000) >> 7;
}

bool Roleta::CheckState(uint8_t _state)
{
    return ((this->STATE) >> (_state)) & 0x01;
}

void Roleta::SetState(uint8_t _state)
{
    this->STATE = 0x00;
    this->STATE |= (1UL << _state);
}

bool Roleta::Loop()
{
    bool retOnOff;
    this->ChckSTATUS();
    if (this->CheckState(SHUTTER_CLOSING) || this->CheckState(SHUTTER_OPENING))
    {
        if (millis() - this->START_TIME > *this->END_TIME)
        {
            // if (this->CheckState(SHUTTER_CLOSING))
            // {
            //     this->SetState(SHUTTER_CLOSED);
            // }
            // else if (this->CheckState(SHUTTER_OPENING))
            // {
            //     this->SetState(SHUTTER_OPEN);
            // }
            // else
            // {
            this->SetState(SHUTTER_STOPPED);
            // }

            retOnOff = false;
        }
        else
        {
            retOnOff = true;
        }
    }
    else if (this->CheckState(SHUTTER_CLOSED) || this->CheckState(SHUTTER_OPEN) || this->CheckState(SHUTTER_STOPPED))
    {
        retOnOff = false;
    }
    return retOnOff;
}
