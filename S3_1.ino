#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// ============================================================
// HARDWARE
// ============================================================

#define SDA_PIN       8
#define SCL_PIN       9

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C

#define RGB_PIN       48
#define RGB_COUNT     1

// ============================================================
// OBJECTS
// ============================================================

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Adafruit_NeoPixel pixel(
  RGB_COUNT,
  RGB_PIN,
  NEO_GRB + NEO_KHZ800
);

// ============================================================
// LIVE GRAPH
// ============================================================

#define GRAPH_POINTS 120

float temperatureHistory[GRAPH_POINTS];

int graphIndex = 0;
bool graphFilled = false;

unsigned long lastSample = 0;
const unsigned long SAMPLE_INTERVAL = 250;

// ============================================================
// RGB STATUS
// ============================================================

enum TemperatureStatus {
  STATUS_COOL,
  STATUS_NORMAL,
  STATUS_ELEVATED,
  STATUS_HIGH
};

TemperatureStatus currentStatus = STATUS_NORMAL;

// ============================================================
// RGB FUNCTIONS
// ============================================================

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

TemperatureStatus getStatus(float temp) {

  if (temp < 20.0) {
    return STATUS_COOL;
  }

  if (temp < 37.5) {
    return STATUS_NORMAL;
  }

  if (temp < 40.0) {
    return STATUS_ELEVATED;
  }

  return STATUS_HIGH;
}

void updateRGB(float temp) {

  currentStatus = getStatus(temp);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  switch (currentStatus) {

    case STATUS_COOL:
      r = 0;
      g = 50;
      b = 180;
      break;

    case STATUS_NORMAL:
      r = 0;
      g = 180;
      b = 60;
      break;

    case STATUS_ELEVATED:
      r = 220;
      g = 100;
      b = 0;
      break;

    case STATUS_HIGH:
      r = 220;
      g = 0;
      b = 20;
      break;
  }

  // Very subtle breathing effect.
  float pulse = (sin(millis() / 500.0) + 1.0) / 2.0;

  float brightness = 0.45 + pulse * 0.25;

  setRGB(
    r * brightness,
    g * brightness,
    b * brightness
  );
}

// ============================================================
// GRAPH
// ============================================================

void addTemperature(float temp) {

  temperatureHistory[graphIndex] = temp;

  graphIndex++;

  if (graphIndex >= GRAPH_POINTS) {
    graphIndex = 0;
    graphFilled = true;
  }
}

int getHistoryCount() {

  if (graphFilled) {
    return GRAPH_POINTS;
  }

  return graphIndex;
}

float getHistoryValue(int position) {

  int count = getHistoryCount();

  if (count == 0) {
    return NAN;
  }

  if (graphFilled) {
    int index = (graphIndex + position) % GRAPH_POINTS;
    return temperatureHistory[index];
  }

  return temperatureHistory[position];
}

// ============================================================
// DRAW LIVE GRAPH
// ============================================================

