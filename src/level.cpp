#include "level.hpp"
#include "item.hpp"
#include "script_engine.hpp"
#include <random>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>

// Глобальный генератор случайных чисел (инициализируется один раз)
static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

static float randRange(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(rng());
}

static int randInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng());
}

// ── Паттерны уровней ─────────────────────────────────────────────────────
// Каждый паттерн строит уровень слева направо (старт слева, цель справа),
// с гарантированным свободным проходом.

// 1. Простой коридор с горизонтальными полками-перегородками,
//    в каждой из которых есть проход (по очереди сверху/снизу).
static void patternCorridor(Level& l, float W, float H, sf::Color c1, int extra) {
    l.ballStart = {W * 0.08f, H * 0.5f};
    l.goal      = {W * 0.92f, H * 0.5f};

    int shelves = 3 + std::min(extra, 3); // 3..6 перегородок
    float gapH = std::max(110.f, 150.f - extra * 8.f); // проход чуть сужается со сложностью

    for (int i = 0; i < shelves; ++i) {
        float x = W * (i + 1) / (shelves + 1);
        bool gapTop = (i % 2 == 0);
        float gapY = gapTop ? H * randRange(0.18f, 0.32f) : H * randRange(0.68f, 0.82f);

        if (gapY - gapH / 2.f > 10.f)
            l.walls.emplace_back(x, 0.f, 16.f, gapY - gapH / 2.f, c1);
        if (gapY + gapH / 2.f < H - 10.f)
            l.walls.emplace_back(x, gapY + gapH / 2.f, 16.f, H - gapY - gapH / 2.f, c1);
    }
}

// 2. Лестница: ступени ведут шар по диагонали вниз/вверх к цели.
static void patternStairs(Level& l, float W, float H, sf::Color c1, int extra) {
    bool goingDown = randInt(0, 1) == 0;
    l.ballStart = {W * 0.08f, goingDown ? H * 0.12f : H * 0.85f};
    l.goal      = {W * 0.92f, goingDown ? H * 0.85f : H * 0.12f};

    int steps = 4 + std::min(extra, 3); // 4..7 ступеней
    for (int i = 0; i < steps; ++i) {
        float t = (float)(i + 1) / (steps + 1);
        float x = W * t - 90.f;
        float y = goingDown
            ? H * (0.12f + t * 0.68f)
            : H * (0.85f - t * 0.68f);
        // чередуем ступени слева/справа от линии движения, оставляя проход
        float stepW = W * randRange(0.16f, 0.24f);
        l.walls.emplace_back(x, y, stepW, 16.f, c1);
    }
}

// 3. Зигзаг: вертикальные стены с проходом то сверху, то снизу — классический лабиринт-змейка.
static void patternZigzag(Level& l, float W, float H, sf::Color c1, sf::Color c2, int extra) {
    l.ballStart = {W * 0.07f, H * 0.5f};
    l.goal      = {W * 0.93f, H * 0.5f};

    int bars = 3 + std::min(extra, 4); // 3..7 барьеров
    float gapH = std::max(120.f, 160.f - extra * 8.f);

    for (int i = 0; i < bars; ++i) {
        float x = W * (i + 1) / (bars + 1);
        bool gapTop = (i % 2 == 0);
        float gapY = gapTop ? H * 0.22f : H * 0.78f;
        sf::Color col = (i % 2 == 0) ? c1 : c2;

        if (gapY - gapH / 2.f > 10.f)
            l.walls.emplace_back(x, 0.f, 18.f, gapY - gapH / 2.f, col);
        if (gapY + gapH / 2.f < H - 10.f)
            l.walls.emplace_back(x, gapY + gapH / 2.f, 18.f, H - gapY - gapH / 2.f, col);
    }
}

