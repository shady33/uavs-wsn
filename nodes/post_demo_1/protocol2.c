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

#include <stdio.h>

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/rime/stbroadcast.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"

#define DRONE0 0x9E
#define DRONE1 0xC8
#define NUMRECORDS sizeof(coords) / sizeof(struct coord)
#define MAX_RETRANSMISSIONS 31
#define DRONE_STUBBORN_TIME CLOCK_SECOND
#define POWER_NODES 7
#define NUM_PACKETS 2
#define PACKET_CHECKER 0x0006
#define REQUEST 1
#define REPLY   2
#define ACCEPT  3
#define DATA    4
#define BACKOFF 5
#define BACKOFF_TIME 10

struct coord{
    uint8_t first;
    uint8_t second;
};
struct coord coords[] = {{0xEC,0xB6}};
// struct coord coords[] = {{0x2,0x0},{0x3,0x0}};

struct packet{
    uint64_t data5;
    uint16_t req_reply;
    uint16_t sequenceno;
    uint16_t totalpackets;
    uint16_t droppedpackets;
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
volatile int num_packets = NUM_PACKETS;
volatile uint16_t received_seq = 0;
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
    printf("Change Channel to %d\n",FFD_CHANNEL_2);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_2);
    process_start(&send_C_D,NULL);
}
/*---------------------------------------------------------------------------*/
static void backoff(void *ptr)
{
    printf("Backoff Timer\n");
    allowed_to_send = 1;
    num_packets = NUM_PACKETS;
    printf("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
}
/*---------------------------------------------------------------------------*/
static void callback(void *ptr)
{
    printf("Callback timer\n");
   ctimer_stop(&timer);
   received_seq = received_seq ^ PACKET_CHECKER;
   if(received_seq == 0){
    process_start(&send_backoff,NULL);
   }else{
    /* some dropped */
   }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_backoff,ev,data){
    PROCESS_BEGIN();
    printf("Send Backoff\n");
    payload.req_reply = BACKOFF;
    packetbuf_copyfrom(&payload,sizeof(payload));
    runicast_send(&runicast_drone,&current_recv,MAX_RETRANSMISSIONS);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_C_D,ev,data){
  PROCESS_BEGIN();

  printf("Num Packets to send:%d\n", num_packets);
  linkaddr_t addr;
  addr.u8[0] = DRONE0;
  addr.u8[1] = DRONE1;

  while(num_packets != 0){
    printf("Sending Data %d\n",num_packets);
    payload.req_reply = DATA;
    payload.sequenceno = num_packets;
    payload.totalpackets = NUM_PACKETS;
    packetbuf_copyfrom(&payload, sizeof(payload));
    sending = 1;
    if(unicast_send(&unicast_drone,&addr) != 0){
        printf("Normal Data: %d\n",num_packets);
        num_packets = num_packets - 1;
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_unicast_drone(struct unicast_conn *c, const linkaddr_t *from)
{
  packetbuf_copyto(&payload);
  printf("%lu:unicast message received from %d.%d, RSSI:%d Packetno:%d\n",
   clock_time(),from->u8[0], from->u8[1],(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload.sequenceno);
  linkaddr_copy(&current_recv,from);
  received_seq = received_seq | (1 << payload.sequenceno);

  if(received_seq == (1 << payload.sequenceno)){
    /* Start timer */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * 5, callback, NULL);
  }

  if(!(received_seq ^ PACKET_CHECKER)){
    received_seq = 0;
    ctimer_stop(&timer);
    process_start(&send_backoff,NULL);
  }
}
static void
sent_unicast_drone(struct unicast_conn *c, int status, int num_tx)
{
  printf("unicast message sent status %d\n",status);
  sending = 0;
}
static const struct unicast_callbacks unicast_drone_callbacks = {recv_unicast_drone,
                   sent_unicast_drone};
/*---------------------------------------------------------------------------*/
static void
recv_runicast_drone(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  packetbuf_copyto(&payload);
  printf("%lu:runicast message received from %d.%d, seqno %d RSSI:%d Packet type:%d\n",
   clock_time(),from->u8[0], from->u8[1], seqno,(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload.req_reply);

  if(is_drone && payload.req_reply == REPLY){
    stbroadcast_cancel(&stbroadcast_drone);
    printf("Sending Runicast message to %d.%d\n", from->u8[0],from->u8[1]);
    payload.req_reply = ACCEPT;
    packetbuf_copyfrom(&payload,sizeof(payload));
    runicast_send(&runicast_drone,from,MAX_RETRANSMISSIONS);
  }else if(is_coordinator && payload.req_reply == ACCEPT){
    if(runicast_is_transmitting(c)){
        runicast_cancel(c);
    }
    /* Switch channels */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * 2, change_send, NULL);
  }else if(is_coordinator && payload.req_reply == BACKOFF){
    /* Start backoff timer */
    allowed_to_send = 0;
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * BACKOFF_TIME, backoff, NULL);
  }
}
static void
sent_runicast_drone(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
    sending = 0;
  printf("runicast message sent to %d.%d, retransmissions %d\n",
   to->u8[0], to->u8[1], retransmissions);

  if(is_drone){
    /* Sent ACCEPT and BACKOFF packet */
    /* Switch channels? */
    if(payload.req_reply == ACCEPT){
        printf("Change Channel to %d\n",FFD_CHANNEL_2);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_2);
    }else if(payload.req_reply == BACKOFF){
        printf("Change Channel to %d\n",FFD_CHANNEL_1);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
        payload.req_reply = REQUEST;
        packetbuf_copyfrom(&payload,sizeof(payload));
        stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
    }
  }else if(is_coordinator){
    /* Sent REPLY packet */
  }
}
static void
timedout_runicast_drone(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
   to->u8[0], to->u8[1], retransmissions);
}
static const struct runicast_callbacks runicast_drone_callbacks = {recv_runicast_drone,
                   sent_runicast_drone,
                   timedout_runicast_drone};
/*---------------------------------------------------------------------------*/
static void
recv_stbroadcast_drone(struct stbroadcast_conn *c)
{
  printf("%lu:stbroadcast message received RSSI:%d\n",
     clock_time(),(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));

  if(allowed_to_send){
    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;
    payload.req_reply = REPLY;
    packetbuf_copyfrom(&payload,sizeof(payload));
    runicast_send(&runicast_drone,&addr,MAX_RETRANSMISSIONS);  
  }
}
static void
sent_stbroadcast_drone(struct stbroadcast_conn *c)
{
    sending = 0;
  printf("stbroadcast message sent\n");
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
    leds_on(LEDS_ORANGE);
  }

  if(!is_drone && !is_coordinator){
    is_sensor = 1;
    // leds_on(LEDS_ORANGE);
  }

  if(is_drone){
    printf("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    runicast_open(&runicast_drone, 144, &runicast_drone_callbacks);
    unicast_open(&unicast_drone, 140, &unicast_drone_callbacks);
    payload.req_reply = REQUEST;
    packetbuf_copyfrom(&payload,sizeof(payload));
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
  }else if(is_coordinator){
    printf("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    runicast_open(&runicast_drone, 144, &runicast_drone_callbacks);
    unicast_open(&unicast_drone, 140, &unicast_drone_callbacks);
  }else if(is_sensor){
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
