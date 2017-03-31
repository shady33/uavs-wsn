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
#include "net/rime/rime.h"
#include "powertrace.h"

#include <stdio.h> /* For printf() */

#define MAX_RETRANSMISSIONS 1

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/
static struct unicast_conn uc;
int is_drone = 0;
int is_coordinator = 0;
int send_data = 0;
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
{
  printf("unicast message received from %d.%d\n",
     from->u8[0], from->u8[1]);
  if(is_coordinator && from->u8[0]==10 && from->u8[1]==0){
    printf("Switching send data\n");
    send_data = 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c,  int status, int num_tx)
// sent_uc(struct unicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast messasge sent status:%d, transmission_num %d\n",
     status, num_tx);
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  // powertrace_start(CLOCK_SECOND * 2);
  unicast_open(&uc, 146, &unicast_callbacks);

  printf("Hello, Laksh\n");

  /* Receiver node: do nothing */
  if(linkaddr_node_addr.u8[0] == 5 &&
     linkaddr_node_addr.u8[1] == 0) {
    is_coordinator = 1;
  }
    
  if(linkaddr_node_addr.u8[0] == 10 &&
     linkaddr_node_addr.u8[1] == 0) {
    is_drone = 1;
  }

  int i = 0;
  char payload[10];

  while(1) {
    static struct etimer et;   
    linkaddr_t addr;

    if(is_drone && !is_coordinator){
        etimer_set(&et, CLOCK_SECOND * 5);
    }else if(!is_drone && is_coordinator){
        etimer_set(&et, CLOCK_SECOND);
    }else {
        etimer_set(&et, 30 * CLOCK_SECOND);
    }

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // if(!runicast_is_transmitting(&uc)) {
        if(is_drone && !is_coordinator) {
            memcpy(payload,"IAMDRONE",8);
            memcpy((payload+8),i,2);
            packetbuf_copyfrom(payload, 10);
            
            addr.u8[0] = 5;
            addr.u8[1] = 0;
            unicast_send(&uc, &addr);
            i++;
        } else if(!is_drone && is_coordinator){
            if(send_data){
                printf("Sending Data\n");
                packetbuf_copyfrom("DATA", 4);
                addr.u8[0] = 10;
                addr.u8[1] = 0;
                unicast_send(&uc, &addr);
                send_data = 0;
                i++;
            }
        }else{
            packetbuf_copyfrom("SENSOR", 6);
            addr.u8[0] = 5;
            addr.u8[1] = 0;
            unicast_send(&uc, &addr);
            i++;
        }
    }
  // }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