void drawGraph() {

  const int graphLeft   = 4;
  const int graphRight  = 123;
  const int graphTop    = 31;
  const int graphBottom = 49;

  int count = getHistoryCount();

  if (count < 2) {
    return;
  }

  // ----------------------------------------------------------
  // Find min/max
  // ----------------------------------------------------------

  float minTemp = 1000.0;
  float maxTemp = -1000.0;

  for (int i = 0; i < count; i++) {

    float value = getHistoryValue(i);

    if (!isnan(value)) {
      if (value < minTemp) minTemp = value;
      if (value > maxTemp) maxTemp = value;
    }
  }

  // Keep a minimum graph range so tiny fluctuations
  // don't make the graph jump around wildly.
  float range = maxTemp - minTemp;

  if (range < 1.0) {
    float center = (maxTemp + minTemp) / 2.0;

    minTemp = center - 0.5;
    maxTemp = center + 0.5;
  } else {

    float padding = range * 0.15;

    minTemp -= padding;
    maxTemp += padding;
  }

  // ----------------------------------------------------------
  // Draw graph baseline
  // ----------------------------------------------------------

  display.drawFastHLine(
    graphLeft,
    graphBottom,
    graphRight - graphLeft,
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // Draw waveform
  // ----------------------------------------------------------

  int previousX = graphLeft;
  int previousY = graphBottom;

  for (int i = 0; i < count; i++) {

    float value = getHistoryValue(i);

    if (isnan(value)) {
      continue;
    }

    int x;

    if (count == 1) {
      x = graphLeft;
    } else {
      x = map(
        i,
        0,
        count - 1,
        graphLeft,
        graphRight
      );
    }

    float normalized =
      (value - minTemp) /
      (maxTemp - minTemp);

    int y =
      graphBottom -
      normalized * (graphBottom - graphTop);

    y = constrain(y, graphTop, graphBottom);

    if (i > 0) {

      display.drawLine(
        previousX,
        previousY,
        x,
        y,
        SSD1306_WHITE
      );
    }

    previousX = x;
    previousY = y;
  }

  // Current reading marker
  display.fillCircle(
    previousX,
    previousY,
    1,
    SSD1306_WHITE
  );
}

// ============================================================
// OLED UI
// ============================================================

void drawUI(float objectTemp, float ambientTemp) {

  display.clearDisplay();

  // ----------------------------------------------------------
  // MAIN TEMPERATURE
  // ----------------------------------------------------------

  char tempString[16];

  snprintf(
    tempString,
    sizeof(tempString),
    "%.1f",
    objectTemp
  );

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setFont(NULL);

  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  display.getTextBounds(
    tempString,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );

  int tempX = (128 - w - 12) / 2;

  display.setCursor(tempX, 1);
  display.print(tempString);

  // Degree symbol
  display.setTextSize(1);
  display.setCursor(tempX + w + 2, 3);
  display.print((char)248);

  // C
  display.setCursor(tempX + w + 7, 4);
  display.print("C");

  // ----------------------------------------------------------
  // LIVE GRAPH
  // ----------------------------------------------------------

  drawGraph();

  // ----------------------------------------------------------
  // FAHRENHEIT
  // ----------------------------------------------------------

  float fahrenheit =
    objectTemp * 9.0 / 5.0 + 32.0;

  display.setTextSize(1);

  display.setCursor(5, 52);
  display.print(fahrenheit, 1);
  display.print(" F");

  // ----------------------------------------------------------
  // AMBIENT
  // ----------------------------------------------------------

  display.setCursor(78, 52);
  display.print("AMB ");

  display.print(ambientTemp, 1);

  display.print("C");

  // ----------------------------------------------------------
  // Small status indicator
  // ----------------------------------------------------------

  display.fillCircle(
    124,
    58,
    2,
    SSD1306_WHITE
  );

  display.display();
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" ESP32-S3 THERMAL MONITOR");
  Serial.println("================================");

  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Serial.print("OLED... ");

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDR
      )) {

    Serial.println("FAILED");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("OK");

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(25, 28);
  display.print("INITIALIZING");

  display.display();

  // ----------------------------------------------------------
  // MLX90614
  // ----------------------------------------------------------

  Serial.print("MLX90614... ");

  if (!mlx.begin()) {

    Serial.println("FAILED");

    display.clearDisplay();

    display.setCursor(22, 28);
    display.print("SENSOR ERROR");

    display.display();

    while (true) {
      delay(1000);
    }
  }

  Serial.println("OK");

  // ----------------------------------------------------------
  // RGB
  // ----------------------------------------------------------

  pixel.begin();
  pixel.clear();
  pixel.show();

  setRGB(0, 0, 80);

  // ----------------------------------------------------------
  // Initialize graph
  // ----------------------------------------------------------

  float initialTemp = mlx.readObjectTempC();

  for (int i = 0; i < GRAPH_POINTS; i++) {
    temperatureHistory[i] = initialTemp;
  }

  graphFilled = true;
  graphIndex = 0;

  Serial.println();
  Serial.println("SYSTEM READY");
  Serial.println("--------------------------------");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL) {

    lastSample = now;

    // --------------------------------------------------------
    // Read sensor
    // --------------------------------------------------------

    float objectTemp = mlx.readObjectTempC();
    float ambientTemp = mlx.readAmbientTempC();

    // --------------------------------------------------------
    // Validate
    // --------------------------------------------------------

    if (isnan(objectTemp) || isnan(ambientTemp)) {

      Serial.println("Sensor reading error");

      return;
    }

    // --------------------------------------------------------
    // Store real reading in graph
    // --------------------------------------------------------

    addTemperature(objectTemp);

    // --------------------------------------------------------
    // Update RGB
    // --------------------------------------------------------

    updateRGB(objectTemp);

    // --------------------------------------------------------
    // Update OLED
    // --------------------------------------------------------

    drawUI(
      objectTemp,
      ambientTemp
    );

    // --------------------------------------------------------
    // Serial monitor
    // --------------------------------------------------------

    float fahrenheit =
      objectTemp * 9.0 / 5.0 + 32.0;

    Serial.print("Object: ");
    Serial.print(objectTemp, 2);
    Serial.print(" C / ");

    Serial.print(fahrenheit, 2);
    Serial.print(" F");

    Serial.print("    Ambient: ");
    Serial.print(ambientTemp, 2);
    Serial.println(" C");
  }
}