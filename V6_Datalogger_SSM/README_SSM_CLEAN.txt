ssm_code_t1_clean

Clean STM32CubeIDE project based on the uploaded .ioc setup.

User app files only:
- Core/Src/main.c + Core/Inc/main.h
- Core/Src/b64.c + Core/Inc/b64.h
- Core/Src/can.c + Core/Inc/can.h
- Core/Src/config.c + Core/Inc/config.h
- Core/Src/sd.c + Core/Inc/sd.h

Notes:
- Default config is SARTA with node IDs: 1716,1825,2571,2793,3063.
- USART3 is used for sARQ communication, copied from the uploaded .ioc setup.
- USART2 is optional debug UART.
- CAN1 setup/pins follow the uploaded .ioc setup.
- SD wrappers are included but currently safe stubs because config/logging is handled in code/sARQ for this clean version.
