CFLAGS = -g -O2

all:
	$(CC) $(CFLAGS) rlnc.c encoder.c decoder.c gf.c -o rlnc
