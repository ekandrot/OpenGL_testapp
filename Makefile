CFLAGS := `pkg-config libpng --cflags` -Wall  -std=c++0x -c -O3
LFLAGS := `pkg-config libpng --libs` -lglut -lGLEW -lGL -lglfw
OBJDIR := objs/
SRC := Win32Project1/
APP_OBJS := $(LIBS_OBJS) $(addprefix $(OBJDIR), shaders.o Texture.o Game0.o)


all: app

$(OBJDIR)shaders.o : $(SRC)shaders.cpp $(SRC)shaders.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)Texture.o : $(SRC)Texture.cpp $(SRC)Texture.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

$(OBJDIR)Game0.o : $(SRC)Game0.cpp $(SRC)shaders.h $(SRC)Texture.h $(SRC)maths.h
	g++ $< -o $@ $(CFLAGS) $(OUTPUT_OPTIONS)

app: $(APP_OBJS)
	g++ $(APP_OBJS) -o $@ $(LFLAGS)


$(LIBS_OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)


