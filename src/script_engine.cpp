#include "script_engine.hpp"
#include <fstream>
#include <sstream>

ScriptEngine::ScriptEngine() {
    // Открываем только безопасные стандартные библиотеки —
    // без os/io, чтобы пользовательские скрипты не могли трогать диск/систему
    // напрямую (кроме того, что мы явно разрешим через биндинги).
    lua_.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string
    );

    registerTypes();
}

void ScriptEngine::registerTypes() {
    // sf::Vector2f — доступен в скриптах как Vec2
    lua_.new_usertype<sf::Vector2f>("Vec2",
        sol::call_constructor,
        sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>(),
        "x", &sf::Vector2f::x,
        "y", &sf::Vector2f::y,
        sol::meta_function::addition, [](const sf::Vector2f& a, const sf::Vector2f& b) { return a + b; },
        sol::meta_function::subtraction, [](const sf::Vector2f& a, const sf::Vector2f& b) { return a - b; },
        sol::meta_function::multiplication, [](const sf::Vector2f& a, float s) { return a * s; }
    );

    // sf::Color — доступен в скриптах как Color
    lua_.new_usertype<sf::Color>("Color",
        sol::call_constructor,
        sol::constructors<sf::Color(), sf::Color(sf::Uint8, sf::Uint8, sf::Uint8), sf::Color(sf::Uint8, sf::Uint8, sf::Uint8, sf::Uint8)>(),
        "r", &sf::Color::r,
        "g", &sf::Color::g,
        "b", &sf::Color::b,
        "a", &sf::Color::a
    );
}

bool ScriptEngine::loadFile(const std::string& path, sol::environment& outEnv) {
    std::ifstream in(path);
    if (!in.is_open()) {
        lastError_ = "Cannot open file: " + path;
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return loadString(ss.str(), path, outEnv);
}

bool ScriptEngine::loadString(const std::string& src, const std::string& chunkName, sol::environment& outEnv) {
    // sol::create + lua_.globals() как fallback-таблица: окружение видит все
    // глобальные функции/usertype'ы (Color, Vec2, math, table и т.д.) на чтение,
    // но новые присваивания (например, локальные функции on_create) идут в
    // собственную таблицу окружения, не загрязняя глобальное пространство.
    sol::environment env(lua_, sol::create, lua_.globals());

    sol::load_result chunk = lua_.load(src, chunkName);
    if (!chunk.valid()) {
        sol::error err = chunk;
        lastError_ = err.what();
        return false;
    }

    sol::protected_function pf = chunk;
    sol::set_environment(env, pf);

    sol::protected_function_result result = pf();
    if (!result.valid()) {
        sol::error err = result;
        lastError_ = err.what();
        return false;
    }

    outEnv = env;
    return true;
}
