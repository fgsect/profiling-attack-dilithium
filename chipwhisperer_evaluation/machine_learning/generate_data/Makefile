SOURCES = sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c 
KECCAK_SOURCES = $(SOURCES) fips202.c symmetric-shake.c
HEADERS = config.h params.h api.h sign.h packing.h polyvec.h poly.h ntt.h \
  reduce.h rounding.h symmetric.h randombytes.h 

all: 
	clang rounding.c ntt.c reduce.c poly.c fips202.c symmetric-shake.c main.c -o main
	clang rounding.c ntt.c reduce.c poly.c fips202.c symmetric-shake.c main_xof.c -o main_xof
