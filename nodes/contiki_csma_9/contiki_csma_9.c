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
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "powertrace.h"
#include "sys/ctimer.h"

#include <stdio.h> /* For printf() */

#define MAX_RETRANSMISSIONS 4
#define NUMPACKETS 15
#define REQUEST 1
#define REPLY 2
#define REQSEQ 3
#define REPSEQ 4
#define BACKOFF 5
// #define DRONE0 0x9F
// #define DRONE1 0x58
// #define COORD0 0xEC
// #define COORD1 0xB6

#if WRITE_TO_FLASH
#define FILENAME "packets"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#endif
#define DRONE0 0x0A
#define DRONE1 0x00
#define COORD00 0x05
#define COORD01 0x00
#define COORD10 0x03
#define COORD11 0x00

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main process");
PROCESS(send_C_D, "Send Coordinator to Drone process");
PROCESS(button_press, "Wait for Button Press Event");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/
static struct unicast_conn uc;
static struct unicast_conn uc_drone_1;
// static struct unicast_conn uc_drone;
static struct broadcast_conn uc_drone;
int is_drone = 0;
int is_coordinator = 0;
int is_sensor = 0;
volatile int sending = 0;
volatile int recieving = 0;
volatile int i = 0;
volatile int num_packets = NUMPACKETS;
volatile uint16_t packets_received = 0;
volatile uint16_t packets_sent = 0;
static struct ctimer timer;
volatile int read_since_reset = 0;
struct record {
    uint16_t recv;
    uint16_t sent;
}record;
struct packet{
    uint16_t req_reply;
    uint16_t sequenceno;
    uint16_t totalpackets;
    uint16_t droppedpackets;
    uint64_t data3;
    uint64_t data4;
    uint64_t data5;
}payload;
volatile uint16_t received_seq = 0;
volatile int allowed_to_send = 1;
 /*---------------------------------------------------------------------------*/
