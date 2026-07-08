#pragma once
#include <SFML/Network.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <string>

// ── Сетевой мультиплеер по локальной сети ───────────────────────────────────
//
// Модель: хост — источник истины. Хост считает всю физику (свой шар + шар
// клиента, управляемый по присланным координатам мыши), клиент только шлёт
// своё положение мыши и отрисовывает то, что прислал хост. Предметы/скрипты
// Lua и декоративные спиннеры в сетевом режиме не участвуют — синхронизация
// произвольного Lua-состояния по сети сильно расширила бы объём работы,
// поэтому первая версия ограничена стенами/платформами, целью и двумя шарами.
//
// Автообнаружение: клиент широковещательно рассылает UDP-пакет "ищу игру" на
// известный порт; любой хост, слушающий этот порт, отвечает клиенту напрямую
// (unicast) своим TCP-портом. Клиент подключается по TCP уже зная точный IP
// хоста (берётся из адреса отправителя UDP-ответа) — вводить IP вручную не нужно.

constexpr unsigned short DISCOVERY_PORT = 54321;
constexpr unsigned short GAME_PORT      = 54322;
const std::string DISCOVER_MAGIC  = "BALLGAME_DISCOVER";
const std::string HOST_REPLY_MAGIC = "BALLGAME_HOST_HERE";

// Геометрия одной стены, которой хост делится с клиентом при старте уровня.
struct NetWallInfo {
    float x, y, w, h;
    sf::Uint8 r, g, b;
    bool kinematic; // если true — хост будет присылать её живую позицию каждый кадр
};

// Полное описание уровня для передачи клиенту один раз при подключении.
struct NetLevelInfo {
    std::string name;
    float ballStartX, ballStartY;
    float goalX, goalY;
    float goalRadius;
    std::vector<NetWallInfo> walls;
};

// Состояние одного кадра игры, которым хост делится с клиентом каждый тик.
struct NetFrameState {
    float ball1X, ball1Y;
    float ball2X, ball2Y;
    bool  ball1Reached;
    bool  ball2Reached;
    bool  won;
    std::vector<sf::Vector2f> kinematicWallPositions; // в том же порядке, что и в NetLevelInfo (только kinematic=true)
};

// Ввод клиента, отправляемый хосту каждый кадр.
struct NetInputState {
    float mouseX, mouseY;
    bool  pressed;
};

class NetworkSession {
public:
    enum class Role { None, Host, Client };

    ~NetworkSession();

    // ── Хост ─────────────────────────────────────────────────────────────
    // Начинает слушать TCP-подключения и отвечать на UDP-обнаружение.
    // Возвращает false при ошибке (например, порт уже занят).
    bool startHosting();

    // Проверяет (неблокирующе), подключился ли клиент. Также отвечает на
    // входящие UDP-запросы обнаружения, пока клиент не подключился.
    // Возвращает true, как только TCP-клиент подключился.
    bool pollHostWaiting();

    // ── Клиент ───────────────────────────────────────────────────────────
    // Начинает широковещательный поиск хоста в локальной сети.
    bool startSearching();

    // Проверяет (неблокирующе), найден ли хост и установлено ли TCP-соединение.
    // Возвращает true, как только подключение к хосту установлено.
    bool pollClientSearching();

    // ── Общее ────────────────────────────────────────────────────────────
    Role role() const { return role_; }
    bool isConnected() const { return connected_; }
    void disconnect();

    // Хост: отправить клиенту описание уровня (один раз, сразу после подключения).
    bool sendLevelInfo(const NetLevelInfo& info);
    // Клиент: неблокирующе проверить, пришло ли описание уровня. true если пришло.
    bool receiveLevelInfo(NetLevelInfo& outInfo);

    // Хост: отправить клиенту состояние текущего кадра.
    bool sendFrameState(const NetFrameState& state);
    // Клиент: неблокирующе получить последнее присланное состояние кадра.
    // Возвращает true если получено новое состояние с момента прошлого вызова.
    bool receiveFrameState(NetFrameState& outState);

    // Клиент: отправить хосту своё текущее состояние ввода (мышь).
    bool sendInput(const NetInputState& input);
    // Хост: неблокирующе получить последнее состояние ввода клиента.
    bool receiveInput(NetInputState& outInput);

private:
    Role role_ = Role::None;
    bool connected_ = false;

    sf::UdpSocket   discoveryUdp_;
    sf::TcpListener listener_;
    sf::TcpSocket   socket_; // общий канал: и хост, и клиент используют один TCP-сокет после подключения

    sf::Clock       discoveryTimer_; // клиент: как часто повторять broadcast
};
