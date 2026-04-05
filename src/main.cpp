#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <EncButton.h>
#include <TimeLib.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
EncButton enc(2, 3, 4);

void setSystemTime();
void drawTime(uint8_t hours, uint8_t minutes);
void drawLabels();
void drawLoadingBar(uint8_t percent, uint8_t row);
void drawTemp(uint8_t temp, uint8_t row);
void drawLoad(uint8_t percent, uint8_t row);
void loadCustomChars();
void drawDigit(byte digit, byte xpos, byte ypos);
void drawDots(bool visible, byte xpos, byte ypos);
void drawDate(uint8_t day, uint8_t month);
void drawDayOfWeek(uint8_t dayOfWeek);

#define BACKLIGHT_PIN 10
#define BACKLIGHT_DEFAULT_BRIGHTNESS 128

byte customChar0[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
byte customChar1[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte customChar2[8] = {B11111, B00011, B00011, B00011, B00011, B00011, B00011, B11111};
byte customChar3[8] = {B11111, B11000, B11000, B11000, B11000, B11000, B11000, B11111};
byte customChar4[8] = {B11111, B11000, B11000, B11000, B11000, B11000, B11000, B11000};
byte customChar5[8] = {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000};
byte customChar6[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte customChar7[8] = {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11111};

byte bold_digits[10][4] = {{4, 5, 7, 5}, {254, 5, 254, 5}, {6, 2, 3, 6}, {0, 2, 1, 2}, {7, 1, 254, 5}, {3, 6, 6, 2}, {3, 6, 3, 2}, {0, 2, 254, 5}, {3, 2, 3, 2}, {3, 2, 6, 2}};

bool menuSelectMode = false;
int currentMenuItem = 0;

int brightness = BACKLIGHT_DEFAULT_BRIGHTNESS;

int cpuUsage = 0;
int cpuTemp = 0;
int gpuUsage = 0;
int gpuTemp = 0;

bool dotsVisible = true;

static unsigned long previousMillis = 0;

void setup()
{
    Serial.begin(9600);
    Serial.setTimeout(10);

    lcd.init();
    lcd.backlight();

    analogWrite(BACKLIGHT_PIN, brightness);

    setSystemTime();

    loadCustomChars();

    drawTime(hour(), minute());
    drawDate(day(), month());
    drawDayOfWeek(weekday());

    drawLabels();

    drawLoadingBar(cpuUsage, 2);
    drawTemp(cpuTemp, 2);
    drawLoad(cpuUsage, 2);

    drawLoadingBar(gpuUsage, 3);
    drawTemp(gpuTemp, 3);
    drawLoad(gpuUsage, 3);

    Serial.println("Setup complete");
}

void loop()
{
    unsigned long currentMillis = millis();

    enc.tick();

    if (enc.click())
    {
        menuSelectMode = !menuSelectMode;
    }

    if (currentMillis - previousMillis >= 1000)
    {
        previousMillis = currentMillis;
        dotsVisible = !dotsVisible;

        drawTime(hour(), minute());
        drawDate(day(), month());
        drawDayOfWeek(weekday());
    }

    if (Serial.available())
    {
        String data = Serial.readStringUntil(';');
        String type = data.substring(0, 4);
        int val = data.substring(5).toInt();
        if (type == "cpuU")
        {
            cpuUsage = val;
            drawLoadingBar(cpuUsage, 2);
            drawLoad(cpuUsage, 2);
        }
        if (type == "gpuU")
        {
            gpuUsage = val;
            drawLoadingBar(gpuUsage, 3);
            drawLoad(gpuUsage, 3);
        }
        if (type == "cpuT")
        {
            cpuTemp = val;
            drawTemp(cpuTemp, 2);
        }
        if (type == "gpuT")
        {
            gpuTemp = val;
            drawTemp(gpuTemp, 3);
        }
    }
}

void setSystemTime()
{
    int h, m, s, d, y;
    char monthStr[4];
    int month;

    sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
    sscanf(__DATE__, "%s %d %d", monthStr, &d, &y);

    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *match = strstr(months, monthStr);
    month = match ? ((match - months) / 3 + 1) : 1;

    setTime(h, m, s, d, month, y);
}

void loadCustomChars()
{
    lcd.createChar(0, customChar0);
    lcd.createChar(1, customChar1);
    lcd.createChar(2, customChar2);
    lcd.createChar(3, customChar3);
    lcd.createChar(4, customChar4);
    lcd.createChar(5, customChar5);
    lcd.createChar(6, customChar6);
    lcd.createChar(7, customChar7);
}

void drawTime(uint8_t hours, uint8_t minutes)
{
    drawDigit(hours / 10, 2, 0);
    drawDigit(hours % 10, 4, 0);
    drawDots(dotsVisible, 6, 0);
    drawDigit(minutes / 10, 7, 0);
    drawDigit(minutes % 10, 9, 0);
}

void drawLabels()
{
    lcd.setCursor(0, 2);
    lcd.print("CPU:");
    lcd.setCursor(0, 3);
    lcd.print("GPU:");
}

void drawLoadingBar(uint8_t percent, uint8_t row)
{
    uint8_t barLength = 10;           // 10 chars for loading bar
    uint8_t fullChars = percent / 10; // 10% per char

    lcd.setCursor(7, row);

    if (fullChars > 0)
    {
        lcd.write(255);
    }
    else
    {
        lcd.write(3);
    }

    for (uint8_t i = 0; i < barLength - 2; i++)
    {
        if (i < fullChars - 1)
        {
            lcd.write(255);
        }
        else
        {
            lcd.write(6);
        }
    }

    if (fullChars >= barLength)
    {
        lcd.write(255);
    }
    else
    {
        lcd.write(2);
    }
}

void drawTemp(uint8_t temp, uint8_t row)
{
    temp = min(temp, 99);
    lcd.setCursor(4, row);
    lcd.print(temp);
    lcd.write(223); // degree symbol
    if (temp < 10)
    {
        lcd.setCursor(6, row);
        lcd.print(" ");
    }
}

void drawLoad(uint8_t percent, uint8_t row)

{
    percent = min(percent, 99);
    lcd.setCursor(17, row);
    lcd.print(percent);
    lcd.print("%");
    if (percent < 10)
    {
        lcd.setCursor(19, row);
        lcd.print(" ");
    }
}

void drawDigit(byte digit, byte xpos, byte ypos)
{
    lcd.setCursor(xpos, ypos);
    lcd.write(bold_digits[digit][0]);
    lcd.write(bold_digits[digit][1]);
    lcd.setCursor(xpos, ypos + 1);
    lcd.write(bold_digits[digit][2]);
    lcd.write(bold_digits[digit][3]);
}

void drawDots(bool visible, byte xpos, byte ypos)
{
    if (visible)
    {
        lcd.setCursor(xpos, ypos);
        lcd.write(165);
        lcd.setCursor(xpos, ypos + 1);
        lcd.write(165);
    }
    else
    {
        lcd.setCursor(xpos, ypos);
        lcd.print(" ");
        lcd.setCursor(xpos, ypos + 1);
        lcd.print(" ");
    }
}

void drawDate(uint8_t day, uint8_t month)
{
    lcd.setCursor(13, 0);
    if (day < 10)
    {
        lcd.print("0");
    }
    lcd.print(day);
    lcd.write(46); // dot symbol
    if (month < 10)
    {
        lcd.print("0");
    }
    lcd.print(month);
}

void drawDayOfWeek(uint8_t dayOfWeek)
{
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    if (dayOfWeek < 7)
    {
        lcd.setCursor(14, 1);
        lcd.print(days[dayOfWeek]);
    }
}