// 4. Узкие ворота: пары стен, образующие сужающийся проход, который нужно пройти аккуратно.
static void patternGates(Level& l, float W, float H, sf::Color c1, int extra) {
    l.ballStart = {W * 0.07f, H * 0.5f};
    l.goal      = {W * 0.93f, H * 0.5f};

    int gates = 2 + std::min(extra / 2, 2); // 2..4 ворот
    float gapH = std::max(95.f, 140.f - extra * 9.f); // ворота сужаются с ростом сложности

    for (int i = 0; i < gates; ++i) {
        float x = W * (i + 1) / (gates + 1);
        float passY = H * randRange(0.30f, 0.70f);
        l.walls.emplace_back(x, 0.f, 16.f, passY - gapH / 2.f, c1);
        l.walls.emplace_back(x, passY + gapH / 2.f, 16.f, H - passY - gapH / 2.f, c1);
    }
}

// 5. Комнаты: несколько прямоугольных "комнат" с дверными проёмами, связанных в цепочку.
static void patternRooms(Level& l, float W, float H, sf::Color c1, sf::Color c2, int extra) {
    l.ballStart = {W * 0.08f, H * 0.5f};
    l.goal      = {W * 0.92f, H * 0.5f};

    int rooms = 2 + std::min(extra / 2, 2); // 2..4 перегородки между комнатами
    float doorH = std::max(120.f, 170.f - extra * 8.f);

    for (int i = 0; i < rooms; ++i) {
        float x = W * (i + 1) / (rooms + 1);
        float doorY = H * randRange(0.25f, 0.75f);
        sf::Color col = (i % 2 == 0) ? c1 : c2;
        if (doorY - doorH / 2.f > 10.f)
            l.walls.emplace_back(x, 0.f, 18.f, doorY - doorH / 2.f, col);
        if (doorY + doorH / 2.f < H - 10.f)
            l.walls.emplace_back(x, doorY + doorH / 2.f, 18.f, H - doorY - doorH / 2.f, col);
        // небольшая платформа внутри комнаты (не перекрывает дверь)
        if (extra > 1) {
            float px = x - W / (rooms + 1) * 0.5f;
            float py = (doorY < H / 2.f) ? H * 0.78f : H * 0.12f;
            l.walls.emplace_back(px, py, W / (rooms + 1) * 0.5f, 14.f, col);
        }
    }
}

// Добавляет несколько движущихся платформ на уровень.
// Платформы располагаются в открытых местах и двигаются строго по одной оси
// (горизонтально или вертикально) в пределах экрана.
static void addMovingPlatforms(Level& l, float W, float H, sf::Color col, int count) {
    for (int i = 0; i < count; ++i) {
        bool horizontal = randInt(0, 1) == 0;
        float pw = horizontal ? randRange(90.f, 150.f) : 18.f;
        float ph = horizontal ? 18.f : randRange(90.f, 150.f);

        float x = randRange(0.15f, 0.80f) * W;
        float y = randRange(0.15f, 0.80f) * H;

        Wall platform(x, y, pw, ph, col);

        float amplitude = horizontal ? randRange(60.f, 140.f) : randRange(60.f, 120.f);
        float speed = randRange(0.6f, 1.3f); // рад/сек

        sf::Vector2f axis = horizontal ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(0.f, 1.f);
        platform.makeMoving(axis, amplitude, speed);

        l.walls.push_back(platform);
    }
}

// Добавляет на уровень предмет Boost (тот же скрипт, что доступен в редакторе)
// в случайной точке, не слишком близко к старту/цели/стенам, чтобы шар мог
// его гарантированно задеть по пути, а не застрять на нём случайно.
static void addBoostItem(Level& l, ScriptEngine& engine, float W, float H) {
    sf::Vector2f pos;
    bool ok = false;
    for (int attempt = 0; attempt < 20 && !ok; ++attempt) {
        pos = {randRange(0.20f, 0.80f) * W, randRange(0.20f, 0.80f) * H};

        float distStart = std::sqrt(std::pow(pos.x - l.ballStart.x, 2) + std::pow(pos.y - l.ballStart.y, 2));
        float distGoal  = std::sqrt(std::pow(pos.x - l.goal.x, 2) + std::pow(pos.y - l.goal.y, 2));
        if (distStart < 80.f || distGoal < 80.f) continue;

        bool overlapsWall = false;
        for (const auto& w : l.walls) {
            float cx = w.rect.left + w.rect.width / 2.f;
            float cy = w.rect.top + w.rect.height / 2.f;
            float dist = std::sqrt(std::pow(pos.x - cx, 2) + std::pow(pos.y - cy, 2));
            if (dist < 50.f) { overlapsWall = true; break; }
        }
        if (!overlapsWall) ok = true;
    }
    // Если за 20 попыток не нашли свободное место — всё равно ставим последнюю
    // позицию: это лишь эстетическое улучшение, а не обязательное условие.

    Item boost;
    boost.pos = pos;
    boost.loadScript(engine, ItemPresets::boostScript(), "boost");
    l.items.push_back(boost);
}

