#include <SFML/Graphics.hpp>
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

using sf::Color;
using sf::Event;
using sf::Font;
using sf::RenderWindow;
using sf::State;
using sf::Text;
using sf::Vector2f;
using sf::Vector2i;
using sf::Vector2u;
using sf::VideoMode;
using sf::Keyboard::Scancode;
using std::deque;
using std::ifstream;
using std::map;
using std::ofstream;
using std::optional;
using std::string;
using std::unique_ptr;
using std::vector;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

uint32_t WINDOW_SIZE = 240;
int32_t DESKTOP_SIZE_X = VideoMode::getDesktopMode().size.x / WINDOW_SIZE * WINDOW_SIZE;
int32_t DESKTOP_SIZE_Y = VideoMode::getDesktopMode().size.y / WINDOW_SIZE * WINDOW_SIZE;
map<string, string> config;
const string CONFIG_NAME = "snake.conf";
const uint32_t WINDOW_STYLE = sf::Style::Titlebar | sf::Style::Close;
const milliseconds SLEEP_TIME(500);
enum Direction { Up, Down, Left, Right };
enum GameState {
    Exit,
    StartGame,
    GameOver,
    ShowMenu,
};
struct Button {
    Text self;
    Color highlight_color;
    GameState state;
};
std::mt19937 rng(std::random_device {}());

Vector2i find_available_position(const deque<Vector2i>& snake_positions) {
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
    std::cout << "food position: " << position.x << 'x' << position.y << std::endl;
    food.clear(Color::Green);
    food.display();
}
void initialize_windows(
    deque<std::unique_ptr<RenderWindow>>& windows,
    deque<Vector2i>& snake_position
) {
    // make sure position is valid
    Vector2i position(
        DESKTOP_SIZE_X / WINDOW_SIZE / 2 * WINDOW_SIZE,
        DESKTOP_SIZE_Y / WINDOW_SIZE / 2 * WINDOW_SIZE
    );
    windows.push_back(
        std::make_unique<RenderWindow>(
            VideoMode({WINDOW_SIZE, WINDOW_SIZE}),
            "Snake Head",
            WINDOW_STYLE
        )
    );
    std::cout << "snake_head position: " << position.x << 'x' << position.y << std::endl;
    windows[0]->setFramerateLimit(0);
    windows[0]->clear(Color::Red);
    windows[0]->display();
    windows[0]->setPosition(position);
    snake_position.push_back(position);
}
// Returns true if current_direction updates
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

    // A new method to proceed the snake movement
    // Useful for some PC which have flickering issue
    // But it makes movement more weird
    //
    // if (windows.size() > 1) {
    //     auto head = std::move(windows.front());
    //     auto tail = std::move(windows.back());
    //     windows.pop_back();
    //     windows.pop_front();
    //     windows.push_front(std::move(tail));
    //     windows.push_front(std::move(head));
    //     windows[1]->setPosition(snake_positions[1]);
    // }
    // windows[0]->setPosition(snake_positions[0]);

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
// Returns true if current direction updates
bool try_update_current_direction(
    const deque<unique_ptr<RenderWindow>>& windows,
    const deque<Vector2i>& snake_positions,
    RenderWindow& food,
    Direction& current_direction
) {
    for (const auto& window: windows) {
        if (window->isOpen()) {
            while (const optional event = window->pollEvent()) {
                if (const auto* key_event = event->getIf<Event::KeyPressed>()) {
                    if (try_update_direction(current_direction, key_event->scancode)) {
                        return true;
                    }
                }
            }
        }
    }
    if (food.isOpen()) {
        while (const optional event = food.pollEvent()) {
            if (const auto* key_event = event->getIf<Event::KeyPressed>()) {
                if (try_update_direction(current_direction, key_event->scancode)) {
                    return true;
                }
            }
        }
    }
    return false;
}
void game_main() {
    bool is_game_running = true;
    Direction current_direction = Right;
    deque<Vector2i> snake_positions;
    deque<unique_ptr<RenderWindow>> windows;
    initialize_windows(windows, snake_positions);
    RenderWindow food;
    spawn_food(food, snake_positions);
    auto now = std::chrono::steady_clock::now();
    while (is_game_running) {
        if (std::chrono::steady_clock::now() - now > SLEEP_TIME) {
            update(is_game_running, windows, snake_positions, food, current_direction);
            now = std::chrono::steady_clock::now();
            continue;
        }
        if (try_update_current_direction(windows, snake_positions, food, current_direction)) {
            update(is_game_running, windows, snake_positions, food, current_direction);
            now = std::chrono::steady_clock::now();
        }
    }
    food.close();
    for (auto& window: windows) {
        window->close();
    }
}
map<string, string> read_config(const string& filename) {
    ifstream file(filename);
    map<string, string> config;
    if (!file.is_open()) {
        return config;
    }
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            config[key] = value;
        }
    }
    return config;
}
void game_start() {
    config = read_config(CONFIG_NAME);
    auto snake_body_size = config["snake_body_size"];
    if (!snake_body_size.empty()) {
        if (snake_body_size == "\"small\"") {
            WINDOW_SIZE = 180;
        } else if (snake_body_size == "\"large\"") {
            WINDOW_SIZE = 360;
        } else {
            WINDOW_SIZE = 240;
        }
    }
    DESKTOP_SIZE_X = VideoMode::getDesktopMode().size.x / WINDOW_SIZE * WINDOW_SIZE;
    DESKTOP_SIZE_Y = VideoMode::getDesktopMode().size.y / WINDOW_SIZE * WINDOW_SIZE;
    std::cout << "window_size: " << WINDOW_SIZE << 'x' << WINDOW_SIZE << std::endl;
    std::cout << "desktop_size: " << DESKTOP_SIZE_X << 'x' << DESKTOP_SIZE_Y << std::endl;
}
Text get_text(
    const Font& font,
    const string& content,
    int32_t size,
    Color color,
    Vector2u window_size,
    float x_factor,
    float y_factor
) {
    Text text(font, content, size);
    text.setFillColor(color);
    sf::FloatRect titleBounds = text.getLocalBounds();
    text.setOrigin(
        {titleBounds.getCenter().x / 2 + static_cast<int32_t>(content.length() * size / 8),
         titleBounds.getCenter().y / 2}
    );
    text.setPosition({window_size.x * x_factor, window_size.y * y_factor});
    return text;
}
// Returns true if the font was loaded successfully
bool load_system_font(Font& font) {
#ifdef _WIN32
    const char* fontPaths[] = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simsun.ttc",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        nullptr
    };
