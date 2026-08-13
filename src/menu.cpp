#include "menu.h"
#include <TimeLib.h>

namespace
{
    enum UiMode
    {
        UI_MAIN_SCREEN,
        UI_MENU,
        UI_EDIT_BRIGHTNESS,
        UI_EDIT_TIME,
        UI_EDIT_DATE,
    };

    struct Menu;
    typedef void (*MenuAction)();

    struct MenuItem
    {
        const char *title;
        const Menu *child;
        MenuAction action;
    };

    struct Menu
    {
        const char *title;
        const MenuItem *items;
        uint8_t itemCount;
    };

    extern const Menu rootMenu;
    extern const Menu displayMenu;
    extern const Menu timeMenu;
    extern const Menu dateMenu;

    void actionBack();
    void actionBackToMain();
    void actionBrightnessOff();
    void actionBrightnessFull();
    void actionBrightnessSet();
    void actionChangeTime();
    void actionChangeDate();

    const MenuItem rootItems[] = {
        {"Back", nullptr, actionBackToMain},
        {"Display", &displayMenu, nullptr},
        {"Display Duplicate", &displayMenu, nullptr},
        {"Time", &timeMenu, nullptr},
        {"Date", &dateMenu, nullptr},
        {"Time Duplicate", &timeMenu, nullptr},
        {"Time Duplicate 2", &timeMenu, nullptr},

    };

    const MenuItem displayItems[] = {
        {"Back", nullptr, actionBack},
        {"Brightness Set", nullptr, actionBrightnessSet},
        {"Brightness Off", nullptr, actionBrightnessOff},
        {"Brightness Full", nullptr, actionBrightnessFull},
    };

    const MenuItem timeItems[] = {
        {"Back", nullptr, actionBack},
        {"Change Time", nullptr, actionChangeTime},
    };

    const MenuItem dateItems[] = {
        {"Back", nullptr, actionBack},
        {"Change Date", nullptr, actionChangeDate},
    };

    const Menu rootMenu = {"Main", rootItems, (uint8_t)(sizeof(rootItems) / sizeof(rootItems[0]))};
    const Menu displayMenu = {"Display", displayItems, (uint8_t)(sizeof(displayItems) / sizeof(displayItems[0]))};
    const Menu timeMenu = {"Time", timeItems, (uint8_t)(sizeof(timeItems) / sizeof(timeItems[0]))};
    const Menu dateMenu = {"Date", dateItems, (uint8_t)(sizeof(dateItems) / sizeof(dateItems[0]))};

    const uint8_t LCD_COLS = 20;
    const uint8_t LCD_ROWS = 4;
    const uint8_t MENU_MAX_DEPTH = 5;

    UiMode uiMode = UI_MAIN_SCREEN;
    bool refreshMainScreen = false;

    const Menu *menuStack[MENU_MAX_DEPTH] = {&rootMenu}; // stack of menu pointers for each level, initialized with root menu
    uint8_t selectionStack[MENU_MAX_DEPTH] = {0};        // selected item index for each menu level
    uint8_t scrollStack[MENU_MAX_DEPTH] = {0};           // scroll offset for each menu level (index of the first visible item)
    uint8_t menuDepth = 0;                               // current menu depth (0 = root menu)

    void printPadded(const char *text, uint8_t width = LCD_COLS)
    {
        char line[LCD_COLS + 1];
        uint8_t i = 0;

        while (i < width && text[i] != '\0')
        {
            line[i] = text[i];
            i++;
        }

        while (i < width)
        {
            line[i] = ' ';
            i++;
        }

        line[width] = '\0';
        lcd.print(line);
    }

    void resetMenuRoot()
    {
        menuDepth = 0;
        menuStack[0] = &rootMenu;
        selectionStack[0] = 0;
        scrollStack[0] = 0;
    }

    const Menu *currentMenu()
    {
        return menuStack[menuDepth];
    }

    uint8_t &currentSelection()
    {
        return selectionStack[menuDepth];
    }

    uint8_t &currentScroll()
    {
        return scrollStack[menuDepth];
    }

    void renderMenu()
    {
        const Menu *menu = currentMenu();
        uint8_t selected = currentSelection();
        uint8_t scroll = currentScroll();

        for (uint8_t row = 0; row < LCD_ROWS; row++)
        {
            uint8_t itemIndex = scroll + row;
            lcd.setCursor(0, row);

            if (itemIndex < menu->itemCount)
            {
                lcd.print(itemIndex == selected ? "> " : "  ");
                printPadded(menu->items[itemIndex].title, LCD_COLS - 2);
            }
            else
            {
                printPadded("");
            }
        }
    }

    void renderEditBrightnessEditor()
    {
        lcd.setCursor(0, 0);
        printPadded("Edit Brightness");
        lcd.setCursor(0, 1);
        printPadded("Value: ");
        lcd.setCursor(7, 1);
        lcd.print(brightness);
        lcd.setCursor(0, 2);
        printPadded("L/R change by 5");
        lcd.setCursor(0, 3);
        printPadded("Press to save");
    }

    void renderTimeEditor(uint8_t stage = 0, int hourValue = hour(), int minuteValue = minute(), int secondValue = second())
    {
        lcd.setCursor(0, 0);
        printPadded("Time Editor");
        lcd.setCursor(0, 1);
        char buf[8];
        sprintf(buf, "%02d:%02d:%02d", hourValue, minuteValue, secondValue);
        printPadded(buf);
        if (stage == 0)
        {
            lcd.setCursor(0, 2);
            printPadded("L/R to change hour");
            lcd.setCursor(0, 3);
            printPadded("Press to set minutes");
        }
        else if (stage == 1)
        {
            lcd.setCursor(0, 2);
            printPadded("L/R to change minute");
            lcd.setCursor(0, 3);
            printPadded("Press to set seconds");
        }
        else if (stage == 2)
        {
            lcd.setCursor(0, 2);
            printPadded("L/R to change second");
            lcd.setCursor(0, 3);
            printPadded("Press to save");
        }
    }

