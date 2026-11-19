#include "main_menu.h"
#include "display.h"
#include "utils.h"
#include <globals.h>

MainMenu::MainMenu() {
    _menuItems = {
#if defined(JOHNNYOS_STICKS3)
        &johnnyAi,
        &johnnyWifi,
        &bleMenu,
        &johnnyGames,
        &irMenu,
        &fileMenu,
        &johnnyThemes,
        &johnnySystem,
        &johnnyPhone,
        &johnnyMouse,
        &johnnyMedia,
        &johnnyHelp,
#endif
        &wifiMenu,
#if !defined(JOHNNYOS_STICKS3)
        &bleMenu,
#endif
        &rfMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
        &loraMenu,
#endif
#if !defined(LITE_VERSION)
        &fmMenu,
#endif
#if !defined(JOHNNYOS_STICKS3)
        &irMenu,
#endif
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &gpsMenu,
        &rfidMenu,
#if !defined(JOHNNYOS_STICKS3)
        &fileMenu,
#endif
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
#endif
        &clockMenu,
        &othersMenu,
#if !defined(LITE_VERSION)
        &wireToolsMenu,
#endif
        &configMenu,
    };

    _totalItems = _menuItems.size();

#if defined(HAS_TOUCH)
    gridPageTapHandler = [](int x, int y, int currentIndex, int &newIndex) -> bool {
        return mainMenu.handleGridPageTap(x, y, currentIndex, newIndex);
    };
#endif
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    bool gridStyle = bruceConfig.mainMenuStyle == MAIN_MENU_GRID;
#if defined(JOHNNYOS_STICKS3)
    gridStyle = true;
#endif

    // Option::hover is a plain function pointer, so both renderers stay capture-less
    // and the grid one resolves its own position from the item pointer it receives.
    bool (*renderItem)(void *menuItem, bool shouldRender) = [](void *menuItem, bool shouldRender) {
        if (!shouldRender) return false;
        drawMainBorder(false);

        MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
        float scale = float((float)tftWidth / (float)240);
        if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
        obj->draw(scale);
#if defined(HAS_TOUCH)
        TouchFooter();
#endif
        return true;
    };

    if (gridStyle) {
        renderItem = [](void *menuItem, bool shouldRender) {
            if (!shouldRender) return false;
            mainMenu.drawGrid(mainMenu.gridIndexOf(static_cast<MenuItemInterface *>(menuItem)));
            return true;
        };
    }

    std::vector<String> l = bruceConfig.disabledMenus;
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) { // If menu item is not disabled
            options.push_back(
                {// selected lambda
                 itemName,
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false, // selected = false
                 renderItem,
                 _menuItems[i]
                }
            );
        }
    }

    // The grid needs a fresh full repaint every time we come back to the main menu,
    // since loopOptions clears the screen before the first render.
    if (gridStyle) {
        buildGridLayout(options.size());
        _gridScroll = 0;
        _gridLastIndex = -1;
        _gridRedrawAll = true;
#if defined(JOHNNYOS_STICKS3)
        _johnnyLastIndex = -1;
        _johnnyLastPage = -1;
        _johnnyLastCount = -1;
        _johnnyLastTheme = 255;
#endif
        mainMenuGridColumns = _grid.cols;
    } else {
        mainMenuGridColumns = 0;
    }

    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
};

/*********************************************************************
**  Function: gridIndexOf
**  Position of a menu item inside the currently built options list.
**  The hover callback is a plain function pointer and cannot capture the
**  index, so it is resolved back from the item pointer it receives.
**********************************************************************/

