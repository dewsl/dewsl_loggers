ssm_code_t1_final
=================

This STM32CubeIDE project was rebuilt from the user's existing .ioc setup and the red-highlighted target files.

Reference mapping:
- Previous SSM code reference: cdrf_main Arduino project
- Current STM32 target: ssm_code_t1
- Current sARQ communication partner: V6_Datalogger_sARQ

Included target files:
- Core/Src/main.c and Core/Inc/main.h
- Core/Src/b64.c and Core/Inc/b64.h
- Core/Src/can.c and Core/Inc/can.h
- Core/Src/config.c and Core/Inc/config.h
- Core/Src/sd.c and Core/Inc/sd.h

Important config behavior:
- config.c is patterned from cdrf_main/config.ino.
- SARTA is added as a config_container[] library entry, not forced as active default.
- To activate SARTA for testing, flash the STM32 then send this through USART2 debug serial:
    D=SARTA
- Verify config with:
    ?

V6_Datalogger_sARQ side assumptions:
- config_ssm.cpp includes SARTA.
- lib_LOGGER_COUNT is updated to 81.
- V6 ssm.ino has getSSMData().
- v6_operation.ino calls:
    if (ssmFlag) getSSMData();

Notes:
- USART3 is used as the sARQ <-> STM32 UART based on the provided .ioc.
- USART2 is used for debug commands.
- CAN1 and SPI2/FATFS follow the provided .ioc setup.
- Current/voltage readings are placeholders unless INA/ADC current-voltage monitor is added to the STM32 project.