static void callback(void *ptr)
{
   ctimer_stop(&timer);
   received_seq = received_seq ^ 0xFFFE; /* FFFE */
   if(received_seq == 0){
    /* All 15 packets received send backoff to node */
    payload.req_reply = BACKOFF;
    payload.droppedpackets = received_seq;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(broadcast_send(&uc_drone) != 0){
      printf("Backoff\n");
    }
   }else{
    /* some dropped */
    payload.req_reply = REQSEQ;
    payload.droppedpackets = received_seq;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(broadcast_send(&uc_drone) != 0){
      printf("Requested new packets\n");
    }
   }
}
/*---------------------------------------------------------------------------*/
static void backoff(void *ptr)
{
    ctimer_stop(&timer);
    allowed_to_send = 1;
    printf("Allowed to send again\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(button_press,ev,data){
  PROCESS_BEGIN();
  
  #if WRITE_TO_FLASH
  int fd;
  int r;
  #endif

  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    #if WRITE_TO_FLASH
    if(read_since_reset == 0){
      fd = cfs_open(FILENAME, CFS_READ);
      if(fd < 0) {
        printf("failed to open %s\n", FILENAME);
      }else{
        printf("Reading first time %d\n",fd);  
        r = cfs_read(fd, &record, sizeof(record));
        if(r == 0) {
          printf("New one\n");
        } else if(r < sizeof(record)) { /* 0x00 Error */ 
          printf("failed to read %d bytes from %s, recv %d sent %d\n",
             (int)sizeof(record), FILENAME, record.recv,record.sent);
          packets_received = packets_received + record.recv;
          packets_sent = packets_sent + record.sent;
        }else{
          packets_received = packets_received + record.recv;
          packets_sent = packets_sent + record.sent;
          printf("Read %d %d\n",packets_received,packets_sent);
        }
        cfs_close(fd);
      }
      read_since_reset = 1;
    }

    record.recv = packets_received;
    record.sent = packets_sent;

    fd = cfs_open(FILENAME, CFS_READ | CFS_WRITE);
    if(fd < 0) {
      printf("failed to open %s\n", FILENAME);
    }
    r = cfs_write(fd, &record, sizeof(record));
    if(r != sizeof(record)) {
      printf("failed to write %d bytes to %s,written %d\n",
       (int)sizeof(record), FILENAME,r);
    }
    cfs_close(fd);
    #endif
    printf("Received Packets: %d, Sent Packets: %d\n", packets_received,packets_sent);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_C_D, ev, data){
    PROCESS_BEGIN();
    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;
    if(data == NULL){
      while(num_packets > 0){
        printf("Sending Data\n");
        payload.req_reply = REPLY;
        payload.sequenceno = num_packets;
        payload.totalpackets = NUMPACKETS;
        payload.data3 = 0xABCDEF0123456789;
        packetbuf_copyfrom(&payload, sizeof(payload));
        sending = 1;
        // if(runicast_send(&uc_drone_1,&addr,MAX_RETRANSMISSIONS) != 0){
        // if(unicast_send(&uc_drone,&addr) != 0){
        if(broadcast_send(&uc_drone) != 0){
          if(num_packets != 0){
            printf("%d\n",num_packets);
            num_packets--;
          }
        }
      }
    }else{
      int bit = 0;
      for(bit = 0; bit < 16; bit++){
        if ((uint16_t)data & (1 << bit)) {
          // Current bit is set to 1
          payload.req_reply = REPSEQ;
          payload.sequenceno = bit;
          payload.totalpackets = NUMPACKETS;
          packetbuf_copyfrom(&payload,sizeof(payload));
          sending = 1;
          payload.data3 = 0x1223344556ABCDEF;
          // printf("Sending runicast:%d %d\n",bit, runicast_send(&uc_drone_1,&addr,MAX_RETRANSMISSIONS));
          if(unicast_send(&uc_drone_1,&addr) != 0){
            printf("Sent request packets\n");
          } 
        }
      }
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
{
  int d = packetbuf_copyto(&payload);
    // int d = 0;
  printf("unicast message received from %d.%d length %d\n",
     from->u8[0], from->u8[1],d);
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c,  int status, int num_tx)
// sent_uc(struct unicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("unicast messasge sent status:%d, transmission_num %d\n",
     status, num_tx);
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static void
recv_drone_uc_1(struct unicast_conn *c, const linkaddr_t *from){
  printf("unicast received\n");
  packetbuf_copyto(&payload);
  received_seq = received_seq ^ (1 << payload.sequenceno); /* Account for wrong duplicates */
  if(received_seq == 0){
    /* Send backoff */
    payload.req_reply = BACKOFF;
    payload.droppedpackets = received_seq;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(broadcast_send(&uc_drone) != 0){
      printf("Backoff\n");
    }
  }
}
static void
sent_drone_uc_1(struct unicast_conn *c, int status, int num_tx){
  printf("unicast sent\n");
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static void
recv_drone_uc(struct broadcast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
// recv_drone_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
// recv_drone_uc(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  int d = packetbuf_copyto(&payload);
 
  printf("Broadcast message received from %d.%d length %d Seq:%d Tot:%d Message type:%dDrone\n",
     from->u8[0], from->u8[1],d,payload.sequenceno,payload.totalpackets,payload.req_reply);
  leds_toggle(LEDS_GREEN);
  if(is_drone){// && from->u8[0]==coordinator0 && from->u8[1]==coordinator1){
    packets_received++;
    if(payload.req_reply == REPLY){
      if(received_seq == 0){
        ctimer_restart(&timer);
        ctimer_set(&timer, CLOCK_SECOND * 2, callback, NULL);
      }
      received_seq = received_seq | (1 << payload.sequenceno);
    }else if(payload.req_reply == REPSEQ){
      printf("Received the dropped sequence\n");
    }
  }

  if(is_coordinator){// && from->u8[0]==drone0 && from->u8[1]==drone1){
    if((payload.req_reply == REQUEST) && (allowed_to_send == 1)){
      if(num_packets > 0){
        process_start(&send_C_D,NULL);
      }else{
        num_packets = NUMPACKETS;
        process_start(&send_C_D,NULL);
      }  
    }else if(payload.req_reply == REQSEQ){
      /* Sending all dropped packets */
      process_start(&send_C_D,(void *)payload.droppedpackets);
    }else if(payload.req_reply == BACKOFF){
        printf("Backoff received\n");
        ctimer_restart(&timer);
        ctimer_set(&timer, CLOCK_SECOND * 15, backoff, NULL); /* 15 seconds */
        allowed_to_send = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_drone_uc(struct broadcast_conn *c,  int status, int num_tx)
// sent_drone_uc(struct unicast_conn *c, int status, int num_tx)
// sent_drone_uc(struct runicast_conn *c, const linkaddr_t *to, uint8_t num_tx)
{
  printf("Broadcast messasge sent transmission_num %d Drone\n",
    num_tx);
  leds_toggle(LEDS_YELLOW);
  packets_sent++;
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks sensor_callbacks = {recv_uc, sent_uc};
static const struct unicast_callbacks drone_callbacks_1 = {recv_drone_uc_1, sent_drone_uc_1};
static const struct broadcast_callbacks drone_callbacks = {recv_drone_uc, sent_drone_uc};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
  PROCESS_EXITHANDLER(unicast_close(&uc_drone_1);)
  // PROCESS_EXITHANDLER(unicast_close(&uc_drone);)
  PROCESS_EXITHANDLER(broadcast_close(&uc_drone);)

  PROCESS_BEGIN();
  
  #if NEED_FORMATTING
    cfs_coffee_format();
  #endif

  // powertrace_start(CLOCK_SECOND * 2);

  printf("Hello, Laksh\n");

  /* Receiver node: do nothing */
  if((linkaddr_node_addr.u8[0] == COORD00 &&
     linkaddr_node_addr.u8[1] == COORD01) || 
     (linkaddr_node_addr.u8[0] == COORD10 &&
     linkaddr_node_addr.u8[1] == COORD11)) {
    is_coordinator = 1;
    leds_on(LEDS_RED);
  }
    
  if(linkaddr_node_addr.u8[0] == DRONE0 &&
     linkaddr_node_addr.u8[1] == DRONE1) {
    is_drone = 1;
    // leds_on(LEDS_ORANGE);
  }

  if(!is_drone && !is_coordinator){
    is_sensor = 1;
    leds_on(LEDS_GREEN);
  }

  if(is_drone){
    broadcast_open(&uc_drone, 137, &drone_callbacks);
    // unicast_open(&uc_drone, 137, &drone_callbacks);
    unicast_open(&uc_drone_1, 143, &drone_callbacks_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,11);
  }else if(is_coordinator){
    broadcast_open(&uc_drone, 137, &drone_callbacks);
    // unicast_open(&uc_drone, 137, &drone_callbacks);
    unicast_open(&uc_drone_1, 143, &drone_callbacks_1);
    unicast_open(&uc, 146, &sensor_callbacks);  
  }else if(is_sensor){
    unicast_open(&uc, 146, &sensor_callbacks);  
  }
  
  SENSORS_ACTIVATE(button_sensor);
  // NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,-24);
  process_start(&button_press,NULL);
  
  while(1) {
    static struct etimer et;   
    linkaddr_t addr;
    
    if(is_drone){
        etimer_set(&et, CLOCK_SECOND * 10);
    }else if(is_sensor){
        etimer_set(&et, CLOCK_SECOND * 30);
    }else {
       break; /* Coordinator*/
    }
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(!sending){
      if(is_drone){
        printf("Sending I am Drone\n");
        payload.req_reply = REQUEST;
        payload.sequenceno = i;
        payload.data3 = 0x1223344556677889;
        packetbuf_copyfrom(&payload, sizeof(payload));
        
        // addr.u8[0] = COORD0;
        // addr.u8[1] = COORD1;
        sending = 1;
        i = i + 1;
        // runicast_send(&uc_drone,&addr,MAX_RETRANSMISSIONS);
        // unicast_send(&uc_drone, &addr);                
        broadcast_send(&uc_drone);
      }else if(is_sensor){
        printf("Sending Sensor\n");
        packetbuf_copyfrom("SENSOR", 6);
        addr.u8[0] = COORD00;
        addr.u8[1] = COORD01;
        sending = 1;
        unicast_send(&uc, &addr);
      }
    }
  }
  printf("Exiting Process\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