#if defined(JOHNNYOS_STICKS3)
// A native 240x135 iOS-inspired launcher: no web assets, no fake app callbacks.
void MainMenu::drawJohnnyLauncher(int index) {
    const int count = (int)options.size();
    if (count == 0) return;
    index = constrain(index, 0, count - 1);
    const int cols = 3;
    const int perPage = 6;
    const int page = index / perPage;
    const uint16_t white = tft.color565(248, 251, 255);
    const uint16_t muted = tft.color565(176, 195, 229);
    const uint16_t accent[] = {
        tft.color565(99, 104, 243), tft.color565(34, 172, 239),
        tft.color565(65, 117, 239), tft.color565(255, 139, 91),
        tft.color565(232, 83, 119), tft.color565(53, 159, 245),
        tft.color565(143, 151, 170), tft.color565(95, 217, 171)
    };
    // Keep the current page in display RAM. Only repaint on page changes.
    const uint8_t theme = johnnyTheme();
    const bool full = _johnnyLastIndex < 0 || _johnnyLastPage != page ||
                      _johnnyLastCount != count || _johnnyLastTheme != theme;
    if (full) {
        johnnyDrawWallpaper();
    tft.setTextSize(1);
    tft.setTextColor(white);
    tft.drawString("Johnny", 9, 5, 2);
    tft.setTextColor(tft.color565(124, 181, 255));
    tft.drawString("OS", 61, 5, 2);
    tft.setTextColor(muted);
    tft.drawRightString(String(getBattery()) + "%", tftWidth - 8, 7, 1);
    tft.drawFastHLine(9, 23, tftWidth - 18, tft.color565(64, 79, 129));

    }
    const int cellW = (tftWidth - 10) / cols;
    for (int i = page * perPage; i < min(count, (page + 1) * perPage); i++) {
        if (!full && i != index && i != _johnnyLastIndex) continue;
        const int local = i - page * perPage;
        const int x = 5 + (local % cols) * cellW;
        const int y = 28 + (local / cols) * 46;
        const int cx = x + cellW / 2;
        const int iconX = cx - 15;
        const int iconY = y;
        const bool selected = i == index;
        options[i].x = x;
        options[i].y = y;
        options[i].w = cellW;
        options[i].h = 45;
        if (!full) {
            // Recreate only the old/new icon cell background, not the screen.
            for (int sy = y - 3; sy < y + 44 && sy < tftHeight - 17; sy += 2) {
                int d = sy < tftHeight / 2 ? sy : tftHeight - sy;
                uint16_t c = theme == 1
                    ? tft.color565(23 + d / 4, 12 + d / 6, 56 + d / 2)
                    : theme == 2
                    ? tft.color565(8 + d / 7, 32 + d / 4, 37 + d / 3)
                    : tft.color565(10 + d / 5, 17 + d / 4, 47 + d / 2);
                tft.fillRect(x, sy, cellW, 2, c);
            }
        }

        const String name = options[i].label;
        uint16_t mainColor = accent[i % 8];
        if (name == "Johnny AI") mainColor = tft.color565(160, 111, 249);
        else if (name == "Wi-Fi Scan" || name == "WiFi") mainColor = tft.color565(53, 173, 248);
        else if (name == "BLE") mainColor = tft.color565(76, 117, 241);
        else if (name == "Games") mainColor = tft.color565(251, 139, 105);
        else if (name == "IR") mainColor = tft.color565(234, 91, 119);
        else if (name == "Files") mainColor = tft.color565(62, 162, 247);
        else if (name == "Themes") mainColor = tft.color565(144, 153, 176);
        else if (name == "System") mainColor = tft.color565(87, 117, 155);
        else if (name == "Phone Remote") mainColor = tft.color565(40, 185, 186);
        else if (name == "Air Mouse") mainColor = tft.color565(52, 195, 157);
        else if (name == "Media") mainColor = tft.color565(218, 106, 196);
        else if (name == "Controls") mainColor = tft.color565(240, 162, 93);
        if (selected) {
            tft.fillRoundRect(iconX - 4, iconY - 4, 38, 39, 12, tft.color565(197, 222, 255));
            tft.fillRoundRect(iconX - 2, iconY - 2, 34, 35, 10, tft.color565(44, 58, 114));
        }
        tft.fillRoundRect(iconX + 1, iconY + 3, 30, 31, 9, tft.color565(9, 16, 48));
        tft.fillRoundRect(iconX, iconY, 30, 30, 9, mainColor);
        tft.drawFastHLine(iconX + 8, iconY + 4, 14, tft.color565(210, 232, 255));
        tft.drawFastHLine(iconX + 10, iconY + 6, 10, tft.color565(162, 196, 238));

        if (name == "Johnny AI") {
            // Sparkle inside the assistant icon.
            tft.drawLine(cx, iconY + 5, cx, iconY + 25, white);
            tft.drawLine(cx - 10, iconY + 15, cx + 10, iconY + 15, white);
            tft.drawLine(cx - 6, iconY + 9, cx + 6, iconY + 21, white);
            tft.drawLine(cx - 6, iconY + 21, cx + 6, iconY + 9, white);
            tft.fillCircle(cx, iconY + 15, 3, white);
        } else if (name == "Wi-Fi Scan" || name == "WiFi") {
            tft.drawArc(cx, iconY + 24, 13, 11, 210, 330, white, mainColor);
            tft.drawArc(cx, iconY + 24, 8, 6, 210, 330, white, mainColor);
            tft.fillCircle(cx, iconY + 22, 2, white);
        } else if (name == "BLE") {
            tft.drawLine(cx, iconY + 5, cx, iconY + 25, white);
            tft.drawLine(cx, iconY + 5, cx + 9, iconY + 12, white);
            tft.drawLine(cx + 9, iconY + 12, cx - 8, iconY + 20, white);
            tft.drawLine(cx - 8, iconY + 10, cx + 9, iconY + 20, white);
            tft.drawLine(cx + 9, iconY + 20, cx, iconY + 25, white);
        } else if (name == "Games") {
            tft.fillRoundRect(cx - 11, iconY + 10, 22, 13, 5, white);
            tft.drawFastHLine(cx - 7, iconY + 16, 7, mainColor);
            tft.drawFastVLine(cx - 4, iconY + 13, 7, mainColor);
            tft.fillCircle(cx + 5, iconY + 15, 2, mainColor);
            tft.fillCircle(cx + 9, iconY + 18, 2, mainColor);
        } else if (name == "IR") {
            tft.drawCircle(cx, iconY + 16, 9, white);
            tft.fillRect(cx - 3, iconY + 4, 6, 12, mainColor);
            tft.drawFastVLine(cx, iconY + 5, 10, white);
        } else if (name == "Files") {
            tft.fillRoundRect(cx - 11, iconY + 10, 22, 14, 3, white);
            tft.fillRect(cx - 10, iconY + 7, 9, 6, white);
            tft.drawFastHLine(cx - 7, iconY + 16, 14, mainColor);
        } else if (name == "Themes") {
            tft.drawCircle(cx, iconY + 15, 9, white);
            tft.fillCircle(cx - 4, iconY + 11, 2, tft.color565(255, 214, 98));
            tft.fillCircle(cx + 5, iconY + 13, 2, tft.color565(91, 230, 193));
            tft.fillCircle(cx, iconY + 20, 2, tft.color565(255, 140, 208));
        } else if (name == "System") {
            tft.drawRoundRect(cx - 10, iconY + 8, 20, 16, 3, white);
            tft.drawFastHLine(cx - 6, iconY + 13, 12, white);
            tft.fillRect(cx - 5, iconY + 17, 10, 3, white);
        } else if (name == "Phone Remote") {
            tft.drawRoundRect(cx - 7, iconY + 5, 14, 21, 3, white);
            tft.drawFastHLine(cx - 3, iconY + 21, 6, white);
            tft.fillCircle(cx, iconY + 23, 1, white);
        } else if (name == "Air Mouse") {
            tft.drawCircle(cx, iconY + 15, 8, white);
            tft.drawLine(cx, iconY + 4, cx, iconY + 26, white);
            tft.drawLine(cx - 10, iconY + 15, cx + 10, iconY + 15, white);
        } else if (name == "Media") {
            tft.fillTriangle(cx - 8, iconY + 7, cx - 8, iconY + 24, cx + 7, iconY + 15, white);
            tft.fillRect(cx + 9, iconY + 9, 3, 12, white);
        } else if (name == "Controls") {
            tft.drawCircle(cx, iconY + 15, 11, white);
            tft.fillCircle(cx, iconY + 9, 2, white);
            tft.drawFastVLine(cx, iconY + 13, 10, white);
        } else {
            // Bruce's real icon is used for all legacy tools.
            MenuItemInterface *obj = static_cast<MenuItemInterface *>(options[i].hoverPointer);
            if (obj) obj->drawIconInBox(cx, iconY + 16, 23, white, mainColor);
        }
        String label = name;
        if (name == "Johnny AI") label = "AI";
        else if (name == "Wi-Fi Scan") label = "Wi-Fi";
        else if (name == "BLE") label = "Bluetooth";
        else if (name == "IR") label = "IR Remote";
        tft.setTextColor(selected ? white : muted);
        tft.drawCentreString(label.substring(0, 9), cx, iconY + 34, 1);
    }
    if (full) {
    // Native input map: GPIO11 next / hold select; GPIO12 previous / hold back.
    tft.fillRoundRect(8, 121, tftWidth - 16, 12, 5, tft.color565(16, 26, 56));
    tft.setTextColor(muted);
    tft.drawString("Tap: move  Hold: select/back", 13, 124, 1);
    tft.drawRightString(String(page + 1) + "/" + String((count + perPage - 1) / perPage), tftWidth - 10, 124, 1);
    }
    _johnnyLastIndex = index;
    _johnnyLastPage = page;
    _johnnyLastCount = count;
    _johnnyLastTheme = theme;
}

