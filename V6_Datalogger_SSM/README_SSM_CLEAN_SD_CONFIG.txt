Clean STM32 SSM project - SD CONFIG version

Important change:
- config.c/.h are now patterned from cdrf_main config.ino + sd.ino.ino.
- CONFIG.TXT is read from the SD card using FatFS.
- The old library-style SARTA list was removed from config.c.

For SARTA test, copy CONFIG_SAMPLE.txt contents into SD card as CONFIG.TXT.
