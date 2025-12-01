#include <SFML/Graphics.hpp>
#include <algorithm>
#include <chrono>
#include <deque>
#include <random>

using sf::Color;
using sf::Event;
using sf::RenderWindow;
using sf::Vector2i;
using sf::VideoMode;
using sf::Keyboard::Scancode;
using std::deque;
using std::optional;

const std::int32_t WINDOW_SIZE = 240;
const std::uint32_t WINDOW_STYLE = sf::Style::Titlebar | sf::Style::Close;
const std::int32_t DESKTOP_SIZE_X =
    sf::VideoMode::getDesktopMode().size.x / WINDOW_SIZE * WINDOW_SIZE;
const std::int32_t DESKTOP_SIZE_Y =
    sf::VideoMode::getDesktopMode().size.y / WINDOW_SIZE * WINDOW_SIZE;
const std::chrono::milliseconds SLEEP_TIME(500);
enum Direction { Up, Down, Left, Right };

std::mt19937 rng(std::random_device {}());
Vector2i find_available_position(const deque<Vector2i>& snake_positions) {
    auto desktop_size = sf::VideoMode::getDesktopMode().size;
    std::uniform_int_distribution<int> dist_x(0, DESKTOP_SIZE_X / WINDOW_SIZE - 1);
    std::uniform_int_distribution<int> dist_y(0, DESKTOP_SIZE_Y / WINDOW_SIZE - 1);
    bool is_position_available = true;
    Vector2i position;
    do {
        is_position_available = true;
        position = Vector2i(WINDOW_SIZE * dist_x(rng), WINDOW_SIZE * dist_y(rng));
        for (const auto& snake_position: snake_positions) {
            is_position_available = is_position_available && (position != snake_position);
        }
    } while (!is_position_available);
    return position;
}
void spawn_food(RenderWindow& food, const deque<Vector2i>& snake_positions) {
    auto position = find_available_position(snake_positions);
    food.create(VideoMode({WINDOW_SIZE, WINDOW_SIZE}), "Food", WINDOW_STYLE);
    food.setFramerateLimit(1);
    food.setPosition(position);

    food.clear(Color::Green);
    food.display();
}
void initialize_windows(
    deque<std::unique_ptr<RenderWindow>>& windows,
    deque<Vector2i>& snake_position
) {
    Vector2i position(DESKTOP_SIZE_X / 2, DESKTOP_SIZE_Y / 2);
    windows.push_back(
        std::make_unique<RenderWindow>(
            VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
            "Snake Head",
            WINDOW_STYLE
        )
    );
    windows[0]->setFramerateLimit(0);
    windows[0]->clear(Color::Red);
    windows[0]->display();
    windows[0]->setPosition(position);
    snake_position.push_back(position);
}
bool try_update_direction(Direction& current_direction, Scancode scancode) {
    switch (scancode) {
        case Scancode::W:
        case Scancode::Up:
            if (current_direction != Down && current_direction != Up) {
                current_direction = Up;
                return true;
            }
            break;
        case Scancode::S:
        case Scancode::Down:
            if (current_direction != Down && current_direction != Up) {
                current_direction = Down;
                return true;
            }
            break;
        case Scancode::A:
        case Scancode::Left:
            if (current_direction != Left && current_direction != Right) {
                current_direction = Left;
                return true;
            }
            break;
        case Scancode::D:
        case Scancode::Right:
            if (current_direction != Left && current_direction != Right) {
                current_direction = Right;
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}
Vector2i get_step(Direction direction) {
    Vector2i step;
    switch (direction) {
        case Up: {
            step = Vector2i(0, -WINDOW_SIZE);
            break;
        }
        case Down: {
            step = Vector2i(0, WINDOW_SIZE);
            break;
        }
        case Left: {
            step = Vector2i(-WINDOW_SIZE, 0);
            break;
        }
        case Right: {
            step = Vector2i(WINDOW_SIZE, 0);
            break;
        }
    }
    return step;
}
void update(
    bool& is_game_running,
    deque<std::unique_ptr<RenderWindow>>& windows,
    deque<Vector2i>& snake_positions,
    RenderWindow& food,
    Direction& current_direction
) {
    if (!is_game_running) {
        return;
    }
    if (windows.empty() || snake_positions.empty() || windows.size() != snake_positions.size()) {
        is_game_running = false;
        return;
    }
    Vector2i last_postion = windows.back()->getPosition();
    Vector2i new_position = windows.front()->getPosition() + get_step(current_direction);
    if (new_position.x < 0 || new_position.x >= DESKTOP_SIZE_X || new_position.y < 0
        || new_position.y >= DESKTOP_SIZE_Y)
    {
        is_game_running = false;
        return;
    }
    snake_positions.pop_back();
    if (std::find(snake_positions.begin(), snake_positions.end(), new_position)
        != snake_positions.end())
    {
        is_game_running = false;
        return;
    }
    snake_positions.push_front(new_position);

    for (int i = 0; i < windows.size(); i++) {
        windows[i]->setPosition(snake_positions[i]);
    }

    if (new_position == food.getPosition()) {
        if (last_postion == new_position) {
            is_game_running = false;
            return;
        }
        spawn_food(food, snake_positions);
        windows.push_back(
            std::make_unique<RenderWindow>(
                VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
                "Body",
                WINDOW_STYLE
            )
        );
        windows.back()->setFramerateLimit(0);
        windows.back()->clear(Color::Yellow);
        windows.back()->display();
        windows.back()->setPosition(last_postion);
        snake_positions.push_back(last_postion);
    }
}
void game_main() {
    bool is_game_running = true;
    Direction current_direction = Right;
    deque<Vector2i> snake_positions;
    deque<std::unique_ptr<RenderWindow>> windows;
    initialize_windows(windows, snake_positions);
    RenderWindow food;
    spawn_food(food, snake_positions);
    auto now = std::chrono::steady_clock::now();
    while (windows[0]->isOpen() && is_game_running) {
        for (const auto& window: windows) {
            if (window->isOpen()) {
                while (const optional event = window->pollEvent()) {
                    if (const auto* key_event = event->getIf<Event::KeyPressed>()) {
                        if (try_update_direction(current_direction, key_event->scancode)) {
                            update(
                                is_game_running,
                                windows,
                                snake_positions,
                                food,
                                current_direction
                            );
                            now = std::chrono::steady_clock::now();
                        }
                    }
                }
            }
        }
        if (food.isOpen()) {
            while (const optional event = food.pollEvent()) {
                if (const auto* key_event = event->getIf<Event::KeyPressed>()) {
                    if (try_update_direction(current_direction, key_event->scancode)) {
                        update(is_game_running, windows, snake_positions, food, current_direction);
                        now = std::chrono::steady_clock::now();
                    }
                }
            }
        }
        if (std::chrono::steady_clock::now() - now > SLEEP_TIME) {
            update(is_game_running, windows, snake_positions, food, current_direction);
            now = std::chrono::steady_clock::now();
        }
    }
    food.close();
    for (auto& window: windows) {
        window->close();
    }
}
int main() {
    game_main();
    return 0;
}
