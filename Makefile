CFLAGS := `pkg-config libpng --cflags` -Wall  -std=c++14 -g -c -O3 -I common
LFLAGS := `pkg-config libpng --libs` -lglut -lGLEW -lGL -lGLU -lglfw -ljpeg
OBJDIR := objs/
SRC := linux_project/
APP_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), texture.o shaders.o game0.o)
CURVED_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), texture.o shaders.o game_view.o curved_test0.o)

VPATH = common:$(SRC)

all: app curved_test0.app

clean:
	-rm curved_test0.app app $(OBJDIR)*

$(OBJDIR)%.o : %.cpp %.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)game0.o : game0.cpp shaders.h texture.h maths.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)curved_test0.o : curved_test0.cpp shaders.h texture.h maths.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

app: $(APP_OBJS)
	g++ $(APP_OBJS) -o $@ $(LFLAGS)

curved_test0.app: $(CURVED_OBJS)
	g++ $(CURVED_OBJS) -o $@ $(LFLAGS)

$(LIBS_OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY : all clean

