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

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include "tcp.h"
#include "eth0.h"

typedef struct _mqttPacket
{
  uint8_t typeFlags;
  uint8_t data[0];
} mqttPacket;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void connectMqtt(void);
void disconnectMqtt(void);
void publishMqtt(char strTopic[], char strData[]);
void subscribeMqtt(char strTopic[]);
void unsubscribeMqtt(char strTopic[]);
void pingReqMqtt(void);
uint8_t* getMQTTPack(void);
uint16_t getMQTTPackLen(void);

#define MQTT_CONNECT        0x10
#define MQTT_CONNACK        0x20
#define MQTT_PUBLISH        0x30
#define MQTT_PUBACK         0x40
#define MQTT_PUBREC         0x50
#define MQTT_PUBREL         0x60
#define MQTT_PUBCOMP        0x70
#define MQTT_SUBSCRIBE      0x80
#define MQTT_SUBACK         0x90
#define MQTT_UNSUBSCRIBE    0xa0
#define MQTT_UNSUBACK       0xb0
#define MQTT_PINGREQ        0xc0
#define MQTT_PINGRESP       0xd0
#define MQTT_DISCONNECT     0xe0

#define MQTT_USER_NAME_FLAG         0x80
#define MQTT_PASSWORD_FLAG          0x40
#define MQTT_WILL_RETAIN            0x20
#define MQTT_WILL_QOS2              0x10
#define MQTT_WILL_QOS1              0x08
#define MQTT_WILL_QOS0              0x00
#define MQTT_WILL_FLAG              0x04
#define MQTT_CLEAN_SESSION          0x02
#define MQTT_RESERVED               0x01

#endif

