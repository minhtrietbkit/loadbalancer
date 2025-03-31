BUILD_DIR=build

program: main.c
	gcc -g  -o $(BUILD_DIR)/program main.c

server: tcpechoserver.c
	gcc -g  -o $(BUILD_DIR)/echoserver tcpechoserver.c -lcap

test: test.c
	gcc -g  -o $(BUILD_DIR)/test test.c