#elif __APPLE__
    const char* fontPaths[] = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Arial.ttf",
        nullptr
    };
#elif __linux__
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        // ArchLinux
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/dejavu/DejaVuSans.ttf",
        nullptr
    };
#endif
    for (int i = 0; fontPaths[i]; i++) {
        if (font.openFromFile(fontPaths[i])) {
            return true;
        }
    }

    return false;
}
// Returns true if user choose to play game
GameState show_menu(RenderWindow& window, const Text& title, vector<Button>& buttons) {
    while (window.isOpen()) {
        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                return Exit;
            }
            Vector2i mouse_position = sf::Mouse::getPosition(window);
            Vector2f mouse_position_float(
                static_cast<float>(mouse_position.x),
                static_cast<float>(mouse_position.y)
            );
            if (const auto* mouse_pressed_event = event->getIf<Event::MouseButtonPressed>()) {
                if (mouse_pressed_event->button == sf::Mouse::Button::Left) {
                    for (auto& button: buttons) {
                        if (button.self.getGlobalBounds().contains(mouse_position_float)) {
                            return button.state;
                        }
                    }
                }
            }

            for (auto& button: buttons) {
                if (button.self.getGlobalBounds().contains(mouse_position_float)) {
                    button.self.setFillColor(button.highlight_color);
                    button.self.setScale({1.1f, 1.1f});
                } else {
                    button.self.setFillColor(Color::White);
                    button.self.setScale({1.0f, 1.0f});
                }
            }
        }
        window.clear(Color::Transparent);
        for (auto& button: buttons) {
            window.draw(button.self);
        }
        window.draw(title);

        // 显示
        window.display();
    }
    return Exit;
}
GameState should_start(Font& font) {
    RenderWindow window(
        sf::VideoMode(VideoMode::getDesktopMode().size),
        "Snake",
        State::Fullscreen
    );
    window.setFramerateLimit(60);

    bool is_first_run = font.getInfo().family.empty();
    if (is_first_run) {
        if (!load_system_font(font)) {
            return GameState::Exit;
        }
    }

    string TITLE_TEXT;
    if (is_first_run) {
        TITLE_TEXT = "Snake";
    } else {
        TITLE_TEXT = "You Lose!";
    }
    const int32_t TITLE_TEXT_SIZE = 60;
    const Color TITLE_TEXT_COLOR = Color::Yellow;
    const float TITLE_TEXT_X_FACTOR = 0.5;
    const float TITLE_TEXT_Y_FACTOR = 0.15;
    Text title_text = get_text(
        font,
        TITLE_TEXT,
        TITLE_TEXT_SIZE,
        TITLE_TEXT_COLOR,
        window.getSize(),
        TITLE_TEXT_X_FACTOR,
        TITLE_TEXT_Y_FACTOR
    );
    title_text.setStyle(Text::Bold);

    const string START_BUTTON = "Start";
    const int32_t START_BUTTON_SIZE = 40;
    const Color START_BUTTON_HIGHLIGHT_COLOR = Color::Green;
    const float START_BUTTON_X_FACTOR = 0.5;
    const float START_BUTTON_Y_FACTOR = 0.45;
    Text start_button = get_text(
        font,
        START_BUTTON,
        START_BUTTON_SIZE,
        START_BUTTON_HIGHLIGHT_COLOR,
        window.getSize(),
        START_BUTTON_X_FACTOR,
        START_BUTTON_Y_FACTOR
    );

    const string EXIT_BUTTON = "Exit";
    const int32_t EXIT_BUTTON_SIZE = 40;
    const Color EXIT_BUTTON_HIGHLIGHT_COLOR = Color::Red;
    const float EXIT_BUTTON_X_FACTOR = 0.5;
    const float EXIT_BUTTON_Y_FACTOR = 0.55;
    Text exit_button = get_text(
        font,
        EXIT_BUTTON,
        EXIT_BUTTON_SIZE,
        EXIT_BUTTON_HIGHLIGHT_COLOR,
        window.getSize(),
        EXIT_BUTTON_X_FACTOR,
        EXIT_BUTTON_Y_FACTOR
    );
    vector<Button> buttons = {
        {start_button, START_BUTTON_HIGHLIGHT_COLOR, GameState::StartGame},
        {exit_button, EXIT_BUTTON_HIGHLIGHT_COLOR, GameState::Exit}
    };
    while (true) {
        GameState current_state = show_menu(window, title_text, buttons);
        switch (current_state) {
            case GameState::StartGame: {
                window.close();
                return GameState::StartGame;
            }
            case GameState::Exit: {
                window.close();
                return GameState::Exit;
            }
            case GameState::GameOver: {
                window.close();
                std::cerr << "Unexpected state: GameOver" << std::endl;
                return GameState::Exit;
                break;
            }
            case GameState::ShowMenu: {
                window.close();
                std::cerr << "Unexpected state: ShowMenu" << std::endl;
                return GameState::Exit;
                break;
            }
        }
    }
}
void write_temp_file(ofstream& temp_file) {
    const string SNAKE_BODY_SIZE_COMMENT = "# Optional values: small, medium, large";
    string snake_body_size = config["snake_body_size"];
    if (snake_body_size.empty()) {
        snake_body_size = "\"medium\"";
    }
    temp_file << SNAKE_BODY_SIZE_COMMENT << std::endl;
    temp_file << "snake_body_size=" << snake_body_size << std::endl;
    temp_file.close();
}
void game_end() {
    const string TEMP_FILE = CONFIG_NAME + ".temp";
    const string BACKUP_FILE = CONFIG_NAME + ".bak";
    ofstream temp_file(TEMP_FILE);
    if (!temp_file) {
        std::cerr << "cannot create temp file" << std::endl;
        return;
    }
    write_temp_file(temp_file);
    if (!temp_file.good()) {
        std::filesystem::remove(TEMP_FILE);
        std::cerr << "cannot write temp file" << std::endl;
        return;
    }

    bool has_original = std::filesystem::exists(CONFIG_NAME);
    if (has_original) {
        std::filesystem::rename(CONFIG_NAME, BACKUP_FILE);
    }
    std::filesystem::rename(TEMP_FILE, CONFIG_NAME);
    if (has_original) {
        std::filesystem::remove(BACKUP_FILE);
    }
}
int main() {
    Font font;
    game_start();
    while (true) {
        GameState state = should_start(font);
        if (state == GameState::StartGame) {
            game_main();
        } else if (state == GameState::Exit) {
            break;
        }
    }
    game_end();
    return 0;
}
