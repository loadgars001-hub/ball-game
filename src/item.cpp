#include "item.hpp"
#include "script_engine.hpp"
#include "ball.hpp"
#include "spinner.hpp"
#include <cmath>
#include <iostream>

static float vec2len(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// Прокси-таблица "item", которую видит Lua-скрипт. Хранит ссылку на сам Item
// через upvalue (замыкание), чтобы изменения из Lua сразу отражались в C++ объекте.
// spinners (может быть nullptr) — если задан, скрипт получает метод
// item:spawn_board(x, y), создающий декоративную вращающуюся мигающую доску.
static sol::table makeItemProxy(sol::state& lua, Item& item, std::vector<Spinner>* spinners = nullptr) {
    sol::table t = lua.create_table();

    t.set_function("log", [](sol::object, const std::string& msg) {
        std::cout << "[item] " << msg << std::endl;
    });

    if (spinners) {
        t.set_function("spawn_board", [spinners](sol::object, float x, float y) {
            spinners->push_back(makeRandomSpinner({x, y}));
        });
    }

    // Свойства через get/set функции, потому что Item — C++ объект,
    // а Lua-таблица должна оставаться лёгкой и не владеть им напрямую.
    // Используем metatable __index/__newindex для прозрачного доступа к полям.
    sol::table mt = lua.create_table();
    mt.set_function("__index", [&item](sol::table, const std::string& key) -> sol::object {
        sol::state_view lv(item.env.lua_state());
        if (key == "x") return sol::make_object(lv, item.pos.x);
        if (key == "y") return sol::make_object(lv, item.pos.y);
        if (key == "radius") return sol::make_object(lv, item.radius);
        if (key == "active") return sol::make_object(lv, item.active);
        if (key == "color") return sol::make_object(lv, item.color);
        return sol::nil;
    });
    mt.set_function("__newindex", [&item](sol::table, const std::string& key, sol::object val) {
        if (key == "x") item.pos.x = val.as<float>();
        else if (key == "y") item.pos.y = val.as<float>();
        else if (key == "radius") item.radius = val.as<float>();
        else if (key == "active") item.active = val.as<bool>();
        else if (key == "color") item.color = val.as<sf::Color>();
    });
    t[sol::metatable_key] = mt;

    return t;
}

void Item::loadScript(ScriptEngine& engine, const std::string& source, const std::string& chunkName) {
    scriptSource = source;
    scriptName   = chunkName;
    scriptOk = engine.loadString(source, chunkName, env);
    if (!scriptOk) {
        lastError = engine.lastError();
        return;
    }

    // ВАЖНО: прокси не кешируется в env["item"] — Item обычно лежит в
    // std::vector<Item> и копируется/переаллоцируется при push_back, что
    // делает захваченный по ссылке адрес "this" невалидным. on_create
    // безопасно вызывать здесь и сейчас (объект ещё на стабильном адресе),
    // но on_update/on_touch создают свежий прокси на актуальном this
    // непосредственно перед вызовом (см. ниже).
    sol::table proxy = makeItemProxy(engine.lua(), *this);

    sol::protected_function onCreate = env["on_create"];
    if (onCreate.valid()) {
        sol::protected_function_result r = onCreate(proxy);
        if (!r.valid()) {
            sol::error err = r;
            lastError = err.what();
            scriptOk = false;
        }
    }
}

void Item::update(ScriptEngine& engine, float dt) {
    if (!scriptOk || !active) return;

    sol::protected_function onUpdate = env["on_update"];
    if (!onUpdate.valid()) return;

    sol::table proxy = makeItemProxy(engine.lua(), *this);
    sol::protected_function_result r = onUpdate(proxy, dt);
    if (!r.valid()) {
        sol::error err = r;
        lastError = err.what();
    }
}

bool Item::checkTouch(ScriptEngine& engine, Ball& ball, std::vector<Spinner>& spawnedSpinners) {
    if (!scriptOk || !active) return false;

    float dist = vec2len(ball.pos - pos);
    if (dist >= radius + Ball::RADIUS) return false;

    sol::protected_function onTouch = env["on_touch"];
    if (!onTouch.valid()) return true; // касание было, но обработчика нет

    // Прокси для шара: даём скрипту доступ к позиции/скорости на чтение и запись.
    sol::table ballProxy = engine.lua().create_table();
    sol::table mt = engine.lua().create_table();
    mt.set_function("__index", [&ball, &engine](sol::table, const std::string& key) -> sol::object {
        if (key == "x") return sol::make_object(engine.lua(), ball.pos.x);
        if (key == "y") return sol::make_object(engine.lua(), ball.pos.y);
        if (key == "vx") return sol::make_object(engine.lua(), ball.vel.x);
        if (key == "vy") return sol::make_object(engine.lua(), ball.vel.y);
        return sol::nil;
    });
    mt.set_function("__newindex", [&ball](sol::table, const std::string& key, sol::object val) {
        if (key == "x") ball.pos.x = val.as<float>();
        else if (key == "y") ball.pos.y = val.as<float>();
        else if (key == "vx") ball.vel.x = val.as<float>();
        else if (key == "vy") ball.vel.y = val.as<float>();
    });
    ballProxy[sol::metatable_key] = mt;

    sol::table proxy = makeItemProxy(engine.lua(), *this, &spawnedSpinners);
    sol::protected_function_result r = onTouch(proxy, ballProxy);
    if (!r.valid()) {
        sol::error err = r;
        lastError = err.what();
    }

    return true;
}

void Item::draw(sf::RenderWindow& window) const {
    if (!active) return;

    sf::CircleShape shape(radius);
    shape.setOrigin(radius, radius);
    shape.setPosition(pos);
    shape.setFillColor(color);
    shape.setOutlineThickness(2.f);
    shape.setOutlineColor(sf::Color(255, 255, 255, 180));
    window.draw(shape);
}

// ── Пресеты скриптов ─────────────────────────────────────────────────────

std::string ItemPresets::coinScript() {
    return R"LUA(
-- Coin: подбираемый предмет, исчезает при касании.
function on_create(item)
    item.radius = 14
    item.color = Color(255, 215, 0)
end

function on_update(item, dt)
    -- ничего не делаем каждый кадр; можно добавить вращение/пульсацию
end

function on_touch(item, ball)
    item.active = false
    item:log("Coin collected!")
end
)LUA";
}

