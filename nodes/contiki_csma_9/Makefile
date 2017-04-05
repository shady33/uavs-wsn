CONTIKI_PROJECT = contiki_csma_9
CONTIKI = ../..

all: $(CONTIKI_PROJECT)

APPS+=powertrace
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include
