#include "game.hpp"
#include "game_utils.hpp"
#include <cmath>
#include <string>

const float Game::GRID_SIZE = 40.f;

Game::Game() {
    
    sf::VideoMode vm = sf::VideoMode::getDesktopMode();
    window.create(vm, "Ball Game", sf::Style::Fullscreen);
    window.setKeyRepeatEnabled(true); 
    window.setVerticalSyncEnabled(true);

    fontOk = font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
    if (!fontOk)
        fontOk = font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf");

    sf::Vector2f screen = {(float)vm.width, (float)vm.height};
    
    float bw = 280.f, bh = 70.f;
    float pairGap = 20.f;
    float pairW = bw * 2.f + pairGap;
    btnStartRect  = sf::FloatRect(screen.x/2.f - pairW/2.f,            screen.y/2.f - 160.f, bw, bh);
    btnEditorRect = sf::FloatRect(screen.x/2.f - pairW/2.f + bw + pairGap, screen.y/2.f - 160.f, bw, bh);
    btnMultiplayerRect = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f - 70.f, bw, bh);
    btnColorRect  = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f + 20.f, bw, bh);
    btnExitRect   = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f + 110.f, bw, bh);

    btnHostNetRect = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f + 20.f, bw, bh);
    btnJoinNetRect = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f + 110.f, bw, bh);
    btnMpBackRect  = sf::FloatRect(screen.x/2.f - bw/2.f, screen.y/2.f + 200.f, bw, bh);

    ballColors[0] = sf::Color(70, 140, 255); 
    ballColors[1] = sf::Color(230, 70, 70); 
    ballColors[2] = sf::Color(70, 200, 100); 
    ballColors[3] = sf::Color(240, 190, 40); 
    ballColors[4] = sf::Color(190, 80, 220); 
    selectedColor = 0;
    ball.setColor(ballColors[selectedColor]);
    ball2.setColor(sf::Color(255, 140, 30)); 

    float swSize = 110.f, swGap = 30.f;
    float totalW = COLOR_COUNT * swSize + (COLOR_COUNT - 1) * swGap;
    float startX = screen.x/2.f - totalW/2.f;
    for (int i = 0; i < COLOR_COUNT; ++i) {
        colorSwatchRects[i] = sf::FloatRect(startX + i * (swSize + swGap),
                                             screen.y/2.f - swSize/2.f, swSize, swSize);
    }
    btnColorBackRect = sf::FloatRect(screen.x/2.f - 100.f, screen.y * 0.78f, 200.f, 60.f);

    float toolW = 120.f, toolH = 50.f, toolGap = 10.f;
    float toolY = 14.f;
    float tx = 16.f;
    btnToolWallRect  = sf::FloatRect(tx, toolY, toolW, toolH); tx += toolW + toolGap;
    btnToolStartRect = sf::FloatRect(tx, toolY, toolW, toolH); tx += toolW + toolGap;
    btnToolGoalRect  = sf::FloatRect(tx, toolY, toolW, toolH); tx += toolW + toolGap;
    btnToolEraseRect = sf::FloatRect(tx, toolY, toolW, toolH); tx += toolW + toolGap;
    btnToolItemRect  = sf::FloatRect(tx, toolY, toolW, toolH);

    float pw = 110.f, ph = 40.f, py = toolY + toolH + 8.f;
    float px = 16.f;
    btnItemCoinRect       = sf::FloatRect(px, py, pw, ph); px += pw + toolGap;
    btnItemTrapRect       = sf::FloatRect(px, py, pw, ph); px += pw + toolGap;
    btnItemBoostRect      = sf::FloatRect(px, py, pw, ph); px += pw + toolGap;
    btnItemSpinBoardsRect = sf::FloatRect(px, py, pw, ph); px += pw + toolGap;
    btnItemBlankRect      = sf::FloatRect(px, py, pw, ph);

    float rbw = 130.f, rbh = 50.f;
    float rx = screen.x - 16.f - rbw;
    btnEditBackRect  = sf::FloatRect(rx, toolY, rbw, rbh); rx -= rbw + toolGap;
    btnEditClearRect = sf::FloatRect(rx, toolY, rbw, rbh); rx -= rbw + toolGap;
    btnEditPlayRect  = sf::FloatRect(rx, toolY, rbw, rbh); rx -= rbw + toolGap;
    btnEditSaveRect  = sf::FloatRect(rx, toolY, rbw, rbh);

    btnScriptApplyRect  = sf::FloatRect(screen.x/2.f - 330.f, screen.y - 80.f, 200.f, 56.f);
    btnScriptCancelRect = sf::FloatRect(screen.x/2.f - 100.f, screen.y - 80.f, 200.f, 56.f);
    btnScriptPasteRect  = sf::FloatRect(screen.x/2.f + 130.f, screen.y - 80.f, 200.f, 56.f);

    state = State::Menu;
}

