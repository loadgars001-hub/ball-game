#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// Декоративная вращающаяся мигающая "доска". Не участвует в физике Box2D
// (это чисто визуальный эффект) — появляется по вызову из Lua-скрипта
// (item:spawn_board(x, y)) и исчезает сама через некоторое время.
struct Spinner {
    sf::Vector2f pos;
    float width  = 60.f;
    float height = 14.f;

    float rotation      = 0.f;  // текущий угол, градусы
    float rotationSpeed = 0.f;  // градусы/сек

    std::vector<sf::Color> colors; // палитра для мигания
    int   colorIndex    = 0;
    float blinkTimer    = 0.f;
    float blinkInterval = 0.12f; // как часто переключать цвет

    float lifetime = 4.f; // секунд до исчезновения; <= 0 значит "удалить в этом кадре"

    void update(float dt) {
        rotation += rotationSpeed * dt;
        blinkTimer += dt;
        if (blinkTimer >= blinkInterval && !colors.empty()) {
            blinkTimer = 0.f;
            colorIndex = (colorIndex + 1) % (int)colors.size();
        }
        lifetime -= dt;
    }

    bool expired() const { return lifetime <= 0.f; }

    void draw(sf::RenderWindow& window) const {
        sf::RectangleShape rs({width, height});
        rs.setOrigin(width / 2.f, height / 2.f);
        rs.setPosition(pos);
        rs.setRotation(rotation);
        rs.setFillColor(colors.empty() ? sf::Color::White : colors[colorIndex]);
        rs.setOutlineThickness(2.f);
        rs.setOutlineColor(sf::Color(255, 255, 255, 160));
        window.draw(rs);
    }
};

// Создаёт спиннер со случайными (в разумных пределах) скоростью вращения,
// размером и яркой мигающей палитрой — используется C++-биндингом
// item:spawn_board(x, y), чтобы Lua-скрипту не нужно было настраивать
// вращение/цвета вручную.
Spinner makeRandomSpinner(sf::Vector2f pos);
