#include "ball.hpp"
#include "physics_world.hpp"
#include <cmath>

static float vec2len(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}
static sf::Vector2f vec2norm(sf::Vector2f v) {
    float l = vec2len(v);
    return l > 0.001f ? sf::Vector2f{v.x / l, v.y / l} : sf::Vector2f{0.f, 0.f};
}

Ball::Ball() {
    shape.setRadius(RADIUS);
    shape.setOrigin(RADIUS, RADIUS);
    shape.setFillColor(sf::Color(70, 140, 255));
    shape.setOutlineThickness(3.f);
    shape.setOutlineColor(sf::Color(30, 60, 160));

    highlight.setRadius(RADIUS * 0.28f);
    highlight.setOrigin(RADIUS * 0.28f, RADIUS * 0.28f);
    highlight.setFillColor(sf::Color(255, 255, 255, 130));

    shadowShape.setRadius(RADIUS * 0.75f);
    shadowShape.setOrigin(RADIUS * 0.75f, RADIUS * 0.4f);
    shadowShape.setFillColor(sf::Color(0, 0, 0, 50));
    shadowShape.setScale(1.f, 0.35f);
}

void Ball::createBody(PhysicsWorld& world, sf::Vector2f startPos) {
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position = pxToM(startPos);
    bd.linearDamping = LINEAR_DAMPING;
    bd.bullet = true;
    body = world.world().CreateBody(&bd);

    b2CircleShape circle;
    circle.m_radius = pxToM(RADIUS);

    b2FixtureDef fd;
    fd.shape = &circle;
    fd.density = 1.0f;
    fd.friction = FRICTION;
    fd.restitution = RESTITUTION;
    body->CreateFixture(&fd);

    pos = startPos;
    vel = {0.f, 0.f};
}

void Ball::reset(sf::Vector2f startPos) {
    pos = startPos;
    vel = {0.f, 0.f};
    if (body) {
        body->SetTransform(pxToM(startPos), 0.f);
        body->SetLinearVelocity(b2Vec2(0.f, 0.f));
        body->SetAwake(true);
    }
}

void Ball::syncFromBody() {
    if (!body) return;
    pos = mToPx(body->GetPosition());
    b2Vec2 v = body->GetLinearVelocity();
    vel = mToPx(v);
}

void Ball::pushToBody() {
    if (!body) return;
    body->SetLinearVelocity(pxToM(vel));
}

void Ball::setColor(sf::Color col) {
    shape.setFillColor(col);
    shape.setOutlineColor(sf::Color(col.r / 2, col.g / 2, col.b / 2));
}

void Ball::tryPush(sf::Vector2f mouse, sf::Vector2f prevMouse, bool pressed, bool wasPressed) {
    if (!body) return;

    // Клик — импульс от курсора
    if (pressed && !wasPressed) {
        sf::Vector2f diff = pos - mouse;
        float dist = vec2len(diff);
        if (dist < RADIUS * 3.5f && dist > 0.1f) {
            float force = PUSH_CLICK * (1.f - dist / (RADIUS * 3.5f));
            sf::Vector2f impulseVelPx = vec2norm(diff) * force;
            body->ApplyLinearImpulseToCenter(body->GetMass() * pxToM(impulseVelPx), true);
        }
    }
    if (pressed && wasPressed) {
        sf::Vector2f diff = pos - mouse;
        float dist = vec2len(diff);
        if (dist < RADIUS * 2.5f && dist > 0.1f) {
            sf::Vector2f delta = mouse - prevMouse;
            if (vec2len(delta) > 0.5f) {
                sf::Vector2f impulseVelPx = delta * PUSH_DRAG;
                body->ApplyLinearImpulseToCenter(body->GetMass() * pxToM(impulseVelPx), true);
            }
        }
    }
}

void Ball::draw(sf::RenderWindow& window) const {
    sf::CircleShape sh = shadowShape;
    sh.setPosition(pos.x, window.getSize().y - 10.f);
    float sc = 0.3f + 0.7f * (pos.y / (float)window.getSize().y);
    sh.setScale(sc, 0.35f * sc);
    window.draw(sh);

    sf::CircleShape s = shape;
    s.setPosition(pos);
    window.draw(s);

    sf::CircleShape h = highlight;
    h.setPosition(pos.x - RADIUS * 0.28f, pos.y - RADIUS * 0.33f);
    window.draw(h);
}
