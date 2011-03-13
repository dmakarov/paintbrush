
# Include debugging symbols (-g) or optimize code (-O or -O2).

DEBUG=-g

# CS248 software directory.

BASEDIR=/afs/ir.stanford.edu/class/cs248/support
CS248_LIBS=-L/opt/local/lib -Xlinker -rpath -Xlinker .
X_LIBS = -lXm -lXp -lXext -lXt -lX11 -lm

# The object files comprising the application code.
# Use spaces to separate multiple files.

OBJS=paint.o

# The name of the executable.

TARGET=paint

# LINKING.

$(TARGET): $(OBJS) Makefile libxsupport.a
	$(CXX) -Wall $(DEBUG) -o $@ $(OBJS) -L. \
	$(CS248_LIBS) -lxsupport $(X_LIBS) 


# COMPILATION.

# xsupport stuff
libxsupport.a:
	make -C xsupport

.c.o:
	$(CXX) -Wall -Wno-write-strings -I$(BASEDIR)/include/ $(DEBUG) -c -o $@ $*.c


# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)
	make -C xsupport clean

