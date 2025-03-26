// TCP Library (includes framework only)
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

#ifndef TCP_H_
#define TCP_H_

#include <stdint.h>
#include <stdbool.h>
#include "ip.h"
#include "mqtt.h"
#include "socket.h"

typedef struct _tcpHeader // 20 or more bytes
{
  uint16_t sourcePort;
  uint16_t destPort;
  uint32_t sequenceNumber;
  uint32_t acknowledgementNumber;
  uint16_t offsetFields;
  uint16_t windowSize;
  uint16_t checksum;
  uint16_t urgentPointer;
  uint8_t  data[0];
} tcpHeader;

typedef struct {
    etherHeader *ether;
    socket* s;
    uint16_t flags;
} sendTcpResponseParams;

typedef struct {
    etherHeader *ether;
    socket* s;
    uint16_t flags;
    uint8_t data[0];
    uint16_t dataSize;
} sendTcpMessageParams;

// TCP states
#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_RECEIVED 2
#define TCP_SYN_SENT 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT_1 5
#define TCP_FIN_WAIT_2 6
#define TCP_CLOSING 7
#define TCP_CLOSE_WAIT 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

// TCP offset/flags
#define FIN 0x0001
#define SYN 0x0002
#define RST 0x0004
#define PSH 0x0008
#define ACK 0x0010
#define URG 0x0020
#define ECE 0x0040
#define CWR 0x0080
#define NS  0x0100
#define OFS_SHIFT 12

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void setTcpState(uint8_t instance, uint8_t state);
uint8_t getTcpState(uint8_t instance);

bool isTcp(etherHeader *ether);
bool isTcpSyn(etherHeader *ether);
bool isTcpAck(etherHeader *ether);
bool isTcpFin(etherHeader *ether);
bool isTcpPsh(etherHeader *ether);
void writeTCPStatus(void);

void sendTcpPendingMessages(etherHeader *ether);
void processDhcpResponse(etherHeader *ether);
void processTcpResponse(etherHeader *ether);
void processTcpArpResponse(etherHeader *ether);

void setTcpPortList(uint16_t ports[], uint8_t count);
bool isTcpPortOpen(etherHeader *ether);
void sendTcpResponse(etherHeader *ether, socket* s, uint16_t flags);
void sendTcpMessage(etherHeader *ether, socket* s, uint16_t flags, uint8_t data[], uint16_t dataSize);

void sendTcpResponseWrapper(void* data);
void sendTcpMessageWrapper(void* data);
void closeConnection(void);
void setmqttPackFlag(uint8_t instance);
void setConStart(uint8_t instance);
void setConEnd(uint8_t instance);
void setMqttCon(uint8_t instance);
#endif

