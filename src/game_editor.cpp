#include "game.hpp"
#include "game_utils.hpp"
#include <cmath>

void Game::enterEditor() {
    editLevel = Level{};
    editLevel.name = "Custom";
    editLevel.goalRadius = 28.f;
    sf::Vector2f screen = {(float)window.getSize().x, (float)window.getSize().y};
    editLevel.ballStart = {screen.x * 0.1f, screen.y * 0.5f};
    editLevel.goal      = {screen.x * 0.9f, screen.y * 0.5f};
    editTool = EditTool::Wall;
    editDrawing = false;
    editStatusMsg.clear();
    editStatusTimer = 0.f;
    state = State::Editor;
}

sf::Vector2f Game::snapToGrid(sf::Vector2f p) const {
    if (!editGridSnap) return p;
    float gx = std::round(p.x / GRID_SIZE) * GRID_SIZE;
    float gy = std::round(p.y / GRID_SIZE) * GRID_SIZE;
    return {gx, gy};
}

void Game::handleEditorMouseDown(sf::Vector2f mouse, sf::Mouse::Button button) {
    // Панель инструментов теперь в две строки: основные инструменты (0-64) +
    // под-панель пресетов предметов (64-114, видна только при инструменте Item).
    float panelBottom = (editTool == EditTool::Item) ? 114.f : 64.f;

    if (mouse.y <= panelBottom) {
        if (pointInRect(mouse, btnToolWallRect))  { editTool = EditTool::Wall;  return; }
        if (pointInRect(mouse, btnToolStartRect)) { editTool = EditTool::Start; return; }
        if (pointInRect(mouse, btnToolGoalRect))  { editTool = EditTool::Goal;  return; }
        if (pointInRect(mouse, btnToolEraseRect)) { editTool = EditTool::Erase; return; }
        if (pointInRect(mouse, btnToolItemRect))  { editTool = EditTool::Item;  return; }
        if (pointInRect(mouse, btnEditSaveRect))  { editorSaveLevel(); return; }
        if (pointInRect(mouse, btnEditPlayRect))  { editorPlayLevel(); return; }
        if (pointInRect(mouse, btnEditClearRect)) { editorClear(); return; }
        if (pointInRect(mouse, btnEditBackRect))  { state = State::Menu; return; }

        if (editTool == EditTool::Item) {
            if (pointInRect(mouse, btnItemCoinRect))  { editItemPreset = ItemPreset::Coin;  return; }
            if (pointInRect(mouse, btnItemTrapRect))  { editItemPreset = ItemPreset::Trap;  return; }
            if (pointInRect(mouse, btnItemBoostRect)) { editItemPreset = ItemPreset::Boost; return; }
            if (pointInRect(mouse, btnItemSpinBoardsRect)) { editItemPreset = ItemPreset::SpinBoards; return; }
            if (pointInRect(mouse, btnItemBlankRect)) { editItemPreset = ItemPreset::Blank; return; }
        }
        return; // клик в области панели, но не по кнопке — игнорируем
    }

    if (button != sf::Mouse::Left) return;

    switch (editTool) {
        case EditTool::Start:
            editLevel.ballStart = snapToGrid(mouse);
            break;
        case EditTool::Goal:
            editLevel.goal = snapToGrid(mouse);
            break;
        case EditTool::Erase: {
            // Сначала проверяем предметы (они обычно меньше и сверху)
            bool erased = false;
            for (int i = (int)editLevel.items.size() - 1; i >= 0; --i) {
                sf::Vector2f d = editLevel.items[i].pos - mouse;
                if (std::sqrt(d.x*d.x + d.y*d.y) <= editLevel.items[i].radius) {
                    editLevel.items.erase(editLevel.items.begin() + i);
                    erased = true;
                    break;
                }
            }
            if (!erased) {
                for (int i = (int)editLevel.walls.size() - 1; i >= 0; --i) {
                    const auto& r = editLevel.walls[i].rect;
                    if (mouse.x >= r.left && mouse.x <= r.left + r.width &&
                        mouse.y >= r.top  && mouse.y <= r.top  + r.height) {
                        editLevel.walls.erase(editLevel.walls.begin() + i);
                        break;
                    }
                }
            }
            break;
        }
        case EditTool::Wall: {
            // Ctrl+клик по уже существующей скриптовой/обычной стене открывает
            // редактор скрипта для неё (вместо рисования новой стены).
            bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
            if (ctrl) {
                for (int i = (int)editLevel.walls.size() - 1; i >= 0; --i) {
                    const auto& r = editLevel.walls[i].rect;
                    if (mouse.x >= r.left && mouse.x <= r.left + r.width &&
                        mouse.y >= r.top  && mouse.y <= r.top  + r.height) {
                        openScriptEditor(ScriptTargetKind::Wall, i);
                        return;
                    }
                }
            }
            editDrawing = true;
            editDragStart = snapToGrid(mouse);
            break;
        }
        case EditTool::Item: {
            // Клик по существующему предмету открывает редактор его скрипта;
            // клик по пустому месту ставит новый предмет с выбранным пресетом.
            for (int i = (int)editLevel.items.size() - 1; i >= 0; --i) {
                sf::Vector2f d = editLevel.items[i].pos - mouse;
                if (std::sqrt(d.x*d.x + d.y*d.y) <= editLevel.items[i].radius) {
                    openScriptEditor(ScriptTargetKind::Item, i);
                    return;
                }
            }

            Item item;
            item.pos = snapToGrid(mouse);
            std::string src, name;
            switch (editItemPreset) {
                case ItemPreset::Coin:  src = ItemPresets::coinScript();  name = "coin";  break;
                case ItemPreset::Trap:  src = ItemPresets::trapScript();  name = "trap";  break;
                case ItemPreset::Boost: src = ItemPresets::boostScript(); name = "boost"; break;
                case ItemPreset::SpinBoards: src = ItemPresets::spinBoardsScript(); name = "spinboards"; break;
                case ItemPreset::Blank: src = ItemPresets::blankScript(); name = "item";  break;
            }
            item.loadScript(scriptEngine, src, name);
            editLevel.items.push_back(item);
            break;
        }
    }
}

