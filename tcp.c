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



#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "arp.h"
#include "uart0.h"
#include "tcp.h"
#include "timer.h"
#include "gpio.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

#define BLUE_LED PORTF,2
#define MAX_TCP_PORTS 4
#define MAX_MQTT_TOPIC 2

char topic[64];
char message[64];
uint16_t tcpPorts[MAX_TCP_PORTS];
uint8_t tcpPortCount = 0;
uint8_t tcpState[MAX_TCP_PORTS];
uint8_t topicLen[MAX_MQTT_TOPIC];
uint8_t conStart = 0;
uint8_t conEnd = 0;
uint8_t synAck = 0;
uint8_t arpRev = 0;
uint8_t mqttConnect = 0;
uint8_t mqttPack = 0;
uint8_t mqttAck = 0;
uint8_t recFinAck = 0;

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Set TCP state

void setConStart(uint8_t instance)
{
    conStart = instance;
}

void setConEnd(uint8_t instance)
{
    conEnd = instance;
}

void setMqttCon(uint8_t instance)
{
    mqttConnect = instance;
}

void setmqttPackFlag(uint8_t instance)
{
    mqttPack = instance;
}

void setTcpState(uint8_t instance, uint8_t state)
{
    tcpState[instance] = state;
}

// Get TCP state
uint8_t getTcpState(uint8_t instance)
{
    return tcpState[instance];
}

void writeTCPStatus()
{
    uint8_t i;
    char str[20];
    putsUart0("  State: ");
    switch (getTcpState(TCP_CLOSED)) {
        case TCP_CLOSED:
            putsUart0("Closed");
            break;
        case TCP_SYN_SENT:
            putsUart0("Sync Sent");
            break;
        case TCP_ESTABLISHED:
            putsUart0("Established");
            break;
        case TCP_FIN_WAIT_1:
            putsUart0("Fin Wait");
            break;
        default:
            break;
    }
    putcUart0('\n');
    uint8_t ip[4];
    getIpAddress(ip);
    putsUart0("  IP:    ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf(str, sizeof(str), "%"PRIu8, ip[i]);
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    getIpMqttBrokerAddress(ip);
    putsUart0("  MQTT:  ");
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        snprintf( str, sizeof(str), "%"PRIu8, ip[i] );
        putsUart0(str);
        if (i < IP_ADD_LENGTH-1)
            putcUart0('.');
    }
    putcUart0('\n');
    if (mqttConnect)
        putsUart0("  MQTT is connected\n");
    else
        putsUart0("  MQTT is disconnected\n");
}

// Determines whether packet is TCP packet
// Must be an IP packet
bool isTcp(etherHeader* ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint16_t tcpLength;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    bool ok;
    uint16_t tmp16;
    uint32_t sum = 0;
    ok = (ip->protocol == PROTOCOL_TCP);                                //check if protocol is tcp
    if (ok)
    {
        // 32-bit sum over pseudo-header
        sumIpWords(ip->sourceIp, 8, &sum);                              //check checksum, sum over packet, and shift and add until < 0xffff, result should be 0
        tmp16 = ip->protocol;
        sum += (tmp16 & 0xff) << 8;

        tcpLength = htons(ntohs(ip->length) - ipHeaderLength);
        // add tcp header and data

        sumIpWords(&tcpLength, 2, &sum);


        sumIpWords(tcp, ntohs(tcpLength), &sum);
        ok = (getIpChecksum(sum) == 0);
    }
    return ok;
}

bool isTcpSyn(etherHeader *ether)
{

    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    return  ntohs(tcp->offsetFields) & SYN;

}

bool isTcpAck(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    return  ntohs(tcp->offsetFields) & ACK;
}

bool isTcpPsh(etherHeader *ether)
{

    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    return  ntohs(tcp->offsetFields) & PSH;

}

bool isTcpFin(etherHeader *ether)
{

    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    return  ntohs(tcp->offsetFields) & FIN;

}

