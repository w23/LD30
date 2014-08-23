KAPUSHA_SIMPLE := 1
PRODUCT := ld30
SOURCES += $(PRODUCT).c main.c live_program.c
CFLAGS += -I./3p/SDL-linux64/include -D_REENTRANT
LDFLAGS += -L./3p/SDL-linux64/lib -lSDL2 -lpthread
all: $(PRODUCT)
include 3p/kapusha/kapusha.mk
