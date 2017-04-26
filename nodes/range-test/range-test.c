/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/netstack.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "net/rime/rime.h"

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(range_test_process, "Power profile process");
AUTOSTART_PROCESSES(&range_test_process);
volatile int i = 0;
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: RSSI:%d\n",
         from->u8[0], from->u8[1], (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
/*---------------------------------------------------------------------------*/
static struct broadcast_conn uc;
static int powers[] = {
    7,5,3,1,0,-1,-3,-5,-7,-9,-11,-13,-15,-24
};
static int total = 14;

void set_leds(int value){
  leds_off(LEDS_ALL);
  switch(value){
    case 0:
      break;
    case 1:
      leds_on(LEDS_RED);
      break;
    case 2:
      leds_on(LEDS_ORANGE);
      break;
    case 3:
      leds_on(LEDS_RED);
      leds_on(LEDS_ORANGE);
      break;
    case 4:
      leds_on(LEDS_YELLOW);
      break;
    case 5:
      leds_on(LEDS_YELLOW);
      leds_on(LEDS_RED);
      break;
    case 6:
      leds_on(LEDS_YELLOW);
      leds_on(LEDS_ORANGE);
      break;
    case 7:
      leds_on(LEDS_YELLOW);
      leds_on(LEDS_ORANGE);
      leds_on(LEDS_RED);
      break;
    case 8:
      leds_on(LEDS_GREEN);
      break;
    case 9:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_RED);
      break;
    case 10:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_ORANGE);
      break;
    case 11:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_ORANGE);
      leds_on(LEDS_RED);
      break;
    case 12:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_YELLOW);
      break;
    case 13:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_YELLOW);
      leds_on(LEDS_RED);
      break;
    case 14:
      leds_on(LEDS_GREEN);
      leds_on(LEDS_YELLOW);
      leds_on(LEDS_ORANGE);
      break;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(range_test_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&uc);)
  PROCESS_BEGIN();

  static struct etimer et;
  char payload[5];
  memset(payload, 5, sizeof(char)*5);
  
  SENSORS_ACTIVATE(button_sensor);
  
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,powers[i]);
  leds_set(8);

  broadcast_open(&uc, 137, &broadcast_call);
  while(1){

    etimer_set(&et, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    packetbuf_copyfrom(payload, 5);
    broadcast_send(&uc);

    if(ev == sensors_event) {
      if(data == &button_sensor) {
        if(button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
           BUTTON_SENSOR_PRESSED_LEVEL) {
          i = i + 1;
          if(i == total){
            i = 0;
          }
          printf("Changing Value %d\n",i);
          NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,powers[i]);
          set_leds(i);
        }
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
