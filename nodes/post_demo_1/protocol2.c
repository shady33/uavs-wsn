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
 *         Master's Thesis Code for UAVs with WSN
 * \author
 *         Laksh Bhatia <bhatialaksh3@gmail.com>
 */

#define DEBUG 0
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

#if WRITE_TO_FLASH
#define FILENAME "logdata"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#endif

#define NUMRECORDS sizeof(coords) / sizeof(struct coord)

/* Timings */
#define MAX_RETRANSMISSIONS 7
#define DRONE_STUBBORN_TIME CLOCK_SECOND
#define WAITFORALLPACKETS   5
#define BACKOFF_TIME 30
#define POWER_NODES 7
#define WAITFORBACKOFF 15

/* Number of packets to send */
#define NUM_PACKETS 1
#define PACKET_CHECKER 0x2
#define TOTAL_PAKCETS 0

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
struct coord coords[] = {{0xEC,0xB6},{0x9F,0x7F},{0xE9,0x94},{0x72,0x88},
                         {0x6B,0x83},{0x9F,0x7B},{0x3E,0x82},{0x0E,0x76},
                         {0x2E,0x5C},{0xFB,0xCE},{0xDB,0x58},{0x4D,0x52},
                         {0xD2,0x64},{0xCE,0x90},{0x85,0x7E},{0x6B,0x83},
                         {0x17,0x73}
                     };
// struct coord coords[] = {{0x2,0x0},{0x3,0x0},{0xA,0x0}};

struct packet{
    uint16_t data5;
    uint16_t req_reply;
    uint16_t sequenceno;
    uint16_t totalpackets;
    uint64_t droppedpackets;
    uint64_t data3;
    uint64_t data4;
};

struct packet payload_send,payload_recv;

