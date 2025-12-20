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
using std::nullopt;
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
    GameOver,
    ShowMenu,
    ShowAbout,
    ShowSettings,
    StartGame,
    Win,
};
struct Button {
    Text text;
    Color highlight_color;
};
struct ClickButton {
    Button button;
    GameState return_state;
};
struct SelectButtons {
    string settings_name;
    Text text;
    vector<Button> buttons;
    int32_t selected_button;
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

    for (int32_t i = 0; i < windows.size(); i++) {
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
GameState game_main() {
    const int32_t win_condition = (DESKTOP_SIZE_X / WINDOW_SIZE) * (DESKTOP_SIZE_Y / WINDOW_SIZE);
    bool is_game_running = true;
    Direction current_direction = Right;
    deque<Vector2i> snake_positions;
    deque<unique_ptr<RenderWindow>> windows;
    initialize_windows(windows, snake_positions);
    RenderWindow food;
    spawn_food(food, snake_positions);
    auto now = std::chrono::steady_clock::now();
    while (is_game_running) {
        if (snake_positions.size() >= win_condition) {
            break;
        }
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
    if (is_game_running) {
        return Win;
    } else {
        return GameOver;
    }
}
void read_config(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        return;
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
}
void reload_config() {
    auto snake_body_size = config["snake_body_size"];
    WINDOW_SIZE = 240;
    if (!snake_body_size.empty()) {
        if (snake_body_size == "small") {
            WINDOW_SIZE = 180;
        } else if (snake_body_size == "large") {
            WINDOW_SIZE = 360;
        }
    } else {
        snake_body_size = "medium";
    }
    DESKTOP_SIZE_X = VideoMode::getDesktopMode().size.x / WINDOW_SIZE * WINDOW_SIZE;
    DESKTOP_SIZE_Y = VideoMode::getDesktopMode().size.y / WINDOW_SIZE * WINDOW_SIZE;
    std::cout << "window_size: " << WINDOW_SIZE << 'x' << WINDOW_SIZE << std::endl;
    std::cout << "desktop_size: " << DESKTOP_SIZE_X << 'x' << DESKTOP_SIZE_Y << std::endl;
}
void game_start() {
    read_config(CONFIG_NAME);
    reload_config();
}
Text get_text(
    const Font& font,
    const string& content,
    int32_t size,
    Vector2u window_size,
    float x_factor,
    float y_factor
) {
    Text text(font, content, size);
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
    for (int32_t i = 0; fontPaths[i]; i++) {
        if (std::filesystem::exists(fontPaths[i]) && font.openFromFile(fontPaths[i])) {
            return true;
        }
    }

    return false;
}
void update_setting(const SelectButtons& select_buttons) {
    const Button& selected_button = select_buttons.buttons[select_buttons.selected_button];
    string value = selected_button.text.getString();
    value[0] = tolower(value[0]);
    config[select_buttons.settings_name] = value;

    reload_config();
}
optional<GameState> handle_click(
    const Event::MouseButtonPressed* mouse_pressed_event,
    vector<ClickButton>& buttons,
    Vector2f mouse_position_float
) {
    if (mouse_pressed_event->button == sf::Mouse::Button::Left) {
        for (auto& click_button: buttons) {
            if (click_button.button.text.getGlobalBounds().contains(mouse_position_float)) {
                return click_button.return_state;
            }
        }
    }
    return nullopt;
}
void handle_click(
    const Event::MouseButtonPressed* mouse_pressed_event,
    SelectButtons& select_buttons,
    Vector2f mouse_position_float
) {
    if (mouse_pressed_event->button == sf::Mouse::Button::Left) {
        for (int32_t i = 0; i < select_buttons.buttons.size(); i++) {
            if (select_buttons.buttons[i].text.getGlobalBounds().contains(mouse_position_float)) {
                select_buttons.selected_button = i;
                update_setting(select_buttons);
            }
        }
    }
}
void handle_hover(Button& button, Vector2f mouse_position_float) {
    if (button.text.getGlobalBounds().contains(mouse_position_float)) {
        button.text.setFillColor(button.highlight_color);
        button.text.setScale({1.1f, 1.1f});
    } else {
        button.text.setFillColor(Color::White);
        button.text.setScale({1.0f, 1.0f});
    }
}
void handle_hover(SelectButtons& select_button, Vector2f mouse_position_float) {
    for (auto& button: select_button.buttons) {
        handle_hover(button, mouse_position_float);
    }
    Button& selected_button = select_button.buttons[select_button.selected_button];
    selected_button.text.setFillColor(selected_button.highlight_color);
    selected_button.text.setScale({1.1f, 1.1f});
}
void handle_hover(vector<ClickButton>& buttons, Vector2f mouse_position_float) {
    for (auto& button: buttons) {
        handle_hover(button.button, mouse_position_float);
    }
}
Text get_title(string text, Font& font, Vector2u window_size) {
    const int32_t TITLE_TEXT_SIZE = 60;
    const Color TITLE_TEXT_COLOR = Color::Yellow;
    const float TITLE_TEXT_X_FACTOR = 0.5;
    const float TITLE_TEXT_Y_FACTOR = 0.15;
    Text title_text = get_text(
        font,
        text,
        TITLE_TEXT_SIZE,
        window_size,
        TITLE_TEXT_X_FACTOR,
        TITLE_TEXT_Y_FACTOR
    );
    title_text.setStyle(Text::Bold);
    return title_text;
}
Text get_about_text(Font& font, Vector2u window_size) {
    const string ABOUT_TEXT =
        "Snake Game.\n"
        "Control the snake to eat food and grow.\n"
        "Controls:\n"
        "- WASD or Arrow Keys to Move.\n";
    const int32_t ABOUT_TEXT_SIZE = 40;
    const float ABOUT_TEXT_X_FACTOR = 0.5f;
    const float ABOUT_TEXT_Y_FACTOR = 0.45f;
    return get_text(
        font,
        ABOUT_TEXT,
        ABOUT_TEXT_SIZE,
        window_size,
        ABOUT_TEXT_X_FACTOR,
        ABOUT_TEXT_Y_FACTOR
    );
}
Button get_normal_button(Font& font, Vector2u window_size, const string& text, float y_factor) {
    const int32_t BUTTON_SIZE = 40;
    const Color BUTTON_HIGHLIGHT_COLOR = Color::Green;
    const float BUTTON_X_FACTOR = 0.5;
    Text button = get_text(font, text, BUTTON_SIZE, window_size, BUTTON_X_FACTOR, y_factor);
    return {button, BUTTON_HIGHLIGHT_COLOR};
}
ClickButton get_start_button(Font& font, Vector2u window_size) {
    const string START_BUTTON_TEXT = "Start";
    const float START_BUTTON_Y_FACTOR = 0.45;
    Button start_button =
        get_normal_button(font, window_size, START_BUTTON_TEXT, START_BUTTON_Y_FACTOR);
    return {start_button, GameState::StartGame};
}
ClickButton get_try_again_button(Font& font, Vector2u window_size) {
    const string START_BUTTON_TEXT = "Try Again";
    const float START_BUTTON_Y_FACTOR = 0.45;
    Button start_button =
        get_normal_button(font, window_size, START_BUTTON_TEXT, START_BUTTON_Y_FACTOR);
    return {start_button, GameState::StartGame};
}
ClickButton get_settings_button(Font& font, Vector2u window_size) {
    const string SETTINGS_BUTTON_TEXT = "Settings";
    const float SETTINGS_BUTTON_Y_FACTOR = 0.55;
    Button setting_button =
        get_normal_button(font, window_size, SETTINGS_BUTTON_TEXT, SETTINGS_BUTTON_Y_FACTOR);
    return {setting_button, GameState::ShowSettings};
}
ClickButton get_about_button(Font& font, Vector2u window_size) {
    const string ABOUT_BUTTON = "About";
    const float ABOUT_BUTTON_Y_FACTOR = 0.65;
    Button about_button = get_normal_button(font, window_size, ABOUT_BUTTON, ABOUT_BUTTON_Y_FACTOR);
    return {about_button, GameState::ShowAbout};
}
ClickButton get_exit_button(Font& font, Vector2u window_size) {
    const string EXIT_BUTTON = "Exit";
    const Color EXIT_BUTTON_HIGHLIGHT_COLOR = Color::Red;
    const float EXIT_BUTTON_Y_FACTOR = 0.75;
    Button exit_button = get_normal_button(font, window_size, EXIT_BUTTON, EXIT_BUTTON_Y_FACTOR);
    exit_button.highlight_color = EXIT_BUTTON_HIGHLIGHT_COLOR;
    return {exit_button, GameState::Exit};
}
ClickButton get_return_button(Font& font, Vector2u window_size) {
    const string RETURN_BUTTON = "Return";
    const Color RETURN_BUTTON_HIGHLIGHT_COLOR = Color::Red;
    const float RETURN_BUTTON_Y_FACTOR = 0.75;
    Button return_button = get_normal_button(
        font,
        window_size,
        RETURN_BUTTON,
        RETURN_BUTTON_Y_FACTOR

    );
    return_button.highlight_color = RETURN_BUTTON_HIGHLIGHT_COLOR;
    return {return_button, GameState::ShowMenu};
}
SelectButtons get_snake_size_button(Font& font, Vector2u window_size) {
    const string SNAKE_BODY_SIZE_TEXT = "Snake Size";
    const float SNAKE_BODY_SIZE_TEXT_SIZE = 40;
    const float SNAKE_BODY_SIZE_TEXT_X_FACTOR = 0.5;
    const float SNAKE_BODY_SIZE_TEXT_Y_FACTOR = 0.25;
    Text snake_body_size_text = get_text(
        font,
        SNAKE_BODY_SIZE_TEXT,
        SNAKE_BODY_SIZE_TEXT_SIZE,
        window_size,
        SNAKE_BODY_SIZE_TEXT_X_FACTOR,
        SNAKE_BODY_SIZE_TEXT_Y_FACTOR
    );

    string SNAKE_BODY_SIZE_BUTTON[3] = {"Small", "Medium", "Large"};
    const float SNAKE_BODY_SIZE_BUTTON_SIZE = 40;
    const Color SNAKE_BODY_SIZE_BUTTON_HIGHLIGHT_COLOR = Color::Green;
    const float SNAKE_BODY_SIZE_BUTTON_X_FACTOR = 0.25;
    const float SNAKE_BODY_SIZE_BUTTON_Y_FACTOR = 0.35;
    vector<Button> snake_body_size_buttons;
    const string& SNAKE_BODY_SIZE_BUTTON_SETTINGS_NAME = "snake_body_size";
    const string& snake_body_size = config[SNAKE_BODY_SIZE_BUTTON_SETTINGS_NAME];
    // Default is 1, which is medium
    int32_t selected_button = 1;
    for (int32_t i = 0; i < 3; i++) {
        Text text = get_text(
            font,
            SNAKE_BODY_SIZE_BUTTON[i],
            SNAKE_BODY_SIZE_BUTTON_SIZE,
            window_size,
            SNAKE_BODY_SIZE_BUTTON_X_FACTOR * (i + 1),
            SNAKE_BODY_SIZE_BUTTON_Y_FACTOR
        );
        snake_body_size_buttons.push_back({text, SNAKE_BODY_SIZE_BUTTON_HIGHLIGHT_COLOR});
        SNAKE_BODY_SIZE_BUTTON[i][0] = tolower(SNAKE_BODY_SIZE_BUTTON[i][0]);
        if (snake_body_size == SNAKE_BODY_SIZE_BUTTON[i]) {
            selected_button = i;
        }
    }
    return {
        SNAKE_BODY_SIZE_BUTTON_SETTINGS_NAME,
        snake_body_size_text,
        snake_body_size_buttons,
        selected_button
    };
}
GameState handle_window_event(
    RenderWindow& window,
    const vector<Text>& texts,
    vector<ClickButton>& buttons,
    vector<SelectButtons> select_buttons = {}
) {
    while (window.isOpen()) {
        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                return GameState::Exit;
            }
            Vector2i mouse_position = sf::Mouse::getPosition(window);
            Vector2f mouse_position_float(
                static_cast<float>(mouse_position.x),
                static_cast<float>(mouse_position.y)
            );
            if (const auto* mouse_pressed_event = event->getIf<Event::MouseButtonPressed>()) {
                if (optional state =
                        handle_click(mouse_pressed_event, buttons, mouse_position_float))
                {
                    return *state;
                }
                for (auto& select_button: select_buttons) {
                    handle_click(mouse_pressed_event, select_button, mouse_position_float);
                }
            }
            handle_hover(buttons, mouse_position_float);
            for (auto& select_button: select_buttons) {
                handle_hover(select_button, mouse_position_float);
            }
        }
        window.clear(Color::Black);
        for (auto& click_button: buttons) {
            window.draw(click_button.button.text);
        }
        for (auto& select_button: select_buttons) {
            window.draw(select_button.text);
            for (auto& button: select_button.buttons) {
                window.draw(button.text);
            }
        }
        for (auto text: texts) {
            window.draw(text);
        }

        window.display();
    }
    return GameState::Exit;
}
GameState show_menu(Font& font, RenderWindow& window) {
    Text title_text = get_title("Snake", font, window.getSize());
    vector<Text> texts = {
        title_text,
    };

    ClickButton start_button = get_start_button(font, window.getSize());
    ClickButton settings_button = get_settings_button(font, window.getSize());
    ClickButton about_button = get_about_button(font, window.getSize());
    ClickButton exit_button = get_exit_button(font, window.getSize());
    vector<ClickButton> buttons = {start_button, settings_button, about_button, exit_button};
    return handle_window_event(window, texts, buttons);
}
GameState show_about(Font& font, RenderWindow& window) {
    Text title_text = get_title("About", font, window.getSize());
    Text about_text = get_about_text(font, window.getSize());
    vector<Text> texts = {title_text, about_text};

    ClickButton return_button = get_return_button(font, window.getSize());
    vector<ClickButton> buttons = {return_button};
    return handle_window_event(window, texts, buttons);
}
GameState show_settings(Font& font, RenderWindow& window, GameState last_state) {
    Text title_text = get_title("Settings", font, window.getSize());
    vector<Text> texts = {title_text};

    ClickButton return_button = get_return_button(font, window.getSize());
    return_button.return_state = last_state;
    vector<ClickButton> click_buttons = {return_button};

    SelectButtons snake_size_button = get_snake_size_button(font, window.getSize());
    vector<SelectButtons> select_buttons = {snake_size_button};

    return handle_window_event(window, texts, click_buttons, select_buttons);
}
GameState show_game_over(Font& font, RenderWindow& window) {
    Text title_text = get_title("Game Over", font, window.getSize());
    vector<Text> texts = {
        title_text,
    };

    ClickButton try_again_button = get_try_again_button(font, window.getSize());
    ClickButton settings_button = get_settings_button(font, window.getSize());
    ClickButton exit_button = get_exit_button(font, window.getSize());
    vector<ClickButton> buttons = {try_again_button, settings_button, exit_button};
    return handle_window_event(window, texts, buttons);
}
GameState show_win(Font& font, RenderWindow& window) {
    Text title_text = get_title("You Win!", font, window.getSize());
    vector<Text> texts = {
        title_text,
    };

    ClickButton try_again_button = get_try_again_button(font, window.getSize());
    ClickButton settings_button = get_settings_button(font, window.getSize());
    ClickButton exit_button = get_exit_button(font, window.getSize());
    vector<ClickButton> buttons = {try_again_button, settings_button, exit_button};
    return handle_window_event(window, texts, buttons);
}
GameState should_start(Font& font, GameState current_state) {
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

    GameState last_state = current_state;
    while (true) {
        switch (current_state) {
            case Exit: {
                window.close();
                return GameState::Exit;
            }
            case StartGame: {
                window.close();
                return GameState::StartGame;
            }
            case ShowMenu: {
                current_state = show_menu(font, window);
                last_state = GameState::ShowMenu;
                break;
            }
            case ShowAbout: {
                current_state = show_about(font, window);
                last_state = GameState::ShowAbout;
                break;
            }
            case ShowSettings: {
                current_state = show_settings(font, window, last_state);
                last_state = GameState::ShowSettings;
                break;
            }
            case GameOver: {
                current_state = show_game_over(font, window);
                last_state = GameState::GameOver;
                break;
            }
            case Win: {
                current_state = show_win(font, window);
                last_state = GameState::Win;
                break;
            }
            default: {
                window.close();
                std::cerr << "Error: Invalid game state" << std::endl;
                return GameState::Exit;
            }
        }
    }
}
void write_temp_file(ofstream& temp_file) {
    const string SNAKE_BODY_SIZE_COMMENT = "# Optional values: small, medium, large";
    string snake_body_size = config["snake_body_size"];
    if (snake_body_size.empty()) {
        snake_body_size = "medium";
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
int32_t main() {
    Font font;
    game_start();
    GameState state = GameState::ShowMenu;
    while (true) {
        state = should_start(font, state);
        if (state == GameState::StartGame) {
            state = game_main();
        } else if (state == GameState::Exit) {
            break;
        }
    }
    game_end();
    return 0;
}
