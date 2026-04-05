#include "menu.h"

namespace
{
    enum UiMode
    {
        UI_MAIN_SCREEN,
        UI_MENU,
        UI_EDIT_BRIGHTNESS,
        UI_EDIT_TIME,
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

    extern const Menu displayMenu;
    extern const Menu timeMenu;

    void actionBack();
    void actionBackToMain();
    void actionEditBrightness();
    void actionUpdateTime();

    const MenuItem rootItems[] = {
        {"Back", nullptr, actionBackToMain},
        {"Display", &displayMenu, nullptr},
        {"Display2", &displayMenu, nullptr},
        {"Time", &timeMenu, nullptr},
        {"Time2", &timeMenu, nullptr},
        {"Time3", &timeMenu, nullptr},
    };

    const MenuItem displayItems[] = {
        {"Back", nullptr, actionBack},
        {"Set Brightness", nullptr, actionEditBrightness},
    };

    const MenuItem timeItems[] = {
        {"Back", nullptr, actionBack},
        {"Update Time", nullptr, actionUpdateTime},
    };

    const Menu rootMenu = {"Main", rootItems, (uint8_t)(sizeof(rootItems) / sizeof(rootItems[0]))};
    const Menu displayMenu = {"Display", displayItems, (uint8_t)(sizeof(displayItems) / sizeof(displayItems[0]))};
    const Menu timeMenu = {"Time", timeItems, (uint8_t)(sizeof(timeItems) / sizeof(timeItems[0]))};

    const uint8_t LCD_COLS = 20;
    const uint8_t LCD_ROWS = 4;
    const uint8_t MENU_MAX_DEPTH = 5;

    UiMode uiMode = UI_MAIN_SCREEN;
    bool refreshMainScreen = false;

    const Menu *menuStack[MENU_MAX_DEPTH] = {&rootMenu};
    uint8_t selectionStack[MENU_MAX_DEPTH] = {0};
    uint8_t scrollStack[MENU_MAX_DEPTH] = {0};
    uint8_t menuDepth = 0;

    void printPadded(const char *text, uint8_t width)
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
                printPadded("", LCD_COLS);
            }
        }
    }

    void renderSetBrightnessEditor()
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        printPadded("Set Brightness", LCD_COLS);
        lcd.setCursor(0, 1);
        lcd.print("Value: ");
        lcd.print(brightness);
        lcd.setCursor(0, 2);
        printPadded("L/R change by 5", LCD_COLS);
        lcd.setCursor(0, 3);
        printPadded("Press to save", LCD_COLS);
    }

    void renderTimeEditor()
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        printPadded("Time Editor", LCD_COLS);
        lcd.setCursor(0, 1);
        printPadded("Not implemented", LCD_COLS);
        lcd.setCursor(0, 2);
        printPadded("Press to go back", LCD_COLS);
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

    void actionEditBrightness()
    {
        uiMode = UI_EDIT_BRIGHTNESS;
        renderSetBrightnessEditor();
    }

    void actionUpdateTime()
    {
        uiMode = UI_EDIT_TIME;
        renderTimeEditor();
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
            renderSetBrightnessEditor();
        }

        if (enc.left())
        {
            brightness += 5;
            if (brightness > 255)
            {
                brightness = 255;
            }
            analogWrite(BACKLIGHT_PIN, brightness);
            renderSetBrightnessEditor();
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
        if (enc.press())
        {
            uiMode = UI_MENU;
            renderMenu();
        }
    }

    return false;
}