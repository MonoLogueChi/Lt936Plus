/*
 Name:		Lt936Plus.ino
 Created:	2022/9/7 18:54:41
 Author:	mc
*/

#include "AdcFilter.h"
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <AiEsp32RotaryEncoder.h>
#include <Preferences.h>

#if ARDUINO_USB_CDC_ON_BOOT
#define HWSerial Serial0
#define USBSerial Serial
#else
#define HWSerial Serial
USBCDC USBSerial;
#endif


# pragma region 定义

// 屏幕
// SDA 8
// SCL 9
bool canRefreshScreen = true;


// 编码器
#if CONFIG_IDF_TARGET_ESP32S2

#define ROTARY_ENCODER_A_PIN 34
#define ROTARY_ENCODER_B_PIN 35
#define ROTARY_ENCODER_BUTTON_PIN 36
#define ROTARY_ENCODER_VCC_PIN 10
#define ROTARY_ENCODER_STEPS 4

#else
#define ROTARY_ENCODER_A_PIN 35
#define ROTARY_ENCODER_B_PIN 36
#define ROTARY_ENCODER_BUTTON_PIN 37
#define ROTARY_ENCODER_VCC_PIN 10
#define ROTARY_ENCODER_STEPS 4

#endif


// 休眠
#define XM_PIN 4
#define SLEEP_TIME 4
uint8_t toSleep = 0;
boolean isSleeping = false;

// 加热
#define JR_PIN 2

// 测温
#define TEMP_ADC_PIN 3

#define TEMP_TARGET_MAX 450
#define TEMP_TARGET_MIN 100
#define TEMP_SAVE_MAX 320
#define TEMP_TARGET_DEFAULT 130

#define TEMP_TARGET_KEY "TARGETT"


// 设定温度
int TargetT = TEMP_TARGET_DEFAULT;

// 当前温度
int CurrentT = 0;
int CurrentTRaw = 0;


constexpr float k0 = 0.19f;
constexpr float b0 = 50.0f;
constexpr uint32_t R1 = 1.0 / (1.0 / 1000.0 + 1.0 / 1500.0);

#if CONFIG_IDF_TARGET_ESP32S2
constexpr float DMin = 0.0f;
constexpr float DMax = 0.75f;
#elif CONFIG_IDF_TARGET_ESP32S3
constexpr float DMin = 0.0f;
constexpr float DMax = 0.95f;
#else
constexpr float DMin = 0.0f;
constexpr float DMax = 3.3f;

#endif

# pragma endregion

# pragma region 数据

Preferences preferences;

int32_t ReadIntData(const char* key, int32_t defaultValue)
{
	return preferences.getInt(key, defaultValue);
}

void SaveIntData(const char* key, int32_t value)
{
	preferences.putInt(key, value);
}

void PreferencesInit()
{
	preferences.begin("lt", false);

	TargetT = ReadIntData(TEMP_TARGET_KEY, TEMP_TARGET_DEFAULT);
}

# pragma endregion

# pragma region 休眠

void NotSleep()
{
	toSleep = 0;
	isSleeping = false;
}


void TaskSleep(void* p)
{
	(void)p;

	for (;;)
	{
		if (!isSleeping)
		{
			if (toSleep > SLEEP_TIME)
			{
				isSleeping = true;
			}
			toSleep++;
		}
		vTaskDelay(1000 * 10);
	}
}


void SleepInit()
{
	pinMode(XM_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(XM_PIN), NotSleep, FALLING);
	xTaskCreatePinnedToCore(
		TaskSleep,
		"TaskSleep",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);
}

# pragma endregion

