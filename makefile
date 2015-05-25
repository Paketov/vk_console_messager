
LOCAL_CFLAGS := -O3 -Wall -g -fexceptions -fpermissive

LOCAL_C_INCLUDES := \
	-I./rapidjson-master/include \
	-I./

LOCAL_LIB := \
	-lssl \
	-lcrypto

all:
	g++ $(LOCAL_CFLAGS) $(LOCAL_LIB) $(LOCAL_C_INCLUDES) -c main.cpp -o obj/Release/main.o
	g++ $(LOCAL_CFLAGS) $(LOCAL_LIB) $(LOCAL_C_INCLUDES) -c queryurl.cpp -o obj/Release/queryurl.o
	g++ -o bin/Release/vk_messager obj/Release/main.o obj/Release/queryurl.o $(LOCAL_LIB) -s

