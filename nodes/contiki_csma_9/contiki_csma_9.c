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
static struct unicast_conn uc_drone;
int is_drone = 0;
int is_coordinator = 0;
int send_data = 0;
volatile int sending = 0;
volatile int i = 0;
volatile int num_packets = 10;
char payload[10];
volatile int allowed_to_send = 1;
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
{
  int d = packetbuf_copyto(payload);
    // int d = 0;
  printf("unicast message received from %d.%d length %d\n",
     from->u8[0], from->u8[1],d);
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c,  int status, int num_tx)
// sent_uc(struct unicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast messasge sent status:%d, transmission_num %d\n",
     status, num_tx);
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static void
recv_drone_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
{
  
  // char *payload_recv;
  // payload = (char*)malloc(10);
  int d = packetbuf_copyto(payload);
    // int d = 0;
  printf("message received from %d.%d length %d Drone\n",
     from->u8[0], from->u8[1],d);

  if(is_drone && from->u8[0]==5 && from->u8[1]==0){
    allowed_to_send = 0;
  }

  if(is_coordinator && from->u8[0]==10 && from->u8[1]==0){
    if(num_packets > 0){
        printf("Switching send data\n");
        send_data = 1;
    }
  }
  // free(payload);
}
/*---------------------------------------------------------------------------*/
static void
sent_drone_uc(struct unicast_conn *c,  int status, int num_tx)
// sent_uc(struct unicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast messasge sent status:%d, transmission_num %d Drone\n",
     status, num_tx);
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static const struct unicast_callbacks drone_callbacks = {recv_drone_uc, sent_drone_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  
  // powertrace_start(CLOCK_SECOND * 2);

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

  if(is_drone){
    unicast_open(&uc_drone, 137, &drone_callbacks);  
  }else if(is_coordinator){
    unicast_open(&uc_drone, 137, &drone_callbacks);  
    unicast_open(&uc, 146, &unicast_callbacks);  
  }else{
    unicast_open(&uc, 146, &unicast_callbacks);  
  }
  
  while(1) {
    static struct etimer et;   
    linkaddr_t addr;
    
    if(is_drone && !is_coordinator){
        if(allowed_to_send == 1){
            etimer_set(&et, CLOCK_SECOND * 4);
        }else{
            allowed_to_send = 1;
            etimer_set(&et, CLOCK_SECOND * 60);
        }
    }else if(!is_drone && is_coordinator){
        if(send_data == 0){
            etimer_set(&et, CLOCK_SECOND);
        }else{
            etimer_set(&et, CLOCK_SECOND * 0.1);
        }
    }else {
        etimer_set(&et, CLOCK_SECOND * 30);
    }

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // if(!runicast_is_transmitting(&uc)) {
    if(!sending){
        if(is_drone && !is_coordinator) {
            memset(payload,i,10);
            memcpy(payload,"IAMDRONE",8);
            packetbuf_copyfrom(payload, 10);
            
            addr.u8[0] = 5;
            addr.u8[1] = 0;
            sending = 1;
            i = i + 1;
            if(unicast_send(&uc_drone, &addr) != 0){
                printf("Sending I am Drone\n");
            }

        } else if(!is_drone && is_coordinator){
            if(send_data){
                printf("Sending Data\n");
                memset(payload,num_packets,10);
                memcpy(payload,"abcdDATA",8);
                packetbuf_copyfrom(payload, 10);
                addr.u8[0] = 10;
                addr.u8[1] = 0;
                sending = 1;
                if(unicast_send(&uc_drone, &addr) != 0){
                    if (num_packets == 0){
                        send_data = 0;
                    }else{
                        printf("%d %d\n", send_data,num_packets);
                        num_packets--;
                    }
                }
            }
        }else{
            printf("Sending Sensor\n");
            packetbuf_copyfrom("SENSOR", 6);
            addr.u8[0] = 5;
            addr.u8[1] = 0;
            sending = 1;
            unicast_send(&uc, &addr);
        }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