void sendTcpPendingMessages(etherHeader *ether)
{
    socket* s = getSocket(0);
    if(getTcpState(0) == TCP_CLOSED){
        if(arpRev){                                                                                 //when arp is received, send syn
            arpRev = 0;
            sendTcpMessageParams StcpS;
            StcpS.ether = ether;
            StcpS.s = s;
            StcpS.flags = SYN;
            StcpS.dataSize = 0;
            sendTcpResponse(ether, s, SYN);
            sendTcpMessageWrapper(&StcpS);
            startPeriodicTimer(sendTcpMessageWrapper, &StcpS, 5);                                   //start timeout
            setTcpState(0, TCP_SYN_SENT);                                                           //set state to sync set
        } else if(conStart){
            conStart = 0;
            sendArpRequestParams SArpRP;
            SArpRP.ether = ether;
            getIpMqttBrokerAddress(SArpRP.ipTo);
            getIpAddress(SArpRP.ipFrom);
            sendArpRequestWrapper(&SArpRP);
            startPeriodicTimer(sendArpRequestWrapper, &SArpRP, 5);
        }
    } else if(getTcpState(0) == TCP_SYN_SENT && synAck){
        synAck = 0;
        sendTcpResponse(ether, s, ACK);                                                             //when synack is received, send ack
        setTcpState(0, TCP_ESTABLISHED);                                                            //set state to established
    } else if(getTcpState(0) == TCP_ESTABLISHED){
        if(mqttPack){
            mqttPack = 0;                                                                           //send mqtt packet
            sendTcpMessage(ether, s, PSH | ACK, getMQTTPack(), getMQTTPackLen()+1);
        } else if(mqttAck){
            mqttAck = 0;                                                                            //send ack in response to mqtt ack
            sendTcpResponse(ether, s, ACK);
        } else if(conEnd){
            conEnd = 0;                                                                             //send finack to end connection
            sendTcpResponseParams StcpF;
            StcpF.ether = ether;
            StcpF.s = s;
            StcpF.flags = FIN | ACK;
            sendTcpMessage(ether, s, FIN | ACK, NULL, 0);
            startPeriodicTimer(sendTcpResponseWrapper, &StcpF, 1);
            setTcpState(0, TCP_FIN_WAIT_1);                                                         //set state to fin wait
        }
    }
    if(recFinAck){
        recFinAck = 0;
        sendTcpResponse(ether, s, ACK);
        if(getTcpState(0) == TCP_FIN_WAIT_1){
            closeConnection();
        } else{
            setTcpState(0, TCP_LAST_ACK);
            sendTcpMessage(ether, s, FIN | ACK, NULL, 0);
            startOneshotTimer(closeConnection, NULL, 5);
        }
    }
}

void closeConnection(void){
    stopTimer(pingReqMqtt);                                                                         //turn off timers
    stopTimer(sendTcpResponseWrapper);
    mqttConnect = 0;
    setTcpState(0, TCP_CLOSED);                                                                     //set state to established
}

