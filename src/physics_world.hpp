#pragma once
#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <memory>

// Box2D работает в метрах и ожидает "разумные" размеры объектов (~0.1-10 м);
// наши уровни оперируют пикселями (сотни-тысячи), поэтому все размеры/позиции
// переводятся через PIXELS_PER_METER перед передачей в Box2D и обратно при
// чтении результатов симуляции.
constexpr float PIXELS_PER_METER = 30.f;

inline float pxToM(float px) { return px / PIXELS_PER_METER; }
inline float mToPx(float m)  { return m * PIXELS_PER_METER; }
inline b2Vec2 pxToM(sf::Vector2f px) { return b2Vec2(pxToM(px.x), pxToM(px.y)); }
inline sf::Vector2f mToPx(b2Vec2 m)  { return sf::Vector2f(mToPx(m.x), mToPx(m.y)); }

// Обёртка над b2World: пересоздаётся при каждой загрузке нового уровня
// (проще и надёжнее, чем пытаться инкрементально удалять/добавлять тела).
class PhysicsWorld {
public:
    PhysicsWorld();

    // Полностью пересоздаёт мир (удаляет все тела). Вызывать при смене уровня.
    // gravityPxPerS2 — ускорение свободного падения в пикселях/сек² (как в
    // старой ручной физике), конвертируется в м/с² для Box2D.
    void reset(float gravityPxPerS2);

    void step(float dt);

    b2World& world() { return *world_; }

    // Четыре тонкие статичные стены по границам экрана, чтобы шар не улетал
    // за пределы окна (аналог ручных проверок границ в старой реализации).
    void addScreenBounds(sf::Vector2f screenSize);

private:
    std::unique_ptr<b2World> world_;
};
