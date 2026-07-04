#include "raylib.h"
#include <algorithm>
#include <cmath>
enum class GameState
{
    MAIN_MENU,
    DIFFICULTY_MENU,
    SETTINGS,
    PLAYING,
    PAUSED,
    GAME_OVER
};
enum class GameMode
{
    VS_AI,
    TWO_PLAYERS
};
enum class AIDifficulty
{
    EASY,
    MEDIUM,
    HARD
};
struct Settings
{
    float sensitivity = 1.0f;
    AIDifficulty difficulty = AIDifficulty::MEDIUM;
};
struct AIProfile
{
    float speed;
    float reactionTime;
    float aimError;
    float returnSpeedScale;
    bool predictBall;
};
struct GameData
{
    Rectangle leftPaddle{};
    Rectangle rightPaddle{};
    Vector2 ballPosition{};
    Vector2 ballVelocity{};
    int leftScore = 0;
    int rightScore = 0;
    int winner = 0;
    int serveDirection = 1;
    float serveTimer = 0.0f;
    float aiReactionTimer = 0.0f;
    float aiTargetY = 0.0f;
};
constexpr int SCREEN_WIDTH = 1000;
constexpr int SCREEN_HEIGHT = 600;
constexpr float BALL_RADIUS = 11.0f;
constexpr float BASE_PADDLE_SPEED = 430.0f;
constexpr float MAX_BALL_SPEED_X = 720.0f;
constexpr int WIN_SCORE = 5;
const Color BACKGROUND_TOP = {13, 17, 28, 255};
const Color BACKGROUND_BOTTOM = {26, 34, 54, 255};
const Color PANEL_COLOR = {22, 29, 46, 242};
const Color ACCENT_COLOR = {68, 214, 160, 255};
const Color ACCENT_DARK = {35, 125, 98, 255};
const Color TEXT_MUTED = {158, 168, 190, 255};
const Color BUTTON_COLOR = {38, 48, 70, 255};
const Color BUTTON_SELECTED = {54, 73, 101, 255};
float ClampFloat(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}
const char *DifficultyName(AIDifficulty difficulty)
{
    switch (difficulty)
    {
        case AIDifficulty::EASY:
            return "Easy";
        case AIDifficulty::HARD:
            return "Hard";
        case AIDifficulty::MEDIUM:
        default:
            return "Medium";
    }
}
AIProfile GetAIProfile(AIDifficulty difficulty)
{
    switch (difficulty)
    {
        case AIDifficulty::EASY:
            return {255.0f, 0.22f, 72.0f, 0.38f, false};
        case AIDifficulty::HARD:
            return {455.0f, 0.035f, 8.0f, 0.72f, true};
        case AIDifficulty::MEDIUM:
        default:
            return {350.0f, 0.10f, 30.0f, 0.52f, true};
    }
}
void DrawTextAt(Font font, const char *text, float x, float y, float fontSize, Color color)
{
    DrawTextEx(font, text, {x, y}, fontSize, 1.0f, color);
}
void DrawCenteredText(Font font, const char *text, float y, float fontSize, Color color)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, 1.0f);
    DrawTextEx(
        font,
        text,
        {(SCREEN_WIDTH - size.x) * 0.5f, y},
        fontSize,
        1.0f,
        color
    );
}
void DrawBackground()
{
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_TOP, BACKGROUND_BOTTOM);
    for (int x = -SCREEN_HEIGHT; x < SCREEN_WIDTH; x += 90)
    {
        DrawLineEx(
            {(float)x, 0.0f},
            {(float)x + SCREEN_HEIGHT, (float)SCREEN_HEIGHT},
            2.0f,
            Fade(RAYWHITE, 0.018f)
        );
    }
}
Rectangle MenuButtonRectangle(int index, float startY, float width = 430.0f)
{
    constexpr float height = 52.0f;
    constexpr float gap = 12.0f;
    return {
        (SCREEN_WIDTH - width) * 0.5f,
        startY + index * (height + gap),
        width,
        height
    };
}
void DrawMenuButton(Font font, const char *text, Rectangle rectangle, bool selected)
{
    DrawRectangleRounded(
        rectangle,
        0.18f,
        8,
        selected ? BUTTON_SELECTED : BUTTON_COLOR
    );
    if (selected)
    {
        DrawRectangleRoundedLines(rectangle, 0.18f, 8, ACCENT_COLOR);
        DrawRectangle(
            (int)rectangle.x,
            (int)rectangle.y + 10,
            5,
            (int)rectangle.height - 20,
            ACCENT_COLOR
        );
    }
    Vector2 textSize = MeasureTextEx(font, text, 25.0f, 1.0f);
    DrawTextEx(
        font,
        text,
        {
            rectangle.x + (rectangle.width - textSize.x) * 0.5f,
            rectangle.y + (rectangle.height - textSize.y) * 0.5f - 1.0f
        },
        25.0f,
        1.0f,
        selected ? RAYWHITE : LIGHTGRAY
    );
}
bool UpdateMenuSelection(int &selected, int itemCount, float startY, float width = 430.0f)
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        selected = (selected - 1 + itemCount) % itemCount;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        selected = (selected + 1) % itemCount;
    }
    Vector2 mousePosition = GetMousePosition();
    bool clicked = false;
    for (int i = 0; i < itemCount; ++i)
    {
        Rectangle rectangle = MenuButtonRectangle(i, startY, width);
        if (CheckCollisionPointRec(mousePosition, rectangle))
        {
            selected = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                clicked = true;
            }
        }
    }
    return clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}
