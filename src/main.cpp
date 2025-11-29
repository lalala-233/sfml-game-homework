#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>

using sf::CircleShape;
using sf::Color;
using sf::Event;
using sf::RenderWindow;
using sf::VideoMode;
using std::optional;
#define WINDOW_SIZE 200
int main() {
    RenderWindow windows[5];
    for (int i = 0; i < 5; i++) {
        windows[i].create(
            VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
            "SFML works!",
            sf::Style::Titlebar | sf::Style::Close
        );
        windows[i].setFramerateLimit(10);
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