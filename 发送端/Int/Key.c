#include "Key.h"

/**
 * @description: ��ⰴ��
 * @return {*} ���µİ���ֵ
 */
uint8_t Key_Detect(void)
{
    uint8_t res = 0;
    if (KEY1 == GPIO_PIN_RESET || KEY2 == GPIO_PIN_RESET || KEY3 == GPIO_PIN_RESET || KEY4 == GPIO_PIN_RESET )  
    {
        HAL_Delay(10);           /* ȥ���� */
        /* �����˳������������ͬʱ�������ȼ���KEY4>KEY3>KEY2>KEY1 */
        if (KEY1 == GPIO_PIN_RESET)  res = KEY1_PRESS;
        if (KEY2 == GPIO_PIN_RESET)  res = KEY2_PRESS;
        if (KEY3 == GPIO_PIN_RESET)  res = KEY3_PRESS;
        if (KEY4 == GPIO_PIN_RESET)  res = KEY4_PRESS;
    }
    return res;
}