    void renderDateEditor()
    {
        lcd.setCursor(0, 0);
        printPadded("Date Editor");
        lcd.setCursor(0, 1);
        printPadded("Not implemented");
        lcd.setCursor(0, 2);
        printPadded("Press to go back");
        lcd.setCursor(0, 3);
        printPadded("");
    }

    void navigate(int8_t direction)
    {
        const Menu *menu = currentMenu();
        uint8_t selected = currentSelection();
        uint8_t scroll = currentScroll();

        if (direction < 0)
        {
            selected = (selected == 0) ? menu->itemCount - 1 : selected - 1;
        }
        else
        {
            selected = (selected + 1) % menu->itemCount;
        }

        if (selected < scroll)
        {
            scroll = selected;
        }
        else if (selected > scroll + LCD_ROWS - 1)
        {
            scroll = selected - LCD_ROWS + 1;
        }

        currentSelection() = selected;
        currentScroll() = scroll;
        renderMenu();
    }

    void enterMenu(const Menu *child)
    {
        if (menuDepth + 1 >= MENU_MAX_DEPTH)
        {
            return;
        }

        menuDepth++;
        menuStack[menuDepth] = child;
        selectionStack[menuDepth] = 0;
        scrollStack[menuDepth] = 0;
        renderMenu();
    }

    void goBack()
    {
        if (menuDepth == 0)
        {
            return;
        }

        menuDepth--;
        renderMenu();
    }

    void runCurrentItem()
    {
        const Menu *menu = currentMenu();
        const MenuItem &item = menu->items[currentSelection()];

        if (item.child != nullptr)
        {
            enterMenu(item.child);
            return;
        }

        if (item.action != nullptr)
        {
            item.action();
        }
    }

    void openMenu()
    {
        resetMenuRoot();
        uiMode = UI_MENU;
        renderMenu();
    }

    void actionBack()
    {
        goBack();
    }

    void actionBackToMain()
    {
        resetMenuRoot();
        uiMode = UI_MAIN_SCREEN;
        refreshMainScreen = true;
    }

    void actionBrightnessSet()
    {
        uiMode = UI_EDIT_BRIGHTNESS;
        lcd.clear();
        renderEditBrightnessEditor();
    }

    void actionChangeTime()
    {
        uiMode = UI_EDIT_TIME;
        lcd.clear();
        renderTimeEditor();
    }

    void actionBrightnessOff()
    {
        brightness = 0;
        analogWrite(BACKLIGHT_PIN, brightness);
    }

    void actionBrightnessFull()
    {
        brightness = 255;
        analogWrite(BACKLIGHT_PIN, brightness);
    }

    void actionChangeDate()
    {
        uiMode = UI_EDIT_DATE;
        lcd.clear();
        renderDateEditor();
    }
} // namespace

bool menuIsActive()
{
    return uiMode != UI_MAIN_SCREEN;
}

bool menuTick()
{
    refreshMainScreen = false;

    if (uiMode == UI_MAIN_SCREEN)
    {
        if (enc.press())
        {
            openMenu();
        }

        return false;
    }

    if (uiMode == UI_MENU)
    {
        if (enc.left())
        {
            navigate(1);
        }

        if (enc.right())
        {
            navigate(-1);
        }

        if (enc.press())
        {
            runCurrentItem();
        }

        return refreshMainScreen;
    }

    if (uiMode == UI_EDIT_BRIGHTNESS)
    {
        if (enc.right())
        {
            brightness -= 5;
            if (brightness < 0)
            {
                brightness = 0;
            }
            analogWrite(BACKLIGHT_PIN, brightness);
            renderEditBrightnessEditor();
        }

        if (enc.left())
        {
            brightness += 5;
            if (brightness > 255)
            {
                brightness = 255;
            }
            analogWrite(BACKLIGHT_PIN, brightness);
            renderEditBrightnessEditor();
        }

        if (enc.press())
        {
            uiMode = UI_MENU;
            renderMenu();
        }

        return false;
    }

    if (uiMode == UI_EDIT_TIME)
    {
        static uint8_t timeStage = 0; // 0 = editing hour, 1 = editing minute, 2 = editing second
        static int lastHour = hour();
        static int lastMinute = minute();
        static int lastSecond = second();

        if (enc.right())
        {
            if (timeStage == 0)
            {
                lastHour = (lastHour - 1 + 24) % 24;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 1)
            {
                lastMinute = (lastMinute - 1 + 60) % 60;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 2)
            {
                lastSecond = (lastSecond - 1 + 60) % 60;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
        }

        if (enc.left())
        {
            if (timeStage == 0)
            {
                lastHour = (lastHour + 1) % 24;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 1)
            {
                lastMinute = (lastMinute + 1) % 60;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 2)
            {
                lastSecond = (lastSecond + 1) % 60;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
        }

        if (enc.press())
        {
            setTime(lastHour, lastMinute, lastSecond, day(), month(), year());

            if (timeStage == 0)
            {
                timeStage = 1;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 1)
            {
                timeStage = 2;
                renderTimeEditor(timeStage, lastHour, lastMinute, lastSecond);
            }
            else if (timeStage == 2)
            {
                timeStage = 0;
                uiMode = UI_MENU;
                renderMenu();
            }
        }
    }

    if (uiMode == UI_EDIT_DATE)
    {
        if (enc.press())
        {
            uiMode = UI_MENU;
            renderMenu();
        }
    }

    return false;
}