CFLAGS  = -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter
LDFLAGS = -lm

usec-133-linux-usb-example:
	$(CC) -o usec-133-linux-usb-example main.c usec_dev.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f usec-133-linux-usb-example *.o *~
