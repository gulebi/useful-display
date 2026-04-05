#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <EncButton.h>
#include <TimeLib.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
EncButton enc(2, 3, 4);

void setSystemTime();
void renderTime(uint8_t hours, uint8_t minutes);
void renderLabels();
void renderLoadingBar(uint8_t percent, uint8_t row);
void renderTemp(uint8_t temp, uint8_t row);
void renderLoad(uint8_t percent, uint8_t row);
void loadCustomChars();
void renderDigit(byte digit, byte xpos, byte ypos);
void renderDots(bool visible, byte xpos, byte ypos);
void renderDate(uint8_t day, uint8_t month);
void renderDayOfWeek(uint8_t dayOfWeek);
void renderMainScreen();
void updateSerial();

#define BACKLIGHT_PIN 10
#define BACKLIGHT_DEFAULT_BRIGHTNESS 128
#define SERIAL_CMD_NAME_LEN 7

byte customChar0[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
byte customChar1[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte customChar2[8] = {B11111, B00011, B00011, B00011, B00011, B00011, B00011, B11111};
byte customChar3[8] = {B11111, B11000, B11000, B11000, B11000, B11000, B11000, B11111};
byte customChar4[8] = {B11111, B11000, B11000, B11000, B11000, B11000, B11000, B11000};
byte customChar5[8] = {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000};
byte customChar6[8] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte customChar7[8] = {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11111};

byte bold_digits[10][4] = {{4, 5, 7, 5}, {254, 5, 254, 5}, {6, 2, 3, 6}, {0, 2, 1, 2}, {7, 1, 254, 5}, {3, 6, 6, 2}, {3, 6, 3, 2}, {0, 2, 254, 5}, {3, 2, 3, 2}, {3, 2, 6, 2}};

int brightness = BACKLIGHT_DEFAULT_BRIGHTNESS;

int cpuLoad = 0;
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

    renderMainScreen();

    Serial.println("Setup complete");
}

void loop()
{
    enc.tick();

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= 1000)
    {
        previousMillis = currentMillis;
        dotsVisible = !dotsVisible;

        renderTime(hour(), minute());
        renderDate(day(), month());
        renderDayOfWeek(weekday());
    }

    updateSerial();
}

void renderMainScreen()
{
    renderTime(hour(), minute());
    renderDate(day(), month());
    renderDayOfWeek(weekday());

    renderLabels();

    renderLoadingBar(cpuLoad, 2);
    renderTemp(cpuTemp, 2);
    renderLoad(cpuLoad, 2);

    renderLoadingBar(gpuUsage, 3);
    renderTemp(gpuTemp, 3);
    renderLoad(gpuUsage, 3);
}

void setSystemTime()
{
    int h, m, s, day, year;
    char monthStr[4];
    int month;

    sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);

    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *match = strstr(months, monthStr);
    month = match ? ((match - months) / 3 + 1) : 1;

    setTime(h, m, s, day, month, year);
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

void renderTime(uint8_t hours, uint8_t minutes)
{
    renderDigit(hours / 10, 2, 0);
    renderDigit(hours % 10, 4, 0);
    renderDots(dotsVisible, 6, 0);
    renderDigit(minutes / 10, 7, 0);
    renderDigit(minutes % 10, 9, 0);
}

void renderLabels()
{
    lcd.setCursor(0, 2);
    lcd.print("CPU:");
    lcd.setCursor(0, 3);
    lcd.print("GPU:");
}

void renderLoadingBar(uint8_t percent, uint8_t row)
{
    uint8_t barLength = 10;           // 10 chars for loading bar
    uint8_t fullChars = percent / 10; // 10% per char

    lcd.setCursor(7, row);

    if (fullChars > 0)
    {
        lcd.write(255); // full block char
    }
    else
    {
        lcd.write(3); // left end char
    }

    for (uint8_t i = 0; i < barLength - 2; i++)
    {
        if (i < fullChars - 1)
        {
            lcd.write(255); // full block char
        }
        else
        {
            lcd.write(6); // empty block char
        }
    }

    if (fullChars >= barLength)
    {
        lcd.write(255); // full block char
    }
    else
    {
        lcd.write(2); // right end char
    }
}

void renderTemp(uint8_t temp, uint8_t row)
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

void renderLoad(uint8_t percent, uint8_t row)

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

void renderDigit(byte digit, byte xpos, byte ypos)
{
    lcd.setCursor(xpos, ypos);
    lcd.write(bold_digits[digit][0]);
    lcd.write(bold_digits[digit][1]);
    lcd.setCursor(xpos, ypos + 1);
    lcd.write(bold_digits[digit][2]);
    lcd.write(bold_digits[digit][3]);
}

void renderDots(bool visible, byte xpos, byte ypos)
{
    if (visible)
    {
        lcd.setCursor(xpos, ypos);
        lcd.write(165); // middle dot char
        lcd.setCursor(xpos, ypos + 1);
        lcd.write(165); // middle dot char
    }
    else
    {
        lcd.setCursor(xpos, ypos);
        lcd.print(" ");
        lcd.setCursor(xpos, ypos + 1);
        lcd.print(" ");
    }
}

void renderDate(uint8_t day, uint8_t month)
{
    lcd.setCursor(13, 0);
    if (day < 10)
    {
        lcd.print("0");
    }
    lcd.print(day);
    lcd.write(46); // dot char
    if (month < 10)
    {
        lcd.print("0");
    }
    lcd.print(month);
}

void renderDayOfWeek(uint8_t dayOfWeek)
{
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    if (dayOfWeek < 7)
    {
        lcd.setCursor(14, 1);
        lcd.print(days[dayOfWeek]);
    }
}

bool parseCommand(const char *frame)
{
    if (frame[SERIAL_CMD_NAME_LEN] != ':')
        return false;

    // Parse bool value: "true"->1, "false"->0, else int
    const char *rawVal = frame + SERIAL_CMD_NAME_LEN + 1;
    int val;
    if (strncmp(rawVal, "true", 4) == 0)
        val = 1;
    else if (strncmp(rawVal, "false", 5) == 0)
        val = 0;
    else
        val = atoi(rawVal);

    if (strncmp(frame, "cpuTemp", SERIAL_CMD_NAME_LEN) == 0)
    {
        cpuTemp = val;
        renderTemp(cpuTemp, 2);
    }
    else if (strncmp(frame, "gpuTemp", SERIAL_CMD_NAME_LEN) == 0)
    {
        gpuTemp = val;
        renderTemp(gpuTemp, 3);
    }
    else if (strncmp(frame, "cpuLoad", SERIAL_CMD_NAME_LEN) == 0)
    {
        cpuLoad = val;
        renderLoadingBar(cpuLoad, 2);
        renderLoad(cpuLoad, 2);
    }
    else if (strncmp(frame, "gpuLoad", SERIAL_CMD_NAME_LEN) == 0)
    {
        gpuUsage = val;
        renderLoadingBar(gpuUsage, 3);
        renderLoad(gpuUsage, 3);
    }
    else
        return false;

    return true;
}

void updateSerial()
{
    static char buf[SERIAL_CMD_NAME_LEN + 9];
    static uint8_t pos = 0;

    while (Serial.available())
    {
        char c = Serial.read();
        if (c == ';')
        {
            buf[pos] = '\0';
            parseCommand(buf);
            pos = 0;
        }
        else if (c != '\r' && c != '\n')
        {
            if (pos < sizeof(buf) - 1)
                buf[pos++] = c;
            else
                pos = 0;
        }
    }
}
