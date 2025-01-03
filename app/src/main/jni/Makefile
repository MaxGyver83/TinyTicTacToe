ANDROID_SDK_ROOT = $(HOME)/Android/Sdk
ANDROID_NDK_ROOT = $(ANDROID_SDK_ROOT)/ndk/27.2.12479018

src = $(wildcard *.c) $(wildcard lib/*.c)
src := $(filter-out main.c audio.c, $(src))
obj = $(src:.c=.o)
test_obj = test/tests.o $(filter-out main_x11.o gameflow.o, $(obj))

# CC = gcc
CFLAGS = -Wno-macro-redefined -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -DX11 -g # -fsanitize=address #-fdiagnostics-color=always
LDFLAGS = -lX11 -lGLESv2 -lEGL -lm -lvorbisfile -lasound

tinytictactoe: $(obj)
	echo "$(src)"
	$(CC) $^ -o $@ $(INC) $(CFLAGS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $< -o $@ -c $(INC) $(CFLAGS)

run: tinytictactoe
	if command -v unbuffer > /dev/null 2>&1; then \
		unbuffer ./tinytictactoe; \
	else \
		./tinytictactoe; \
	fi

tags:
	ctags_for_includes --skip-gcc -I $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64,/usr/include .

test/acutest.h:; wget --compression=gzip -P test https://raw.githubusercontent.com/mity/acutest/master/include/acutest.h

test/tests.o: test/tests.c test/acutest.h
	$(CC) -I/usr/include $(CFLAGS) -c $< -o $@ #-fno-math-errno

tests: utils.o test/tests.o
	$(CC) $^ -o $@ -I/usr/include $(CFLAGS) #-fno-math-errno

test: tests
	./tests -v

.PHONY: clean
clean:
	rm -f $(obj) test/tests.o tinytictactoe tests