#endif

int MainMenu::gridIndexOf(MenuItemInterface *item) {
    for (size_t i = 0; i < options.size(); i++) {
        if (options[i].hoverPointer == item) return (int)i;
    }
    return 0;
}

/*********************************************************************
**  Function: buildGridLayout
**  Picks the column count that makes cells (and therefore icons) as large as possible for the
**  available area, instead of just cramming in as many columns as fit at the minimum size. Only
**  falls back to the minimum size, with paging, once even one column can't make cells that big.
**********************************************************************/
void MainMenu::buildGridLayout(int itemCount) {
    // Floor so a cell never gets too small to tap/read — this is what triggers paging, not a
    // preferred size. Scaled by FP so it still means something on high-res boards.
    const int MIN_CELL_W = 54 * FP;
    const int MIN_CELL_H = 40 * FP;
    // Right-edge column reserved for the scrollbar and the page-up/page-down tap zone (see
    // MainMenu::handleGridPageTap()) — wide enough to actually tap, not just the 3px bar itself.
    const int PAGE_TAP_W = 4 * BORDER_OFFSET_FROM_SCREEN_EDGE;

    if (itemCount < 1) itemCount = 1;

    int areaX = BORDER_OFFSET_FROM_SCREEN_EDGE + 2;
    int areaY = STATUS_BAR_HEIGHT - 2; // first line free under the status bar separator
    int areaW = tftWidth - 2 * areaX - PAGE_TAP_W;
    int areaH = tftHeight - areaY - areaX;

    // Try every column count down to the MIN_CELL_W floor and keep the one whose resulting cells
    // are largest in their smaller dimension (min(cellW, cellH)) — that's what a lopsided grid
    // wastes: a handful of very wide, short rows or very tall, narrow columns both score low here,
    // while a balanced, roughly-square cell that actually fills the screen scores high.
    int maxCols = max(1, min(itemCount, areaW / MIN_CELL_W));
    int bestCols = 1;
    int bestScore = -1;
    for (int cols = 1; cols <= maxCols; cols++) {
        int rows = (itemCount + cols - 1) / cols;
        int score = min(areaW / cols, areaH / rows);
        if (score > bestScore) {
            bestScore = score;
            bestCols = cols;
        }
    }

    _grid.cols = bestCols;
    _grid.cellW = areaW / _grid.cols;
    _grid.rows = (itemCount + _grid.cols - 1) / _grid.cols;

    // Rows still get capped to what fits at MIN_CELL_H — that's the paging trigger when there
    // are simply too many items for one screen even at the smallest usable size.
    _grid.visibleRows = max(1, areaH / MIN_CELL_H);
    if (_grid.visibleRows > _grid.rows) _grid.visibleRows = _grid.rows;
    _grid.cellH = areaH / _grid.visibleRows; // fills whatever height is actually available

    _grid.labelSize = _grid.cellW >= 8 * LW * FM ? FM : FP;
    // Everything the cell has left once the label and a thin margin are taken out
    _grid.iconBox = _grid.cellH - LH * _grid.labelSize - 6;
    if (_grid.iconBox > _grid.cellW - 4) _grid.iconBox = _grid.cellW - 4; // narrow cells cap it
    if (_grid.iconBox < 12) _grid.iconBox = 12;

    // Center whatever is left over so the grid doesn't hug the border
    _grid.x = areaX + (areaW - _grid.cols * _grid.cellW) / 2;
    _grid.y = areaY + (areaH - _grid.visibleRows * _grid.cellH) / 2;

    _gridItemCount = itemCount;
    _gridRotation = bruceConfigPins.rotation;
}