volatile int no_records = 0;
volatile struct record {
    linkaddr_t from_to;
    clock_time_t time;
    signed short RSSI;
    uint16_t packet_type;
    uint16_t packet_no_retrans;
}records[20];

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
static struct ctimer timer_completed;
volatile linkaddr_t current_recv;
volatile int allowed_to_send = 1;
volatile int waitng_for_packets = 0;
volatile int total_packets = TOTAL_PAKCETS;
volatile int replypacketnumber = 1;
/*---------------------------------------------------------------------------*/
#if WRITE_TO_FLASH
PROCESS(write_flash, "Write To flash");
PROCESS(read_flash, "Read flash");
#endif
PROCESS(send_C_D, "Send Coordinator to Drone process");
PROCESS(send_backoff, "Send Backoff");
PROCESS(main_process, "main process");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/
static void callback_nopacketrecieved(void *ptr)
{
    PRINTF("No packets received\n");
    ctimer_stop(&timer);
    waitng_for_packets = 0;
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    payload_send.req_reply = REQUEST;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
    leds_on(LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
static void callback_switch(void *ptr)
{
    PRINTF("Switch Timer\n");
    ctimer_stop(&timer_completed);
    allowed_to_send = 1;
    num_packets = NUM_PACKETS;
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    leds_off(LEDS_YELLOW);
}
/*---------------------------------------------------------------------------*/
static void change_send(void *ptr)
{
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
    payload_send.req_reply = REQSEQ;
    payload_send.droppedpackets = received_seq ^ PACKET_CHECKER;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
    sending = 1;
    if(runicast_send(&runicast_drone,&current_recv,MAX_RETRANSMISSIONS) != 0){
        PRINTF("Requested new packets\n");
    }
   }
}
/*---------------------------------------------------------------------------*/
#if WRITE_TO_FLASH
PROCESS_THREAD(read_flash,ev,data)
{
  PROCESS_BEGIN();

    while(1){
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

        int fd;
        int r;
        fd = cfs_open(FILENAME, CFS_READ);
        if(fd < 0) {
            printf("failed to open %s\n", FILENAME);
        }else{
            printf("Reading first time %d\n",fd);
            while(1){
                r = cfs_read(fd, &records, sizeof(records));
                if(r == 0) {
                    PRINTF("No records found\n");
                    break;
                }else if(r < sizeof(records)) { /* 0x00 Error */ 
                    PRINTF("failed to read %d bytes from %s\n",
                        (int)sizeof(records), FILENAME);
                }else{
                    PRINTF("Read Properly\n");
                }
                PRINTF("Read Bytes:%d\n",r);
                int num = (r/sizeof(struct record));
                if(r%sizeof(struct record) != 0){
                    num = num + 1;
                }
                for(int i = 0; i < num ; i++){
                    printf("%lu: %d.%d Packet_Type:%d PacketNo_Noretrans:%d RSSI:%d\n",records[i].time,records[i].from_to.u8[0],records[i].from_to.u8[1],
                            records[i].packet_type,records[i].packet_no_retrans,records[i].RSSI);
                }

            }
        printf("Closing the file\n");
        cfs_close(fd);
        }
    }
  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
#if WRITE_TO_FLASH
PROCESS_THREAD(write_flash,ev,data)
{
    PROCESS_BEGIN();

    PRINTF("Going to Write to File\n");
    int fd;
    int r;
    fd = cfs_open(FILENAME, CFS_WRITE | CFS_APPEND);
    cfs_offset_t size = cfs_get_seek(fd);
    if(size%(sizeof(struct record)) != 0){
        int new_seek = ((size / sizeof(struct record)) + 1) * sizeof(struct record);
        size = cfs_seek(fd,new_seek,CFS_SEEK_SET);
    }
    if(fd < 0) {
        PRINTF("failed to open %s\n", FILENAME);
        fd = cfs_open(FILENAME, CFS_READ | CFS_WRITE);
        if(fd < 0){
            PRINTF("Still Did not open\n");
        }
    }

    for(int i = 0; i < no_records ; i++){
        PRINTF("%lu: %d.%d Packet_Type:%d PacketNo_Noretrans:%d RSSI:%d\n",records[i].time,records[i].from_to.u8[0],records[i].from_to.u8[1],
                records[i].packet_type,records[i].packet_no_retrans,records[i].RSSI);
    }
    
    r = cfs_write(fd, &records, sizeof(struct record)*no_records);
    if(r != sizeof(struct record)*no_records) {
        PRINTF("failed to write %d bytes to %s,written %d\n",
        (int)sizeof(records), FILENAME,r);
    }else{
        PRINTF("Written Data\n");
        no_records = 0;
    }
    cfs_close(fd);
    PRINTF("File Write Complete\n");

    PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_backoff,ev,data)
{
    PROCESS_BEGIN();
    PRINTF("%lu:Send Backoff\n",clock_time());
    if(runicast_is_transmitting(&runicast_drone)){
        runicast_cancel(&runicast_drone);
    }
    payload_send.req_reply = BACKOFF;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
    runicast_send(&runicast_drone,&current_recv,MAX_RETRANSMISSIONS);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_C_D,ev,data)
{
  PROCESS_BEGIN();

    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;

  if(data == NULL){
    PRINTF("Num Packets to send:%d\n", num_packets);

    while(num_packets != 0){
        PRINTF("Sending Data %d\n",num_packets);
        payload_send.req_reply = DATA;
        payload_send.sequenceno = num_packets;
        payload_send.totalpackets = NUM_PACKETS;
        packetbuf_copyfrom(&payload_send, sizeof(payload_send));
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
            payload_send.req_reply = REPSEQ;
            payload_send.sequenceno = bit;
            payload_send.totalpackets = NUM_PACKETS;
            packetbuf_copyfrom(&payload_send, sizeof(payload_send));
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

  packetbuf_copyto(&payload_recv);
  PRINTF("%lu:unicast message received from %d.%d, RSSI:%d Packetno:%d\n",
   clock_time(),from->u8[0], from->u8[1],(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload_recv.sequenceno);
  linkaddr_copy(&current_recv,from);
  if(!(received_seq & (1 << payload_recv.sequenceno))){
    received_seq = received_seq | (1 << payload_recv.sequenceno);
  }

    #if WRITE_TO_FLASH
        records[no_records].time = clock_time();
        linkaddr_copy(&records[no_records].from_to,from);
        records[no_records].packet_type = payload_recv.req_reply;
        records[no_records].RSSI = packetbuf_attr(PACKETBUF_ATTR_RSSI);
        records[no_records].packet_no_retrans = payload_recv.sequenceno;
        no_records = no_records + 1;
    #endif


  if(received_seq == (1 << payload_recv.sequenceno)){
    waitng_for_packets = 0;
    /* Start timer */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * WAITFORALLPACKETS, callback, NULL);
  }

  // PRINTF("%x,%x\n",received_seq,(1 << payload.sequenceno));

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
    #if TOTAL_PAKCETS
    if(status == 0){
        total_packets = total_packets - 1;        
    }
    #endif
  if(num_packets == 0){
    ctimer_set(&timer_completed, CLOCK_SECOND * WAITFORBACKOFF,callback_switch,NULL);
    ctimer_restart(&timer_completed);
  }
}
static const struct unicast_callbacks unicast_drone_callbacks = {recv_unicast_drone,
                   sent_unicast_drone};
/*---------------------------------------------------------------------------*/
static void
recv_runicast_drone(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  packetbuf_copyto(&payload_recv);
  PRINTF("%lu:runicast message received from %d.%d, seqno %d RSSI:%d Packet type:%d\n",
   clock_time(),from->u8[0], from->u8[1], seqno,(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),payload_recv.req_reply);

    #if WRITE_TO_FLASH
        records[no_records].time = clock_time();
        linkaddr_copy(&records[no_records].from_to,from);
        records[no_records].packet_type = payload_recv.req_reply;
        records[no_records].RSSI = packetbuf_attr(PACKETBUF_ATTR_RSSI);
        records[no_records].packet_no_retrans = payload_recv.sequenceno;
        no_records = no_records + 1;
    #endif

  if(is_drone && payload_recv.req_reply == REPLY){
    stbroadcast_cancel(&stbroadcast_drone);
    leds_off(LEDS_GREEN);
    if(!runicast_is_transmitting(&runicast_drone)){
        PRINTF("%lu:Sending Runicast message to %d.%d\n",clock_time(), from->u8[0],from->u8[1]);
        payload_send.req_reply = ACCEPT;
        packetbuf_copyfrom(&payload_send,sizeof(payload_send));
        runicast_send(&runicast_drone,from,MAX_RETRANSMISSIONS);
        leds_on(LEDS_YELLOW);
    }
  }else if(is_coordinator && payload_recv.req_reply == ACCEPT){
    if(runicast_is_transmitting(c)){
        runicast_cancel(c);
    }
    leds_on(LEDS_GREEN);
    /* Switch channels */
    ctimer_restart(&timer);
    ctimer_set(&timer, CLOCK_SECOND * 2, change_send, NULL);
  }else if(is_coordinator && payload_recv.req_reply == REQSEQ){
    process_start(&send_C_D,(void *)payload_recv.droppedpackets);
  }else if(is_coordinator && payload_recv.req_reply == BACKOFF){
    /* Start backoff timer */
    leds_on(LEDS_YELLOW);
    allowed_to_send = 0;
    ctimer_stop(&timer_completed);
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

    #if WRITE_TO_FLASH
        records[no_records].time = clock_time();
        linkaddr_copy(&records[no_records].from_to,to);
        records[no_records].packet_no_retrans = retransmissions;
        records[no_records].packet_type = payload_send.req_reply;
        records[no_records].RSSI = 0;
        no_records = no_records + 1;
    #endif

  if(is_drone){
    /* Sent ACCEPT and BACKOFF packet */
    /* Switch channels? */
    if(payload_send.req_reply == ACCEPT){
        PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_2);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_2);

        /* Wait for WAITFORALLPACKETS in FFD_CHANNEL_2 */
        ctimer_restart(&timer);
        ctimer_set(&timer, CLOCK_SECOND * WAITFORALLPACKETS, callback_nopacketrecieved, NULL);
        waitng_for_packets = 1;

        leds_off(LEDS_YELLOW);
    }else if(payload_send.req_reply == BACKOFF){
        PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
        payload_send.req_reply = REQUEST;
        packetbuf_copyfrom(&payload_send,sizeof(payload_send));
        stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
        leds_on(LEDS_GREEN);
    #if WRITE_TO_FLASH
        process_start(&write_flash,NULL);
    #endif
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
  #if WRITE_TO_FLASH
    records[no_records].time = clock_time();
    linkaddr_copy(&records[no_records].from_to,to);
    records[no_records].packet_no_retrans = retransmissions;
    records[no_records].packet_type = payload_send.req_reply;
    records[no_records].RSSI = 0;
    no_records = no_records + 1;
  #endif

  if(is_drone){
    if(waitng_for_packets){
        ctimer_stop(&timer);
        waitng_for_packets = 0;    
    }
    
    PRINTF("%lu:Change Channel to %d\n",clock_time(),FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    payload_send.req_reply = REQUEST;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
    stbroadcast_send_stubborn(&stbroadcast_drone, DRONE_STUBBORN_TIME);
    leds_on(LEDS_GREEN);

    #if WRITE_TO_FLASH
        process_start(&write_flash,NULL);
    #endif
  }
  leds_on(LEDS_RED);
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

  #if TOTAL_PAKCETS
  if(total_packets > 0){
  #endif
  if(allowed_to_send){
    linkaddr_t addr;
    addr.u8[0] = DRONE0;
    addr.u8[1] = DRONE1;
    payload_send.req_reply = REPLY;
    payload_send.sequenceno = replypacketnumber;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
    replypacketnumber = replypacketnumber + 1;
    PRINTF("Sending Runicast to Drone\n");
    runicast_send(&runicast_drone,&addr,MAX_RETRANSMISSIONS);  
  }
  #if TOTAL_PAKCETS
    }
  #endif
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
  int val;
  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER,&val);
  PRINTF("Power: %d\n", val);
  if(linkaddr_node_addr.u8[0] == DRONE0 &&
     linkaddr_node_addr.u8[1] == DRONE1) {
    is_drone = 1;
    // leds_on(LEDS_ORANGE);
  }

  if(!is_drone && !is_coordinator){
    is_sensor = 1;
    // leds_on(LEDS_ORANGE);
  }

  #if NEED_FORMATTING
    cfs_coffee_format();
  #endif

  #if WRITE_TO_FLASH
      SENSORS_ACTIVATE(button_sensor);
      process_start(&read_flash,NULL);
  #endif

  if(is_drone){
    PRINTF("Change Channel to %d\n",FFD_CHANNEL_1);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,FFD_CHANNEL_1);
    stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);
    runicast_open(&runicast_drone, 144, &runicast_drone_callbacks);
    unicast_open(&unicast_drone, 140, &unicast_drone_callbacks);
    payload_send.req_reply = REQUEST;
    packetbuf_copyfrom(&payload_send,sizeof(payload_send));
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