std::string ItemPresets::trapScript() {
    return R"LUA(
-- Trap: отбрасывает шар назад при касании.
function on_create(item)
    item.radius = 18
    item.color = Color(220, 40, 40)
end

function on_touch(item, ball)
    local dx = ball.x - item.x
    local dy = ball.y - item.y
    local len = math.sqrt(dx*dx + dy*dy)
    if len < 0.001 then
        dx, dy, len = 0, -1, 1
    end
    local force = 500
    ball.vx = ball.vx + (dx / len) * force
    ball.vy = ball.vy + (dy / len) * force
    item:log("Trap triggered!")
end
)LUA";
}

std::string ItemPresets::boostScript() {
    return R"LUA(
-- Boost: придаёт шару импульс в заданном направлении при касании.
local DIR_X = 1.0
local DIR_Y = 0.0
local FORCE = 600

function on_create(item)
    item.radius = 20
    item.color = Color(60, 200, 255)
end

function on_touch(item, ball)
    ball.vx = ball.vx + DIR_X * FORCE
    ball.vy = ball.vy + DIR_Y * FORCE
    item:log("Boost!")
end
)LUA";
}

std::string ItemPresets::spinBoardsScript() {
    return R"LUA(
-- Spin Boards: при касании создаёт 3 случайные вращающиеся мигающие доски
-- вокруг предмета (используя item:spawn_board(x, y)).
function on_create(item)
    item.radius = 18
    item.color = Color(180, 100, 220)
end

function on_touch(item, ball)
    for i = 1, 3 do
        local angle = math.random() * math.pi * 2
        local dist = 40 + math.random() * 60
        local bx = item.x + math.cos(angle) * dist
        local by = item.y + math.sin(angle) * dist
        item:spawn_board(bx, by)
    end
    item:log("Spawned 3 spinning boards!")
end
)LUA";
}

std::string ItemPresets::blankScript() {
    return R"LUA(
-- Blank template: write your own item logic here.
function on_create(item)
    item.radius = 18
    item.color = Color(200, 200, 200)
end

function on_update(item, dt)
end

function on_touch(item, ball)
end
)LUA";
}