/*********************************************************************
**  Function: drawGrid
**  Draws the module grid highlighting `index`. Only the two cells that
**  changed are repainted unless the whole view has to come back.
**********************************************************************/
void MainMenu::drawGrid(int index) {
#if defined(JOHNNYOS_STICKS3)
    drawJohnnyLauncher(index);
    return;
#endif
    int itemCount = options.size();
    if (itemCount < 1) return;
    if (index < 0 || index >= itemCount) index = 0;

    if (itemCount != _gridItemCount || _gridRotation != bruceConfigPins.rotation) {
        buildGridLayout(itemCount);
        _gridScroll = 0;
        _gridLastIndex = -1;
        _gridRedrawAll = true;
    }

    // Keep the selected cell inside the visible window
    int scroll = _gridScroll;
    int row = index / _grid.cols;
    if (row < scroll) scroll = row;
    else if (row >= scroll + _grid.visibleRows) scroll = row - _grid.visibleRows + 1;

    bool redrawAll = _gridRedrawAll || scroll != _gridScroll;
    _gridScroll = scroll;

    if (redrawAll) {
        // Cells scrolled out of the visible window won't get a drawGridCell() call below to refresh
        // their tap rect, so clear everyone's first (mirrors drawOptions()'s same stale-rect concern).
        for (auto &opt : options) {
            opt.w = 0;
            opt.h = 0;
        }
        drawMainBorder(false);
        tft.fillRect(
            _grid.x, _grid.y, _grid.cols * _grid.cellW, _grid.visibleRows * _grid.cellH, bruceConfig.bgColor
        );
        int first = _gridScroll * _grid.cols;
        int last = min(itemCount, first + _grid.visibleRows * _grid.cols);
        for (int i = first; i < last; i++) drawGridCell(i, i == index);
        drawGridScrollBar();
#if defined(HAS_TOUCH)
        TouchFooter();
#endif
    } else {
        if (_gridLastIndex >= 0 && _gridLastIndex != index && _gridLastIndex < itemCount)
            drawGridCell(_gridLastIndex, false);
        drawGridCell(index, true);
    }

    _gridLastIndex = index;
    _gridRedrawAll = false;
}