void Game::setupPhysicsForLevel() {
    sf::Vector2f screen = {(float)window.getSize().x, (float)window.getSize().y};
    physicsWorld.reset(Ball::GRAVITY);
    physicsWorld.addScreenBounds(screen);
    level.createBodies(physicsWorld);
    ball.createBody(physicsWorld, level.ballStart);

    ball1Reached = false;
    ball2Reached = false;
    if (isMultiplayer) {
        sf::Vector2f start2 = level.ballStart + sf::Vector2f(Ball::RADIUS * 2.5f, 0.f);
        ball2.createBody(physicsWorld, start2);
    }
}

void Game::startNewGame(bool multiplayer) {
    stageNum    = 1;
    isMultiplayer = multiplayer;
    state       = State::Playing;
    returnStateAfterPlay = State::Menu;
    stateTimer  = 0.f;
    totalPushes = 0;
    sf::Vector2f screen = {(float)window.getSize().x, (float)window.getSize().y};
    level = generateRandomLevel(screen, stageNum, &scriptEngine);
    setupPhysicsForLevel();
}

void Game::nextLevel() {
    stageNum++;
    state       = State::Playing;
    stateTimer  = 0.f;
    sf::Vector2f screen = {(float)window.getSize().x, (float)window.getSize().y};
    level = generateRandomLevel(screen, stageNum, &scriptEngine);
    setupPhysicsForLevel();
}

void Game::startNetworkHost() {
    isNetworked   = true;
    isNetworkHost = true;
    isMultiplayer = true;
    netStatusTimer = 0.f;
    if (!netSession.startHosting()) {
        // Не удалось занять порт — возвращаемся в меню (например, порт уже
        // занят другим запущенным экземпляром игры на этой же машине).
        isNetworked = false;
        state = State::Menu;
        return;
    }
    state = State::NetHostWaiting;
}

void Game::startNetworkClient() {
    isNetworked   = true;
    isNetworkHost = false;
    isMultiplayer = true;
    netStatusTimer = 0.f;
    if (!netSession.startSearching()) {
        isNetworked = false;
        state = State::Menu;
        return;
    }
    state = State::NetClientSearching;
}

void Game::updateNetHostWaiting() {
    if (netSession.pollHostWaiting()) {
        stageNum    = 1;
        state       = State::Playing;
        returnStateAfterPlay = State::Menu;
        stateTimer  = 0.f;
        totalPushes = 0;
        sf::Vector2f screen = {(float)window.getSize().x, (float)window.getSize().y};
        level = generateRandomLevel(screen, stageNum);
        setupPhysicsForLevel();

        NetLevelInfo info;
        buildNetLevelInfo(info);
        netSession.sendLevelInfo(info);

        lastClientInput = {0.f, 0.f, false};
    }
}

void Game::updateNetClientSearching() {
    if (netSession.pollClientSearching()) {
        level = Level{};
        level.name = "Waiting for level...";
        state = State::Playing;
        returnStateAfterPlay = State::Menu;
        stateTimer = 0.f;
        totalPushes = 0;
        ball1Reached = false;
        ball2Reached = false;
    }
}

void Game::buildNetLevelInfo(NetLevelInfo& outInfo) {
    outInfo.name = level.name;
    outInfo.ballStartX = level.ballStart.x;
    outInfo.ballStartY = level.ballStart.y;
    outInfo.goalX = level.goal.x;
    outInfo.goalY = level.goal.y;
    outInfo.goalRadius = level.goalRadius;

    kinematicWallIndices.clear();
    outInfo.walls.clear();
    for (int i = 0; i < (int)level.walls.size(); ++i) {
        const Wall& w = level.walls[i];
        NetWallInfo nw;
        nw.x = w.rect.left; nw.y = w.rect.top;
        nw.w = w.rect.width; nw.h = w.rect.height;
        nw.r = w.color.r; nw.g = w.color.g; nw.b = w.color.b;
        nw.kinematic = (w.amplitude > 0.f) || w.hasScript;
        outInfo.walls.push_back(nw);
        if (nw.kinematic) kinematicWallIndices.push_back(i);
    }
}