void Game::handleEditorMouseUp(sf::Vector2f mouse) {
    if (editTool != EditTool::Wall || !editDrawing) { editDrawing = false; return; }
    editDrawing = false;

    sf::Vector2f end = snapToGrid(mouse);
    float x = std::min(editDragStart.x, end.x);
    float y = std::min(editDragStart.y, end.y);
    float w = std::abs(end.x - editDragStart.x);
    float h = std::abs(end.y - editDragStart.y);

    // Минимальный размер — одна ячейка сетки, чтобы клик без перетаскивания тоже ставил блок
    if (w < GRID_SIZE * 0.5f) w = GRID_SIZE;
    if (h < GRID_SIZE * 0.5f) h = GRID_SIZE;

    editLevel.walls.emplace_back(x, y, w, h, sf::Color(160, 80, 30));
}

void Game::handleEditorKey(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::G) {
        editGridSnap = !editGridSnap;
    } else if (key == sf::Keyboard::Num1) {
        editTool = EditTool::Wall;
    } else if (key == sf::Keyboard::Num2) {
        editTool = EditTool::Start;
    } else if (key == sf::Keyboard::Num3) {
        editTool = EditTool::Goal;
    } else if (key == sf::Keyboard::Num4) {
        editTool = EditTool::Erase;
    } else if (key == sf::Keyboard::Num5) {
        editTool = EditTool::Item;
    } else if (key == sf::Keyboard::S) {
        editorSaveLevel();
    } else if (key == sf::Keyboard::P) {
        editorPlayLevel();
    }
}

