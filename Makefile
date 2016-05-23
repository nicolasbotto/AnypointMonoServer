CFILES = CondVar.cpp JniManager.cpp Mutex.cpp Server.cpp Task.cpp ThreadPool.cpp TypeConverter.cpp b64.cpp main.cpp

all: g++ 

clean:
	rm -f -r build/
	rm -f output/anypointmonoserver
	rm -f *.o *.obj *.so* *.dll *.exe *.pdb *.exp *.lib

g++:
	mkdir -p build
	mkdir -p output

	g++ `pkg-config --cflags --libs mono-2` -fPIC -c \
		$(CFILES) 

	mv *.o build/
	
	g++ `pkg-config --cflags --libs mono-2` -pthread \
	-o output/anypointmonoserver build/CondVar.o build/JniManager.o build/Mutex.o \
	build/Server.o build/Task.o build/ThreadPool.o build/TypeConverter.o build/b64.o build/main.o \
	`pkg-config --cflags --libs mono-2` -pthread
