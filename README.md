# lte-pic32-writer

## IMPORTANT.
- Don't use the sendto.txt file in old version(<0.0.3).
- Please see the security advisories.

## How to build.
### PIC32MX

- On mplabx container. https://github.com/paijp/mplabx
- You may be required '--cap-add=SYS_RAWIO --device=/dev/bus/usb:/dev/bus/usb' options when creating a container.

```
# make bootloader_nostartup.hex
# make bootloader_nostartup.burn
```

### Wio LTE

- On arduino-cli container. https://github.com/paijp/arduino-cli-docker
- You may be required '--device=/dev/bus/usb:/dev/bus/usb' option when creating a container.

```
# arduino-cli compile --fqbn SeeedJP:Seeed_STM32F4:wioGpsM4 ltedollog
# arduino-cli upload --fqbn SeeedJP:Seeed_STM32F4:wioGpsM4 ltedollog
```

## connection

- WioLTE:TX -> PIC32MX:MCLR
- WioLTE:RX <- PIC32MX:PGED2(=RPB10=UTX2)
	- For log. Not required.
- WioLTE:D19 -> PIC32MX:PGED2
- WioLTE:D20 -> PIC32MX:PGEC2

## setup

- When using soracom beam... ( https://users.soracom.io/ja-jp/docs/beam/ )
	- udp://beam.soracom.io:23080 -> https://yourdomain.com/yourpath/
	- http://beam.soracom.io:18080 -> https://yourdomain.com/

- Deploy index.php to yourdomain/yourpath.
- And yourdomain/yourpath/yourIMEI/download.hex will be downloaded to the device when device powered.
- Log will be put to yourdomain/yourpath/yourIMEI/logYYMMDD.txt.
