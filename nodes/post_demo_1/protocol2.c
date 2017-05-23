/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Reliable single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/rime/stbroadcast.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"

#define NUMRECORDS sizeof(coords) / sizeof(struct coord)

/* Timings */
#define MAX_RETRANSMISSIONS 31
#define DRONE_STUBBORN_TIME CLOCK_SECOND
#define WAITFORALLPACKETS   5
#define BACKOFF_TIME 10
#define POWER_NODES -7

/* Number of packets to send */
#define NUM_PACKETS 1
#define PACKET_CHECKER 0x2

/* Packet Types */
#define REQUEST 1
#define REPLY   2
#define ACCEPT  3
#define DATA    4
#define BACKOFF 5
#define REQSEQ  6
#define REPSEQ  7

/* Rime addresses */
#define DRONE0 0x9E
#define DRONE1 0xC8
struct coord{
    uint8_t first;
    uint8_t second;
};
struct coord coords[] = {{0xEC,0xB6},{0x9F,0x7F},{0xE9,0x94}};
// struct coord coords[] = {{0x2,0x0},{0x3,0x0}};

struct packet{
    uint16_t data5;
    uint16_t req_reply;
    uint16_t sequenceno;
    uint16_t totalpackets;
    uint64_t droppedpackets;
    uint64_t data3;
    uint64_t data4;
}payload;
volatile int sending = 0;
int is_drone = 0;
int is_coordinator = 0;
int is_sensor = 0;
static struct stbroadcast_conn stbroadcast_drone;
static struct runicast_conn runicast_drone;
static struct unicast_conn unicast_drone;
volatile uint16_t num_packets = NUM_PACKETS;
volatile uint64_t received_seq = 0;
static struct ctimer timer;
volatile linkaddr_t current_recv;
volatile int allowed_to_send = 1;
/*---------------------------------------------------------------------------*/
PROCESS(send_C_D, "Send Coordinator to Drone process");
PROCESS(send_backoff, "Send Backoff");
PROCESS(main_process, "main process");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/
static void change_send(void *ptr){
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_2);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_2);
    process_start(&send_C_D,NULL);
}
/*---------------------------------------------------------------------------*/
static void backoff(void *ptr)
{
    PRINTF("Backoff Timer\n");
    ctimer_stop(&timer);
    allowed_to_send = 1;
    num_packets = NUM_PACKETS;
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    leds_off(LEDS_YELLOW);
}
/*---------------------------------------------------------------------------*/
static void callback(void *ptr)
{
    PRINTF("Callback timer\n");
   ctimer_stop(&timer);
   // received_seq = received_seq ^ PACKET_CHECKER;
   if((received_seq ^ PACKET_CHECKER) == 0){
    process_start(&send_backoff,NULL);
   }else{
    /* some dropped */
    payload.req_reply = REQSEQ;
    payload.droppedpackets = received_seq ^ PACKET_CHECKER;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(runicast_send(&runicast_drone,&current_recv,MAX_RETRANSMISSIONS) != 0){
        PRINTF("Requested new packets\n");
    }
   }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_backoff,ev,data){
    PROCESS_BEGIN();
    PRINTF("%lu:Send Backoff\n",clock_time());
    payload.req_reply = BACKOFF;
    packetbuf_copyfrom(&payload,sizeof(payload));
    runicast_send(&runicast_drone,&current_recv,MAX_RETRANSMISSIONS);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_C_D,ev,data){
  PROCESS_BEGIN();

    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;

  if(data == NULL){
    PRINTF("Num Packets to send:%d\n", num_packets);

    while(num_packets != 0){
        PRINTF("Sending Data %d\n",num_packets);
        payload.req_reply = DATA;
        payload.sequenceno = num_packets;
        payload.totalpackets = NUM_PACKETS;
        packetbuf_copyfrom(&payload, sizeof(payload));
        sending = 1;
        if(unicast_send(&unicast_drone,&addr) != 0){
            PRINTF("Normal Data: %d\n",num_packets);
            num_packets = num_packets - 1;
        }
        leds_toggle(LEDS_GREEN);
    }
  }else{
    PRINTF("Packets to send:%04x\n",(uint16_t)data);
    int bit = 0;
    for(bit = 0; bit < 16; bit++){
        if ((uint16_t)data & (1 << bit)) {
           PRINTF("Sending packet: %d\n",bit);
            payload.req_reply = REPSEQ;
            payload.sequenceno = bit;
            payload.totalpackets = NUM_PACKETS;
            packetbuf_copyfrom(&payload, sizeof(payload));
            sending = 1;
            if(unicast_send(&unicast_drone,&addr) != 0){
                PRINTF("Normal Data: %d\n",bit);
            }
            leds_toggle(LEDS_GREEN);
        }
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_unicast_drone(struct unicast_conn *c, const linkaddr_t *from)
{
    if(runicast_is_transmitting(&runicast_drone)){
        runicast_cancel(&runicast_drone);
    }

  packetbuf_copyto(&payload);
  PRINTF("%lu:unicast message received from %d.%d, RSSI:%d Packetno:%d\n",
   clock_time(),from->u8[0], from->u8[1],(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload.sequenceno);
  linkaddr_copy(&current_recv,from);
  received_seq = received_seq | (1 << payload.sequenceno);

  if(received_seq == (1 << payload.sequenceno)){
    /* Start timer */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * WAITFORALLPACKETS, callback, NULL);
  }

  PRINTF("%x,%x\n",received_seq,(1 << payload.sequenceno));

  if(!(received_seq ^ PACKET_CHECKER)){
    received_seq = 0;
    ctimer_stop(&timer);
    process_start(&send_backoff,NULL);
  }
}
static void
sent_unicast_drone(struct unicast_conn *c, int status, int num_tx)
{
  PRINTF("unicast message sent status %d\n",status);
  sending = 0;
}
static const struct unicast_callbacks unicast_drone_callbacks = {recv_unicast_drone,
                   sent_unicast_drone};
/*---------------------------------------------------------------------------*/
static void
recv_runicast_drone(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  packetbuf_copyto(&payload);
  PRINTF("%lu:runicast message received from %d.%d, seqno %d RSSI:%d Packet type:%d\n",
   clock_time(),from->u8[0], from->u8[1], seqno,(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload.req_reply);

  if(is_drone && payload.req_reply == REPLY){
    stbroadcast_cancel(&stbroadcast_drone);
    leds_off(LEDS_GREEN);
    PRINTF("%lu:Sending Runicast message to %d.%d\n",clock_time(), from->u8[0],from->u8[1]);
    payload.req_reply = ACCEPT;
    packetbuf_copyfrom(&payload,sizeof(payload));
    runicast_send(&runicast_drone,from,MAX_RETRANSMISSIONS);
    leds_on(LEDS_YELLOW);
  }else if(is_coordinator && payload.req_reply == ACCEPT){
    if(runicast_is_transmitting(c)){
        runicast_cancel(c);
    }
    leds_on(LEDS_GREEN);
    /* Switch channels */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * 2, change_send, NULL);
  }else if(is_coordinator && payload.req_reply == REQSEQ){
    process_start(&send_C_D,(void *)payload.droppedpackets);
  }else if(is_coordinator && payload.req_reply == BACKOFF){
    /* Start backoff timer */
    leds_on(LEDS_YELLOW);
    allowed_to_send = 0;
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * BACKOFF_TIME, backoff, NULL);
  }
}
static void
sent_runicast_drone(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
    sending = 0;
  PRINTF("%lu:runicast message sent to %d.%d, retransmissions %d\n",
   clock_time(),to->u8[0], to->u8[1], retransmissions);

  if(is_drone){
    /* Sent ACCEPT and BACKOFF packet */
    /* Switch channels? */
    if(payload.req_reply == ACCEPT){
        PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_2);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_2);
        leds_off(LEDS_YELLOW);
    }else if(payload.req_reply == BACKOFF){
        PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
        payload.req_reply = REQUEST;
        packetbuf_copyfrom(&payload,sizeof(payload));
        stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
        leds_on(LEDS_GREEN);
    }
  }else if(is_coordinator){
    /* Sent REPLY packet */
  }
}
static void
timedout_runicast_drone(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  PRINTF("%lu:runicast message timed out when sending to %d.%d, retransmissions %d\n",
   clock_time(),to->u8[0], to->u8[1], retransmissions);
  if(is_drone){
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    payload.req_reply = REQUEST;
    packetbuf_copyfrom(&payload,sizeof(payload));
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
    leds_on(LEDS_GREEN);
  }
}
static const struct runicast_callbacks runicast_drone_callbacks = {recv_runicast_drone,
                   sent_runicast_drone,
                   timedout_runicast_drone};