void Game::applyNetLevelInfo(const NetLevelInfo& info) {
    level = Level{};
    level.name = info.name;
    level.ballStart = {info.ballStartX, info.ballStartY};
    level.goal = {info.goalX, info.goalY};
    level.goalRadius = info.goalRadius;

    kinematicWallIndices.clear();
    for (int i = 0; i < (int)info.walls.size(); ++i) {
        const NetWallInfo& nw = info.walls[i];
        level.walls.emplace_back(nw.x, nw.y, nw.w, nw.h,
                                  sf::Color(nw.r, nw.g, nw.b));
        if (nw.kinematic) {
            level.walls.back().amplitude = 1.f;
            kinematicWallIndices.push_back(i);
        }
    }

    ball.pos  = level.ballStart;
    ball2.pos = level.ballStart + sf::Vector2f(Ball::RADIUS * 2.5f, 0.f);
    ball1Reached = false;
    ball2Reached = false;
}

void Game::updateNetworkHostPlaying(float dt) {
    NetInputState in;
    while (netSession.receiveInput(in)) lastClientInput = in; // выбираем самый свежий из накопившихся

    level.update(dt, &scriptEngine);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};
    bool pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    if (pressed && !wasPressed) {
        sf::Vector2f diff = ball.pos - mouse;
        if (vec2len(diff) < Ball::RADIUS * 3.5f) totalPushes++;
    }

    ball.tryPush(mouse, prevMouse, pressed, wasPressed);

    static bool clientWasPressed = false;
    sf::Vector2f clientMouse(lastClientInput.mouseX, lastClientInput.mouseY);
    ball2.tryPush(clientMouse, clientMouse, lastClientInput.pressed, clientWasPressed);
    clientWasPressed = lastClientInput.pressed;

    wasPressed = pressed;
    prevMouse  = mouse;

    physicsWorld.step(dt);
    ball.syncFromBody();
    ball2.syncFromBody();
    level.syncFromBodies();

    if (!ball1Reached && vec2len(ball.pos - level.goal) < level.goalRadius + Ball::RADIUS * 0.5f)
        ball1Reached = true;
    if (!ball2Reached && vec2len(ball2.pos - level.goal) < level.goalRadius + Ball::RADIUS * 0.5f)
        ball2Reached = true;

    if (ball1Reached && ball2Reached && state != State::Win) {
        state = State::Win;
        stateTimer = 2.0f;
    }

    NetFrameState out;
    out.ball1X = ball.pos.x;  out.ball1Y = ball.pos.y;
    out.ball2X = ball2.pos.x; out.ball2Y = ball2.pos.y;
    out.ball1Reached = ball1Reached;
    out.ball2Reached = ball2Reached;
    out.won = (state == State::Win);
    for (int idx : kinematicWallIndices)
        out.kinematicWallPositions.push_back({level.walls[idx].rect.left, level.walls[idx].rect.top});
    netSession.sendFrameState(out);
}

void Game::updateNetworkClientPlaying(float dt) {
    NetLevelInfo li;
    if (level.walls.empty() && level.goalRadius == 0.f) {
        if (netSession.receiveLevelInfo(li)) {
            applyNetLevelInfo(li);
        }
        return;
    }

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};
    bool pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    NetInputState in{mouse.x, mouse.y, pressed};
    netSession.sendInput(in);

    NetFrameState st;
    bool gotAny = false;
    while (netSession.receiveFrameState(st)) gotAny = true;
    if (gotAny) {
        lastNetFrameState = st;
        ball.pos  = {st.ball1X, st.ball1Y};
        ball2.pos = {st.ball2X, st.ball2Y};
        ball1Reached = st.ball1Reached;
        ball2Reached = st.ball2Reached;

        for (size_t i = 0; i < kinematicWallIndices.size() && i < st.kinematicWallPositions.size(); ++i) {
            Wall& w = level.walls[kinematicWallIndices[i]];
            w.rect.left = st.kinematicWallPositions[i].x;
            w.rect.top  = st.kinematicWallPositions[i].y;
        }

        if (st.won && state != State::Win) {
            state = State::Win;
            stateTimer = 2.0f;
        }
    }
}

