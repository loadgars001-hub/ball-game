#include "game.hpp"
#include "game_utils.hpp"
#include <cmath>

void Game::drawGoal() {
    float pulse = level.goalRadius * 0.25f * std::sin(goalPulse);

    sf::CircleShape ring(level.goalRadius + pulse);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(level.goal);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(3.f);
    ring.setOutlineColor(sf::Color(255, 210, 0, 180));
    window.draw(ring);

    for (int i = 0; i < 5; ++i) {
        float angle = i * 72.f * 3.14159f / 180.f + goalPulse * 0.4f;
        float r = level.goalRadius * 0.55f;
        sf::CircleShape dot(5.f);
        dot.setOrigin(5.f, 5.f);
        dot.setFillColor(sf::Color(255, 220, 50));
        dot.setPosition(level.goal.x + r * std::cos(angle), level.goal.y + r * std::sin(angle));
        window.draw(dot);
    }

    sf::CircleShape center(level.goalRadius * 0.4f);
    center.setOrigin(center.getRadius(), center.getRadius());
    center.setPosition(level.goal);
    center.setFillColor(sf::Color(255, 200, 0, 200));
    window.draw(center);
}

void Game::drawHUD() {
    if (!fontOk) return;
    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    drawText(window, font, level.name, 22, {16.f, 10.f}, sf::Color(220,220,220));
    // Толчки справа
    drawText(window, font, "Pushes: " + std::to_string(totalPushes), 20,
             {sz.x - 160.f, 10.f}, sf::Color(180,220,255));

    if (isMultiplayer) {
        std::string p1 = ball1Reached ? "P1: at goal" : "P1: on the way";
        std::string p2 = ball2Reached ? "P2: at goal" : "P2: on the way";
        drawText(window, font, p1, 18, {sz.x/2.f - 160.f, 10.f},
                 ball1Reached ? sf::Color(120, 230, 120) : sf::Color(70, 140, 255), true);
        drawText(window, font, p2, 18, {sz.x/2.f + 160.f, 10.f},
                 ball2Reached ? sf::Color(120, 230, 120) : sf::Color(255, 140, 30), true);
    }

    drawText(window, font, "LMB - push   R - reset   Esc - menu",
             17, {sz.x/2.f, sz.y - 28.f}, sf::Color(140,140,140), true);
}

void Game::drawOverlay(const std::string& title, const std::string& sub) {
    if (!fontOk) return;
    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    sf::RectangleShape bg(sz);
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(bg);

    drawText(window, font, title, 52, {sz.x/2.f, sz.y/2.f - 40.f},
             sf::Color(255, 220, 60), true);
    drawText(window, font, sub, 24, {sz.x/2.f, sz.y/2.f + 28.f},
             sf::Color(200, 200, 200), true);
}
