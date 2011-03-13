# MAKE FILE OF XSUPPORT PACKAGE; NON-SGI VERSION.
#
# Apostolos Lerios - tolis@cs.


# CONSTANT DEFINITIONS.

# Include dbx debugging symbols or optimize code.

DEBUG=-g

# Directories for targets and dependents.

TARGET=../

# Shorthand for C++ compiler invocation.

C_COMPILE=g++ $(DEBUG) -I/opt/local/include -c -o $@

AR=/usr/bin/ar

# LINKING.

OBJS=xsupport.o scene_io.o xgetscene.o ppm.o

install:	$(TARGET)libxsupport.a

$(TARGET)libxsupport.a: $(OBJS) $(MAKEFILE)
	$(AR) ru $@ $(OBJS)


# COMPILATION.

xsupport.o: xsupport.c xsupport.h $(MAKEFILE)
	$(C_COMPILE) xsupport.c

scene_io.o: scene_io.c scene_io.h $(MAKEFILE)
	$(C_COMPILE) scene_io.c

xgetscene.o: xgetscene.c $(MAKEFILE)
	$(C_COMPILE) xgetscene.c

ppm.o: ppm.c xsupport.h $(MAKEFILE)
	$(C_COMPILE) ppm.c

# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)libxsupport.a
