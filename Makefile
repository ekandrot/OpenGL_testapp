CFLAGS := `pkg-config libpng --cflags` -Wall  -std=c++14 -g -c -O3 -I common
LFLAGS := `pkg-config libpng --libs` -lglut -lGLEW -lGL -lGLU -lglfw -ljpeg
OBJDIR := objs/
SRC := linux_project/
INFINITE_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), texture.o shaders.o game2.o)
APP_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), texture.o shaders.o game0.o player.o)
CURVED_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), texture.o shaders.o game_view.o curved_test0.o)

VPATH = common:$(SRC)

all: app curved_test0.app infinite

clean:
	-rm curved_test0.app app infinite $(OBJDIR)*

$(OBJDIR)%.o : %.cpp %.h constants.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)game0.o : game0.cpp shaders.h texture.h maths.h player.h constants.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)game2.o : game2.cpp shaders.h texture.h maths.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)curved_test0.o : curved_test0.cpp shaders.h texture.h maths.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

infinite: $(INFINITE_OBJS)
	g++ $(INFINITE_OBJS) -o $@ $(LFLAGS)

app: $(APP_OBJS)
	g++ $(APP_OBJS) -o $@ $(LFLAGS)

curved_test0.app: $(CURVED_OBJS)
	g++ $(CURVED_OBJS) -o $@ $(LFLAGS)

$(LIBS_OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY : all clean

