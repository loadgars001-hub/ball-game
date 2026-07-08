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
    void setupPhysicsForLevel(); 
    void drawGoal();
    void drawHUD();
    void drawOverlay(const std::string& title, const std::string& sub);

    void startNetworkHost();
    void startNetworkClient();
    void updateNetHostWaiting();
    void renderNetHostWaiting();
    void updateNetClientSearching();
    void renderNetClientSearching();
    void buildNetLevelInfo(NetLevelInfo& outInfo);
    void applyNetLevelInfo(const NetLevelInfo& info);
    void updateNetworkHostPlaying(float dt);   
    void updateNetworkClientPlaying(float dt);

    void renderMenu();
    void handleMenuClick(sf::Vector2f mouse);
    void renderMultiplayerMenu();
    void handleMultiplayerMenuClick(sf::Vector2f mouse);
    void renderColorPicker();
    void handleColorPickerClick(sf::Vector2f mouse);

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

    enum class ScriptTargetKind { Item, Wall };
    void openScriptEditor(ScriptTargetKind kind, int index);
    void renderScriptEditor();
    void handleScriptEditorTextEntered(sf::Uint32 unicode);
    void handleScriptEditorKey(sf::Keyboard::Key key, bool ctrl);
    void pasteClipboardIntoScript(); 
    void applyScriptEditorChanges(); 
    sf::RenderWindow window;
    sf::Font         font;
    bool             fontOk = false;

    ScriptEngine     scriptEngine; 
    PhysicsWorld     physicsWorld; 
    Ball             ball;
    Ball             ball2;              
    bool             isMultiplayer = false;
    bool             ball1Reached  = false;
    bool             ball2Reached  = false; 
    Level            level;       
    int              stageNum = 1; 

    NetworkSession   netSession;
    bool             isNetworked   = false;
    bool             isNetworkHost = false;
    NetInputState    lastClientInput{0.f, 0.f, false}; 
    NetFrameState    lastNetFrameState{};              
    std::vector<int> kinematicWallIndices; 
    float            netStatusTimer = 0.f; 

    sf::Vector2f prevMouse;
    bool         wasPressed = false;

    // UI / анимация
    enum class State { Menu, MultiplayerMenu, ColorPicker, Editor, ScriptEdit, Playing, Win, NetHostWaiting, NetClientSearching };
    State  state      = State::Menu;
    State  returnStateAfterPlay = State::Menu; 
    float  stateTimer = 0.f; 
    float  goalPulse  = 0.f;
    int    totalPushes = 0;   
  
    sf::FloatRect btnStartRect;
    sf::FloatRect btnEditorRect;
    sf::FloatRect btnMultiplayerRect;
    sf::FloatRect btnHostNetRect;
    sf::FloatRect btnJoinNetRect;
    sf::FloatRect btnExitRect;
    sf::FloatRect btnColorRect;
    sf::FloatRect btnMpBackRect; 

    static const int COLOR_COUNT = 5;
    sf::Color    ballColors[COLOR_COUNT];
    int          selectedColor = 0;
    sf::FloatRect colorSwatchRects[COLOR_COUNT];
    sf::FloatRect btnColorBackRect;

    static const float GRID_SIZE;          
    Level        editLevel;                
    EditTool     editTool = EditTool::Wall;
    ItemPreset   editItemPreset = ItemPreset::Coin; 
    bool         editDrawing = false;      
    sf::Vector2f editDragStart;            
    bool         editGridSnap = true;      
    std::string  editStatusMsg;             
    float        editStatusTimer = 0.f;

    sf::FloatRect btnToolWallRect, btnToolStartRect, btnToolGoalRect, btnToolEraseRect, btnToolItemRect;
    sf::FloatRect btnEditSaveRect, btnEditPlayRect, btnEditClearRect, btnEditBackRect;
    sf::FloatRect btnItemCoinRect, btnItemTrapRect, btnItemBoostRect, btnItemSpinBoardsRect, btnItemBlankRect;

    // ── Состояние текстового редактора скриптов ─────────────────────────
    ScriptTargetKind scriptTargetKind = ScriptTargetKind::Item;
    int              scriptTargetIndex = -1; 
    std::string      scriptEditText;        
    std::string      scriptEditError;       
    sf::FloatRect    btnScriptApplyRect, btnScriptCancelRect, btnScriptPasteRect;
};
