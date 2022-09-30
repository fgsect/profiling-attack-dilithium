TARGET = unpack
# Every .c and .S file which should be included in the build has to be added
SRC += rounding.c poly.c polyvec.c fips202.c symmetric-shake.c main.c 
define SS_VER
SS_VER_2_0
endef
#Add simpleserial project to build
include ../../hardware/victims/firmware/simpleserial/Makefile.simpleserial
FIRMWAREPATH = ../../hardware/victims/firmware
include $(FIRMWAREPATH)/Makefile.inc
