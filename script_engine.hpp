#pragma once
#include <SFML/Graphics.hpp>
#include <sol/sol.hpp>
#include <string>
#include <memory>

// Тонкая обёртка над sol2/Lua, общая для всего проекта.
// Создаёт один sol::state, открывает безопасные стандартные библиотеки
// и регистрирует общие типы (Vector2f, Color), доступные скриптам.
class ScriptEngine {
public:
    ScriptEngine();

    // Загружает (и кеширует) Lua-файл по пути; возвращает true при успехе.
    // При ошибке компиляции пишет сообщение в lastError() и возвращает false.
    bool loadFile(const std::string& path, sol::environment& outEnv);

    // Загружает скрипт из текста (используется, когда исходник встроен прямо
    // в файл уровня/предмета, без отдельного .lua файла).
    bool loadString(const std::string& src, const std::string& chunkName, sol::environment& outEnv);

    sol::state& lua() { return lua_; }
    const std::string& lastError() const { return lastError_; }

private:
    void registerTypes();

    sol::state  lua_;
    std::string lastError_;
};
