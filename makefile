IDIR = ./include
CC = gcc
CFLAGS = -I$(IDIR) -std=gnu99 -O3
LDFLAGS = -L./lib
ODIR = ./obj
SDIR = ./src
LIBS = -lm -l:raylibdll.lib -lopengl32 -lgdi32 -lwinmm



##################################################################

_DEPS = 
_OBJ = main.o sim.o arena.o sized_string.o

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
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(ODIR)/*.o fizyka *~ core $(IDIR)/*~