Level generateRandomLevel(sf::Vector2f screen, int stageNum, ScriptEngine* engine) {
    Level l;
    float W = screen.x, H = screen.y;

    sf::Color palette[][2] = {
        {sf::Color(160, 80, 30),  sf::Color(100, 110, 120)},
        {sf::Color(30, 140, 130), sf::Color(160, 80, 30)},
        {sf::Color(120, 60, 150), sf::Color(40, 130, 90)},
        {sf::Color(180, 60, 60),  sf::Color(80, 90, 160)},
    };
    int palIdx = randInt(0, 3);
    sf::Color c1 = palette[palIdx][0];
    sf::Color c2 = palette[palIdx][1];

    l.name = "Level " + std::to_string(stageNum);
    l.goalRadius = randRange(26.f, 32.f);

    // Сложность растёт со стадией, но с разумным потолком
    int extra = std::min(stageNum / 2, 6);

    int pattern = randInt(0, 4);
    switch (pattern) {
        case 0: patternCorridor(l, W, H, c1, extra); break;
        case 1: patternStairs(l, W, H, c1, extra); break;
        case 2: patternZigzag(l, W, H, c1, c2, extra); break;
        case 3: patternGates(l, W, H, c1, extra); break;
        default: patternRooms(l, W, H, c1, c2, extra); break;
    }

    // Начиная со 2-й стадии добавляем 1-3 движущиеся платформы (больше — на сложных уровнях)
    if (stageNum >= 2) {
        int platformCount = std::min(1 + extra / 3, 3);
        addMovingPlatforms(l, W, H, c2, platformCount);
    }

    // Начиная с 5-го уровня добавляем предмет Boost — тот же, что доступен в
    // редакторе, отталкивает/подгоняет шар при касании. Требует ScriptEngine
    // для компиляции Lua-скрипта; если engine не передан (например, для
    // сетевых уровней — см. комментарий в level.hpp), предмет не добавляется.
    if (stageNum >= 5 && engine) {
        addBoostItem(l, *engine, W, H);
    }

    return l;
}

