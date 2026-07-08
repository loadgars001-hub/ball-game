#include "network.hpp"
#include <iostream>

NetworkSession::~NetworkSession() {
    disconnect();
}

void NetworkSession::disconnect() {
    socket_.disconnect();
    listener_.close();
    discoveryUdp_.unbind();
    connected_ = false;
    role_ = Role::None;
}

// ── Хост ─────────────────────────────────────────────────────────────────

bool NetworkSession::startHosting() {
    role_ = Role::Host;
    connected_ = false;

    if (listener_.listen(GAME_PORT) != sf::Socket::Done) {
        std::cerr << "[net] Failed to listen on TCP port " << GAME_PORT << "\n";
        return false;
    }
    listener_.setBlocking(false);

    if (discoveryUdp_.bind(DISCOVERY_PORT) != sf::Socket::Done) {
        std::cerr << "[net] Failed to bind UDP discovery port " << DISCOVERY_PORT << "\n";
        return false;
    }
    discoveryUdp_.setBlocking(false);

    return true;
}

bool NetworkSession::pollHostWaiting() {
    // Отвечаем на входящие запросы обнаружения от клиентов, пока никто не подключился.
    sf::Packet discPacket;
    sf::IpAddress senderIp;
    unsigned short senderPort;
    if (discoveryUdp_.receive(discPacket, senderIp, senderPort) == sf::Socket::Done) {
        std::string magic;
        if (discPacket >> magic && magic == DISCOVER_MAGIC) {
            sf::Packet reply;
            reply << HOST_REPLY_MAGIC << (sf::Uint16)GAME_PORT;
            discoveryUdp_.send(reply, senderIp, senderPort);
        }
    }

    // Проверяем, подключился ли уже кто-то по TCP.
    if (listener_.accept(socket_) == sf::Socket::Done) {
        socket_.setBlocking(false);
        connected_ = true;
        listener_.close(); // больше не принимаем новых клиентов — игра на двоих
        return true;
    }
    return false;
}

// ── Клиент ───────────────────────────────────────────────────────────────

bool NetworkSession::startSearching() {
    role_ = Role::Client;
    connected_ = false;

    if (discoveryUdp_.bind(sf::Socket::AnyPort) != sf::Socket::Done) {
        std::cerr << "[net] Failed to bind client UDP socket\n";
        return false;
    }
    discoveryUdp_.setBlocking(false);
    discoveryTimer_.restart();
    return true;
}

bool NetworkSession::pollClientSearching() {
    // Раз в полсекунды повторяем широковещательный запрос — на случай, если
    // первый пакет потерялся или хост запустился чуть позже клиента.
    if (discoveryTimer_.getElapsedTime().asSeconds() > 0.5f) {
        discoveryTimer_.restart();
        sf::Packet req;
        req << DISCOVER_MAGIC;
        discoveryUdp_.send(req, sf::IpAddress::Broadcast, DISCOVERY_PORT);
    }

    sf::Packet reply;
    sf::IpAddress hostIp;
    unsigned short hostUdpPort;
    if (discoveryUdp_.receive(reply, hostIp, hostUdpPort) == sf::Socket::Done) {
        std::string magic;
        sf::Uint16 tcpPort = 0;
        if (reply >> magic >> tcpPort && magic == HOST_REPLY_MAGIC) {
            // Найден хост — подключаемся по TCP. Это единственное блокирующее
            // место во всей сетевой логике, и оно происходит один раз, с
            // коротким таймаутом, чтобы не подвесить игру надолго при сбое.
            socket_.setBlocking(true);
            if (socket_.connect(hostIp, tcpPort, sf::seconds(1.5f)) == sf::Socket::Done) {
                socket_.setBlocking(false);
                connected_ = true;
                discoveryUdp_.unbind();
                return true;
            }
        }
    }
    return false;
}

// ── Обмен данными уровня и состояния кадра ──────────────────────────────

bool NetworkSession::sendLevelInfo(const NetLevelInfo& info) {
    sf::Packet p;
    p << std::string("LEVEL") << info.name
      << info.ballStartX << info.ballStartY
      << info.goalX << info.goalY << info.goalRadius
      << (sf::Uint32)info.walls.size();
    for (const auto& w : info.walls) {
        p << w.x << w.y << w.w << w.h << w.r << w.g << w.b << w.kinematic;
    }
    return socket_.send(p) == sf::Socket::Done;
}

bool NetworkSession::receiveLevelInfo(NetLevelInfo& outInfo) {
    sf::Packet p;
    sf::Socket::Status st = socket_.receive(p);
    if (st != sf::Socket::Done) return false;

    std::string tag;
    p >> tag;
    if (tag != "LEVEL") return false;

    sf::Uint32 wallCount = 0;
    p >> outInfo.name >> outInfo.ballStartX >> outInfo.ballStartY
      >> outInfo.goalX >> outInfo.goalY >> outInfo.goalRadius >> wallCount;

    outInfo.walls.clear();
    outInfo.walls.reserve(wallCount);
    for (sf::Uint32 i = 0; i < wallCount; ++i) {
        NetWallInfo w{};
        p >> w.x >> w.y >> w.w >> w.h >> w.r >> w.g >> w.b >> w.kinematic;
        outInfo.walls.push_back(w);
    }
    return true;
}

bool NetworkSession::sendFrameState(const NetFrameState& state) {
    sf::Packet p;
    p << std::string("STATE")
      << state.ball1X << state.ball1Y << state.ball2X << state.ball2Y
      << state.ball1Reached << state.ball2Reached << state.won
      << (sf::Uint32)state.kinematicWallPositions.size();
    for (const auto& pos : state.kinematicWallPositions)
        p << pos.x << pos.y;

    sf::Socket::Status st = socket_.send(p);
    // Партиальная отправка на маленьких пакетах при локальной сети практически
    // не встречается; если случится — просто теряем этот кадр состояния,
    // следующий придёт через ~16мс и восстановит актуальную картину.
    return st == sf::Socket::Done;
}

bool NetworkSession::receiveFrameState(NetFrameState& outState) {
    sf::Packet p;
    sf::Socket::Status st = socket_.receive(p);
    if (st != sf::Socket::Done) return false;

    std::string tag;
    p >> tag;
    if (tag != "STATE") return false;

    sf::Uint32 wallCount = 0;
    p >> outState.ball1X >> outState.ball1Y >> outState.ball2X >> outState.ball2Y
      >> outState.ball1Reached >> outState.ball2Reached >> outState.won
      >> wallCount;

    outState.kinematicWallPositions.clear();
    outState.kinematicWallPositions.reserve(wallCount);
    for (sf::Uint32 i = 0; i < wallCount; ++i) {
        float x, y;
        p >> x >> y;
        outState.kinematicWallPositions.push_back({x, y});
    }
    return true;
}

bool NetworkSession::sendInput(const NetInputState& input) {
    sf::Packet p;
    p << std::string("INPUT") << input.mouseX << input.mouseY << input.pressed;
    return socket_.send(p) == sf::Socket::Done;
}

bool NetworkSession::receiveInput(NetInputState& outInput) {
    sf::Packet p;
    sf::Socket::Status st = socket_.receive(p);
    if (st != sf::Socket::Done) return false;

    std::string tag;
    p >> tag;
    if (tag != "INPUT") return false;

    p >> outInput.mouseX >> outInput.mouseY >> outInput.pressed;
    return true;
}
