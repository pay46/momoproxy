.PHONY:clean
BIN_DIR=./bin
CC=gcc
#CC=/home/blobt/Documents/tool/mt7620_toolchain/9.33.2/bin/mipsel-openwrt-linux-gcc
CFLAGES=-Wall -g
LDFLAGES=-L$(PRE_PATH)/lib
BIN=$(BIN_DIR)/momoproxy
OBJS=main.o str.o LinkList.o socket.o session.o http.o buffer.o
LIBS=
$(BIN):$(OBJS)
	$(CC) $(CFLAGES) $(LDFLAGES) $^ -o $@ $(LIBS)
%.o:%.c
	$(CC) $(CFLAGES) -c $< -o $@
clean:
	rm -f *.o $(BIN)

