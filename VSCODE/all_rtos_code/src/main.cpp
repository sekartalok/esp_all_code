
#include <Adafruit_SSD1306.h>
#include "task.h"

//call my class
#include "interupt.h"


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
  String test = "HELLOWORLD";
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
static const unsigned int debauch_t = 550;
//queue
static QueueHandle_t interupt_count_queue;
static const unsigned int interupt_count_queue_max = 12;
//class
interupt my_interupt;
//ISR VAR
//static volatile bool interupt = false;
//static volatile unsigned long last_millis;

// OLED
Adafruit_SSD1306 display(lcd.screen_width, lcd.screen_height, &Wire, lcd.oled_reset);
/*
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
*/
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
//int prosseror_usage() {return 100 - ulTaskGetIdleRunTimeCounter(); }

void led(int *ptr_delay) {
  digitalWrite(leds, HIGH);
  vTaskDelay(*ptr_delay / portTICK_PERIOD_MS);
  digitalWrite(leds, LOW);
  vTaskDelay(*ptr_delay / portTICK_PERIOD_MS);
}

void lcds(int param,unsigned int interupt_count, passing *ptr_pass) {
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

  //display.print("CPU : ");
  //display.println( prosseror_usage());

  display.print("INTERUPT : ");
  display.println( interupt_count );

  display.setCursor(0, 40);
  display.print("test line :");
  display.println(ptr_pass->test);

  display.setCursor(0, 54);
  display.print("Count: ");
  display.println(param);

  display.display();
}

void fled_in(void *ptr_param) {
  unsigned int interupt_count = 0;
  while (1) {
     if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) == pdPASS) {

      interupt_count++;

      if(xQueueSend(interupt_count_queue , (void *) &interupt_count, 100 ) != pdTRUE){
        Serial.println("queue full or timeout");
      }


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
  int delay = *((int *)ptr_param);
  while (1) {
    if(xSemaphoreTake(led_gate, 1) == pdPASS){
      led(&delay);
      xSemaphoreGive(led_gate);
    }
   
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      count++;
      xSemaphoreGive(mutex);
    } else {
      // optional: Serial.println("in use LED");
    }
    vTaskDelay(delay / portTICK_PERIOD_MS);
  }
}

void flcd(void *ptr_param) {
  passing pass = *((passing *)ptr_param);
  unsigned int interupt_count = 0;
  while (1) {
    xQueueReceive(interupt_count_queue, (void * ) &interupt_count, 0);



    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      lcds(count,interupt_count, &pass);
      xSemaphoreGive(mutex);
    }  

   
    vTaskDelay(pass.delay / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(9600);
  delay(500);

  mutex = xSemaphoreCreateMutex();
  led_gate = xSemaphoreCreateBinary();
  xSemaphoreGive(led_gate);
  interupt_count_queue = xQueueCreate(interupt_count_queue_max, sizeof(int));

  pinMode(leds, OUTPUT);
  //pinMode(button, INPUT_PULLUP);

  // use FALLING to avoid double triggers on mechanical bounce when you only want presses
  //attachInterrupt(digitalPinToInterrupt(button), interupts, FALLING);



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
  //setup begine is Begin(int 32,TaskHandle_t,unsigned int 32)
 
  if(my_interupt.begin(button,task3,debauch_t) != 0){
    Serial.println("fail to allocate the interupt");
  }
}

void loop() {
  // empty
}