/*---------------------------------------------------------------------------*/
static void
recv_stbroadcast_drone(struct stbroadcast_conn *c)
{
  PRINTF("%lu:stbroadcast message received RSSI:%d\n",
     clock_time(),(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));

  if(allowed_to_send){
    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;
    payload.req_reply = REPLY;
    packetbuf_copyfrom(&payload,sizeof(payload));
    PRINTF("Sending Runicast to Drone\n");
    runicast_send(&runicast_drone,&addr,MAX_RETRANSMISSIONS);  
  }
}
static void
sent_stbroadcast_drone(struct stbroadcast_conn *c)
{
    sending = 0;
  PRINTF("stbroadcast message sent\n");
}
static const struct stbroadcast_callbacks stbroadcast_drone_callbacks = {recv_stbroadcast_drone,
                                 sent_stbroadcast_drone};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_EXITHANDLER(stbroadcast_close(&stbroadcast_drone);)
  PROCESS_EXITHANDLER(runicast_close(&runicast_drone);)
  PROCESS_EXITHANDLER(unicast_close(&unicast_drone);)

  PROCESS_BEGIN();

  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,POWER_NODES);

  /* Receiver node: do nothing */
  int j;
  for(j = 0 ; j < NUMRECORDS; j++){
    if((linkaddr_node_addr.u8[0] == coords[j].first) &&
     (linkaddr_node_addr.u8[1] == coords[j].second)) {
        is_coordinator = 1;
        leds_on(LEDS_RED);
    }
  }
  
  if(linkaddr_node_addr.u8[0] == DRONE0 &&
     linkaddr_node_addr.u8[1] == DRONE1) {
    is_drone = 1;
    // leds_on(LEDS_ORANGE);
  }

  if(!is_drone && !is_coordinator){
    is_sensor = 1;
    // leds_on(LEDS_ORANGE);
  }

  if(is_drone){
    PRINTF("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    runicast_open(&runicast_drone, 144, &runicast_drone_callbacks);
    unicast_open(&unicast_drone, 140, &unicast_drone_callbacks);
    payload.req_reply = REQUEST;
    packetbuf_copyfrom(&payload,sizeof(payload));
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
    leds_on(LEDS_GREEN);
  }else if(is_coordinator){
    PRINTF("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    runicast_open(&runicast_drone, 144, &runicast_drone_callbacks);
    unicast_open(&unicast_drone, 140, &unicast_drone_callbacks);
  }else if(is_sensor){
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
