#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

class PhysicsWorld;

class Ball {
public:
    static constexpr float RADIUS      = 22.f;
    static constexpr float GRAVITY     = 700.f;   // px/s², передаётся в PhysicsWorld как гравитация мира
    static constexpr float RESTITUTION = 0.65f;
    static constexpr float FRICTION    = 0.15f;   // трение о поверхности (Box2D fixture friction)
    static constexpr float LINEAR_DAMPING = 0.12f; // аналог прежнего FRICTION=0.988 (сопротивление воздуха)
    static constexpr float PUSH_CLICK  = 550.f;
    static constexpr float PUSH_DRAG   = 9.f;

    // Кэш позиции/скорости, синхронизируется с Box2D телом каждый кадр через
    // syncFromBody(). Публичные поля сохранены ради совместимости с Item/Lua
    // скриптами, которые читают/пишут ball.pos и ball.vel напрямую.
    sf::Vector2f pos;
    sf::Vector2f vel;

    Ball();

    // Создаёт Box2D тело шара в указанном мире на позиции startPos (пиксели).
    // Вызывать при каждой загрузке уровня (мир пересоздаётся, старое тело невалидно).
    void createBody(PhysicsWorld& world, sf::Vector2f startPos);

    // Телепортирует шар на startPos и обнуляет скорость (используется по R и при рестарте).
    void reset(sf::Vector2f startPos);

    // Считывает текущие позицию/скорость из Box2D тела в pos/vel (вызывать после world.step()).
    void syncFromBody();

    // Записывает pos/vel обратно в тело (используется после того как Lua-скрипт
    // предмета изменил ball.vel в on_touch — иначе изменение потеряется на
    // следующем шаге симуляции, т.к. Box2D — источник истины для скорости).
    void pushToBody();

    void tryPush(sf::Vector2f mouse, sf::Vector2f prevMouse, bool pressed, bool wasPressed);
    void draw(sf::RenderWindow& window) const;
    void setColor(sf::Color col);

    b2Body* body = nullptr; // не владеющий указатель; жизненный цикл управляется b2World

private:
    sf::CircleShape shape;
    sf::CircleShape highlight;
    sf::CircleShape shadowShape;
};
