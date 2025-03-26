// MQTT Library (includes framework only)
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: -
// Target uC:       -
// System Clock:    -

// Hardware configuration:
// -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "timer.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

uint8_t data[512];
uint16_t packetLen;

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

uint8_t* getMQTTPack(){
    return data;

}

uint16_t getMQTTPackLen(){
    return packetLen;

}

void connectMqtt()
{
    if(getTcpState(0) == TCP_ESTABLISHED){
        uint16_t i;

        mqttPacket* mqtt = (mqttPacket*)data;
        mqtt->typeFlags = MQTT_CONNECT;                     //connect flag

        packetLen = 1;
        uint8_t headerLength[] = {0x00, 0x04};              //length of variable header
        for (i = 0; i < sizeof(headerLength); i++)
            mqtt->data[packetLen++] = headerLength[i];
        uint8_t protocol[] = {0x4d, 0x51, 0x54, 0x54};      //name of protocal (Mqtt)
        for (i = 0; i < sizeof(protocol); i++)
            mqtt->data[packetLen++] = protocol[i];
        uint8_t protocolLevel = 0x04;                       //level of mqtt
        mqtt->data[packetLen++] = protocolLevel;
        uint8_t connectFlags = MQTT_CLEAN_SESSION;          //flags
        mqtt->data[packetLen++] = connectFlags;
        uint8_t keepAlive[] = {0x00, 0x3c};                 //keep alive
        for (i = 0; i < sizeof(keepAlive); i++)
            mqtt->data[packetLen++] = keepAlive[i];

        uint8_t user[] = {0x57,  0x69, 0x6c, 0x6c};         //name of client
        uint8_t userLength[] = {0x00, sizeof(user)};        //size of client name
        for (i = 0; i < sizeof(userLength); i++)
            mqtt->data[packetLen++] = userLength[i];
        for (i = 0; i < sizeof(user); i++)
            mqtt->data[packetLen++] = user[i];

        mqtt->data[0] = packetLen-1;                        //size of total packet
        setmqttPackFlag(1);                                 //set flag for sending packet
        setMqttCon(1);
        startPeriodicTimer(pingReqMqtt, NULL, 0x3c);        //enable MQTTping
    }
}

void disconnectMqtt()
{
    if(getTcpState(0) == TCP_ESTABLISHED){
        mqttPacket* mqtt = (mqttPacket*)data;
        mqtt->typeFlags = MQTT_DISCONNECT;                  //disconnect flag and length

        packetLen=1;

        mqtt->data[0] = packetLen-1;
        setmqttPackFlag(1);
        setMqttCon(0);
        stopTimer(pingReqMqtt);                             //disable ping
    }

}

void publishMqtt(char strTopic[], char strData[])
{
    if(getTcpState(0) == TCP_ESTABLISHED){
        uint16_t i;
        mqttPacket* mqtt = (mqttPacket*)data;
        mqtt->typeFlags = MQTT_PUBLISH;                     //publish flag

        packetLen = 1;
        uint16_t topLength = strlen(strTopic);              //length of topic
        if(strlen(strTopic) > 255){
            mqtt->data[packetLen++] = topLength/256;
        } else{
            mqtt->data[packetLen++] = 0;
        }
        mqtt->data[packetLen++] = topLength%256;
        for (i = 0; i < topLength; i++)                     //topic
            mqtt->data[packetLen++] = strTopic[i];

        for (i = 0; i < strlen(strData); i++)               //message
            mqtt->data[packetLen++] = strData[i];

        mqtt->data[0] = packetLen-1;
        setmqttPackFlag(1);
    }
}

void subscribeMqtt(char strTopic[])
{
    if(getTcpState(0) == TCP_ESTABLISHED){
        uint16_t i;
        mqttPacket* mqtt = (mqttPacket*)data;
        mqtt->typeFlags = MQTT_SUBSCRIBE | 0x02;            //subscribe flag and reserved bits

        packetLen = 1;

        uint16_t identifier = random32() & 0xffff;          //random number for identifier
        mqtt->data[packetLen++] = (identifier & 0xff00) >> 8;
        mqtt->data[packetLen++] = identifier & 0xff;

        uint16_t topLength = strlen(strTopic);              //topic length
        if(strlen(strTopic) > 255){
            mqtt->data[packetLen++] = topLength/256;
        } else{
            mqtt->data[packetLen++] = 0;
        }
        mqtt->data[packetLen++] = topLength%256;
        for (i = 0; i < topLength; i++)
            mqtt->data[packetLen++] = strTopic[i];          //topic

        mqtt->data[packetLen++] = 0x00;

        mqtt->data[0] = packetLen-1;
        setmqttPackFlag(1);
    }
}

void unsubscribeMqtt(char strTopic[])
{
    if(getTcpState(0) == TCP_ESTABLISHED){
            uint16_t i;
            mqttPacket* mqtt = (mqttPacket*)data;
            mqtt->typeFlags = MQTT_UNSUBSCRIBE | 0x02;      //subscribe flag and reserved bits

            packetLen = 1;

            uint16_t identifier = random32() & 0xffff;      //random number for identifier
            mqtt->data[packetLen++] = (identifier & 0xff00) >> 8;
            mqtt->data[packetLen++] = identifier & 0xff;

            uint16_t topLength = strlen(strTopic);          //topic length
            if(strlen(strTopic) > 255){
                mqtt->data[packetLen++] = topLength;
            } else{
                mqtt->data[packetLen++] = 0;
            }
            mqtt->data[packetLen++] = topLength%256;
            for (i = 0; i < topLength; i++)
                mqtt->data[packetLen++] = strTopic[i];      //topic

            mqtt->data[0] = packetLen-1;
            setmqttPackFlag(1);
        }
}

void pingReqMqtt()
{
    mqttPacket* mqtt = (mqttPacket*)data;
    mqtt->typeFlags = MQTT_PINGREQ;                     //ping flag

    packetLen = 1;
    mqtt->data[0] = packetLen-1;
    setmqttPackFlag(1);

}
