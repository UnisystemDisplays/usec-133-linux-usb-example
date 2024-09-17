CFLAGS  = -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter
LDFLAGS = -lm

eink-demo:
	$(CC) -o eink-demo main.c usec_dev.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f eink-demo *.o *~
