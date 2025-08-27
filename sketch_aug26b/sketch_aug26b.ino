#include <Adafruit_SSD1306.h>

TaskHandle_t task1;
TaskHandle_t task2;
TaskHandle_t task3;

struct lcd_var {
  unsigned int screen_width = 128;
  unsigned int screen_height = 64;
  int oled_reset = -1;
  unsigned int oled_sda = 18;
  unsigned int oled_scl = 17;
};

struct passing {
  unsigned int delay = 500;
  String test = "HELLO WORLD";
};

static const lcd_var lcd;
static passing pass;

static SemaphoreHandle_t mutex;
static SemaphoreHandle_t led_gate;

static const int leds = 37;
static volatile unsigned int count = 0;
static int heapalloc_t1 = 1000;
static int heapalloc_t2 = 1524;
const int button = 48;

static bool ledcon = false;
static bool reopen = true;

static int delay_time = 500;
static const unsigned int debauch_t = 150;

//ISR VAR
//static volatile bool interupt = false;
static volatile unsigned long last_millis;

// OLED
Adafruit_SSD1306 display(lcd.screen_width, lcd.screen_height, &Wire, lcd.oled_reset);

void IRAM_ATTR interupts() {
  unsigned long _current_millis = millis();
  if (_current_millis - last_millis < debauch_t) {
    return;
  }
 // xSemaphoreGive(led_gate);
  xTaskNotifyGive(task3);
  last_millis = _current_millis;
  Serial.println("INTERUPTING ALL TASK ");
}

bool display_status() {
  Wire.begin(lcd.oled_sda, lcd.oled_scl);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return true;
  }
  return false;
}

int stack_val_t1() { return uxTaskGetStackHighWaterMark(task1); }
int stack_val_t2() { return uxTaskGetStackHighWaterMark(task2); }
int stack_val_t3() { return uxTaskGetStackHighWaterMark(task3); }

void led(int *ptr_delay) {
  digitalWrite(leds, HIGH);
  vTaskDelay(*ptr_delay / portTICK_PERIOD_MS);
  digitalWrite(leds, LOW);
  vTaskDelay(*ptr_delay / portTICK_PERIOD_MS);
}

void lcds(int _param, passing *ptr_pass) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("HEAP 1 :");
  display.println(stack_val_t1());

  display.print("HEAP 2 :");
  display.println(stack_val_t2());

  display.print("HEAP 3 :");
  display.println(stack_val_t3());

  display.setCursor(0, 40);
  display.print("test line : ");
  display.println(ptr_pass->test);

  display.setCursor(0, 54);
  display.print("Count: ");
  display.println(_param);

  display.display();
}

void fled_in(void *ptr_param) {
  (void)ptr_param;
  while (1) {
     if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) == pdPASS) {
      if(!reopen){
        xSemaphoreGive(led_gate);
      }
      if(xSemaphoreTake(led_gate, portMAX_DELAY) == pdPASS){
        ledcon = !ledcon;
        digitalWrite(leds, ledcon);
        reopen = false;
      }
     }    
      else{
          if(!reopen && !ledcon){
          xSemaphoreGive(led_gate);
          reopen = true;
          }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }


void fled(void *ptr_param) {
  int _delay = *((int *)ptr_param);
  while (1) {
    if(xSemaphoreTake(led_gate, 1) == pdPASS){
      led(&_delay);
      xSemaphoreGive(led_gate);
    }
   
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      count++;
      xSemaphoreGive(mutex);
    } else {
      // optional: Serial.println("in use LED");
    }
    vTaskDelay(_delay / portTICK_PERIOD_MS);
  }
}

void flcd(void *ptr_param) {
  passing _pass = *((passing *)ptr_param);
  while (1) {
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      lcds(count, &_pass);
      xSemaphoreGive(mutex);
    } else {
      // optional: Serial.println("in use LCD");
    }
    vTaskDelay(_pass.delay / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(9600);
  delay(500);

  mutex = xSemaphoreCreateMutex();
  led_gate = xSemaphoreCreateBinary();
  xSemaphoreGive(led_gate);

  pinMode(leds, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  // use FALLING to avoid double triggers on mechanical bounce when you only want presses
  attachInterrupt(digitalPinToInterrupt(button), interupts, FALLING);

  if (display_status()) {
    Serial.println("alloc faill");
  }

  xTaskCreatePinnedToCore(
    fled,
    "LED",
    heapalloc_t1,
    &delay_time,
    1,
    &task1,
    1
  );

  xTaskCreatePinnedToCore(
    flcd,
    "LCD",
    heapalloc_t2,
    &pass,
    2,
    &task2,
    0
  );

  xTaskCreatePinnedToCore(
    fled_in,
    "LED_INTER",
    heapalloc_t1,
    NULL,
    2,
    &task3,
    1
  );
}

void loop() {
  // empty
}