void Game::renderNetHostWaiting() {
    window.clear(sf::Color(20, 22, 32));
    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, "Hosting Network Game", 42, {sz.x/2.f, sz.y * 0.35f},
             sf::Color(120, 200, 255), true);

    std::string dots(1 + (int)(netStatusTimer * 2.f) % 4, '.');
    drawText(window, font, "Waiting for Player 2 to join" + dots, 24, {sz.x/2.f, sz.y * 0.46f},
             sf::Color(200, 200, 200), true);

    drawText(window, font, "Any player on this LAN can join automatically — no IP needed.",
             18, {sz.x/2.f, sz.y * 0.54f}, sf::Color(140, 140, 140), true);

    drawText(window, font, "Esc - cancel", 18, {sz.x/2.f, sz.y - 40.f},
             sf::Color(140, 140, 140), true);

    window.display();
}

void Game::renderNetClientSearching() {
    window.clear(sf::Color(20, 22, 32));
    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, "Searching for Game", 42, {sz.x/2.f, sz.y * 0.35f},
             sf::Color(255, 170, 90), true);

    std::string dots(1 + (int)(netStatusTimer * 2.f) % 4, '.');
    drawText(window, font, "Looking for a host on your LAN" + dots, 24, {sz.x/2.f, sz.y * 0.46f},
             sf::Color(200, 200, 200), true);

    drawText(window, font, "Make sure the host has started \"Host Network Game\" first.",
             18, {sz.x/2.f, sz.y * 0.54f}, sf::Color(140, 140, 140), true);

    drawText(window, font, "Esc - cancel", 18, {sz.x/2.f, sz.y - 40.f},
             sf::Color(140, 140, 140), true);

    window.display();
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleEvents();
        update(dt);
        render();
    }
}

void Game::handleEvents() {
    sf::Event ev;
    while (window.pollEvent(ev)) {
        if (ev.type == sf::Event::Closed)
            window.close();

        if (ev.type == sf::Event::TextEntered && state == State::ScriptEdit) {
            handleScriptEditorTextEntered(ev.text.unicode);
        }

        if (ev.type == sf::Event::KeyPressed) {
            bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

            if (state == State::ScriptEdit) {
                handleScriptEditorKey(ev.key.code, ctrl);
                continue; 
            }

            if (ev.key.code == sf::Keyboard::Escape) {
                if (state == State::Menu) window.close();
                else if (state == State::ColorPicker) state = State::Menu;
                else if (state == State::MultiplayerMenu) state = State::Menu;
                else if (state == State::Editor) state = State::Menu;
                else if (state == State::NetHostWaiting || state == State::NetClientSearching) {
                    netSession.disconnect();
                    isNetworked = false;
                    isMultiplayer = false;
                    state = State::Menu;
                } else if (state == State::Playing || state == State::Win) {
                    if (isNetworked) {
                        netSession.disconnect();
                        isNetworked = false;
                        isMultiplayer = false;
                    }
                    state = returnStateAfterPlay;
                } else state = State::Menu;
            }
            if (ev.key.code == sf::Keyboard::R && state == State::Playing && !isNetworked) {
                ball.reset(level.ballStart);
                if (isMultiplayer) {
                    sf::Vector2f start2 = level.ballStart + sf::Vector2f(Ball::RADIUS * 2.5f, 0.f);
                    ball2.reset(start2);
                    ball1Reached = false;
                    ball2Reached = false;
                }
            }
            if (ev.key.code == sf::Keyboard::Enter && state == State::Win) {
                if (returnStateAfterPlay == State::Editor)
                    state = State::Editor;
                else
                    nextLevel();
            }
            if (state == State::Editor) {
                handleEditorKey(ev.key.code);
            }
        }

        if (ev.type == sf::Event::MouseButtonPressed) {
            sf::Vector2f mp = {(float)ev.mouseButton.x, (float)ev.mouseButton.y};
            if (state == State::Menu && ev.mouseButton.button == sf::Mouse::Left) {
                handleMenuClick(mp);
            } else if (state == State::MultiplayerMenu && ev.mouseButton.button == sf::Mouse::Left) {
                handleMultiplayerMenuClick(mp);
            } else if (state == State::ColorPicker && ev.mouseButton.button == sf::Mouse::Left) {
                handleColorPickerClick(mp);
            } else if (state == State::Editor) {
                handleEditorMouseDown(mp, ev.mouseButton.button);
            } else if (state == State::ScriptEdit && ev.mouseButton.button == sf::Mouse::Left) {
                if (pointInRect(mp, btnScriptApplyRect)) applyScriptEditorChanges();
                else if (pointInRect(mp, btnScriptCancelRect)) state = State::Editor;
                else if (pointInRect(mp, btnScriptPasteRect)) pasteClipboardIntoScript();
            }
        }

        if (ev.type == sf::Event::MouseButtonReleased &&
            ev.mouseButton.button == sf::Mouse::Left &&
            state == State::Editor) {
            sf::Vector2f mp = {(float)ev.mouseButton.x, (float)ev.mouseButton.y};
            handleEditorMouseUp(mp);
        }
    }
}

