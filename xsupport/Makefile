# MAKE FILE OF XSUPPORT PACKAGE; NON-SGI VERSION.
#
# Apostolos Lerios - tolis@cs.


UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  INCS := -I/opt/X11/include -I/usr/OpenMotif/include
endif
ifeq ($(UNAME), Linux)
  LIBS := -lGL -lGLU -lglut -lglui
endif

# CONSTANT DEFINITIONS.

# Include dbx debugging symbols or optimize code.

DEBUG=-g

# Directories for targets and dependents.

TARGET=../

# Shorthand for C++ compiler invocation.

C_COMPILE=g++ -O2 $(DEBUG) $(INCS) -c -o $@

AR=/usr/bin/ar

# LINKING.

OBJS=xsupport.o scene_io.o xgetscene.o ppm.o

install:	$(TARGET)libxsupport.a

$(TARGET)libxsupport.a: $(OBJS) $(MAKEFILE)
	$(AR) ru $@ $(OBJS)


# COMPILATION.

xsupport.o: xsupport.cpp xsupport.h $(MAKEFILE)
	$(C_COMPILE) xsupport.cpp

scene_io.o: scene_io.cpp scene_io.h $(MAKEFILE)
	$(C_COMPILE) scene_io.cpp

xgetscene.o: xgetscene.cpp $(MAKEFILE)
	$(C_COMPILE) xgetscene.cpp

ppm.o: ppm.cpp xsupport.h $(MAKEFILE)
	$(C_COMPILE) ppm.cpp

# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)libxsupport.a