void Game::editorSaveLevel() {
    std::string dir = customLevelsDir();
    // Имя файла на основе счётчика, чтобы не перезаписывать прошлые уровни
    static int saveCounter = 1;
    std::string path = dir + "/level_" + std::to_string(saveCounter++) + ".lvl";

    if (saveLevelToFile(editLevel, path)) {
        editStatusMsg = "Saved: " + path;
    } else {
        editStatusMsg = "Save failed!";
    }
    editStatusTimer = 2.5f;
}

void Game::editorPlayLevel() {
    level = editLevel;
    if (level.walls.empty() && level.name.empty()) level.name = "Custom";
    returnStateAfterPlay = State::Editor;
    stateTimer  = 0.f;
    totalPushes = 0;
    isMultiplayer = false; // тест уровня из редактора всегда одиночный, для простоты и предсказуемости
    setupPhysicsForLevel();
    state = State::Playing;
}

void Game::editorClear() {
    editLevel.walls.clear();
    editLevel.items.clear();
    editStatusMsg = "Cleared";
    editStatusTimer = 1.5f;
}

void Game::renderEditor() {
    window.clear(sf::Color(24, 26, 36));

    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    // ── Сетка ────────────────────────────────────────────────────────────
    if (editGridSnap) {
        sf::VertexArray grid(sf::Lines);
        sf::Color gridCol(60, 64, 78, 120);
        for (float x = 0.f; x <= sz.x; x += GRID_SIZE) {
            grid.append(sf::Vertex({x, 0.f}, gridCol));
            grid.append(sf::Vertex({x, sz.y}, gridCol));
        }
        for (float y = 0.f; y <= sz.y; y += GRID_SIZE) {
            grid.append(sf::Vertex({0.f, y}, gridCol));
            grid.append(sf::Vertex({sz.x, y}, gridCol));
        }
        window.draw(grid);
    }

    // ── Стены ────────────────────────────────────────────────────────────
    for (const auto& w : editLevel.walls)
        w.draw(window);

    // ── Предметы ─────────────────────────────────────────────────────────
    for (const auto& it : editLevel.items)
        it.draw(window);

    // ── Превью рисуемой стены ───────────────────────────────────────────
    if (editTool == EditTool::Wall && editDrawing) {
        sf::Vector2i mi = sf::Mouse::getPosition(window);
        sf::Vector2f cur = snapToGrid({(float)mi.x, (float)mi.y});
        float x = std::min(editDragStart.x, cur.x);
        float y = std::min(editDragStart.y, cur.y);
        float w = std::max(std::abs(cur.x - editDragStart.x), GRID_SIZE * 0.2f);
        float h = std::max(std::abs(cur.y - editDragStart.y), GRID_SIZE * 0.2f);

        sf::RectangleShape preview({w, h});
        preview.setPosition(x, y);
        preview.setFillColor(sf::Color(160, 80, 30, 110));
        preview.setOutlineThickness(2.f);
        preview.setOutlineColor(sf::Color(255, 220, 120, 200));
        window.draw(preview);
    }

    // ── Старт и цель ─────────────────────────────────────────────────────
    sf::CircleShape startMark(16.f);
    startMark.setOrigin(16.f, 16.f);
    startMark.setPosition(editLevel.ballStart);
    startMark.setFillColor(sf::Color(70, 140, 255));
    startMark.setOutlineThickness(2.f);
    startMark.setOutlineColor(sf::Color::White);
    window.draw(startMark);

    sf::CircleShape goalMark(editLevel.goalRadius);
    goalMark.setOrigin(editLevel.goalRadius, editLevel.goalRadius);
    goalMark.setPosition(editLevel.goal);
    goalMark.setFillColor(sf::Color(255, 200, 0, 160));
    goalMark.setOutlineThickness(2.f);
    goalMark.setOutlineColor(sf::Color(255, 220, 60));
    window.draw(goalMark);

    // ── Панель инструментов ──────────────────────────────────────────────
    float panelH = (editTool == EditTool::Item) ? 114.f : 64.f;
    sf::RectangleShape topBar({sz.x, panelH});
    topBar.setPosition(0.f, 0.f);
    topBar.setFillColor(sf::Color(15, 16, 22, 230));
    window.draw(topBar);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};

    auto drawToolButton = [&](const sf::FloatRect& r, const std::string& label,
                               bool active, sf::Color base) {
        bool hover = pointInRect(mouse, r);
        sf::RectangleShape rect({r.width, r.height});
        rect.setPosition(r.left, r.top);
        sf::Color fill = active ? sf::Color(base.r+50, base.g+50, base.b+50)
                                 : (hover ? sf::Color(base.r+25, base.g+25, base.b+25) : base);
        rect.setFillColor(fill);
        rect.setOutlineThickness(active ? 3.f : 1.5f);
        rect.setOutlineColor(active ? sf::Color::White : sf::Color(255,255,255,100));
        window.draw(rect);
        if (fontOk)
            drawText(window, font, label, 16,
                     {r.left + r.width/2.f, r.top + r.height/2.f - 11.f},
                     sf::Color::White, true);
    };

    drawToolButton(btnToolWallRect,  "Wall (1)",  editTool == EditTool::Wall,  sf::Color(90, 60, 30));
    drawToolButton(btnToolStartRect, "Start (2)", editTool == EditTool::Start, sf::Color(30, 60, 110));
    drawToolButton(btnToolGoalRect,  "Goal (3)",  editTool == EditTool::Goal,  sf::Color(110, 90, 10));
    drawToolButton(btnToolEraseRect, "Erase (4)", editTool == EditTool::Erase, sf::Color(110, 30, 30));
    drawToolButton(btnToolItemRect,  "Item (5)",  editTool == EditTool::Item,  sf::Color(30, 100, 90));

    drawToolButton(btnEditSaveRect,  "Save (S)",  false, sf::Color(30, 110, 60));
    drawToolButton(btnEditPlayRect,  "Play (P)",  false, sf::Color(30, 90, 110));
    drawToolButton(btnEditClearRect, "Clear",     false, sf::Color(110, 60, 30));
    drawToolButton(btnEditBackRect,  "Back (Esc)",false, sf::Color(70, 70, 80));

    // Под-панель выбора пресета предмета
    if (editTool == EditTool::Item) {
        drawToolButton(btnItemCoinRect,  "Coin",  editItemPreset == ItemPreset::Coin,  sf::Color(120, 100, 10));
        drawToolButton(btnItemTrapRect,  "Trap",  editItemPreset == ItemPreset::Trap,  sf::Color(120, 30, 30));
        drawToolButton(btnItemBoostRect, "Boost", editItemPreset == ItemPreset::Boost, sf::Color(20, 90, 120));
        drawToolButton(btnItemSpinBoardsRect, "SpinBoards", editItemPreset == ItemPreset::SpinBoards, sf::Color(100, 40, 130));
        drawToolButton(btnItemBlankRect, "Blank", editItemPreset == ItemPreset::Blank, sf::Color(70, 70, 70));
        if (fontOk)
            drawText(window, font, "Click item to edit its Lua script. Ctrl+click a wall for script.", 14,
                     {16.f, panelH + 6.f}, sf::Color(150,150,150));
    }

    // Подсказка по сетке
    if (fontOk) {
        std::string gridHint = std::string("Grid snap: ") + (editGridSnap ? "ON" : "OFF") + " (G)";
        drawText(window, font, gridHint, 14, {sz.x - 110.f, panelH + 6.f}, sf::Color(150,150,150));
    }

    // Статусное сообщение (например, "Saved: ...")
    if (fontOk && editStatusTimer > 0.f && !editStatusMsg.empty()) {
        drawText(window, font, editStatusMsg, 20, {sz.x/2.f, panelH + 26.f}, sf::Color(140, 230, 140), true);
    }

    window.display();
}
