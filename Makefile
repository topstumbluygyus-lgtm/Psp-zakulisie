TARGET = zakulise
OBJS = main.o
CFLAGS = -O2 -G0 -Wall -std=gnu99
LIBS = -lpspdebug -lpspdisplay -lpspge -lpspctrl -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP: Zakulise
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
