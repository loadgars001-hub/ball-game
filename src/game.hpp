#pragma once
#include <SFML/Graphics.hpp>
#include "ball.hpp"
#include "level.hpp"
#include "script_engine.hpp"
#include "physics_world.hpp"
#include "network.hpp"
#include <vector>
#include <string>

class Game {
public:
    explicit Game();
    void run();

private:
    void handleEvents();
    void update(float dt);
    void render();

    void startNewGame(bool multiplayer = false);
    void nextLevel();
    void setupPhysicsForLevel(); // пересоздаёт Box2D мир и тела для текущего level
    void drawGoal();
    void drawHUD();
    void drawOverlay(const std::string& title, const std::string& sub);

    // ── Сетевой мультиплеер по LAN ───────────────────────────────────────
    // Хост считает физику и присылает клиенту готовое состояние кадра;
    // клиент только шлёт своё положение мыши и рисует то, что получил.
    // Предметы/Lua-скрипты/спиннеры в сетевом режиме не участвуют (см. network.hpp).
    void startNetworkHost();
    void startNetworkClient();
    void updateNetHostWaiting();
    void renderNetHostWaiting();
    void updateNetClientSearching();
    void renderNetClientSearching();
    void buildNetLevelInfo(NetLevelInfo& outInfo); // хост: собирает описание level для отправки
    void applyNetLevelInfo(const NetLevelInfo& info); // клиент: строит level из полученного описания
    void updateNetworkHostPlaying(float dt);   // хост: физика + приём ввода клиента + отправка состояния
    void updateNetworkClientPlaying(float dt); // клиент: отправка своего ввода + приём состояния

    void renderMenu();
    void handleMenuClick(sf::Vector2f mouse);
    void renderMultiplayerMenu();
    void handleMultiplayerMenuClick(sf::Vector2f mouse);
    void renderColorPicker();
    void handleColorPickerClick(sf::Vector2f mouse);

    // ── Редактор уровней ────────────────────────────────────────────────
    // Wall/Start/Goal/Erase — геометрия; Item размещает предмет с одним из
    // Lua-пресетов (Coin/Trap/Boost/Blank), которые затем можно редактировать
    // прямо на экране редактора скриптов (ScriptEdit).
    enum class EditTool { Wall, Start, Goal, Erase, Item };
    enum class ItemPreset { Coin, Trap, Boost, SpinBoards, Blank };

    void enterEditor();
    void renderEditor();
    void handleEditorMouseDown(sf::Vector2f mouse, sf::Mouse::Button button);
    void handleEditorMouseUp(sf::Vector2f mouse);
    void handleEditorKey(sf::Keyboard::Key key);
    sf::Vector2f snapToGrid(sf::Vector2f p) const;
    void editorSaveLevel();
    void editorPlayLevel();
    void editorClear();

    // ── Редактор скриптов (текстовый ввод Lua) ──────────────────────────
    // Открывается при клике по уже размещённому предмету/скриптовой стене
    // инструментом Wall в режиме Item, либо сразу после установки предмета.
    enum class ScriptTargetKind { Item, Wall };
    void openScriptEditor(ScriptTargetKind kind, int index);
    void renderScriptEditor();
    void handleScriptEditorTextEntered(sf::Uint32 unicode);
    void handleScriptEditorKey(sf::Keyboard::Key key, bool ctrl);
    void pasteClipboardIntoScript(); // общая логика для Ctrl+V и кнопки Paste
    void applyScriptEditorChanges(); // компилирует текст и применяет к цели

    sf::RenderWindow window;
    sf::Font         font;
    bool             fontOk = false;

    ScriptEngine     scriptEngine; // общий Lua-движок для предметов и скриптовых стен
    PhysicsWorld     physicsWorld; // Box2D мир; пересоздаётся при каждой загрузке уровня

