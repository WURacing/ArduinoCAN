#define TMP006_TAMB 0x01
#define TMP006_CONFIG 0x02


// Configures sensor, use before reading from it
void config_TMP006(uint8_t addr, uint16_t samples);

// Read raw sensor temperature
int16_t readRawDieTemperature(uint8_t addr);

// Read raw thermopile voltage
int16_t readRawVoltage(uint8_t addr);

// Calculate object temperature based on raw sensor temp and thermopile voltage
double readObjTempC(uint8_t addr);

// Caculate sensor temperature based on raw reading
double readDieTempC(uint8_t addr);
