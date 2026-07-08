#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>

// Общие мелкие хелперы для отрисовки/геометрии, используемые во всех
// game_*.cpp файлах. inline — чтобы не плодить нарушения ODR при подключении
// в несколько единиц трансляции.

inline float vec2len(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }

inline void drawText(sf::RenderWindow& w, const sf::Font& f,
                      const std::string& str, unsigned sz,
                      sf::Vector2f pos, sf::Color col,
                      bool center = false) {
    sf::Text t(str, f, sz);
    t.setFillColor(col);
    if (center) {
        // ВАЖНО: getLocalBounds() у sf::Text не начинается в (0,0) — левый/
        // верхний край (left/top) обычно ненулевой из-за внутренних отступов
        // шрифта (особенно заметно на крупном жирном тексте). Если это не
        // учитывать, setOrigin(width/2, height/2) центрирует неточно, и текст
        // выглядит визуально смещённым/"кривым" — именно это было с "BALL".
        auto b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    }
    t.setPosition(pos);
    w.draw(t);
}

inline bool pointInRect(sf::Vector2f p, const sf::FloatRect& r) {
    return p.x >= r.left && p.x <= r.left + r.width &&
           p.y >= r.top  && p.y <= r.top  + r.height;
}
