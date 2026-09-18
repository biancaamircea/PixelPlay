/*
 * ESP32-CAM-MB trigger capture over USB serial, no WiFi needed.
 *
 * Wiring used with FRDM-MCXA153:
 *   ESP32 IO14 <- FRDM P3_14  trigger input
 *   ESP32 IO13 -> FRDM P1_10  done/status output
 *   ESP32 GND  -> FRDM GND    common ground
 *
 * Keep the ESP32-CAM-MB connected to the laptop by USB.
 * Power both boards from their own USB ports. Do not connect 5V/3V3 between boards.
 */

#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#define TRIGGER_PIN 14
#define DONE_PIN 13
#define SERIAL_BAUD 921600
#define VIDEO_TRIGGER_MIN_MS 900
#define VIDEO_DURATION_MS 5000
#define VIDEO_FRAME_DELAY_MS 120

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#endif

static bool triggerWasHigh = false;
static unsigned long triggerHighSince = 0;

static bool init_camera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("CAMERA_INIT_FAILED 0x%x\n", err);
    return false;
  }

  return true;
}

static void capture_and_send()
{
  digitalWrite(DONE_PIN, LOW);
  Serial.println("CAPTURE_REQUESTED");

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("CAPTURE_FAILED");
    return;
  }

  Serial.printf("PHOTO_BEGIN %u\n", fb->len);
  Serial.write(fb->buf, fb->len);
  Serial.print("\nPHOTO_END\n");
  Serial.flush();
  esp_camera_fb_return(fb);

  digitalWrite(DONE_PIN, HIGH);
  delay(3000);
  digitalWrite(DONE_PIN, LOW);
  Serial.println("READY");
}

static void video_and_send()
{
  digitalWrite(DONE_PIN, LOW);
  Serial.println("VIDEO_REQUESTED");
  Serial.printf("VIDEO_BEGIN %u\n", VIDEO_DURATION_MS);

  unsigned long started = millis();
  uint16_t frames = 0;
  while ((millis() - started) < VIDEO_DURATION_MS)
  {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("FRAME_FAILED");
      delay(VIDEO_FRAME_DELAY_MS);
      continue;
    }

    Serial.printf("FRAME_BEGIN %u\n", fb->len);
    Serial.write(fb->buf, fb->len);
    Serial.print("\nFRAME_END\n");
    Serial.flush();
    esp_camera_fb_return(fb);
    frames++;
    delay(VIDEO_FRAME_DELAY_MS);
  }

  Serial.printf("VIDEO_END %u\n", frames);
  Serial.flush();

  digitalWrite(DONE_PIN, HIGH);
  delay(3000);
  digitalWrite(DONE_PIN, LOW);
  Serial.println("READY");
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(500);

  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  while (!init_camera())
  {
    Serial.println("CAMERA_RETRY_IN_2S");
    delay(2000);
  }

  Serial.println("READY");
}

void loop()
{
  if (Serial.available() > 0)
  {
    char command = Serial.read();
    if (command == 'p' || command == 'P')
    {
      capture_and_send();
    }
    else if (command == 'v' || command == 'V')
    {
      video_and_send();
    }
  }

  bool trigger = digitalRead(TRIGGER_PIN) == HIGH;
  if (trigger && !triggerWasHigh)
  {
    triggerHighSince = millis();
    triggerWasHigh = true;
  }
  else if (!trigger && triggerWasHigh)
  {
    unsigned long pulseMs = millis() - triggerHighSince;
    triggerWasHigh = false;
    if (pulseMs >= VIDEO_TRIGGER_MIN_MS)
    {
      video_and_send();
    }
    else
    {
      capture_and_send();
    }
  }
  delay(20);
}
