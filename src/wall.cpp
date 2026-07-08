#include "wall.hpp"
#include "script_engine.hpp"
#include "physics_world.hpp"
#include <algorithm>
#include <cmath>

Wall::Wall(float x, float y, float w, float h, sf::Color col)
    : rect(x, y, w, h), color(col), startPos(x, y) {}

void Wall::makeMoving(sf::Vector2f moveAxis, float moveAmplitude, float moveSpeed) {
    axis      = moveAxis;
    amplitude = moveAmplitude;
    speed     = moveSpeed;
    phase     = 0.f;
}

static sol::table makeWallProxy(sol::state& lua, Wall& wall) {
    sol::table t = lua.create_table();
    sol::table mt = lua.create_table();
    mt.set_function("__index", [&lua, &wall](sol::table, const std::string& key) -> sol::object {
        if (key == "x") return sol::make_object(lua, wall.rect.left);
        if (key == "y") return sol::make_object(lua, wall.rect.top);
        if (key == "start_x") return sol::make_object(lua, wall.startPos.x);
        if (key == "start_y") return sol::make_object(lua, wall.startPos.y);
        if (key == "w") return sol::make_object(lua, wall.rect.width);
        if (key == "h") return sol::make_object(lua, wall.rect.height);
        if (key == "t") return sol::make_object(lua, wall.scriptTime);
        if (key == "color") return sol::make_object(lua, wall.color);
        return sol::nil;
    });
    mt.set_function("__newindex", [&wall](sol::table, const std::string& key, sol::object val) {
        if (key == "x") wall.rect.left = val.as<float>();
        else if (key == "y") wall.rect.top = val.as<float>();
        else if (key == "color") wall.color = val.as<sf::Color>();
    });
    t[sol::metatable_key] = mt;
    return t;
}

void Wall::loadScript(ScriptEngine& engine, const std::string& source, const std::string& chunkName) {
    scriptSource = source;
    scriptName   = chunkName;
    hasScript    = true;
    scriptTime   = 0.f;

    scriptOk = engine.loadString(source, chunkName, scriptEnv);
    if (!scriptOk) {
        lastError = engine.lastError();
        return;
    }

    // Прокси не кешируется здесь — Wall обычно лежит в std::vector<Wall> и
    // копируется/переаллоцируется при push_back, поэтому захваченный по
    // ссылке адрес "this" быстро становится невалидным. on_create безопасно
    // вызвать сейчас (объект ещё на стабильном адресе), on_update создаёт
    // свежий прокси перед каждым вызовом (см. update()).
    sol::table proxy = makeWallProxy(engine.lua(), *this);

    sol::protected_function onCreate = scriptEnv["on_create"];
    if (onCreate.valid()) {
        sol::protected_function_result r = onCreate(proxy);
        if (!r.valid()) {
            sol::error err = r;
            lastError = err.what();
            scriptOk = false;
        }
    }
}

void Wall::createBody(PhysicsWorld& world) {
    bool kinematic = hasScript || amplitude > 0.f;

    b2BodyDef bd;
    bd.type = kinematic ? b2_kinematicBody : b2_staticBody;
    sf::Vector2f center(rect.left + rect.width / 2.f, rect.top + rect.height / 2.f);
    bd.position = pxToM(center);
    body = world.world().CreateBody(&bd);

    b2PolygonShape box;
    box.SetAsBox(pxToM(rect.width / 2.f), pxToM(rect.height / 2.f));

    b2FixtureDef fd;
    fd.shape = &box;
    fd.friction = 0.4f;
    fd.restitution = 0.0f; // упругость самого шара уже даёт нужный отскок
    body->CreateFixture(&fd);
}

void Wall::update(float dt, ScriptEngine* engine) {
    if (!body) return;

    sf::Vector2f before(rect.left, rect.top);
    bool moved = false;

    // Lua-скрипт имеет приоритет над встроенным синусом
    if (hasScript && scriptOk && engine) {
        scriptTime += dt;
        sol::protected_function onUpdate = scriptEnv["on_update"];
        if (onUpdate.valid()) {
            sol::table proxy = makeWallProxy(engine->lua(), *this);
            sol::protected_function_result r = onUpdate(proxy, dt);
            if (!r.valid()) {
                sol::error err = r;
                lastError = err.what();
            } else {
                moved = true;
            }
        }
    } else if (amplitude > 0.f) {
        phase += speed * dt;
        float offset = std::sin(phase) * amplitude;
        sf::Vector2f newPos = startPos + axis * offset;
        rect.left = newPos.x;
        rect.top  = newPos.y;
        moved = true;
    }

    if (!moved) return;

    // Разница между желаемой (только что вычисленной в rect) и текущей
    // позицией тела определяет скорость, которую Box2D применит к
    // кинематическому телу при следующем world.step() — сама интеграция
    // позиции происходит внутри Box2D, а не вручную здесь.
    sf::Vector2f after(rect.left, rect.top);
    velocity = (dt > 0.f) ? (after - before) / dt : sf::Vector2f{0.f, 0.f};

    if (body->GetType() == b2_kinematicBody) {
        body->SetLinearVelocity(pxToM(velocity));
    }
}

void Wall::syncFromBody() {
    if (!body || body->GetType() != b2_kinematicBody) return;
    sf::Vector2f center = mToPx(body->GetPosition());
    rect.left = center.x - rect.width / 2.f;
    rect.top  = center.y - rect.height / 2.f;
}

void Wall::draw(sf::RenderWindow& window) const {
    sf::RectangleShape rs({rect.width, rect.height});
    rs.setPosition(rect.left, rect.top);
    rs.setFillColor(color);
    rs.setOutlineThickness(2.f);
    if (hasScript)
        rs.setOutlineColor(sf::Color(120, 230, 255, 220)); // Lua-стены — голубая обводка
    else if (amplitude > 0.f)
        rs.setOutlineColor(sf::Color(255, 255, 255, 200)); // встроенные движущиеся платформы — белая
    else
        rs.setOutlineColor(sf::Color(color.r / 2, color.g / 2, color.b / 2));
    window.draw(rs);
}
