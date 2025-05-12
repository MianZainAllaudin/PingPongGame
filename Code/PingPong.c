#include <stdio.h> // To run this game 1st build bash by writing bash build.bash in terminal
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <raylib.h>
#include <time.h> 
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 100
#define BALL_RADIUS 10
#define PADDLE_SPEED 7.0f
#define INITIAL_BALL_SPEED 7.5f      
#define MAX_BALL_SPEED 15.0f         
#define MAX_SCORE 10
typedef struct {
    float leftPaddleY;
    float rightPaddleY;
    Vector2 ballPosition;
    Vector2 ballVelocity;
    int leftScore;
    int rightScore;
    bool gameOver;
    bool gamePaused;
    int level;
    bool twoPlayerMode;     
    bool modeSelected;      
    pthread_mutex_t stateMutex;     // Synchronization
} GameState;
GameState gameState;
void* ballThreadFunc(void* arg) {
    while (1) {
        // Sleep to control update rate
        usleep(16000); // ~60 updates per second
        pthread_mutex_lock(&gameState.stateMutex);
        if (gameState.gameOver || gameState.gamePaused || !gameState.modeSelected) {         // Skip if game is paused, over, or mode not selected
            pthread_mutex_unlock(&gameState.stateMutex);
            continue;
        }
        gameState.ballPosition.x += gameState.ballVelocity.x;         // Update ball position
        gameState.ballPosition.y += gameState.ballVelocity.y;
        if (gameState.ballPosition.y <= 0 || gameState.ballPosition.y >= SCREEN_HEIGHT) {         // Ball collision with top and bottom walls
            gameState.ballVelocity.y *= -1.0f;
            PlaySound(LoadSound("resources/wall_hit.wav"));
            if (gameState.ballPosition.y < 0) gameState.ballPosition.y = 0;             // Ensure ball stays within bounds
            if (gameState.ballPosition.y > SCREEN_HEIGHT) gameState.ballPosition.y = SCREEN_HEIGHT;
        }
        if (gameState.ballPosition.x - BALL_RADIUS <= PADDLE_WIDTH &&
            gameState.ballPosition.y >= gameState.leftPaddleY && 
            gameState.ballPosition.y <= gameState.leftPaddleY + PADDLE_HEIGHT) {
            float hitPosition = (gameState.ballPosition.y - gameState.leftPaddleY) / PADDLE_HEIGHT;             // Calculate reflection angle based on where ball hits paddle
            float bounceAngle = (hitPosition - 0.5f) * PI/3; // Between -PI/6 and PI/6 radians
            float currentSpeed = Vector2Length(gameState.ballVelocity);
            float newSpeed = fmaxf(currentSpeed, INITIAL_BALL_SPEED * (1.0f + gameState.level * 0.2f));
            newSpeed = fminf(newSpeed * 1.05f, MAX_BALL_SPEED);
            gameState.ballVelocity.x = fabs(newSpeed * cosf(bounceAngle)); // Force positive x direction
            gameState.ballVelocity.y = newSpeed * sinf(bounceAngle);
            gameState.ballPosition.x = PADDLE_WIDTH + BALL_RADIUS + 1;             // Move ball outside paddle to prevent multiple collisions
            PlaySound(LoadSound("resources/paddle_hit.wav"));
        }
        if (gameState.ballPosition.x + BALL_RADIUS >= SCREEN_WIDTH - PADDLE_WIDTH &&
            gameState.ballPosition.y >= gameState.rightPaddleY && 
            gameState.ballPosition.y <= gameState.rightPaddleY + PADDLE_HEIGHT) {
            float hitPosition = (gameState.ballPosition.y - gameState.rightPaddleY) / PADDLE_HEIGHT;
            float bounceAngle = (hitPosition - 0.5f) * PI/3; 
            float currentSpeed = Vector2Length(gameState.ballVelocity);
            float newSpeed = fmaxf(currentSpeed, INITIAL_BALL_SPEED * (1.0f + gameState.level * 0.2f));
            newSpeed = fminf(newSpeed * 1.05f, MAX_BALL_SPEED);
            gameState.ballVelocity.x = -fabs(newSpeed * cosf(bounceAngle)); // Force negative x direction
            gameState.ballVelocity.y = newSpeed * sinf(bounceAngle);
            gameState.ballPosition.x = SCREEN_WIDTH - PADDLE_WIDTH - BALL_RADIUS - 1;
            PlaySound(LoadSound("resources/paddle_hit.wav"));
        }
        if (gameState.ballPosition.x < 0) {
            gameState.rightScore++;
            PlaySound(LoadSound("resources/score.wav"));
            gameState.ballPosition.x = SCREEN_WIDTH / 2;             // Reset ball with level-based speed
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;
            gameState.ballVelocity.x = INITIAL_BALL_SPEED * levelSpeedMultiplier;
            gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 
                                      (0.6f + ((float)GetRandomValue(0, 40) / 100.0f)) * levelSpeedMultiplier;
            if (gameState.rightScore >= MAX_SCORE) {
                gameState.gameOver = true;
            }
        }
        if (gameState.ballPosition.x > SCREEN_WIDTH) {
            gameState.leftScore++;
            PlaySound(LoadSound("resources/score.wav"));
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;
            gameState.ballVelocity.x = -INITIAL_BALL_SPEED * levelSpeedMultiplier;
            gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 
                                      (0.6f + ((float)GetRandomValue(0, 40) / 100.0f)) * levelSpeedMultiplier;
            if (gameState.leftScore >= MAX_SCORE) {
                gameState.gameOver = true;
            }
        }
        pthread_mutex_unlock(&gameState.stateMutex);
    }
    return NULL;
}
void* aiThreadFunc(void* arg) { // AI (Right Paddle)
    while (1) {
        usleep(16000); // thinking time
        pthread_mutex_lock(&gameState.stateMutex);
        if (gameState.twoPlayerMode || gameState.gameOver || gameState.gamePaused || !gameState.modeSelected) {         // Skip AI control if in two-player mode or game is paused/over/not started
            pthread_mutex_unlock(&gameState.stateMutex);
            continue;
        }
        float difficultyFactor = 0.4f + (gameState.level * 0.25f); // Level 1: 0.65, Level 2: 0.9, Level 3: 1.15
        if (gameState.ballVelocity.x > 0) { 
            float targetY = gameState.ballPosition.y;
            if (gameState.level > 1) {
                float timeToReach = (SCREEN_WIDTH - PADDLE_WIDTH - gameState.ballPosition.x) / gameState.ballVelocity.x;
                float predictedY = gameState.ballPosition.y + (gameState.ballVelocity.y * timeToReach);
                while (predictedY < 0 || predictedY > SCREEN_HEIGHT) {
                    if (predictedY < 0) predictedY = -predictedY;
                    if (predictedY > SCREEN_HEIGHT) predictedY = 2 * SCREEN_HEIGHT - predictedY;
                }
                targetY = predictedY * difficultyFactor + gameState.ballPosition.y * (1 - difficultyFactor);
            }
            float errorRange = (4.0f - gameState.level) * 25.0f;  // Level 1: 75, Level 2: 50, Level 3: 25
            targetY += (GetRandomValue(-100, 100) / 100.0f) * errorRange;
            float aiSpeed = PADDLE_SPEED * (0.8f + gameState.level * 0.2f);  // Level-based speed
            if (targetY < gameState.rightPaddleY + PADDLE_HEIGHT/2 - 10) {
                gameState.rightPaddleY -= aiSpeed * difficultyFactor;
            } else if (targetY > gameState.rightPaddleY + PADDLE_HEIGHT/2 + 10) {
                gameState.rightPaddleY += aiSpeed * difficultyFactor;
            }
        } else {
            if (gameState.rightPaddleY + PADDLE_HEIGHT/2 < SCREEN_HEIGHT/2 - 20) {
                gameState.rightPaddleY += PADDLE_SPEED * 0.5f;
            } else if (gameState.rightPaddleY + PADDLE_HEIGHT/2 > SCREEN_HEIGHT/2 + 20) {
                gameState.rightPaddleY -= PADDLE_SPEED * 0.5f;
            }
        }
        if (gameState.rightPaddleY < 0) gameState.rightPaddleY = 0;
        if (gameState.rightPaddleY > SCREEN_HEIGHT - PADDLE_HEIGHT) gameState.rightPaddleY = SCREEN_HEIGHT - PADDLE_HEIGHT;
        pthread_mutex_unlock(&gameState.stateMutex);
        // Level 1: slow reactions, Level 3: quick reactions
        int reactionTimeMs = 80 - (gameState.level * 25);  // Level 1: 55ms, Level 2: 30ms, Level 3: 5ms
        usleep(reactionTimeMs * 1000);
    }
    return NULL;
}
void drawModeSelection() {
    BeginDrawing();
    ClearBackground((Color){20, 20, 50, 255});  // Dark blue background
    const char* titleText = "Zain Allaudin_PING PONG";
    DrawText(titleText, SCREEN_WIDTH/2 - MeasureText(titleText, 40)/2, 100, 40, WHITE);
    DrawRectangle(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 60, 400, 60, Fade(WHITE, 0.3f));
    DrawRectangle(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + 20, 400, 60, Fade(WHITE, 0.3f));
    const char* singlePlayerText = "1 - SINGLE PLAYER";
    const char* multiPlayerText = "2 - TWO PLAYER";
    DrawText(singlePlayerText, SCREEN_WIDTH/2 - MeasureText(singlePlayerText, 30)/2, SCREEN_HEIGHT/2 - 45, 30, WHITE);
    DrawText(multiPlayerText, SCREEN_WIDTH/2 - MeasureText(multiPlayerText, 30)/2, SCREEN_HEIGHT/2 + 35, 30, WHITE);
    const char* instructionText = "Press 1 or 2 to select game mode";
    DrawText(instructionText, SCREEN_WIDTH/2 - MeasureText(instructionText, 20)/2, SCREEN_HEIGHT - 100, 20, GRAY);
    EndDrawing();
}
void initializeGame() {
    gameState.leftPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    gameState.rightPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    gameState.ballPosition.x = SCREEN_WIDTH / 2;
    gameState.ballPosition.y = SCREEN_HEIGHT / 2;
    float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;
    float initialSpeed = INITIAL_BALL_SPEED * (1.0f + gameState.level * 0.2f);
    gameState.ballVelocity.x = initialSpeed * (GetRandomValue(0, 1) ? 1 : -1);
    gameState.ballVelocity.y = initialSpeed * 0.5f * (GetRandomValue(0, 1) ? 1 : -1);
    gameState.leftScore = 0;
    gameState.rightScore = 0;
    gameState.gameOver = false;
    gameState.gamePaused = false;
    gameState.level = 1; // Start at level 1
    gameState.twoPlayerMode = false;  // Default to single player
    gameState.modeSelected = false;   // Mode not selected yet
    pthread_mutex_init(&gameState.stateMutex, NULL);
}
void drawGame() {
    BeginDrawing();
    Color bgColor;
    switch (gameState.level) {
        case 1:
            bgColor = (Color){20, 20, 50, 255};    // Dark blue
            break;
        case 2:
            bgColor = (Color){20, 50, 20, 255};    // Dark green
            break;
        case 3:
            bgColor = (Color){50, 20, 50, 255};    // Dark purple
            break;
        default:
            bgColor = BLACK;
    }
    ClearBackground(bgColor);
    pthread_mutex_lock(&gameState.stateMutex);
    for (int y = 0; y < SCREEN_HEIGHT; y += 20) {     // Draw center line
        DrawRectangle(SCREEN_WIDTH/2 - 5, y, 10, 10, Fade(WHITE, 0.5f));
    }
    Color paddleColor;
    switch (gameState.level) {
        case 1:
            paddleColor = WHITE;               // White for level 1
            break;
        case 2:
            paddleColor = LIME;                // Lime for level 2
            break;
        case 3:
            paddleColor = RED;                 // Red for level 3
            break;
        default:
            paddleColor = WHITE;
    }
    DrawRectangleRounded((Rectangle){0, gameState.leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT}, 0.3f, 8, paddleColor);
    DrawRectangleRounded((Rectangle){SCREEN_WIDTH - PADDLE_WIDTH, gameState.rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT}, 0.3f, 8, paddleColor);
    DrawText("P1", 10, gameState.leftPaddleY - 25, 20, WHITE);
    if (gameState.twoPlayerMode) {
        DrawText("P2", SCREEN_WIDTH - PADDLE_WIDTH - 10, gameState.rightPaddleY - 25, 20, WHITE);
    } else {
        DrawText("CPU", SCREEN_WIDTH - PADDLE_WIDTH - 35, gameState.rightPaddleY - 25, 20, WHITE);
    }
    Color ballColor;
    switch (gameState.level) {
        case 1:
            ballColor = WHITE;                 // White for level 1
            break;
        case 2:
            ballColor = YELLOW;                // Yellow for level 2
            break;
        case 3:
            ballColor = (Color){255, 100, 100, 255};  // Light red for level 3
            break;
        default:
            ballColor = WHITE;
    }
    int trailLength = 3 + gameState.level * 2;  // Level 1: 5, Level 2: 7, Level 3: 9
    for (int i = 0; i < trailLength; i++) {
        float alpha = 0.3f - (i * 0.03f);
        if (alpha > 0) {
            Vector2 trailPos = {
                gameState.ballPosition.x - gameState.ballVelocity.x * (i * 1.5f),
                gameState.ballPosition.y - gameState.ballVelocity.y * (i * 1.5f)
            };
            DrawCircle(trailPos.x, trailPos.y, BALL_RADIUS - i * 0.5f, Fade(ballColor, alpha));
        }
    }
    DrawCircle(gameState.ballPosition.x, gameState.ballPosition.y, BALL_RADIUS, ballColor);
    char scoreText[32];
    sprintf(scoreText, "%d", gameState.leftScore);
    DrawText("P1", SCREEN_WIDTH/4 - 50, 30, 30, WHITE);
    DrawText(scoreText, SCREEN_WIDTH/4, 30, 60, WHITE);
    sprintf(scoreText, "%d", gameState.rightScore);
    if (gameState.twoPlayerMode) {
        DrawText("P2", 3*SCREEN_WIDTH/4 - 70, 30, 30, WHITE);
    } else {
        DrawText("CPU", 3*SCREEN_WIDTH/4 - 90, 30, 30, WHITE);
    }
    DrawText(scoreText, 3*SCREEN_WIDTH/4 - 20, 30, 60, WHITE);
    char levelText[32];
    sprintf(levelText, "Level: %d", gameState.level);
    Color levelColor;
    switch (gameState.level) {
        case 1:
            levelColor = SKYBLUE;
            break;
        case 2:
            levelColor = GREEN;
            break;
        case 3:
            levelColor = MAGENTA;
            break;
        default:
            levelColor = GOLD;
    }
    DrawRectangle(SCREEN_WIDTH/2 - MeasureText(levelText, 24)/2 - 10, 5, 
                 MeasureText(levelText, 24) + 20, 30, Fade(BLACK, 0.7f));
    DrawText(levelText, SCREEN_WIDTH/2 - MeasureText(levelText, 24)/2, 10, 24, levelColor);
    if (gameState.gameOver) {
        const char* gameOverText = "GAME OVER";
        const char* winnerText;
        if (gameState.twoPlayerMode) {
            winnerText = (gameState.leftScore > gameState.rightScore) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
        } else {
            winnerText = (gameState.leftScore > gameState.rightScore) ? "PLAYER WINS!" : "CPU WINS!";
        }
        const char* restartText = "Press R to Restart";
        const char* modeSelectText = "Press M to Mode Select";
        DrawRectangle(0, SCREEN_HEIGHT/2 - 70, SCREEN_WIDTH, 140, Fade(BLACK, 0.8f));
        DrawText(gameOverText, SCREEN_WIDTH/2 - MeasureText(gameOverText, 40)/2, SCREEN_HEIGHT/2 - 60, 40, WHITE);
        DrawText(winnerText, SCREEN_WIDTH/2 - MeasureText(winnerText, 30)/2, SCREEN_HEIGHT/2 - 10, 30, YELLOW);
        DrawText(restartText, SCREEN_WIDTH/2 - MeasureText(restartText, 20)/2, SCREEN_HEIGHT/2 + 30, 20, GREEN);
        DrawText(modeSelectText, SCREEN_WIDTH/2 - MeasureText(modeSelectText, 20)/2, SCREEN_HEIGHT/2 + 60, 20, GREEN);
    }
    if (gameState.gamePaused && !gameState.gameOver) {
        const char* pausedText = "GAME PAUSED";
        const char* resumeText = "Press P to Resume";
        DrawRectangle(0, SCREEN_HEIGHT/2 - 60, SCREEN_WIDTH, 120, Fade(BLACK, 0.8f));
        DrawText(pausedText, SCREEN_WIDTH/2 - MeasureText(pausedText, 40)/2, SCREEN_HEIGHT/2 - 40, 40, WHITE);
        DrawText(resumeText, SCREEN_WIDTH/2 - MeasureText(resumeText, 20)/2, SCREEN_HEIGHT/2 + 20, 20, GREEN);
    }
    if (!gameState.gameOver && !gameState.gamePaused) {
        DrawText("W/S - P1 Move", 10, SCREEN_HEIGHT - 60, 20, Fade(WHITE, 0.7f));
        
        if (gameState.twoPlayerMode) {
            DrawText("UP/DOWN (Arrow Keys) - P2 Move", 10, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
        } else {
            DrawText("P - Pause", 10, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
        }
        DrawText("L - Change Level", SCREEN_WIDTH - MeasureText("L - Change Level", 20) - 10, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
        DrawText("P - Pause", SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
    }
    pthread_mutex_unlock(&gameState.stateMutex);
    EndDrawing();
}
int main() {
    srand(time(NULL));  // Initialize random seed
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zain Allaudin_PING PONG");     // Initialize raylib
    InitAudioDevice();
    SetTargetFPS(60);
    initializeGame();
    pthread_t ballThread, aiThread;
    pthread_create(&ballThread, NULL, ballThreadFunc, NULL);
    pthread_create(&aiThread, NULL, aiThreadFunc, NULL);
    Sound paddleHitSound = LoadSound("resources/paddle_hit.wav");
    Sound wallHitSound = LoadSound("resources/wall_hit.wav");
    Sound scoreSound = LoadSound("resources/score.wav");
    while (!WindowShouldClose()) {     // Main game loop
        if (!gameState.modeSelected) {
            if (IsKeyPressed(KEY_ONE)) { // ASCII of 1=>49 (Decimal)
                pthread_mutex_lock(&gameState.stateMutex);
                gameState.twoPlayerMode = false;
                gameState.modeSelected = true;
                pthread_mutex_unlock(&gameState.stateMutex);
            }
            else if (IsKeyPressed(KEY_TWO)) {
                pthread_mutex_lock(&gameState.stateMutex);
                gameState.twoPlayerMode = true;
                gameState.modeSelected = true;
                pthread_mutex_unlock(&gameState.stateMutex);
            }
            drawModeSelection();
            continue;  // Skip the rest of the loop
        }
        pthread_mutex_lock(&gameState.stateMutex);
        if (IsKeyPressed(KEY_P)) {
            gameState.gamePaused = !gameState.gamePaused;
        }
        if (IsKeyPressed(KEY_L) && !gameState.gameOver && !gameState.gamePaused) {
            gameState.level = (gameState.level % 3) + 1;
        }
        if (IsKeyPressed(KEY_R) && gameState.gameOver) {
            gameState.leftScore = 0;
            gameState.rightScore = 0;
            gameState.gameOver = false;
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            float resetSpeed = INITIAL_BALL_SPEED * (1.0f + gameState.level * 0.2f);
            gameState.ballVelocity.x = resetSpeed * (GetRandomValue(0, 1) ? 1 : -1);
            gameState.ballVelocity.y = resetSpeed * 0.5f * (GetRandomValue(0, 1) ? 1 : -1);
        }
        if (IsKeyPressed(KEY_M) && gameState.gameOver) {
            gameState.modeSelected = false;
            gameState.gameOver = false;
            gameState.leftScore = 0;
            gameState.rightScore = 0;
            gameState.leftPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
            gameState.rightPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
        }
        if (!gameState.gameOver && !gameState.gamePaused) {
            if (IsKeyDown(KEY_W)) {
                gameState.leftPaddleY -= PADDLE_SPEED;
                if (gameState.leftPaddleY < 0) gameState.leftPaddleY = 0;
            }
            if (IsKeyDown(KEY_S)) {
                gameState.leftPaddleY += PADDLE_SPEED;
                if (gameState.leftPaddleY > SCREEN_HEIGHT - PADDLE_HEIGHT) {
                    gameState.leftPaddleY = SCREEN_HEIGHT - PADDLE_HEIGHT;
                }
            }
            if (gameState.twoPlayerMode) {
                if (IsKeyDown(KEY_UP)) {
                    gameState.rightPaddleY -= PADDLE_SPEED;
                    if (gameState.rightPaddleY < 0) gameState.rightPaddleY = 0;
                }
                
                if (IsKeyDown(KEY_DOWN)) {
                    gameState.rightPaddleY += PADDLE_SPEED;
                    if (gameState.rightPaddleY > SCREEN_HEIGHT - PADDLE_HEIGHT) {
                        gameState.rightPaddleY = SCREEN_HEIGHT - PADDLE_HEIGHT;
                    }
                }
            }
        }
        pthread_mutex_unlock(&gameState.stateMutex);
        drawGame();
    }
    pthread_mutex_destroy(&gameState.stateMutex);     // Cleanup
    CloseWindow();
    return 0;
}