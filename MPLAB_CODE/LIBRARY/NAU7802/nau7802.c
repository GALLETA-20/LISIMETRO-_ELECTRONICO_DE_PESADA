#include "nau7802.h"
#include "mcc_generated_files/system/system.h"

/* --- Capa de bajo nivel I2C (MCC Melody) --- */
static bool i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    if (!I2C1_Write(NAU7802_I2C_ADDR, buf, 2)) return false;
    while (I2C1_IsBusy());
    return (I2C1_ErrorGet() == I2C_ERROR_NONE);
}

static bool i2c_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    if (!I2C1_WriteRead(NAU7802_I2C_ADDR, &reg, 1, data, len)) return false;
    while (I2C1_IsBusy());
    return (I2C1_ErrorGet() == I2C_ERROR_NONE);
}

static bool i2c_read_reg(uint8_t reg, uint8_t *val) {
    return i2c_read_regs(reg, val, 1);
}

/* --- Funciones públicas --- */
bool NAU7802_Init(void) {
    uint8_t reg = 0;

    // 1. Reset de registros (RR = 1, luego RR = 0)
    if (!i2c_write_reg(NAU7802_PU_CTRL, 0x01)) return false;
    __delay_ms(2);
    if (!i2c_write_reg(NAU7802_PU_CTRL, 0x00)) return false;

    // 2. Encender digital, analógico y LDO interno (AVDDS=1, PUA=1, PUD=1 -> 0x86)
    if (!i2c_write_reg(NAU7802_PU_CTRL, 0x86)) return false;

    // Esperar a que PUR (bit 3) esté listo (máx. 200 ms)
    uint16_t timeout = 200;
    do {
        if (!i2c_read_reg(NAU7802_PU_CTRL, &reg)) return false;
        if (reg & 0x08) break;
        __delay_ms(1);
    } while (--timeout);
    if (timeout == 0) return false;

    // 3. Configurar LDO = 3.0V (5 << 3) y Ganancia = x128 (7) -> 0x2F
    if (!i2c_write_reg(NAU7802_CTRL1, (5U << 3) | NAU7802_GAIN_128)) return false;

    // 4. Configurar 10 SPS (0 << 4) y Canal 1 (0)
    if (!i2c_write_reg(NAU7802_CTRL2, (NAU7802_SPS_10 << 4))) return false;

    // 5. Desactivar chopper clock (recomendado por datasheet para menor ruido)
    if (!i2c_write_reg(NAU7802_ADC, 0x30)) return false;

    // 6. Habilitar capacitor de desacoplo del PGA (módulos con C de 330nF en REFP/REFN)
    if (!i2c_write_reg(NAU7802_PWR_CTRL, 0x80)) return false;

    // 7. Iniciar ciclo de conversiones (CS = 1 -> bit 4)
    if (!i2c_read_reg(NAU7802_PU_CTRL, &reg)) return false;
    if (!i2c_write_reg(NAU7802_PU_CTRL, reg | 0x10)) return false;

    // 8. Calibración interna del ADC
    return NAU7802_Calibrate();
}

bool NAU7802_Calibrate(void) {
    uint8_t reg = 0;

    if (!i2c_read_reg(NAU7802_CTRL2, &reg)) return false;
    // Iniciar calibración interna (CALS = bit 2)
    if (!i2c_write_reg(NAU7802_CTRL2, reg | 0x04)) return false;

    uint16_t timeout = 500;
    do {
        __delay_ms(5);
        if (!i2c_read_reg(NAU7802_CTRL2, &reg)) return false;
        if ((reg & 0x04) == 0) break; // Terminó cuando CALS vuelve a 0
    } while (--timeout);

    if (timeout == 0) return false;
    if (reg & 0x08)   return false;   // CAL_ERR = 1 indica error de calibración

    return true;
}

bool NAU7802_DataReady(void) {
    uint8_t reg = 0;
    if (!i2c_read_reg(NAU7802_PU_CTRL, &reg)) return false;
    return (reg & 0x20) != 0; // Bit 5 (CR): Cycle Ready
}

bool NAU7802_ReadRaw(int32_t *out) {
    uint8_t buf[3];
    if (!i2c_read_regs(NAU7802_ADCO_B2, buf, 3)) return false;

    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    if (raw & 0x00800000UL) {
        raw |= 0xFF000000UL; // Extensión de signo de 24 bits a 32 bits
    }

    *out = (int32_t)raw;
    return true;
}

bool NAU7802_ReadAverage(uint8_t samples, int32_t *out) {
    if (samples == 0) return false;

    int64_t suma = 0;
    int32_t val = 0;

    for (uint8_t i = 0; i < samples; i++) {
        uint16_t timeout = 200;
        while (!NAU7802_DataReady()) {
            __delay_ms(2);
            if (--timeout == 0) return false;
        }
        if (!NAU7802_ReadRaw(&val)) return false;
        suma += val;
    }

    *out = (int32_t)(suma / samples);
    return true;
}
