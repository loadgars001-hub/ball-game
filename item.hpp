#pragma once
#include <SFML/Graphics.hpp>
#include <sol/sol.hpp>
#include <string>
#include <vector>

class ScriptEngine;
class Ball;
struct Spinner;

// Предмет на уровне с поведением, заданным Lua-скриптом.
// Скрипт должен определять функции (все необязательны, кроме on_create):
//
//   function on_create(item)            -- вызывается один раз при создании
//   function on_update(item, dt)        -- вызывается каждый кадр пока активен
//   function on_touch(item, ball)       -- вызывается при касании шаром
//                                          ball.x, ball.y, ball.vx, ball.vy доступны на чтение/запись
//
// Глобальный объект item (через первый параметр) предоставляет:
//   item.x, item.y       -- позиция (читать/писать)
//   item.radius          -- радиус для проверки касания
//   item.active          -- true/false, можно деактивировать предмет (исчезает)
//   item.color           -- Color
//   item:log(msg)        -- вывод отладочного сообщения в консоль
//   item:spawn_board(x, y) -- создаёт декоративную вращающуюся мигающую "доску"
//                             в точке (x, y); можно вызвать несколько раз подряд
struct Item {
    sf::Vector2f pos;
    float        radius = 18.f;
    sf::Color    color  = sf::Color(255, 215, 0);
    bool         active = true;
    std::string  scriptSource;   // исходный код Lua (для сохранения/отображения)
    std::string  scriptName;     // имя для UI/отладки (например "coin", "trap")

    sol::environment env;        // окружение скрипта (хранит локальные переменные/функции)
    bool         scriptOk = false;
    std::string  lastError;

    // Привязывает и выполняет скрипт; вызывает on_create если определена.
    void loadScript(ScriptEngine& engine, const std::string& source, const std::string& chunkName);

    void update(ScriptEngine& engine, float dt);

    // Проверяет касание шаром; если есть — вызывает on_touch. Возвращает true если было касание.
    // spawnedSpinners — сюда попадают все "доски", созданные скриптом через item:spawn_board()
    // за это касание (Game добавляет их в level.spinners после вызова).
    bool checkTouch(ScriptEngine& engine, Ball& ball, std::vector<Spinner>& spawnedSpinners);

    void draw(sf::RenderWindow& window) const;
};

// Несколько встроенных примеров-пресетов скриптов, доступных в редакторе
// (используются как стартовый шаблон, который пользователь может менять).
namespace ItemPresets {
    std::string coinScript();      // подбираемая монетка: исчезает и засчитывает очко
    std::string trapScript();      // ловушка: отбрасывает шар назад при касании
    std::string boostScript();     // ускоритель: придаёт шару импульс в заданном направлении
    std::string spinBoardsScript();// при касании создаёт 3 случайные вращающиеся мигающие доски
    std::string blankScript();     // пустой шаблон для написания своего скрипта
}
