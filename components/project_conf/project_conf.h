//
// Created by HAIRONG ZHU on 25-1-3.
//

#ifndef FOCKNOB_PROJECT_CONF_H
#define FOCKNOB_PROJECT_CONF_H


#define IIC_MASTER_NUM                  I2C_NUM_0         // IIC port number for master
#define IIC_MASTER_FREQ_HZ              100000            // IIC master clock frequency
#define IIC_MASTER_SDA_IO               GPIO_NUM_15
#define IIC_MASTER_SCL_IO               GPIO_NUM_16

#define IIC_AS5600_ADDR                 0x36
#define IIC_AS5600_RAW_ANGLE_REG        0x0C
#define IIC_AS5600_RESOLUTION           4096

#define FOC_DRV_EN_GPIO                 GPIO_NUM_4
#define FOC_MCPWM_U_GPIO                GPIO_NUM_5
#define FOC_MCPWM_V_GPIO                GPIO_NUM_6
#define FOC_MCPWM_W_GPIO                GPIO_NUM_7
#define FOC_CALC_PERIOD                 1000    // 电机控制周期，单位(us) 1s = 1000ms = 1000000us, 1us = 1e-6s
#define FOC_MCPWM_TIMER_RESOLUTION_HZ   80000000
#define FOC_MCPWM_PERIOD                2000   // 最大力矩为 FOC_MCPWM_PERIOD / 2
#define FOC_LOW_PASS_FILTER_ALPHA       0.3
#define FOC_MOTOR_POLE_PAIRS            7

#define SPI_LCD_HOST                    SPI2_HOST  // 或者 SPI3_HOST，根据具体使用的 SPI 总线
#define SPI_LCD_H_RES                   240        // 根据你的 LCD 分辨率定义
#define SPI_LCD_V_RES                   240        // 根据你的 LCD 分辨率定义
#define SPI_LCD_RST                     13
#define SPI_LCD_PCLK                    12
#define SPI_LCD_DC                      11
#define SPI_LCD_CS                      10
#define SPI_LCD_MOSI                    9


#endif //FOCKNOB_PROJECT_CONF_H