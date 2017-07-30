
all:
	find . -maxdepth 1 -type f \
	| grep -v Makefile \
    | sed -e 's@./@@' \
    | xargs -n1 xxd -i \
    | sed \
        -e 's/\[\] =/\[\] ICACHE_RODATA_ATTR STORE_ATTR =/' \
        -e 's/unsigned char/static const char'/ \
        -e 's/unsigned int \([^ ]*\) = \([0-9]*\);/#define \1 \2/' >../include/files.h

clean:
	rm -f ../include/files.h