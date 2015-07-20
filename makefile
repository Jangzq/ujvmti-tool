GCC	:= gcc
LD	:= ld

LDFLAGS	:= -shared -fpic
CFLAGS	:= -O2  -fpic  -Wall -g

SRC	:= $(wildcard ./*.c)
OBJ	:= $(patsubst %.c, ./%.o, $(notdir ${SRC}))

SHARE_LIB	:= libujvmti.so

INC	:= -I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/linux
 
all:$(OBJ)
	$(LD) $(LDFLAGS) -o  $(SHARE_LIB) $(OBJ)

./%.o:./%.c
	@echo Compiling $(OBJ) ...
	$(GCC) $(CFLAGS) -o $@ -c $< ${INC} 
	
clean:

	rm -rf  ./*.o ./*.so 