#pragma once
#include "wall.hpp"
#include "item.hpp"
#include "spinner.hpp"
#include <vector>
#include <string>
#include <algorithm>

class ScriptEngine;
class PhysicsWorld;

struct Level {
    std::string      name;         // название уровня
    sf::Vector2f     ballStart;    // стартовая позиция шара
    sf::Vector2f     goal;         // позиция цели (звезда/флаг)
    float            goalRadius;   // радиус зоны попадания
    std::vector<Wall> walls;
    std::vector<Item> items;       // предметы со скриптовым поведением
    std::vector<Spinner> spinners; // декоративные вращающиеся "доски", спавнятся из Lua

    // Создаёт Box2D тела для всех стен уровня в указанном мире.
    // Вызывать один раз сразу после загрузки/генерации уровня, до первого update().
    void createBodies(PhysicsWorld& world) {
        for (auto& w : walls) w.createBody(world);
    }

    // Вызывать ПЕРЕД world.step(): вычисляет желаемое движение платформ/предметов
    // и передаёт скорость в кинематические Box2D тела.
    void update(float dt, ScriptEngine* engine = nullptr) {
        for (auto& w : walls) w.update(dt, engine);
        if (engine)
            for (auto& it : items) it.update(*engine, dt);

        for (auto& s : spinners) s.update(dt);
        spinners.erase(std::remove_if(spinners.begin(), spinners.end(),
                                       [](const Spinner& s) { return s.expired(); }),
                        spinners.end());
    }

    // Вызывать ПОСЛЕ world.step(): считывает финальные позиции кинематических
    // тел стен обратно в rect для отрисовки и коллизий предметов.
    void syncFromBodies() {
        for (auto& w : walls) w.syncFromBody();
    }

    void drawSpinners(sf::RenderWindow& window) const {
        for (const auto& s : spinners) s.draw(window);
    }
};

// Процедурная генерация одного случайного уровня.
// stageNum используется чтобы постепенно усложнять уровни (больше препятствий).
class ScriptEngine;

// Процедурная генерация одного случайного уровня.
// stageNum используется чтобы постепенно усложнять уровни (больше препятствий).
// Если engine != nullptr и stageNum >= 5, на уровень добавляется предмет
// Boost (тот же скрипт, что доступен в редакторе) — он толкает шар при касании.
// Без engine (nullptr) предмет не добавляется (например, для сетевых уровней,
// где предметы намеренно не синхронизируются между хостом и клиентом).
Level generateRandomLevel(sf::Vector2f screen, int stageNum, ScriptEngine* engine = nullptr);

// ── Кастомные уровни (редактор) ────────────────────────────────────────────
// Сохраняет уровень в текстовый файл по указанному пути.
// Возвращает true при успехе.
bool saveLevelToFile(const Level& level, const std::string& path);

// Загружает уровень из текстового файла. Возвращает true при успехе,
// заполняя outLevel; при неудаче outLevel не изменяется.
// Если engine != nullptr, скриптовые стены и предметы будут скомпилированы
// и готовы к работе; если nullptr, они будут загружены как статичные/неактивные
// (полезно для предпросмотра списка уровней без запуска Lua).
bool loadLevelFromFile(Level& outLevel, const std::string& path, ScriptEngine* engine = nullptr);

// Папка, где хранятся кастомные уровни (создаётся при необходимости).
std::string customLevelsDir();

// Список путей ко всем сохранённым кастомным уровням (*.lvl) в порядке имени файла.
std::vector<std::string> listCustomLevels();