void processTcpResponse(etherHeader *ether)
{
    uint8_t i;
    uint8_t j;
    uint8_t mqttLength;
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + (ip->size * 4));
    uint8_t ipHeaderLength = ip->size * 4;
    uint16_t tcpLength = ntohs(ip->length) - ipHeaderLength;
    mqttPacket* mqtt = (mqttPacket*)(tcp->data);
    socket* s = getSocket(0);
    uint16_t topicLength;
    if(isTcpAck(ether)){
        if(isTcpFin(ether)){
            recFinAck = 1;                                                                          //receiving finack - end connection
            s->sequenceNumber = htonl(tcp->acknowledgementNumber);
            s->acknowledgementNumber =  htonl(tcp->sequenceNumber)+1;
        } else if(getTcpState(0) == TCP_SYN_SENT & isTcpSyn(ether)){                                //receiving synack - set to established
            stopTimer(sendTcpMessageWrapper);

            s->sequenceNumber = htonl(tcp->acknowledgementNumber);
            s->acknowledgementNumber =  htonl(tcp->sequenceNumber)+1;
            synAck = 1;
        } else if(getTcpState(0) == TCP_ESTABLISHED){
            if(isTcpPsh(ether)){                                                                    //receiving pshack - send ack back/process published message
                mqttAck = 1;
                mqttLength = mqtt->data[0] + 2;
                s->sequenceNumber = htonl(tcp->acknowledgementNumber);                              //new seq = rec ack
                s->acknowledgementNumber =  htonl(tcp->sequenceNumber) + mqttLength;                //new ack = rec seq + datalength
                if(mqtt->typeFlags == MQTT_PUBLISH){                                                //publish messages
                    mqttLength-=1;
                    j = 1;
                    topicLength = mqtt->data[j++];
                    topicLength = (topicLength << 4) | mqtt->data[j++];                             //extract topic
                    for(i = 0; i < topicLength; i++){
                        topic[i] = mqtt->data[j++];
                    }
                    topic[i] = 0;
                    for(i = 0; j < mqttLength; i++){                                                //extract message
                        message[i] = mqtt->data[j++];
                    }
                    message[i] = 0;
                    if (strcmp(topic, "project1_blue_led") == 0){                                   //process topics and messages
                        if(strcmp(message, "on") == 0){
                            setPinValue(BLUE_LED, 1);

                        }
                        if(strcmp(message, "off") == 0){
                            setPinValue(BLUE_LED, 0);
                        }
                    }
                    if (strcmp(topic, "project1_message") == 0){
                        for(i = 0; i < mqttLength - 3 - topicLength; i++){
                            putcUart0(message[i]);
                        }
                        putcUart0('\n');
                    }
                }
            } else if(getTcpState(0) == TCP_LAST_ACK){
                closeConnection();
            } else {
                s->sequenceNumber = htonl(tcp->acknowledgementNumber);
                s->acknowledgementNumber =  htonl(tcp->sequenceNumber);
            }
        }
    } else if(isTcpFin(ether)){                                                                     //respond to fin
        s->sequenceNumber = htonl(tcp->acknowledgementNumber);
        s->acknowledgementNumber =  htonl(tcp->sequenceNumber)+1;
        closeConnection();
        sendTcpResponse(ether, s, FIN | ACK);

    }
}

void processTcpArpResponse(etherHeader *ether)
{
    arpPacket* arp = (arpPacket*)ether->data;
    uint8_t check = 1;
    uint8_t i;
    uint8_t mqttIP[4];

    getIpMqttBrokerAddress(mqttIP);                                     //check is arp is from mqtt
    for (i = 0; i < IP_ADD_LENGTH; i++){
        if(arp->sourceIp[i] != mqttIP[i]){
            check = 0;
        }
    }
    if(check & getTcpState(0) == TCP_CLOSED){
        stopTimer(sendArpRequestWrapper);                               //stop sending arps

        socket* s = newSocket();                                        //build socket
        getSocketInfoFromArpResponse(ether, s);
        s->localPort= random32()%10000 + 50000;
        uint16_t ports[] = {s->localPort};
        setTcpPortList(ports, 1);
        s->remotePort = 1883;
        s->sequenceNumber = random32();
        setSocket(0, s);

        arpRev = 1;
    }

}

void setTcpPortList(uint16_t ports[], uint8_t count)
{
    uint8_t i;
    if(count < MAX_TCP_PORTS){
        for(i = 0; i < count; i++){
            tcpPorts[i] = ports[i];
        }
        tcpPortCount = count;
    }
}

bool isTcpPortOpen(etherHeader *ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    uint8_t i;
    for(i = 0; i < tcpPortCount; i++){                                  //check if tcp message is sent to open port
        if(tcpPorts[i] == ntohs(tcp->destPort)){
            return true;
        }
    }
    return false;
}

void sendTcpResponseWrapper(void* data)
{
    sendTcpResponseParams* params = (sendTcpResponseParams*)data;
    sendTcpResponse(params->ether, params->s, params->flags);
}

void sendTcpMessageWrapper(void* data)
{
    sendTcpMessageParams* params = (sendTcpMessageParams*)data;
    sendTcpMessage(params->ether, params->s, params->flags, params->data, params->dataSize);
}

