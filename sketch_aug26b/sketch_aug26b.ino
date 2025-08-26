#include <Adafruit_SSD1306.h>

TaskHandle_t Task1;
TaskHandle_t Task2;

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

static PASSING PASS;
static int DELAY = 500;
// other var
Adafruit_SSD1306 display(LCD.SCREEN_WIDTH, LCD.SCREEN_HEIGHT, &Wire, LCD.OLED_RESET);


bool display_status() {
  Wire.begin(LCD.OLED_SDA, LCD.OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return true;
  }
  return false;
}


int STACKVALT1() {
  return (uxTaskGetStackHighWaterMark(Task1));
}

int STACKVALT2() {
  return (uxTaskGetStackHighWaterMark(Task2));
}



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
  //for the memory left
  display.setCursor(0, 0);
  //HEAP SIZE 1
  display.print("HEAP 1 :");
  display.println(STACKVALT1());
  display.setTextSize(1);
  //HEAP SIZE 2
  display.print("HEAP 2 :");
  display.println(STACKVALT2());
  display.setTextSize(1);
  //test show
  display.setCursor(0, 20);
  display.print("test line : ");
  display.println(_pass->test);
  display.setTextSize(1);



  //LAST LINE
  display.setCursor(0, 54);
  display.print("Count: ");
  display.println(_param);
  display.display();
}










void FLED(void *_param) {

  int _delay = *((int *)_param);
  while (1) {
    LED(&_delay);
    //take mutex
    if (xSemaphoreTake(MUTEX, 0) == pdTRUE) {
      COUNT++;  //increment glob var
      xSemaphoreGive(MUTEX);
    } else {
      Serial.println("in use LED");
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
      Serial.println("in use LCD");
    }
    vTaskDelay(_pass.delay / portTICK_PERIOD_MS);
  }
}

void setup() {
  //setup serial
  Serial.begin(9600);
  delay(500);

  MUTEX = xSemaphoreCreateMutex();

  //led
  pinMode(LEDS, OUTPUT);

  //lcd
  if (display_status()) {
    Serial.println("alloc faill");
  }
  //rtos alloc

  xTaskCreatePinnedToCore(
    FLED,
    "LED",
    HEAPALLOCT1,
    & DELAY,
    1,
    &Task1,
    1);

  xTaskCreatePinnedToCore(
    FLCD,
    "LCD",
    HEAPALLOCT2,
    & PASS,
    2,
    &Task2,
    0);
}

void loop() {
  // put your main code here, to run repeatedly:
}
