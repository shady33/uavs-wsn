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
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "contiki.h"
#define FILENAME "logdata"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#include "net/rime/rime.h"
#include "net/rime/stbroadcast.h"
#include "dev/button-sensor.h"
#include "net/netstack.h"

volatile int no_records = 0;
volatile struct record {
    clock_time_t time;
    uint16_t sequence_number;
    signed short RSSI;
}records[20];

static struct stbroadcast_conn stbroadcast_drone;
/*---------------------------------------------------------------------------*/
PROCESS(write_flash, "Write To flash");
PROCESS(read_flash, "Read flash");
PROCESS(sniffer_flash_process, "Sniffer with flash process");
AUTOSTART_PROCESSES(&sniffer_flash_process);
/*---------------------------------------------------------------------------*/
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
                    printf("%lu: Sequence No:%u RSSI:%d\n",records[i].time,records[i].sequence_number,records[i].RSSI);
                }

            }
        printf("Closing the file\n");
        cfs_close(fd);
        }
    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
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
        PRINTF("%lu: Sequence No:%u RSSI:%d\n",records[i].time,records[i].sequence_number,records[i].RSSI);
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
/*---------------------------------------------------------------------------*/
static void
recv_stbroadcast_drone(struct stbroadcast_conn *c)
{
  PRINTF("%lu:stbroadcast message received RSSI:%d\n",
     clock_time(),(signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));

    records[no_records].time = clock_time();
    records[no_records].sequence_number = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID_DR);
    records[no_records].RSSI = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    no_records = no_records + 1;

    if (no_records == 18){
        process_start(&write_flash,NULL);
    }
}
static const struct stbroadcast_callbacks stbroadcast_drone_callbacks = {recv_stbroadcast_drone};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sniffer_flash_process, ev, data)
{
  PROCESS_EXITHANDLER(stbroadcast_close(&stbroadcast_drone);)
  PROCESS_BEGIN();

  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,26);
  stbroadcast_open(&stbroadcast_drone, 137, &stbroadcast_drone_callbacks);

  #if NEED_FORMATTING
    cfs_coffee_format();
  #endif

  SENSORS_ACTIVATE(button_sensor);
  process_start(&read_flash,NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
