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
#define DRONE0 0x0A
#define DRONE1 0x00

struct coord{
    uint8_t first;
    uint8_t second;
};
// struct coord coords[] = {{0xEC,0xB6},{0x9E,0xC8}};
struct coord coords[] = {{0x03,0x00},{0x05,0x00}};

#define NUMRECORDS sizeof(coords) / sizeof(struct coord)

#if WRITE_TO_FLASH
#define FILENAME "packets"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#endif

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main process");
PROCESS(send_C_D, "Send Coordinator to Drone process");
PROCESS(send_backoff, "Send Backoff from drone");
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
// volatile uint16_t packets_received = 0;
// volatile uint16_t packets_sent = 0;
static struct ctimer timer;
volatile int read_since_reset = 0;
volatile struct record {
    linkaddr_t from;
    linkaddr_t to;
    uint16_t recv;
    uint16_t sent;
}records[NUMRECORDS];
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
volatile linkaddr_t current_recv;
 /*---------------------------------------------------------------------------*/
static void callback(void *ptr)
{
   ctimer_stop(&timer);
   received_seq = received_seq ^ 0xFFFE; /* FFFE */
   if(received_seq == 0){
    /* All 15 packets received send backoff to node */
    process_start(&send_backoff,NULL);
   }else{
    /* some dropped */
    payload.req_reply = REQSEQ;
    payload.droppedpackets = received_seq;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(unicast_send(&uc_drone_1,&current_recv) != 0){
    // if(broadcast_send(&uc_drone) != 0){
      printf("Requested new packets\n");
    }
   }
}
/*---------------------------------------------------------------------------*/
static void backoff(void *ptr)
{
    ctimer_stop(&timer);
    allowed_to_send = 1;
    num_packets = NUMPACKETS;
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
        fd = cfs_open(FILENAME, CFS_READ | CFS_WRITE);
        if(fd < 0) {
          printf("failed to open %s\n", FILENAME);
        }
        r = cfs_write(fd, &records, sizeof(records));
        if(r != sizeof(records)) {
          printf("failed to write %d bytes to %s,written %d\n",
           (int)sizeof(records), FILENAME,r);
        }
        cfs_close(fd);
    #endif
    // printf("Received Packets: %d, Sent Packets: %d\n", packets_received,packets_sent);
    if(is_drone){
        int j;
        for(j = 0;j < NUMRECORDS;j++){
            printf("From: %x.%x Received %d\n", (records[j].from).u8[0],(records[j].from).u8[1],records[j].recv);
        }
    }
    if(is_coordinator){
        printf("To: %x.%x Sent %d\n", (records[0].to).u8[0],(records[0].to).u8[1],records[0].sent);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_backoff, ev, data){
    PROCESS_BEGIN();
    static struct etimer et;   
    
    etimer_set(&et, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    payload.req_reply = BACKOFF;
    packetbuf_copyfrom(&payload,sizeof(payload));
    sending = 1;
    if(unicast_send(&uc_drone_1,&current_recv) != 0){
    // if(broadcast_send(&uc_drone) != 0){
      printf("Backoff\n");
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
      while(num_packets != 0){
        printf("Sending Data %d\n",num_packets);
        payload.req_reply = REPLY;
        payload.sequenceno = num_packets;
        payload.totalpackets = NUMPACKETS;
        payload.data3 = 0xABCDEF0123456789;
        packetbuf_copyfrom(&payload, sizeof(payload));
        sending = 1;
        // if(runicast_send(&uc_drone_1,&addr,MAX_RETRANSMISSIONS) != 0){
        // if(unicast_send(&uc_drone_1,&addr) != 0){
        if(broadcast_send(&uc_drone) != 0){
            printf("Normal Data: %d\n",num_packets);
            num_packets = num_packets - 1;
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
    packetbuf_copyto(&payload);
    if(is_coordinator){
        if(payload.req_reply == REQSEQ){
            /* Sending all dropped packets */
            printf("Request Sequence Received\n");
            process_start(&send_C_D,(void *)payload.droppedpackets);
        }else if(payload.req_reply == BACKOFF){
            printf("Backoff received\n");
            ctimer_restart(&timer);
            ctimer_set(&timer, CLOCK_SECOND * 15, backoff, NULL); /* 15 seconds */
            allowed_to_send = 0;
        }
    }  
    if(is_drone){
        printf("Unicast Received Seq No: %d\n",payload.sequenceno);
        // packets_received++;
        int j;
        for(j = 0;j < NUMRECORDS;j++){
            if((records[j].from.u8[0] == current_recv.u8[0]) && (records[j].from.u8[1] == current_recv.u8[1])){
                records[j].recv = records[j].recv + 1;
            }
        }
        received_seq = received_seq ^ (1 << payload.sequenceno); /* Account for wrong duplicates */
        if(received_seq == 0){
            /* Send backoff */
            process_start(&send_backoff,NULL);
        }
    }  
}
static void
sent_drone_uc_1(struct unicast_conn *c, int status, int num_tx){
  printf("Unicast sent Drone Channel\n");
  sending = 0;
}
/*---------------------------------------------------------------------------*/
static void
recv_drone_uc(struct broadcast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
// recv_drone_uc(struct unicast_conn *c, const linkaddr_t *from)//, uint8_t seqno)
// recv_drone_uc(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  int d = packetbuf_copyto(&payload);
 
  printf("Broadcast message received from %x.%x length %d Seq:%d Tot:%d Message type:%dDrone\n",
     from->u8[0], from->u8[1],d,payload.sequenceno,payload.totalpackets,payload.req_reply);
  leds_toggle(LEDS_GREEN);
  if(is_drone){// && from->u8[0]==coordinator0 && from->u8[1]==coordinator1){
    // packets_received++;
    current_recv.u8[0] = from->u8[0];
    current_recv.u8[1] = from->u8[1];
    int j;
    for(j = 0;j < NUMRECORDS;j++){
        if(((records[j].from).u8[0] == current_recv.u8[0]) && ((records[j].from).u8[1] == current_recv.u8[1])){
            records[j].recv = records[j].recv + 1;
        }
    }
    if(payload.req_reply == REPLY){
      if(received_seq == 0){
        ctimer_restart(&timer);
        ctimer_set(&timer, CLOCK_SECOND * 2, callback, NULL); /* Check with hardware */
      }
      received_seq = received_seq | (1 << payload.sequenceno);
    }
  }

  if(is_coordinator){// && from->u8[0]==drone0 && from->u8[1]==drone1){
    if((payload.req_reply == REQUEST) && (allowed_to_send == 1)){
        process_start(&send_C_D,NULL);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_drone_uc(struct broadcast_conn *c,  int status, int num_tx)
// sent_drone_uc(struct unicast_conn *c, int status, int num_tx)
// sent_drone_uc(struct runicast_conn *c, const linkaddr_t *to, uint8_t num_tx)
{
  printf("Broadcast message sent status:%d transmission_num %d Drone\n",
    status,num_tx);
  leds_toggle(LEDS_YELLOW);
  // packets_sent++;
  sending = 0;
  if(is_coordinator){
    records[0].sent = records[0].sent + 1;
  }
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

  #if WRITE_TO_FLASH
    int fd;
    int r;
    fd = cfs_open(FILENAME, CFS_READ);
    if(fd < 0) {
        printf("failed to open %s\n", FILENAME);
    }else{
        printf("Reading first time %d\n",fd);  
        r = cfs_read(fd, &records, sizeof(records));
        if(r == 0) {
        printf("New one\n");
        }else if(r < sizeof(records)) { /* 0x00 Error */ 
        printf("failed to read %d bytes from %s\n",
             (int)sizeof(records), FILENAME);
        }else{
            printf("Read Properly\n");
        }
    cfs_close(fd);
    }
  #endif
  // powertrace_start(CLOCK_SECOND * 2);

  printf("Hello, Laksh\n");

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
    leds_on(LEDS_GREEN);
  }

  if(is_drone){
    broadcast_open(&uc_drone, 137, &drone_callbacks);
    // unicast_open(&uc_drone, 137, &drone_callbacks);
    unicast_open(&uc_drone_1, 143, &drone_callbacks_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,11);
    /* Create records in drone */
    for(j = 0; j < NUMRECORDS;j++){
        (records[j].from).u8[0] = coords[j].first;
        (records[j].from).u8[1] = coords[j].second;
    }
  }else if(is_coordinator){
    broadcast_open(&uc_drone, 137, &drone_callbacks);
    // unicast_open(&uc_drone, 137, &drone_callbacks);
    unicast_open(&uc_drone_1, 143, &drone_callbacks_1);
    unicast_open(&uc, 146, &sensor_callbacks);
    (records[0].to).u8[0] = DRONE0;
    (records[0].to).u8[1] = DRONE1;
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
        if(broadcast_send(&uc_drone) != 0){
            printf("Sent I am Drone\n");
        }
      }else if(is_sensor){
        printf("Sending Sensor\n");
        packetbuf_copyfrom("SENSOR", 6);
        addr.u8[0] = coords[0].first;
        addr.u8[1] = coords[0].second;
        sending = 1;
        unicast_send(&uc, &addr);
      }
    }
  }
  printf("Exiting Process\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
