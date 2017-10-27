# Distributed CPS with UAVs

This repository will contain the code for Master's Thesis of Laksh Bhatia

To start working with the contiki code in this repository.
1. Clone contiki https://github.com/contiki-os/contiki.git
2. cd contiki/
3. Apply patch to *contiki patch -p1 < contiki.patch* 

The following five folder contain the following codes:   
1. contiki_csma_9: Code designed for 9 motes and does not contain a lot of additions done to the protocol
2. post_demo_1: Final thesis code
3. power-profile: Code to measure power for different operations in the communication stack
4. range-test: Code to check the range of motes for different power levels
5. sniffer_flash: Code to sniff all packets on a particular channel and write some data to flash

All of them were designed for openmote-cc2538 and can be compiled with *make TARGET=openmote-cc2538*

The complete thesis is available in the repository as sci_2017_bhatia_laksh.pdf and at http://urn.fi/URN:NBN:fi:aalto-201709046870