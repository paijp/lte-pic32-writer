
DEVICE	= 32MX270F256B
CC	= /opt/microchip/xc32/v1.42/bin/xc32-gcc -mprocessor=$(DEVICE) -ffreestanding -fno-hosted -nodefaultlibs -membedded-data -Wall -Wno-pointer-sign -O1 -I .
AS	= /opt/microchip/xc32/v1.42/bin/xc32-as
LD	= /opt/microchip/xc32/v1.42/bin/xc32-gcc -mprocessor=$(DEVICE) -mno-float
BIN2HEX	= /opt/microchip/xc32/v1.42/bin/xc32-bin2hex
DATE	= $(shell date '+%y%m%d_%H%M%S')


.SECONDARY:


help:	makefile.done
	@awk '/#[#]/{sub(":[^#]*", "\t\t");print;}' makefile

%.build:	%.hex		## build target.hex
	@echo

%.burn:	%.build %.mdb		## program a device.
	mdb $*.mdb

%.burnp:	%.build %.mdbp		## program a device with power.
	mdb $*.mdbp

%.mdbp:	makefile.done
	echo 'device pic$(DEVICE)' > $@
	echo 'set AutoSelectMemRanges auto' >> $@
	echo 'set poweroptions.powerenable true' >> $@
	echo 'set voltagevalue 3.3' >> $@
	echo 'hwtool pickit3 -p' >> $@
	echo 'program $*.hex' >> $@
	echo 'quit' >> $@

%.mdb:	makefile.done
	echo 'device pic$(DEVICE)' > $@
	echo 'set AutoSelectMemRanges auto' >> $@
#	echo 'set poweroptions.powerenable true' >> $@
	echo 'set voltagevalue 3.3' >> $@
	echo 'hwtool pickit3 -p' >> $@
	echo 'program $*.hex' >> $@
	echo 'quit' >> $@

%_nostartup.hex:	%.o
	$(LD) -nostartfiles -Wl,-Map=$*_nostartup.map -o $*_nostartup.out $^
	$(BIN2HEX) $*_nostartup.out
	@mkdir -p hist
	cp $@ hist/$@_$(DATE)

%.hex:	%.o
	$(LD) -Wl,-Map=$*.map -o $*.out $^
	$(BIN2HEX) $*.out
	@mkdir -p hist
	cp $@ hist/$@_$(DATE)

%.o:	%.c
	@mkdir -p hist
	cp $< hist/$<_$(DATE)
	$(CC) -c $<

%.o:	%.s
	@mkdir -p hist
	cp $< hist/$<_$(DATE)
	$(AS) -o $@ $<

%.done:	%
	@mkdir -p hist
	cp $< hist/$<_$(DATE)
	touch $@

