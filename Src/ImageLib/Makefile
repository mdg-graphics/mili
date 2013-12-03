
# CFLAGS	= -xansi -D__STDC__ -O
# CFLAGS	= -xansi -O
CC = icc
CFLAGS	=  -O2
OBJS	= close.o filbuf.o flsbuf.o name.o open.o pix.o rdwr.o rle.o row.o

TARGETS = libimage.a

default: $(TARGETS)

$(TARGETS): $(OBJS)
	ar rcv $(TARGETS) $(OBJS)
