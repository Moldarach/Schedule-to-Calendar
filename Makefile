CC = g++
PROJECT = output
SRC = test.cpp
LIBS = `pkg-config --cflags --libs opencv4 tesseract libxml-2.0` -lcurl
$(PROJECT) : $(SRC)
	$(CC) $(SRC) -o $(PROJECT) $(LIBS)