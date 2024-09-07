CC = g++
PROJECT = new_output
SRC = test.cpp
LIBS = `pkg-config --cflags --libs opencv4 tesseract`
$(PROJECT) : $(SRC)
	$(CC) $(SRC) -o $(PROJECT) $(LIBS)