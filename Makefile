KAPUSHA_SIMPLE := 1
PRODUCT := ld30
SOURCES += $(PRODUCT).c
CFLAGS += -I./3p/glfw/include
LDFLAGS += -L./3p/glfw/build/src -lglfw3 -lX11 -lXrandr -lXi -lXxf86vm -lXcursor
all: $(PRODUCT)
include 3p/kapusha/kapusha.mk
