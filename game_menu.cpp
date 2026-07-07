#include "game.hpp"
#include "game_utils.hpp"

void Game::handleMenuClick(sf::Vector2f mouse) {
    if (pointInRect(mouse, btnStartRect)) {
        startNewGame();
    } else if (pointInRect(mouse, btnEditorRect)) {
        enterEditor();
    } else if (pointInRect(mouse, btnMultiplayerRect)) {
        state = State::MultiplayerMenu;
    } else if (pointInRect(mouse, btnColorRect)) {
        state = State::ColorPicker;
    } else if (pointInRect(mouse, btnExitRect)) {
        window.close();
    }
}

void Game::handleMultiplayerMenuClick(sf::Vector2f mouse) {
    if (pointInRect(mouse, btnMultiplayerRect)) {
        startNewGame(true); // Same Screen — локальный кооп-мультиплеер, общая мышь
    } else if (pointInRect(mouse, btnHostNetRect)) {
        startNetworkHost();
    } else if (pointInRect(mouse, btnJoinNetRect)) {
        startNetworkClient();
    } else if (pointInRect(mouse, btnMpBackRect)) {
        state = State::Menu;
    }
}

void Game::handleColorPickerClick(sf::Vector2f mouse) {
    for (int i = 0; i < COLOR_COUNT; ++i) {
        if (pointInRect(mouse, colorSwatchRects[i])) {
            selectedColor = i;
            ball.setColor(ballColors[selectedColor]);
            return;
        }
    }
    if (pointInRect(mouse, btnColorBackRect)) {
        state = State::Menu;
    }
}

void Game::renderMenu() {
    window.clear(sf::Color(20, 22, 32));

    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, "BALL", 72, {sz.x/2.f, sz.y/2.f - 180.f},
             sf::Color(70, 140, 255), true);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};

    auto drawButton = [&](const sf::FloatRect& r, const std::string& label, sf::Color base) {
        bool hover = pointInRect(mouse, r);
        sf::RectangleShape rect({r.width, r.height});
        rect.setPosition(r.left, r.top);
        rect.setFillColor(hover ? sf::Color(base.r+30, base.g+30, base.b+30) : base);
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(sf::Color(255,255,255,120));
        window.draw(rect);
        if (fontOk)
            drawText(window, font, label, 28,
                     {r.left + r.width/2.f, r.top + r.height/2.f - 16.f},
                     sf::Color::White, true);
    };

    drawButton(btnStartRect, "Start", sf::Color(40, 120, 60));
    drawButton(btnEditorRect, "Editor", sf::Color(120, 100, 30));
    drawButton(btnMultiplayerRect, "Local Multiplayer", sf::Color(30, 100, 130));
    drawButton(btnColorRect, "Ball Color", sf::Color(60, 80, 150));
    drawButton(btnExitRect,  "Exit", sf::Color(140, 40, 40));

    window.display();
}

void Game::renderMultiplayerMenu() {
    window.clear(sf::Color(20, 22, 32));

    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, "Local Multiplayer", 48, {sz.x/2.f, sz.y/2.f - 160.f},
             sf::Color(120, 200, 255), true);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};

    auto drawButton = [&](const sf::FloatRect& r, const std::string& label, sf::Color base) {
        bool hover = pointInRect(mouse, r);
        sf::RectangleShape rect({r.width, r.height});
        rect.setPosition(r.left, r.top);
        rect.setFillColor(hover ? sf::Color(base.r+30, base.g+30, base.b+30) : base);
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(sf::Color(255,255,255,120));
        window.draw(rect);
        if (fontOk)
            drawText(window, font, label, 26,
                     {r.left + r.width/2.f, r.top + r.height/2.f - 15.f},
                     sf::Color::White, true);
    };

    // btnMultiplayerRect переиспользуется здесь как "Same Screen" — та же
    // геометрия кнопки, что и на главном меню, но другой смысл на этом экране.
    drawButton(btnMultiplayerRect, "Same Screen (shared mouse)", sf::Color(30, 100, 130));
    drawButton(btnHostNetRect, "Host Network Game", sf::Color(20, 110, 90));
    drawButton(btnJoinNetRect, "Join Network Game", sf::Color(150, 90, 20));
    drawButton(btnMpBackRect, "Back", sf::Color(70, 70, 80));

    window.display();
}

void Game::renderColorPicker() {
    window.clear(sf::Color(20, 22, 32));

    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, "Choose Ball Color", 48, {sz.x/2.f, sz.y * 0.22f},
             sf::Color(220, 220, 220), true);

    // Предпросмотр текущего выбранного цвета — большой шар по центру
    sf::CircleShape preview(60.f);
    preview.setOrigin(60.f, 60.f);
    preview.setPosition(sz.x/2.f, sz.y * 0.40f);
    preview.setFillColor(ballColors[selectedColor]);
    preview.setOutlineThickness(4.f);
    sf::Color sel = ballColors[selectedColor];
    preview.setOutlineColor(sf::Color(sel.r/2, sel.g/2, sel.b/2));
    window.draw(preview);

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};

    // Свотчи 5 цветов
    for (int i = 0; i < COLOR_COUNT; ++i) {
        const sf::FloatRect& r = colorSwatchRects[i];
        bool hover = pointInRect(mouse, r);
        bool isSel = (i == selectedColor);

        sf::CircleShape sw(r.width / 2.f);
        sw.setOrigin(r.width / 2.f, r.width / 2.f);
        sw.setPosition(r.left + r.width / 2.f, r.top + r.height / 2.f);
        sw.setFillColor(ballColors[i]);
        sw.setOutlineThickness(isSel ? 6.f : (hover ? 4.f : 2.f));
        sw.setOutlineColor(isSel ? sf::Color::White : sf::Color(255,255,255, hover ? 180 : 90));
        window.draw(sw);
    }

    // Кнопка "назад"
    bool backHover = pointInRect(mouse, btnColorBackRect);
    sf::RectangleShape backRect({btnColorBackRect.width, btnColorBackRect.height});
    backRect.setPosition(btnColorBackRect.left, btnColorBackRect.top);
    backRect.setFillColor(backHover ? sf::Color(90, 90, 100) : sf::Color(70, 70, 80));
    backRect.setOutlineThickness(2.f);
    backRect.setOutlineColor(sf::Color(255,255,255,120));
    window.draw(backRect);
    drawText(window, font, "Back", 26,
             {btnColorBackRect.left + btnColorBackRect.width/2.f,
              btnColorBackRect.top + btnColorBackRect.height/2.f - 15.f},
             sf::Color::White, true);

    window.display();
}
