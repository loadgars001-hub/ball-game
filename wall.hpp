#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <sol/sol.hpp>
#include <string>

class ScriptEngine;
class PhysicsWorld;

struct Wall {
    sf::FloatRect rect;
    sf::Color     color;

    // Параметры встроенного синусоидального движения (платформа).
    // Если amplitude == 0 И нет Lua-скрипта, стена статична (b2_staticBody).
    // Иначе — кинематическое тело (b2_kinematicBody): Box2D сам интегрирует
    // его позицию по скорости, которую мы вычисляем каждый кадр как разницу
    // между желаемой (синус/скрипт) и текущей позицией.
    sf::Vector2f startPos;     // исходная позиция (left, top) — точка отсчёта для движения
    sf::Vector2f axis;         // направление встроенного движения (1,0) или (0,1)
    float        amplitude = 0.f; // размах встроенного колебания в пикселях
    float        speed     = 0.f; // скорость фазы (рад/сек)
    float        phase     = 0.f; // текущая фаза
    sf::Vector2f velocity;        // скорость платформы (px/сек), передаётся в Box2D тело

    // ── Lua-поведение (опционально) ─────────────────────────────────────
    // Если задан scriptSource, движение стены полностью определяется скриптом:
    // встроенный синус (amplitude/speed) игнорируется, позицию задаёт on_update.
    //
    //   function on_create(wall)          -- один раз при создании
    //   function on_update(wall, dt)      -- каждый кадр; задаёт wall.x, wall.y
    //
    // wall.x, wall.y       -- позиция левого верхнего угла (читать/писать)
    // wall.start_x/start_y -- исходная позиция (только чтение)
    // wall.t                -- накопленное время с создания (только чтение)
    std::string  scriptSource;
    std::string  scriptName;
    bool         hasScript = false;
    bool         scriptOk  = false;
    std::string  lastError;

    Wall(float x, float y, float w, float h, sf::Color col = sf::Color(180, 90, 40));

    // Настроить как движущуюся платформу со встроенным синусом (без Lua).
    void makeMoving(sf::Vector2f moveAxis, float moveAmplitude, float moveSpeed);

    // Привязать Lua-скрипт для кастомного поведения движения.
    void loadScript(ScriptEngine& engine, const std::string& source, const std::string& chunkName);

    // Создаёт Box2D тело (статичное или кинематическое) в указанном мире.
    // Вызывать при загрузке уровня, до первого update().
    void createBody(PhysicsWorld& world);

    // Вычисляет желаемую позицию (встроенный синус или Lua-скрипт), переводит
    // разницу в скорость и передаёт её кинематическому Box2D телу — саму
    // интеграцию позиции по этой скорости выполняет world.step().
    void update(float dt, ScriptEngine* engine = nullptr);

    // Считывает финальную позицию тела после world.step() обратно в rect
    // (для статичных стен ничего не меняется, для кинематических — синхронизация).
    void syncFromBody();

    void draw(sf::RenderWindow& window) const;

    b2Body* body = nullptr; // не владеющий указатель; жизненный цикл управляется b2World

    sol::environment scriptEnv;
    float scriptTime = 0.f;
};
