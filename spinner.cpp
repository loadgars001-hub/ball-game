#include "spinner.hpp"
#include <random>

static std::mt19937& spinnerRng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

static float randRange(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(spinnerRng());
}

Spinner makeRandomSpinner(sf::Vector2f pos) {
    Spinner s;
    s.pos = pos;
    s.width  = randRange(50.f, 90.f);
    s.height = randRange(10.f, 18.f);

    // Вращение в случайную сторону с заметной, но не слишком бешеной скоростью
    float speed = randRange(120.f, 320.f);
    if (randRange(0.f, 1.f) < 0.5f) speed = -speed;
    s.rotationSpeed = speed;

    // Яркая случайная палитра из 4 контрастных цветов для мигания
    auto randColor = [] {
        return sf::Color(
            (sf::Uint8)randRange(80.f, 255.f),
            (sf::Uint8)randRange(80.f, 255.f),
            (sf::Uint8)randRange(80.f, 255.f)
        );
    };
    for (int i = 0; i < 4; ++i) s.colors.push_back(randColor());

    s.blinkInterval = randRange(0.08f, 0.18f);
    s.lifetime = randRange(3.5f, 5.5f);

    return s;
}