void Game::update(float dt) {
    goalPulse += dt * 2.5f;

    if (state == State::Menu || state == State::MultiplayerMenu || state == State::ColorPicker || state == State::Editor || state == State::ScriptEdit) {
        if (state == State::Editor && editStatusTimer > 0.f) {
            editStatusTimer -= dt;
        }
        return;
    }

    if (state == State::NetHostWaiting) {
        netStatusTimer += dt;
        updateNetHostWaiting();
        return;
    }
    if (state == State::NetClientSearching) {
        netStatusTimer += dt;
        updateNetClientSearching();
        return;
    }

    if (state == State::Win) {
        stateTimer -= dt;
        if (stateTimer <= 0.f) {
            if (isNetworked) {
                netSession.disconnect();
                isNetworked = false;
                isMultiplayer = false;
                state = State::Menu;
            } else if (returnStateAfterPlay == State::Editor) {
                state = State::Editor;
            } else {
                nextLevel();
            }
        }
        return;
    }

    // state == Playing
    if (isNetworked) {
        if (isNetworkHost) updateNetworkHostPlaying(dt);
        else updateNetworkClientPlaying(dt);
        return;
    }

    level.update(dt, &scriptEngine); 

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};
    bool pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    // Считаем толчки
    if (pressed && !wasPressed) {
        sf::Vector2f diff = ball.pos - mouse;
        if (vec2len(diff) < Ball::RADIUS * 3.5f)
            totalPushes++;
    }

    ball.tryPush(mouse, prevMouse, pressed, wasPressed);
    if (isMultiplayer)
        ball2.tryPush(mouse, prevMouse, pressed, wasPressed);
    wasPressed = pressed;
    prevMouse  = mouse;

    physicsWorld.step(dt);

    ball.syncFromBody();
    if (isMultiplayer) ball2.syncFromBody();
    level.syncFromBodies();

    for (auto& it : level.items) {
        it.checkTouch(scriptEngine, ball, level.spinners);
        if (isMultiplayer) it.checkTouch(scriptEngine, ball2, level.spinners);
    }

    ball.pushToBody();
    if (isMultiplayer) ball2.pushToBody();

    if (!isMultiplayer) {
        float dist = vec2len(ball.pos - level.goal);
        if (dist < level.goalRadius + Ball::RADIUS * 0.5f) {
            state = State::Win;
            stateTimer = 2.0f;
        }
    } else {
        if (!ball1Reached && vec2len(ball.pos - level.goal) < level.goalRadius + Ball::RADIUS * 0.5f)
            ball1Reached = true;
        if (!ball2Reached && vec2len(ball2.pos - level.goal) < level.goalRadius + Ball::RADIUS * 0.5f)
            ball2Reached = true;

        if (ball1Reached && ball2Reached) {
            state = State::Win;
            stateTimer = 2.0f;
        }
    }
}

void Game::render() {
    if (state == State::Menu) {
        renderMenu();
        return;
    }
    if (state == State::MultiplayerMenu) {
        renderMultiplayerMenu();
        return;
    }
    if (state == State::ColorPicker) {
        renderColorPicker();
        return;
    }
    if (state == State::Editor) {
        renderEditor();
        return;
    }
    if (state == State::ScriptEdit) {
        renderScriptEditor();
        return;
    }
    if (state == State::NetHostWaiting) {
        renderNetHostWaiting();
        return;
    }
    if (state == State::NetClientSearching) {
        renderNetClientSearching();
        return;
    }
    if (state == State::Playing && isNetworked && !isNetworkHost && level.walls.empty() && level.goalRadius == 0.f) {
        renderNetClientSearching();
        return;
    }

    window.clear(sf::Color(20, 22, 32));

    for (const auto& wall : level.walls)
        wall.draw(window);

    for (const auto& it : level.items)
        it.draw(window);

    // Декоративные вращающиеся доски (спавнятся из Lua-скриптов)
    level.drawSpinners(window);

    drawGoal();
    ball.draw(window);
    if (isMultiplayer) ball2.draw(window);
    drawHUD();

    if (state == State::Win)
        drawOverlay("Level Complete!", "Next level in a moment... (Enter)");

    window.display();
}