/*********************************************************************
**  Function: drawGridCell
**  Paints a single module cell, inverted when it is the selected one
**********************************************************************/
void MainMenu::drawGridCell(int index, bool selected) {
    int row = index / _grid.cols - _gridScroll;
    int col = index % _grid.cols;
    if (row < 0 || row >= _grid.visibleRows) return;

    int x = _grid.x + col * _grid.cellW;
    int y = _grid.y + row * _grid.cellH;

    options[index].x = x;
    options[index].y = y;
    options[index].w = _grid.cellW;
    options[index].h = _grid.cellH;

    uint16_t bgColor = selected ? bruceConfig.priColor : bruceConfig.bgColor;
    uint16_t fgColor = selected ? bruceConfig.bgColor : bruceConfig.priColor;

    tft.fillRect(x, y, _grid.cellW, _grid.cellH, bruceConfig.bgColor);
    if (selected) tft.fillRoundRect(x + 1, y + 1, _grid.cellW - 2, _grid.cellH - 2, 3, bgColor);

    MenuItemInterface *item = static_cast<MenuItemInterface *>(options[index].hoverPointer);
    if (item)
        item->drawIconInBox(x + _grid.cellW / 2, y + 3 + _grid.iconBox / 2, _grid.iconBox, fgColor, bgColor);

    int maxChars = (_grid.cellW - 4) / (LW * _grid.labelSize);
    tft.setTextSize(_grid.labelSize);
    tft.setTextColor(fgColor, bgColor);
    tft.drawCentreString(
        options[index].label.substring(0, maxChars),
        x + _grid.cellW / 2,
        y + _grid.cellH - LH * _grid.labelSize - 3,
        1
    );
}

