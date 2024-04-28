add_executable(main
        hc06.c
        ssd1306.c
        main.c
)

target_link_libraries(main pico_stdlib hardware_adc hardware_i2c freertos)
pico_add_extra_outputs(main)
