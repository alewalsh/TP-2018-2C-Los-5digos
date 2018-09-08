RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" ! -iname "feature_tests.c" ! -iname "CMakeCCompilerId.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# Clean and compile .so
all: build/libtpcommons.so

create-dirs:
	mkdir -p build/tpcommons

build/libtpcommons.so: create-dirs $(OBJS)
	$(CC) -DDEBUG -g -shared -o "build/libtpcommons.so" $(OBJS)

build/%.o: %.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
debug: CC += -DDEBUG -g
debug: all

debug: all

# Clean build files
clean:
	$(RM) build

.PHONY: all create-dirs clean
