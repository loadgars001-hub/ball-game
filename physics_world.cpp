#include "physics_world.hpp"

PhysicsWorld::PhysicsWorld() {
    world_ = std::make_unique<b2World>(b2Vec2(0.f, 0.f));
}

void PhysicsWorld::reset(float gravityPxPerS2) {
    world_ = std::make_unique<b2World>(b2Vec2(0.f, pxToM(gravityPxPerS2)));
}

void PhysicsWorld::step(float dt) {
    // Box2D рекомендует фиксированный шаг и 8/3 итераций скорости/позиции
    // для стабильной симуляции; при dt=0 (пауза/меню) просто не шагаем.
    if (dt <= 0.f) return;
    world_->Step(dt, 8, 3);
}

void PhysicsWorld::addScreenBounds(sf::Vector2f screenSize) {
    float w = pxToM(screenSize.x);
    float h = pxToM(screenSize.y);
    float thickness = 0.2f; // ~6px, достаточно чтобы шар не проскакивал сквозь тонкую границу

    auto makeEdgeBody = [&](float cx, float cy, float hw, float hh) {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position.Set(cx, cy);
        b2Body* body = world_->CreateBody(&bd);
        b2PolygonShape box;
        box.SetAsBox(hw, hh);
        b2FixtureDef fd;
        fd.shape = &box;
        fd.friction = 0.3f;
        fd.restitution = 0.0f; // отскок от границ экрана уже даёт restitution самого шара
        body->CreateFixture(&fd);
    };

    // низ, верх, лево, право — каждая стена чуть шире экрана, чтобы перекрыть углы
    makeEdgeBody(w / 2.f, h + thickness / 2.f, w / 2.f + thickness, thickness / 2.f); // низ
    makeEdgeBody(w / 2.f, -thickness / 2.f,    w / 2.f + thickness, thickness / 2.f); // верх
    makeEdgeBody(-thickness / 2.f, h / 2.f,    thickness / 2.f, h / 2.f + thickness); // лево
    makeEdgeBody(w + thickness / 2.f, h / 2.f, thickness / 2.f, h / 2.f + thickness); // право
}
