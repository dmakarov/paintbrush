# Include debugging symbols (-g) or optimize code (-O or -O2).

DEBUG=-g

X_LIBS = -lXm -lXp -lXext -lXt -lX11 -lm

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  INCS := -I/opt/X11/include
  LIBS := -L/opt/X11/lib -L/usr/OpenMotif/lib
endif
ifeq ($(UNAME), Linux)
  LIBS := -L/opt/local/lib
endif

# The object files comprising the application code.
# Use spaces to separate multiple files.

OBJS=paint.o

# The name of the executable.

TARGET=paint

# LINKING.

$(TARGET): $(OBJS) Makefile libxsupport.a
	$(CXX) -Wall $(DEBUG) -o $@ $(OBJS) -L. $(LIBS) -lxsupport $(X_LIBS) 

# COMPILATION.

# xsupport stuff
libxsupport.a:
	make -C xsupport

.cpp.o:
	$(CXX) $(INCS) -Wall -Wno-write-strings -O2 $(DEBUG) -c -o $@ $*.cpp

# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)
	make -C xsupport clean
