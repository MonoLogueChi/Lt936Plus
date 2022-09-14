/*
 Name:		Lt936Plus.ino
 Created:	2022/9/7 18:54:41
 Author:	mc
*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <AiEsp32RotaryEncoder.h>
#include <Preferences.h>


# pragma region 定义

// 屏幕
// SDA 8
// SCL 9
bool canRefreshScreen = true;


// 编码器
#define ROTARY_ENCODER_A_PIN 12
#define ROTARY_ENCODER_B_PIN 13
#define ROTARY_ENCODER_BUTTON_PIN 11
#define ROTARY_ENCODER_VCC_PIN 10
#define ROTARY_ENCODER_STEPS 4

// 测温
#define TEMP_ADC_PIN 3

#define TEMP_TARGET_MAX 420
#define TEMP_TARGET_MIN 50
#define TEMP_SAVE_MAX 320
#define TEMP_TARGET_DEFAULT 150

#define TEMP_TARGET_KEY "TARGETT"


// 设定温度
int TargetT = TEMP_TARGET_DEFAULT;

// 当前温度
int CurrentT = 0;

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

# pragma region 屏幕
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
// 刷新屏幕
void RefreshScreen()
{
	if (canRefreshScreen)
	{
		canRefreshScreen = false;

		uint8_t y1, y2 = 2;

		if (TargetT / 100 > 0)
		{
			y1 = 2;
		}
		else if (TargetT / 10 > 0)
		{
			y1 = 22;
		}
		else
		{
			y1 = 42;
		}

		if (CurrentT / 100 > 0)
		{
			y2 = 2;
		}
		else if (CurrentT / 10 > 0)
		{
			y2 = 22;
		}
		else
		{
			y2 = 42;
		}


		u8g2.clearBuffer();

		u8g2.drawFrame(0, 0, 63, 64);
		u8g2.drawFrame(64, 0, 63, 64);

		u8g2.setFontDirection(1);

		u8g2.setFont(u8g2_font_logisoso32_tr);
		u8g2.setCursor(8, y2);
		u8g2.print(CurrentT);
		u8g2.setCursor(72, y1);
		u8g2.print(TargetT);

		// 温度
		u8g2.setFont(u8g2_font_wqy12_t_gb2312a);
		u8g2.setCursor(48, 2);
		u8g2.print("当前温度");
		u8g2.setCursor(112, 2);
		u8g2.print("设定温度");

		u8g2.sendBuffer();

		canRefreshScreen = true;
	}
}

void ScreenInit()
{
	u8g2.begin();
	u8g2.enableUTF8Print();
	RefreshScreen();
}

void TaskRefreshScreen(void* p)
{
	(void)p;

	for (;;)
	{
		RefreshScreen();
		vTaskDelay(2000);
	}
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
	rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);
	rotaryEncoder.setBoundaries(TEMP_TARGET_MIN, TEMP_TARGET_MAX, false);
	rotaryEncoder.setEncoderValue(TargetT);

	for (;;)
	{
		if (rotaryEncoder.encoderChanged())
		{
			TargetT = rotaryEncoder.readEncoder();
			RefreshScreen();
			vTaskDelay(5);
		}
		if (rotaryEncoder.isEncoderButtonClicked())
		{
			if (TargetT < TEMP_SAVE_MAX)
			{
				SaveIntData(TEMP_TARGET_KEY, TargetT);
			}
		}
	}
};

# pragma endregion


void setup()
{
	//初始化数据
	PreferencesInit();

	// 初始化屏幕
	ScreenInit();
	xTaskCreatePinnedToCore(
		TaskRefreshScreen,
		"TaskRotary",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);

	// 编码器
	xTaskCreatePinnedToCore(
		TaskRotary,
		"TaskRotary",
		1024 * 8,
		nullptr,
		1,
		nullptr,
		ARDUINO_RUNNING_CORE);
}

void loop()
{
}