/*********************************************************************
**  Function: drawGridScrollBar
**  Thin indicator on the right telling there are more rows off screen
**********************************************************************/
void MainMenu::drawGridScrollBar() {
    int barX = tftWidth - BORDER_OFFSET_FROM_SCREEN_EDGE - 4;
    int trackY = _grid.y;
    int trackH = _grid.visibleRows * _grid.cellH;

    tft.fillRect(barX, trackY, 3, trackH, bruceConfig.bgColor);
    if (_grid.rows <= _grid.visibleRows) return;

    int thumbH = max(6, trackH * _grid.visibleRows / _grid.rows);
    int thumbY = trackY + (trackH - thumbH) * _gridScroll / (_grid.rows - _grid.visibleRows);
    tft.fillRect(barX, thumbY, 3, thumbH, bruceConfig.priColor);
}

/*********************************************************************
**  Function: handleGridPageTap
**  Right-edge strip (see PAGE_TAP_W in buildGridLayout): tapping the top half jumps a full page
**  up, the bottom half a full page down. Needed because a tap can only select a cell that's
**  already on screen, unlike Prev/Next which auto-scroll into view one cell at a time.
**********************************************************************/
bool MainMenu::handleGridPageTap(int x, int y, int currentIndex, int &newIndex) {
    if (_grid.rows <= _grid.visibleRows) return false; // everything already fits on one page

    int zoneX = _grid.x + _grid.cols * _grid.cellW;
    int zoneY = _grid.y;
    int zoneW = tftWidth - BORDER_OFFSET_FROM_SCREEN_EDGE - zoneX;
    int zoneH = _grid.visibleRows * _grid.cellH;
    if (x < zoneX || x >= zoneX + zoneW || y < zoneY || y >= zoneY + zoneH) return false;

    int maxScroll = _grid.rows - _grid.visibleRows;
    int targetScroll = (y < zoneY + zoneH / 2) ? max(0, _gridScroll - _grid.visibleRows)
                                               : min(maxScroll, _gridScroll + _grid.visibleRows);

    // Keep the same column, jump to the first row of the new page.
    int col = currentIndex % _grid.cols;
    int itemCount = static_cast<int>(options.size());
    newIndex = min(targetScroll * _grid.cols + col, itemCount - 1);
    return true;
}

/*********************************************************************
**  Function: hideAppsMenu
**  Menu to Hide or show menus
**********************************************************************/

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
    int index = 0;
RESTART: // using gotos to avoid stackoverflow after many choices
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back(
            {label,
             [this, label, enabled]() {
                 if (enabled) bruceConfig.addDisabledMenu(label);
                 else bruceConfig.removeDisabledMenu(label);
             },
             enabled}
        );
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    index = loopOptions(options, index);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