# pragma region 屏幕
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// 刷新屏幕
void RefreshScreen()
{
	if (canRefreshScreen)
	{
		canRefreshScreen = false;

		uint8_t y1, y2;

		if (TargetT / 100 > 0)
		{
			y1 = 62;
		}
		else if (TargetT / 10 > 0)
		{
			y1 = 42;
		}
		else
		{
			y1 = 22;
		}

		if (CurrentT / 100 > 0)
		{
			y2 = 62;
		}
		else if (CurrentT / 10 > 0)
		{
			y2 = 42;
		}
		else
		{
			y2 = 22;
		}


		u8g2.clearBuffer();

		u8g2.drawFrame(0, 0, 63, 64);
		u8g2.drawFrame(64, 0, 63, 64);

		u8g2.setFontDirection(3);

		// 非休眠状态
		if (!isSleeping)
		{
			u8g2.setFont(u8g2_font_logisoso32_tr);
			u8g2.setCursor(55, y2);
			u8g2.print(CurrentT);
			u8g2.setCursor(120, y1);
			u8g2.print(TargetT);

			// 温度
			u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
			u8g2.setCursor(14, 62);
			u8g2.print("当前温度");
			u8g2.setCursor(78, 62);
			u8g2.print("设定温度");
		}
		else
		{
			u8g2.setFont(u8g2_font_logisoso32_tr);
			u8g2.setCursor(55, y2);
			u8g2.print(CurrentT);
			u8g2.setCursor(120, y1);
			u8g2.print(TEMP_TARGET_DEFAULT);

			// 温度
			u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
			u8g2.setCursor(14, 62);
			u8g2.print("当前温度");
			u8g2.setCursor(78, 62);
			u8g2.print("休眠状态");
		}


		u8g2.sendBuffer();

		canRefreshScreen = true;
	}
}

void TaskRefreshScreen(void* p)
{
	(void)p;

	for (;;)
	{
		RefreshScreen();
		vTaskDelay(1000);
	}
}

void ScreenInit()
{
	u8g2.begin();
	u8g2.enableUTF8Print();
	RefreshScreen();

	xTaskCreatePinnedToCore(
		TaskRefreshScreen,
		"TaskRotary",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);
}

# pragma endregion

# pragma region 编码器

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(
	ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN,
	ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}

void TaskRotary(void* p)
{
	(void)p;

	for (;;)
	{
		if (rotaryEncoder.encoderChanged())
		{
			if (!isSleeping)
			{
				TargetT = rotaryEncoder.readEncoder();
			}
			else
			{
				NotSleep();
			}
			RefreshScreen();
		}
		if (rotaryEncoder.isEncoderButtonClicked())
		{
			if (TargetT < TEMP_SAVE_MAX)
			{
				SaveIntData(TEMP_TARGET_KEY, TargetT);
			}
		}
		vTaskDelay(5);
	}
};

void RotaryInit()
{
	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(TEMP_TARGET_MIN, TEMP_TARGET_MAX, false);
	rotaryEncoder.setEncoderValue(TargetT);

	xTaskCreatePinnedToCore(
		TaskRotary,
		"TaskRotary",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);
}

# pragma endregion

# pragma region 设置参数

void TaskSerial(void* p)
{
	(void)p;

	for (;;)
	{
		if (USBSerial.available())
		{
		}
		vTaskDelay(1000);
	}
}

void SerialInit()
{
	USBSerial.begin(115200);

	xTaskCreatePinnedToCore(
		TaskSerial,
		"TaskSerial",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);
}
# pragma endregion


# pragma region 加热控制

AdcFilter adc = AdcFilter(10, 8);

int V2T(int u)
{
	return static_cast<int>((static_cast<float>(u) / 1000 * (b0 + R1) - 3.3f * b0) / ((3.3f - 1.0f) * k0));
}

void JRAdc()
{
	CurrentTRaw = adc.readMiliVolts(TEMP_ADC_PIN, CurrentTRaw);
	CurrentT = V2T(CurrentTRaw);
	// USBSerial.print(CurrentTRaw);
	// USBSerial.print(",");
	// USBSerial.println(CurrentT);
}

void JR()
{
	if (!isSleeping && TargetT > CurrentT)
	{
		digitalWrite(JR_PIN, HIGH);
	}
	else if (isSleeping && TEMP_TARGET_DEFAULT > CurrentT)
	{
		digitalWrite(JR_PIN, HIGH);
	}
	else
	{
		digitalWrite(JR_PIN, LOW);
	}
}


void TaskJR(void* p)
{
	(void)p;

	for (;;)
	{
		JRAdc();
		JR();
		delay(50);
	}
}

void JRInit()
{
	analogSetPinAttenuation(TEMP_ADC_PIN, ADC_0db);

	pinMode(JR_PIN, OUTPUT);
	digitalWrite(JR_PIN, LOW);

	xTaskCreatePinnedToCore(
		TaskJR,
		"TaskJR",
		1024 * 16,
		nullptr,
		2,
		nullptr,
		ARDUINO_RUNNING_CORE);
}

# pragma endregion

void setup()
{
	//初始化数据
	PreferencesInit();

	// 初始化屏幕
	ScreenInit();

	// 编码器
	RotaryInit();

	// 休眠
	SleepInit();

	// 加热
	JRInit();

	// 初始化串口
	SerialInit();
}

void loop()
{
}