// ── Сериализация кастомных уровней ──────────────────────────────────────────
// Простой текстовый формат:
//   LEVEL <name>
//   GOAL_RADIUS <r>
//   BALL_START <x> <y>
//   GOAL <x> <y>
//   WALL <x> <y> <w> <h> <r> <g> <b>                       (статичная стена)
//   PWALL <x> <y> <w> <h> <r> <g> <b> <ax> <ay> <amp> <spd> (движущаяся платформа, встроенный синус)
//   SWALL <x> <y> <w> <h> <r> <g> <b> <base64-lua>          (стена с Lua-поведением)
//   ITEM <x> <y> <radius> <r> <g> <b> <name> <base64-lua>   (предмет с Lua-поведением)
//
// Lua-исходники хранятся в base64 одной строкой, чтобы переносы строк внутри
// скрипта не ломали построчный текстовый формат .lvl файла.

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::string& in) {
    std::string out;
    int val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(B64_CHARS[(val >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) out.push_back(B64_CHARS[((val << 8) >> (bits + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::string base64Decode(const std::string& in) {
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i) T[(unsigned char)B64_CHARS[i]] = i;

    std::string out;
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

std::string customLevelsDir() {
    std::filesystem::path dir = std::filesystem::path(".") / "custom_levels";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.string();
}

bool saveLevelToFile(const Level& level, const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "LEVEL " << (level.name.empty() ? "Custom" : level.name) << "\n";
    out << "GOAL_RADIUS " << level.goalRadius << "\n";
    out << "BALL_START " << level.ballStart.x << " " << level.ballStart.y << "\n";
    out << "GOAL " << level.goal.x << " " << level.goal.y << "\n";

    for (const auto& w : level.walls) {
        if (w.hasScript) {
            out << "SWALL "
                << w.startPos.x << " " << w.startPos.y << " "
                << w.rect.width << " " << w.rect.height << " "
                << (int)w.color.r << " " << (int)w.color.g << " " << (int)w.color.b << " "
                << base64Encode(w.scriptSource) << "\n";
        } else if (w.amplitude > 0.f) {
            out << "PWALL "
                << w.startPos.x << " " << w.startPos.y << " "
                << w.rect.width << " " << w.rect.height << " "
                << (int)w.color.r << " " << (int)w.color.g << " " << (int)w.color.b << " "
                << w.axis.x << " " << w.axis.y << " "
                << w.amplitude << " " << w.speed << "\n";
        } else {
            out << "WALL "
                << w.rect.left << " " << w.rect.top << " "
                << w.rect.width << " " << w.rect.height << " "
                << (int)w.color.r << " " << (int)w.color.g << " " << (int)w.color.b << "\n";
        }
    }

    for (const auto& it : level.items) {
        out << "ITEM "
            << it.pos.x << " " << it.pos.y << " " << it.radius << " "
            << (int)it.color.r << " " << (int)it.color.g << " " << (int)it.color.b << " "
            << (it.scriptName.empty() ? "item" : it.scriptName) << " "
            << base64Encode(it.scriptSource) << "\n";
    }

    return true;
}

bool loadLevelFromFile(Level& outLevel, const std::string& path, ScriptEngine* engine) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    Level l;
    l.goalRadius = 28.f;
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "LEVEL") {
            std::getline(iss, l.name);
            // обрезаем ведущий пробел
            if (!l.name.empty() && l.name.front() == ' ') l.name.erase(0, 1);
        } else if (tag == "GOAL_RADIUS") {
            iss >> l.goalRadius;
        } else if (tag == "BALL_START") {
            iss >> l.ballStart.x >> l.ballStart.y;
        } else if (tag == "GOAL") {
            iss >> l.goal.x >> l.goal.y;
        } else if (tag == "WALL") {
            float x, y, w, h; int r, g, b;
            iss >> x >> y >> w >> h >> r >> g >> b;
            l.walls.emplace_back(x, y, w, h, sf::Color((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b));
        } else if (tag == "PWALL") {
            float x, y, w, h; int r, g, b;
            float ax, ay, amp, spd;
            iss >> x >> y >> w >> h >> r >> g >> b >> ax >> ay >> amp >> spd;
            Wall wall(x, y, w, h, sf::Color((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b));
            wall.makeMoving({ax, ay}, amp, spd);
            l.walls.push_back(wall);
        } else if (tag == "SWALL") {
            float x, y, w, h; int r, g, b;
            std::string b64;
            iss >> x >> y >> w >> h >> r >> g >> b >> b64;
            Wall wall(x, y, w, h, sf::Color((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b));
            std::string src = base64Decode(b64);
            if (engine) {
                wall.loadScript(*engine, src, path + "#wall");
            } else {
                // без движка просто запоминаем исходник, скрипт не активен
                wall.scriptSource = src;
                wall.hasScript = true;
                wall.scriptOk = false;
            }
            l.walls.push_back(wall);
        } else if (tag == "ITEM") {
            float x, y, radius; int r, g, b;
            std::string name, b64;
            iss >> x >> y >> radius >> r >> g >> b >> name >> b64;
            Item item;
            item.pos = {x, y};
            item.radius = radius;
            item.color = sf::Color((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b);
            std::string src = base64Decode(b64);
            if (engine) {
                item.loadScript(*engine, src, path + "#" + name);
            } else {
                item.scriptSource = src;
                item.scriptName = name;
                item.scriptOk = false;
            }
            l.items.push_back(item);
        }
    }

    if (l.name.empty()) l.name = "Custom";
    outLevel = l;
    return true;
}

std::vector<std::string> listCustomLevels() {
    std::vector<std::string> result;
    std::error_code ec;
    std::string dir = customLevelsDir();

    if (!std::filesystem::exists(dir, ec)) return result;

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lvl")
            result.push_back(entry.path().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}