void sendTcpResponse(etherHeader *ether, socket* s, uint16_t flags)
{
    uint8_t i;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];

    // Ether frame
    getEtherMacAddress(localHwAddress);
    getIpAddress(localIpAddress);
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        ether->destAddress[i] = s->remoteHwAddress[i];
        ether->sourceAddress[i] = localHwAddress[i];
    }
    ether->frameType = htons(TYPE_IP);

    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->rev = 0x4;
    ip->size = 0x5;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = PROTOCOL_TCP;
    ip->headerChecksum = 0;
     for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        ip->destIp[i] = s->remoteIpAddress[i];
        ip->sourceIp[i] = localIpAddress[i];
    }
    uint8_t ipHeaderLength = ip->size * 4;

    // TCP header
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + (ip->size * 4));
    tcp->sourcePort = htons(s->localPort);
    tcp->destPort = htons(s->remotePort);

    // adjust lengths
    tcpLength = sizeof(tcpHeader);
    uint16_t tcpLengthBE = htons(tcpLength);
    ip->length = htons(ipHeaderLength + tcpLength);
    calcIpChecksum(ip);

    tcp->sequenceNumber = htonl(s->sequenceNumber);
    tcp->acknowledgementNumber = htonl(s->acknowledgementNumber);
    tcp->offsetFields = htons(sizeof(tcpHeader)/4  << 12| (flags));
    tcp->windowSize = htons(1500);
    tcp->urgentPointer = 0;

    // 32-bit sum over pseudo-header
    sum = 0;
    sumIpWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    sumIpWords(&tcpLengthBE, 2, &sum);

    // add tcp header
    tcp->checksum = 0;
    sumIpWords(tcp, tcpLength, &sum);
    tcp->checksum = getIpChecksum(sum);

    // send packet with size = ether + tcp hdr + ip header + tcp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);
}

// Send TCP message
void sendTcpMessage(etherHeader *ether, socket *s, uint16_t flags, uint8_t data[], uint16_t dataSize)
{
    uint8_t i;
    uint16_t j;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength;
    uint8_t *copyData;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];

    // Ether frame
    getEtherMacAddress(localHwAddress);
    getIpAddress(localIpAddress);
    for (i = 0; i < HW_ADD_LENGTH; i++)                                     //hardware address = socket hardware address
    {
        ether->destAddress[i] = s->remoteHwAddress[i];
        ether->sourceAddress[i] = localHwAddress[i];
    }
    ether->frameType = htons(TYPE_IP);

    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->rev = 0x4;
    ip->size = 0x5;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = PROTOCOL_TCP;
    ip->headerChecksum = 0;
     for (i = 0; i < IP_ADD_LENGTH; i++)
    {                                                                       //ip address = socket ip address
        ip->destIp[i] = s->remoteIpAddress[i];
        ip->sourceIp[i] = localIpAddress[i];
    }
    uint8_t ipHeaderLength = ip->size * 4;

    // TCP header
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + (ip->size * 4));
    tcp->sourcePort = htons(s->localPort);
    tcp->destPort = htons(s->remotePort);

    // adjust lengths
    tcpLength = sizeof(tcpHeader) + dataSize;
    uint16_t tcpLengthBE = htons(tcpLength);
    ip->length = htons(ipHeaderLength + tcpLength);
    calcIpChecksum(ip);

    tcp->sequenceNumber = htonl(s->sequenceNumber);                         //set seq and ack
    tcp->acknowledgementNumber = htonl(s->acknowledgementNumber);
    tcp->offsetFields = htons(sizeof(tcpHeader)/4  << 12| (flags));         //set size of header and flags
    tcp->windowSize = htons(1500);
    tcp->urgentPointer = 0;

    // copy data
    copyData = tcp->data;                                                   //copy packet into tcp
    for (j = 0; j < dataSize; j++)
        copyData[j] = data[j];

    // 32-bit sum over pseudo-header
    sum = 0;                                                                //calculate checksum
    sumIpWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    sumIpWords(&tcpLengthBE, 2, &sum);

    // add tcp header
    tcp->checksum = 0;
    sumIpWords(tcp, tcpLength, &sum);
    tcp->checksum = getIpChecksum(sum);

    // send packet with size = ether + tcp hdr + ip header + tcp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);        //send packet
}