    Ball             ball;
    Ball             ball2;              // второй шар для локального/сетевого кооп-мультиплеера
    bool             isMultiplayer = false;
    bool             ball1Reached  = false; // достиг ли шар 1 цели (для кооп-победы)
    bool             ball2Reached  = false; // достиг ли шар 2 цели
    Level            level;        // текущий (сгенерированный/кастомный) уровень
    int              stageNum = 1; // номер уровня в бесконечной последовательности

    // ── Сетевой мультиплеер ───────────────────────────────────────────────
    NetworkSession   netSession;
    bool             isNetworked   = false; // true для Host/Client-режима поверх обычного isMultiplayer
    bool             isNetworkHost = false; // среди двух сетевых игроков — кто именно я
    NetInputState    lastClientInput{0.f, 0.f, false}; // хост: последнее полученное состояние ввода клиента
    NetFrameState    lastNetFrameState{};               // клиент: последнее полученное состояние кадра
    std::vector<int> kinematicWallIndices; // индексы в level.walls, которые движутся (для синхронизации по сети)
    float            netStatusTimer = 0.f; // анимация точек "Searching..." на экранах ожидания/поиска

    // Состояние мыши
    sf::Vector2f prevMouse;
    bool         wasPressed = false;

    // UI / анимация
    enum class State { Menu, MultiplayerMenu, ColorPicker, Editor, ScriptEdit, Playing, Win, NetHostWaiting, NetClientSearching };
    State  state      = State::Menu;
    State  returnStateAfterPlay = State::Menu; // куда вернуться по Esc/после Win, если играли тест из редактора
    float  stateTimer = 0.f;   // секунды до авто-перехода
    float  goalPulse  = 0.f;   // анимация цели
    int    totalPushes = 0;    // счётчик толчков

    // Кнопки главного меню
    sf::FloatRect btnStartRect;
    sf::FloatRect btnEditorRect;
    sf::FloatRect btnMultiplayerRect;
    sf::FloatRect btnHostNetRect;
    sf::FloatRect btnJoinNetRect;
    sf::FloatRect btnExitRect;
    sf::FloatRect btnColorRect;
    sf::FloatRect btnMpBackRect; // "Back" в подменю мультиплеера

    // Выбор цвета шара
    static const int COLOR_COUNT = 5;
    sf::Color    ballColors[COLOR_COUNT];
    int          selectedColor = 0;
    sf::FloatRect colorSwatchRects[COLOR_COUNT];
    sf::FloatRect btnColorBackRect;

    // ── Состояние редактора ─────────────────────────────────────────────
    static const float GRID_SIZE;          // размер ячейки сетки в пикселях
    Level        editLevel;                // уровень, который сейчас редактируется
    EditTool     editTool = EditTool::Wall;
    ItemPreset   editItemPreset = ItemPreset::Coin; // какой пресет ставить инструментом Item
    bool         editDrawing = false;       // идёт ли сейчас рисование стены (ЛКМ зажата)
    sf::Vector2f editDragStart;             // точка начала перетаскивания (мировые координаты)
    bool         editGridSnap = true;       // привязка к сетке вкл/выкл (клавиша G)
    std::string  editStatusMsg;             // сообщение в HUD редактора (например, "Saved!")
    float        editStatusTimer = 0.f;

    sf::FloatRect btnToolWallRect, btnToolStartRect, btnToolGoalRect, btnToolEraseRect, btnToolItemRect;
    sf::FloatRect btnEditSaveRect, btnEditPlayRect, btnEditClearRect, btnEditBackRect;
    sf::FloatRect btnItemCoinRect, btnItemTrapRect, btnItemBoostRect, btnItemSpinBoardsRect, btnItemBlankRect;

    // ── Состояние текстового редактора скриптов ─────────────────────────
    ScriptTargetKind scriptTargetKind = ScriptTargetKind::Item;
    int              scriptTargetIndex = -1; // индекс в editLevel.items или editLevel.walls
    std::string      scriptEditText;         // редактируемый текст (с переносами \n)
    std::string      scriptEditError;        // последняя ошибка компиляции, если есть
    sf::FloatRect    btnScriptApplyRect, btnScriptCancelRect, btnScriptPasteRect;
};
