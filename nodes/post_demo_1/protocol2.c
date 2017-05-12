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

struct coord{
    uint8_t first;
    uint8_t second;
};
struct coord coords[] = {{0xEC,0xB6},{0x9E,0xC8}};

struct packet{
    uint64_t data5;
    uint16_t req_reply;
    uint16_t sequenceno;
    uint16_t totalpackets;
    uint16_t droppedpackets;
    uint64_t data3;
    uint64_t data4;
}payload;
int is_drone = 0;
int is_coordinator = 0;
int is_sensor = 0;

#define DRONE0 0x9F
#define DRONE1 0x58
#define NUMRECORDS sizeof(coords) / sizeof(struct coord)
#define MAX_RETRANSMISSIONS 31
#define DRONE_PRIMARY_CHANNEL 25
#define DRONE_STUBBORN_TIME CLOCK_SECOND
#define POWER_NODES 7
/*---------------------------------------------------------------------------*/
PROCESS(main_process, "main process");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/
static void
recv_stbroadcast_drone(struct stbroadcast_conn *c)
{
  printf("%lu:stbroadcast message received RSSI:%d\n",
	 clock_time(),(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));
  leds_toggle(LEDS_YELLOW);
}
static void
sent_stbroadcast_drone(struct stbroadcast_conn *c)
{
  printf("stbroadcast message sent\n");
  leds_toggle(LEDS_GREEN);
}
static const struct stbroadcast_callbacks stbroadcast_drone_callbacks = {recv_stbroadcast_drone,
							     sent_stbroadcast_drone};
static struct stbroadcast_conn stbroadcast_drone;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_EXITHANDLER(stbroadcast_close(&stbroadcast_drone);)

  PROCESS_BEGIN();

  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,26);
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,7);
  
  /* Receiver node: do nothing */
  if(linkaddr_node_addr.u8[0] == 1 &&
     linkaddr_node_addr.u8[1] == 0) {
    PROCESS_WAIT_EVENT_UNTIL(0);
  } 

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
    leds_on(LEDS_GREEN);
  }

  if(!is_drone && !is_coordinator){
    is_sensor = 1;
    // leds_on(LEDS_ORANGE);
  }

  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,POWER_NODES);

  if(is_drone){
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,DRONE_PRIMARY_CHANNEL);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
  }else if(is_coordinator){
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
  }else if(is_sensor){
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
