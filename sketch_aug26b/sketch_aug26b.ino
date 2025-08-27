#include <Adafruit_SSD1306.h>

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;

struct LCD_VAR {
  unsigned int SCREEN_WIDTH = 128;
  unsigned int SCREEN_HEIGHT = 64;
  int OLED_RESET = -1;
  unsigned int OLED_SDA = 18;
  unsigned int OLED_SCL = 17;
};

struct PASSING {
  unsigned int delay = 500;
  String test = "HELLO WORLD";
};

static const LCD_VAR LCD;

static SemaphoreHandle_t MUTEX;

static const int LEDS = 37;
static int COUNT = 0;
static int HEAPALLOCT1 = 1000;
static int HEAPALLOCT2 = 1524;
const int BUTTON = 48;

static PASSING PASS;
static int DELAY = 500;

// must be volatile: written in ISR, read in task
static volatile bool INTERUPT = false;
static volatile bool LEDCON = false;

// OLED
Adafruit_SSD1306 display(LCD.SCREEN_WIDTH, LCD.SCREEN_HEIGHT, &Wire, LCD.OLED_RESET);


void IRAM_ATTR interupts() {
  INTERUPT = true;
 // Serial.println("INTERUPTING ALL TASK ");
}

bool display_status() {
  Wire.begin(LCD.OLED_SDA, LCD.OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return true;
  }
  return false;
}

int STACKVALT1() { return uxTaskGetStackHighWaterMark(Task1); }
int STACKVALT2() { return uxTaskGetStackHighWaterMark(Task2); }

void LED(int *delay) {
  digitalWrite(LEDS, HIGH);
  vTaskDelay(*delay / portTICK_PERIOD_MS);
  digitalWrite(LEDS, LOW);
  vTaskDelay(*delay / portTICK_PERIOD_MS);
}

void LCDS(int _param, PASSING *_pass) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("HEAP 1 :");
  display.println(STACKVALT1());

  display.print("HEAP 2 :");
  display.println(STACKVALT2());

  display.setCursor(0, 20);
  display.print("test line : ");
  display.println(_pass->test);

  display.setCursor(0, 54);
  display.print("Count: ");
  display.println(_param);

  display.display();
}

void FLED_IN(void * _param) {
  (void)_param;
  while (1) {
    if (INTERUPT) {
      vTaskDelay(pdMS_TO_TICKS(150));
      LEDCON = !LEDCON;
      vTaskDelay(150 / portTICK_PERIOD_MS);
      INTERUPT = false;
      digitalWrite(LEDS, LEDCON);
      Serial.println(LEDCON);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void FLED(void *_param) {
  int _delay = *((int *)_param);
  while (1) {
    if (xSemaphoreTake(MUTEX, 0) == pdTRUE) {
      COUNT++;
      xSemaphoreGive(MUTEX);
    } else {
      // optional: Serial.println("in use LED");
    }
    vTaskDelay(_delay / portTICK_PERIOD_MS);
  }
}

void FLCD(void *_param) {
  PASSING _pass = *((PASSING *)_param);
  while (1) {
    if (xSemaphoreTake(MUTEX, 0) == pdTRUE) {
      LCDS(COUNT, &_pass);
      xSemaphoreGive(MUTEX);
    } else {
      // optional: Serial.println("in use LCD");
    }
    vTaskDelay(_pass.delay / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(9600);
  delay(500);

  MUTEX = xSemaphoreCreateMutex();

  pinMode(LEDS, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // use FALLING to avoid double triggers on mechanical bounce when you only want presses
  attachInterrupt(digitalPinToInterrupt(BUTTON), interupts, FALLING);

  if (display_status()) {
    Serial.println("alloc faill");
  }

  xTaskCreatePinnedToCore(
    FLED,
    "LED",
    HEAPALLOCT1,
    &DELAY,
    1,
    &Task1,
    1
  );

  xTaskCreatePinnedToCore(
    FLCD,
    "LCD",
    HEAPALLOCT2,
    &PASS,
    2,
    &Task2,
    0
  );

  xTaskCreatePinnedToCore(
    FLED_IN,
    "LED_INTER",
    HEAPALLOCT1,
    NULL,
    2,
    &Task3,
    1
  );
}

void loop() {
  // empty
}