void ClampPaddle(Rectangle &paddle)
{
    paddle.y = ClampFloat(paddle.y, 0.0f, SCREEN_HEIGHT - paddle.height);
}
void ResetPaddles(GameData &game)
{
    game.leftPaddle = {
        42.0f,
        SCREEN_HEIGHT * 0.5f - 60.0f,
        18.0f,
        120.0f
    };
    game.rightPaddle = {
        SCREEN_WIDTH - 60.0f,
        SCREEN_HEIGHT * 0.5f - 60.0f,
        18.0f,
        120.0f
    };
}
void StartServe(GameData &game, int direction)
{
    game.ballPosition = {SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f};
    game.ballVelocity = {0.0f, 0.0f};
    game.serveDirection = direction;
    game.serveTimer = 1.0f;
    game.aiReactionTimer = 0.0f;
    game.aiTargetY = SCREEN_HEIGHT * 0.5f;
}
void StartMatch(GameData &game)
{
    game.leftScore = 0;
    game.rightScore = 0;
    game.winner = 0;
    ResetPaddles(game);
    StartServe(game, GetRandomValue(0, 1) == 0 ? -1 : 1);
}
void LaunchBall(GameData &game)
{
    float verticalSpeed = static_cast<float>(GetRandomValue(-190, 190));
    if (std::fabs(verticalSpeed) < 75.0f)
    {
        verticalSpeed = verticalSpeed < 0.0f ? -105.0f : 105.0f;
    }
    game.ballVelocity = {
        370.0f * game.serveDirection,
        verticalSpeed
    };
}
float PredictBallY(const GameData &game, float targetX)
{
    if (game.ballVelocity.x <= 0.0f)
    {
        return SCREEN_HEIGHT * 0.5f;
    }
    float travelTime = (targetX - game.ballPosition.x) / game.ballVelocity.x;
    if (travelTime <= 0.0f)
    {
        return game.ballPosition.y;
    }
    const float minimumY = BALL_RADIUS;
    const float maximumY = SCREEN_HEIGHT - BALL_RADIUS;
    const float range = maximumY - minimumY;
    float predictedY = game.ballPosition.y + game.ballVelocity.y * travelTime;
    float wrapped = std::fmod(predictedY - minimumY, range * 2.0f);
    if (wrapped < 0.0f)
    {
        wrapped += range * 2.0f;
    }
    if (wrapped > range)
    {
        wrapped = range * 2.0f - wrapped;
    }
    return minimumY + wrapped;
}
void UpdateAI(GameData &game, const Settings &settings, float deltaTime)
{
    AIProfile profile = GetAIProfile(settings.difficulty);
    float paddleCenter = game.rightPaddle.y + game.rightPaddle.height * 0.5f;
    if (game.ballVelocity.x > 0.0f && game.serveTimer <= 0.0f)
    {
        game.aiReactionTimer -= deltaTime;
        if (game.aiReactionTimer <= 0.0f)
        {
            float target = profile.predictBall
                ? PredictBallY(game, game.rightPaddle.x)
                : game.ballPosition.y;
            target += static_cast<float>(GetRandomValue(
                -(int)profile.aimError,
                (int)profile.aimError
            ));
            game.aiTargetY = target;
            game.aiReactionTimer = profile.reactionTime;
        }
        float difference = game.aiTargetY - paddleCenter;
        if (std::fabs(difference) > 7.0f)
        {
            game.rightPaddle.y += (difference > 0.0f ? 1.0f : -1.0f)
                * profile.speed * deltaTime;
        }
    }
    else
    {
        float difference = SCREEN_HEIGHT * 0.5f - paddleCenter;
        if (std::fabs(difference) > 8.0f)
        {
            game.rightPaddle.y += (difference > 0.0f ? 1.0f : -1.0f)
                * profile.speed * profile.returnSpeedScale * deltaTime;
        }
    }
}
void BounceFromPaddle(GameData &game, const Rectangle &paddle, bool isLeft)
{
    if (isLeft)
    {
        game.ballPosition.x = paddle.x + paddle.width + BALL_RADIUS;
        game.ballVelocity.x = std::fabs(game.ballVelocity.x) * 1.055f;
    }
    else
    {
        game.ballPosition.x = paddle.x - BALL_RADIUS;
        game.ballVelocity.x = -std::fabs(game.ballVelocity.x) * 1.055f;
    }
    float paddleCenter = paddle.y + paddle.height * 0.5f;
    float hitPosition = (game.ballPosition.y - paddleCenter) / (paddle.height * 0.5f);
    hitPosition = ClampFloat(hitPosition, -1.0f, 1.0f);
    game.ballVelocity.y = hitPosition * 340.0f;
    game.ballVelocity.x = ClampFloat(
        game.ballVelocity.x,
        -MAX_BALL_SPEED_X,
        MAX_BALL_SPEED_X
    );
}
void UpdatePlaying(
    GameData &game,
    const Settings &settings,
    GameMode mode,
    float deltaTime,
    Sound bounceSound,
    Sound scoreSound,
    bool bounceSoundLoaded,
    bool scoreSoundLoaded,
    GameState &state
)
{
    float playerSpeed = BASE_PADDLE_SPEED * settings.sensitivity;
    if (IsKeyDown(KEY_W))
    {
        game.leftPaddle.y -= playerSpeed * deltaTime;
    }
    if (IsKeyDown(KEY_S))
    {
        game.leftPaddle.y += playerSpeed * deltaTime;
    }
    if (mode == GameMode::VS_AI)
    {
        UpdateAI(game, settings, deltaTime);
    }
    else
    {
        if (IsKeyDown(KEY_UP))
        {
            game.rightPaddle.y -= playerSpeed * deltaTime;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            game.rightPaddle.y += playerSpeed * deltaTime;
        }
    }
    ClampPaddle(game.leftPaddle);
    ClampPaddle(game.rightPaddle);
    if (game.serveTimer > 0.0f)
    {
        game.serveTimer -= deltaTime;
        if (game.serveTimer <= 0.0f)
        {
            LaunchBall(game);
        }
    }
    else
    {
        game.ballPosition.x += game.ballVelocity.x * deltaTime;
        game.ballPosition.y += game.ballVelocity.y * deltaTime;
    }
    if (game.ballPosition.y - BALL_RADIUS <= 0.0f && game.ballVelocity.y < 0.0f)
    {
        game.ballPosition.y = BALL_RADIUS;
        game.ballVelocity.y *= -1.0f;
        if (bounceSoundLoaded)
        {
            PlaySound(bounceSound);
        }
    }
    if (game.ballPosition.y + BALL_RADIUS >= SCREEN_HEIGHT && game.ballVelocity.y > 0.0f)
    {
        game.ballPosition.y = SCREEN_HEIGHT - BALL_RADIUS;
        game.ballVelocity.y *= -1.0f;
        if (bounceSoundLoaded)
        {
            PlaySound(bounceSound);
        }
    }
    if (
        game.ballVelocity.x < 0.0f &&
        CheckCollisionCircleRec(game.ballPosition, BALL_RADIUS, game.leftPaddle)
    )
    {
        BounceFromPaddle(game, game.leftPaddle, true);
        if (bounceSoundLoaded)
        {
            PlaySound(bounceSound);
        }
    }
    if (
        game.ballVelocity.x > 0.0f &&
        CheckCollisionCircleRec(game.ballPosition, BALL_RADIUS, game.rightPaddle)
    )
    {
        BounceFromPaddle(game, game.rightPaddle, false);
        if (bounceSoundLoaded)
        {
            PlaySound(bounceSound);
        }
    }
    if (game.ballPosition.x + BALL_RADIUS < 0.0f)
    {
        ++game.rightScore;
        if (scoreSoundLoaded)
        {
            PlaySound(scoreSound);
        }
        if (game.rightScore >= WIN_SCORE)
        {
            game.winner = 2;
            state = GameState::GAME_OVER;
        }
        else
        {
            StartServe(game, -1);
        }
    }
    if (game.ballPosition.x - BALL_RADIUS > SCREEN_WIDTH)
    {
        ++game.leftScore;
        if (scoreSoundLoaded)
        {
            PlaySound(scoreSound);
        }
        if (game.leftScore >= WIN_SCORE)
        {
            game.winner = 1;
            state = GameState::GAME_OVER;
        }
        else
        {
            StartServe(game, 1);
        }
    }
}
void DrawMainMenu(Font font, int selected)
{
    DrawBackground();
    DrawCenteredText(font, "PONG ARENA", 64.0f, 68.0f, RAYWHITE);
    DrawCenteredText(font, "Fast matches. Clean controls. No excuses.", 142.0f, 21.0f, TEXT_MUTED);
    const char *items[] = {
        "Play vs AI",
        "Two Players",
        "Settings",
        "Quit Game"
    };
    constexpr float startY = 220.0f;
    for (int i = 0; i < 4; ++i)
    {
        DrawMenuButton(font, items[i], MenuButtonRectangle(i, startY), selected == i);
    }
    DrawCenteredText(
        font,
        "W/S or Arrow Keys to select  |  Enter to confirm",
        530.0f,
        18.0f,
        TEXT_MUTED
    );
}
void DrawDifficultyMenu(Font font, int selected, AIDifficulty currentDifficulty)
{
    DrawBackground();
    DrawCenteredText(font, "SELECT AI DIFFICULTY", 78.0f, 50.0f, RAYWHITE);
    DrawCenteredText(
        font,
        TextFormat("Current: %s", DifficultyName(currentDifficulty)),
        142.0f,
        21.0f,
        TEXT_MUTED
    );
    const char *items[] = {
        "Easy - Slow reactions",
        "Medium - Balanced",
        "Hard - Predictive AI",
        "Back"
    };
    constexpr float startY = 215.0f;
    for (int i = 0; i < 4; ++i)
    {
        DrawMenuButton(font, items[i], MenuButtonRectangle(i, startY, 520.0f), selected == i);
    }
    DrawCenteredText(font, "ESC to go back", 530.0f, 18.0f, TEXT_MUTED);
}
void DrawSettings(Font font, int selected, const Settings &settings)
{
    DrawBackground();
    DrawCenteredText(font, "SETTINGS", 72.0f, 54.0f, RAYWHITE);
    DrawCenteredText(font, "Adjust player movement speed", 142.0f, 21.0f, TEXT_MUTED);
    Rectangle sensitivityBox = MenuButtonRectangle(0, 225.0f, 560.0f);
    DrawRectangleRounded(
        sensitivityBox,
        0.18f,
        8,
        selected == 0 ? BUTTON_SELECTED : BUTTON_COLOR
    );
    if (selected == 0)
    {
        DrawRectangleRoundedLines(sensitivityBox, 0.18f, 8, ACCENT_COLOR);
    }
    DrawTextAt(font, "Control Sensitivity", sensitivityBox.x + 22.0f, sensitivityBox.y + 14.0f, 23.0f, RAYWHITE);
    DrawTextAt(
        font,
        TextFormat("%d%%", (int)std::round(settings.sensitivity * 100.0f)),
        sensitivityBox.x + sensitivityBox.width - 88.0f,
        sensitivityBox.y + 14.0f,
        23.0f,
        ACCENT_COLOR
    );
    Rectangle slider = {
        sensitivityBox.x,
        sensitivityBox.y + 78.0f,
        sensitivityBox.width,
        12.0f
    };
    DrawRectangleRounded(slider, 1.0f, 8, BUTTON_COLOR);
    float normalized = (settings.sensitivity - 0.6f) / 1.0f;
    Rectangle fill = slider;
    fill.width *= normalized;
    DrawRectangleRounded(fill, 1.0f, 8, ACCENT_COLOR);
    DrawCircleV({slider.x + slider.width * normalized, slider.y + slider.height * 0.5f}, 11.0f, RAYWHITE);
    DrawCircleV({slider.x + slider.width * normalized, slider.y + slider.height * 0.5f}, 7.0f, ACCENT_COLOR);
    DrawTextAt(font, "60%", slider.x, slider.y + 24.0f, 17.0f, TEXT_MUTED);
    DrawTextAt(font, "160%", slider.x + slider.width - 46.0f, slider.y + 24.0f, 17.0f, TEXT_MUTED);
    Rectangle backButton = MenuButtonRectangle(1, 340.0f, 430.0f);
    DrawMenuButton(font, "Save and Back", backButton, selected == 1);
    DrawCenteredText(font, "Left/Right to adjust  |  ESC to go back", 515.0f, 18.0f, TEXT_MUTED);
}
void DrawCourt(Font font, const GameData &game, GameMode mode, const Settings &settings)
{
    DrawBackground();
    for (int y = 8; y < SCREEN_HEIGHT; y += 34)
    {
        DrawRectangle(SCREEN_WIDTH / 2 - 2, y, 4, 19, Fade(RAYWHITE, 0.22f));
    }
    DrawRectangleRounded(game.leftPaddle, 0.35f, 8, RAYWHITE);
    DrawRectangleRounded(game.rightPaddle, 0.35f, 8, RAYWHITE);
    DrawCircleV(game.ballPosition, BALL_RADIUS + 4.0f, Fade(YELLOW, 0.12f));
    DrawCircleV(game.ballPosition, BALL_RADIUS, YELLOW);
    DrawTextAt(font, TextFormat("%d", game.leftScore), SCREEN_WIDTH * 0.5f - 115.0f, 24.0f, 54.0f, RAYWHITE);
    DrawTextAt(font, TextFormat("%d", game.rightScore), SCREEN_WIDTH * 0.5f + 78.0f, 24.0f, 54.0f, RAYWHITE);
    const char *modeText = mode == GameMode::VS_AI
        ? TextFormat("VS AI | %s", DifficultyName(settings.difficulty))
        : "Two Players";
    DrawTextAt(font, modeText, 18.0f, 15.0f, 19.0f, TEXT_MUTED);
    DrawTextAt(font, "P/ESC: Pause  |  R: Restart", 18.0f, SCREEN_HEIGHT - 31.0f, 18.0f, TEXT_MUTED);
    if (game.serveTimer > 0.0f)
    {
        int countdown = std::max(1, (int)std::ceil(game.serveTimer));
        DrawCenteredText(font, TextFormat("%d", countdown), SCREEN_HEIGHT * 0.5f - 52.0f, 76.0f, YELLOW);
    }
}
void DrawPauseMenu(Font font, int selected)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.72f));
    DrawCenteredText(font, "PAUSED", 74.0f, 54.0f, YELLOW);
    const char *items[] = {
        "Resume",
        "Restart Match",
        "Settings",
        "Main Menu",
        "Quit Game"
    };
    constexpr float startY = 170.0f;
    for (int i = 0; i < 5; ++i)
    {
        DrawMenuButton(font, items[i], MenuButtonRectangle(i, startY), selected == i);
    }
}
void DrawGameOver(Font font, const GameData &game, GameMode mode, int selected)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.78f));
    DrawCenteredText(font, "MATCH OVER", 70.0f, 52.0f, RED);
    const char *winnerText = nullptr;
    if (game.winner == 1)
    {
        winnerText = "Player 1 Wins!";
    }
    else if (mode == GameMode::VS_AI)
    {
        winnerText = "AI Wins! Time for a rematch.";
    }
    else
    {
        winnerText = "Player 2 Wins!";
    }
    DrawCenteredText(font, winnerText, 145.0f, 30.0f, YELLOW);
    const char *items[] = {
        "Restart Match",
        "Main Menu",
        "Quit Game"
    };
    constexpr float startY = 245.0f;
    for (int i = 0; i < 3; ++i)
    {
        DrawMenuButton(font, items[i], MenuButtonRectangle(i, startY), selected == i);
    }
}
Font LoadUIFont()
{
    return GetFontDefault();
}
int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong Arena");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    InitAudioDevice();
    bool bounceSoundLoaded = FileExists("assets/bounce.wav");
    bool scoreSoundLoaded = FileExists("assets/score.wav");
    Sound bounceSound{};
    Sound scoreSound{};
    if (bounceSoundLoaded)
    {
        bounceSound = LoadSound("assets/bounce.wav");
    }
    if (scoreSoundLoaded)
    {
        scoreSound = LoadSound("assets/score.wav");
    }
    Font uiFont = LoadUIFont();
    Settings settings{};
    GameData game{};
    ResetPaddles(game);
    StartServe(game, 1);
    GameState state = GameState::MAIN_MENU;
    GameState settingsReturnState = GameState::MAIN_MENU;
    GameMode mode = GameMode::VS_AI;
    int mainMenuSelection = 0;
    int difficultySelection = 1;
    int settingsSelection = 0;
    int pauseSelection = 0;
    int gameOverSelection = 0;
    bool running = true;
    while (running && !WindowShouldClose())
    {
        float deltaTime = std::min(GetFrameTime(), 1.0f / 30.0f);
        switch (state)
        {
            case GameState::MAIN_MENU:
            {
                bool activated = UpdateMenuSelection(mainMenuSelection, 4, 220.0f);
                if (activated)
                {
                    switch (mainMenuSelection)
                    {
                        case 0:
                            difficultySelection = (int)settings.difficulty;
                            state = GameState::DIFFICULTY_MENU;
                            break;
                        case 1:
                            mode = GameMode::TWO_PLAYERS;
                            StartMatch(game);
                            state = GameState::PLAYING;
                            break;
                        case 2:
                            settingsReturnState = GameState::MAIN_MENU;
                            settingsSelection = 0;
                            state = GameState::SETTINGS;
                            break;
                        case 3:
                            running = false;
                            break;
                    }
                }
                break;
            }
            case GameState::DIFFICULTY_MENU:
            {
                bool activated = UpdateMenuSelection(difficultySelection, 4, 215.0f, 520.0f);
                if (IsKeyPressed(KEY_ESCAPE))
                {
                    state = GameState::MAIN_MENU;
                }
                else if (activated)
                {
                    if (difficultySelection == 3)
                    {
                        state = GameState::MAIN_MENU;
                    }
                    else
                    {
                        settings.difficulty = static_cast<AIDifficulty>(difficultySelection);
                        mode = GameMode::VS_AI;
                        StartMatch(game);
                        state = GameState::PLAYING;
                    }
                }
                break;
            }
            case GameState::SETTINGS:
            {
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
                    IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                {
                    settingsSelection = 1 - settingsSelection;
                }
                Rectangle sensitivityBox = MenuButtonRectangle(0, 225.0f, 560.0f);
                Rectangle backButton = MenuButtonRectangle(1, 340.0f, 430.0f);
                Vector2 mousePosition = GetMousePosition();
                if (CheckCollisionPointRec(mousePosition, sensitivityBox))
                {
                    settingsSelection = 0;
                }
                else if (CheckCollisionPointRec(mousePosition, backButton))
                {
                    settingsSelection = 1;
                }
                if (settingsSelection == 0)
                {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
                    {
                        settings.sensitivity -= 0.1f;
                    }
                    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
                    {
                        settings.sensitivity += 0.1f;
                    }
                    Rectangle slider = {
                        sensitivityBox.x,
                        sensitivityBox.y + 70.0f,
                        sensitivityBox.width,
                        30.0f
                    };
                    if (CheckCollisionPointRec(mousePosition, slider) &&
                        IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                    {
                        float ratio = (mousePosition.x - slider.x) / slider.width;
                        settings.sensitivity = 0.6f + ClampFloat(ratio, 0.0f, 1.0f);
                    }
                    settings.sensitivity = ClampFloat(
                        std::round(settings.sensitivity * 10.0f) / 10.0f,
                        0.6f,
                        1.6f
                    );
                }
                bool goBack = IsKeyPressed(KEY_ESCAPE);
                if (settingsSelection == 1 &&
                    (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                     (CheckCollisionPointRec(mousePosition, backButton) &&
                      IsMouseButtonPressed(MOUSE_BUTTON_LEFT))))
                {
                    goBack = true;
                }
                if (goBack)
                {
                    state = settingsReturnState;
                }
                break;
            }
            case GameState::PLAYING:
            {
                if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
                {
                    pauseSelection = 0;
                    state = GameState::PAUSED;
                    break;
                }
                if (IsKeyPressed(KEY_R))
                {
                    StartMatch(game);
                }
                UpdatePlaying(
                    game,
                    settings,
                    mode,
                    deltaTime,
                    bounceSound,
                    scoreSound,
                    bounceSoundLoaded,
                    scoreSoundLoaded,
                    state
                );
                if (state == GameState::GAME_OVER)
                {
                    gameOverSelection = 0;
                }
                break;
            }
            case GameState::PAUSED:
            {
                bool activated = UpdateMenuSelection(pauseSelection, 5, 170.0f);
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
                {
                    state = GameState::PLAYING;
                }
                else if (activated)
                {
                    switch (pauseSelection)
                    {
                        case 0:
                            state = GameState::PLAYING;
                            break;
                        case 1:
                            StartMatch(game);
                            state = GameState::PLAYING;
                            break;
                        case 2:
                            settingsReturnState = GameState::PAUSED;
                            settingsSelection = 0;
                            state = GameState::SETTINGS;
                            break;
                        case 3:
                            state = GameState::MAIN_MENU;
                            break;
                        case 4:
                            running = false;
                            break;
                    }
                }
                break;
            }
            case GameState::GAME_OVER:
            {
                bool activated = UpdateMenuSelection(gameOverSelection, 3, 245.0f);
                if (IsKeyPressed(KEY_ESCAPE))
                {
                    state = GameState::MAIN_MENU;
                }
                else if (activated)
                {
                    switch (gameOverSelection)
                    {
                        case 0:
                            StartMatch(game);
                            state = GameState::PLAYING;
                            break;
                        case 1:
                            state = GameState::MAIN_MENU;
                            break;
                        case 2:
                            running = false;
                            break;
                    }
                }
                break;
            }
        }
        BeginDrawing();
        if (state == GameState::MAIN_MENU)
        {
            DrawMainMenu(uiFont, mainMenuSelection);
        }
        else if (state == GameState::DIFFICULTY_MENU)
        {
            DrawDifficultyMenu(uiFont, difficultySelection, settings.difficulty);
        }
        else if (state == GameState::SETTINGS)
        {
            DrawSettings(uiFont, settingsSelection, settings);
        }
        else
        {
            DrawCourt(uiFont, game, mode, settings);
            if (state == GameState::PAUSED)
            {
                DrawPauseMenu(uiFont, pauseSelection);
            }
            else if (state == GameState::GAME_OVER)
            {
                DrawGameOver(uiFont, game, mode, gameOverSelection);
            }
        }
        EndDrawing();
    }
    if (bounceSoundLoaded)
    {
        UnloadSound(bounceSound);
    }
    if (scoreSoundLoaded)
    {
        UnloadSound(scoreSound);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
