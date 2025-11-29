#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/WindowEnums.hpp>

using sf::CircleShape;
using sf::Color;
using sf::Event;
using sf::RenderWindow;
using sf::VideoMode;
using std::optional;
const int WINDOW_SIZE = 200;
const int INITIAL_WINDOW_SIZE = 5;
int main() {
    RenderWindow windows[INITIAL_WINDOW_SIZE];
    for (int i = 0; i < INITIAL_WINDOW_SIZE; i++) {
        windows[i].create(
            VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
            "SNAKE",
            sf::Style::Titlebar | sf::Style::Close
        );
        windows[i].setFramerateLimit(1);
        windows[i].setPosition({i * WINDOW_SIZE, 0});
    }

    CircleShape shape(100.f);
    shape.setFillColor(Color::Green);
    // window.setVerticalSyncEnabled(true);
    while (windows[0].isOpen()) {
        for (auto& window: windows) {
            if (window.isOpen()) {
                while (const optional event = window.pollEvent()) {
                    if (event->is<Event::Closed>()) {
                        window.close();
                    }
                }
                window.clear();
                window.draw(shape);
                window.display();
            }
        }
    }
}