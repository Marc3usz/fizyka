IDIR = ./include
CC = gcc
CFLAGS = -I$(IDIR)
ODIR = ./obj
SDIR = ./src
LIBS = -lm



##################################################################

_DEPS = 
_OBJ = main.o

##################################################################



DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

.PHONY: all clean
all: fizyka

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir $@

fizyka: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -rf $(ODIR)/*.o fizyka *~ core $(IDIR)/*~