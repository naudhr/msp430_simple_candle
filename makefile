TARGET="p.hex"
CC=msp430-gcc
CFLAGS=-std=c99 -mmcu=msp430g2131 -O2


all: $(TARGET)

$(TARGET): candle.o
	$(CC) $(CFLAGS) -o $(TARGET) "$<"

.c.o:
	$(CC) $(CFLAGS) -o "$@" -c "$<"

install: $(TARGET)
	mspdebug rf2500 "prog $(TARGET)"
