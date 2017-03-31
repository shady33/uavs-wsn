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
// #include "powertrace.h"
#include "net/netstack.h"
#include "random.h"
#include "dev/leds.h"

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(power_profile_process, "Power profile process");
AUTOSTART_PROCESSES(&power_profile_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(power_profile_process, ev, data)
{

  static struct etimer et;
  char payload[5];
  memset(payload, 5, sizeof(char)*4);

  // static unsigned long last_cpu, last_lpm, last_transmit, last_listen;
  // static unsigned long last_idle_transmit, last_idle_listen;

  // unsigned long cpu, lpm, transmit, listen;
  // unsigned long all_cpu, all_lpm, all_transmit, all_listen;
  // unsigned long idle_transmit, idle_listen;
  // unsigned long all_idle_transmit, all_idle_listen;

  PROCESS_BEGIN();
  
  // powertrace_start(CLOCK_SECOND * 30);
  
  printf("Hello, world\n");

  NETSTACK_MAC.off(0);
  leds_on(LEDS_YELLOW);

  etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  NETSTACK_RADIO.send(payload,5);
  leds_off(LEDS_YELLOW);
  leds_on(LEDS_ORANGE);

  etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  NETSTACK_RADIO.channel_clear();
  leds_off(LEDS_ORANGE);
  leds_on(LEDS_GREEN);

  etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  NETSTACK_MAC.off(0);
  leds_off(LEDS_GREEN);
  
  // while(1){
  //   printf("Hello, world\n");
  //   leds_on(LEDS_YELLOW);
  //   NETSTACK_RADIO.channel_clear();
  //   etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
  //   PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  //   leds_off(LEDS_YELLOW);
  //   leds_on(LEDS_ALL);
  //   NETSTACK_RADIO.off();
  //   etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));
  //   PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  //   leds_off(LEDS_ALL);
  //   NETSTACK_RADIO.on();
  //   etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));
  //   PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
       // energest_flush();

      // all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      // all_lpm = energest_type_time(ENERGEST_TYPE_LPM);
      // all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      // all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

      // cpu = all_cpu - last_cpu;
      // lpm = all_lpm - last_lpm;
      // transmit = all_transmit - last_transmit;
      // listen = all_listen - last_listen;

      // last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
      // last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
      // last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      // last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

      // printf("%lu P %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
      //        clock_time(), all_cpu, all_lpm, all_transmit, all_listen, all_idle_transmit, all_idle_listen,
      //        cpu, lpm, transmit, listen, idle_transmit, idle_listen);

      // etimer_set(&et, CLOCK_SECOND * 30 + random_rand() % (CLOCK_SECOND * 30));
      // PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  // }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
