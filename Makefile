INCLUDES := types.h error.h
SOURCES := error types rio

IDIR := include
ODIR := obj
SDIR := src

FLAGS := -I$(IDIR) -O2 -Wall

DEPS := $(patsubst %,$(IDIR)/%,$(INCLUDES))
OBJ := $(patsubst %,$(ODIR)/%.o,$(SOURCES))

rio: $(OBJ)
	cc -o rio $^ $(FLAGS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	cc $(FLAGS) -o $@ -c $<
