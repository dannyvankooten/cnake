CFLAGS+= -std=c99 -Wall -Wextra -Wpedantic  \
-Wundef -Winline -Wimplicit-fallthrough -Wformat=2  \
-Wconversion -Wstringop-overflow=3 -Wvla

ifndef RELEASE
CFLAGS+= -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS  \
	-fsanitize=address -fsanitize=undefined -g        \
	-fstack-protector-strong -fstrict-aliasing
endif

cnake: cnake.c

.PHONY: clean
clean:
	rm -f cnake
