#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <raylib.h>
#include <time.h>

// Game constants
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 100
#define BALL_RADIUS 10
#define PADDLE_SPEED 7.0f
#define INITIAL_BALL_SPEED 7.5f       // Increased from 5.0f
#define MAX_BALL_SPEED 15.0f          // Increased from 12.0f
#define MAX_SCORE 10

// Game state
typedef struct {
    // Paddle positions
    float leftPaddleY;
    float rightPaddleY;
    
    // Ball position and velocity
    Vector2 ballPosition;
    Vector2 ballVelocity;
    
    // Scores
    int leftScore;
    int rightScore;
    
    // Game state flags
    bool gameOver;
    bool gamePaused;
    
    // Level (affects ball speed and AI difficulty)
    int level;
    
    // Synchronization
    pthread_mutex_t stateMutex;
} GameState;

// Global game state
GameState gameState;

// Thread function to update ball movement
void* ballThreadFunc(void* arg) {
    while (1) {
        // Sleep to control update rate
        usleep(16000); // ~60 updates per second
        
        pthread_mutex_lock(&gameState.stateMutex);
        
        if (gameState.gameOver || gameState.gamePaused) {
            pthread_mutex_unlock(&gameState.stateMutex);
            continue;
        }
        
        // Update ball position
        gameState.ballPosition.x += gameState.ballVelocity.x;
        gameState.ballPosition.y += gameState.ballVelocity.y;
        
        // Ball collision with top and bottom walls
        if (gameState.ballPosition.y <= 0 || gameState.ballPosition.y >= SCREEN_HEIGHT) {
            gameState.ballVelocity.y *= -1.0f;
            PlaySound(LoadSound("resources/wall_hit.wav"));
            
            // Ensure ball stays within bounds
            if (gameState.ballPosition.y < 0) gameState.ballPosition.y = 0;
            if (gameState.ballPosition.y > SCREEN_HEIGHT) gameState.ballPosition.y = SCREEN_HEIGHT;
        }
        
        // Ball collision with paddles
        if (gameState.ballPosition.x - BALL_RADIUS <= PADDLE_WIDTH &&
            gameState.ballPosition.y >= gameState.leftPaddleY && 
            gameState.ballPosition.y <= gameState.leftPaddleY + PADDLE_HEIGHT) {
            
            // Calculate reflection angle based on where ball hits paddle
            float hitPosition = (gameState.ballPosition.y - gameState.leftPaddleY) / PADDLE_HEIGHT;
            float bounceAngle = (hitPosition - 0.5f) * 1.5f; // -0.75 to 0.75 radians
            
            // Increase ball speed slightly with each hit
            float speed = Vector2Length(gameState.ballVelocity);
            if (speed < MAX_BALL_SPEED) speed *= 1.1f;  // Increased speed multiplier from 1.05f
            
            // Apply level-based speed bonus
            speed *= (1.0f + (gameState.level - 1) * 0.3f);  // Significant speed difference between levels
            
            // Set new velocity
            gameState.ballVelocity.x = speed * cosf(bounceAngle);
            gameState.ballVelocity.y = speed * sinf(bounceAngle);
            
            // Ensure ball moves right
            if (gameState.ballVelocity.x < 0) gameState.ballVelocity.x *= -1;
            
            // Move ball outside paddle to prevent multiple collisions
            gameState.ballPosition.x = PADDLE_WIDTH + BALL_RADIUS + 1;
            
            // Play sound
            PlaySound(LoadSound("resources/paddle_hit.wav"));
        }
        
        if (gameState.ballPosition.x + BALL_RADIUS >= SCREEN_WIDTH - PADDLE_WIDTH &&
            gameState.ballPosition.y >= gameState.rightPaddleY && 
            gameState.ballPosition.y <= gameState.rightPaddleY + PADDLE_HEIGHT) {
            
            // Calculate reflection angle based on where ball hits paddle
            float hitPosition = (gameState.ballPosition.y - gameState.rightPaddleY) / PADDLE_HEIGHT;
            float bounceAngle = (hitPosition - 0.5f) * 1.5f; // -0.75 to 0.75 radians
            
            // Increase ball speed slightly with each hit
            float speed = Vector2Length(gameState.ballVelocity);
            if (speed < MAX_BALL_SPEED) speed *= 1.1f;  // Increased speed multiplier from 1.05f
            
            // Apply level-based speed bonus
            speed *= (1.0f + (gameState.level - 1) * 0.3f);  // Significant speed difference between levels
            
            // Set new velocity
            gameState.ballVelocity.x = speed * cosf(bounceAngle);
            gameState.ballVelocity.y = speed * sinf(bounceAngle);
            
            // Ensure ball moves left
            if (gameState.ballVelocity.x > 0) gameState.ballVelocity.x *= -1;
            
            // Move ball outside paddle to prevent multiple collisions
            gameState.ballPosition.x = SCREEN_WIDTH - PADDLE_WIDTH - BALL_RADIUS - 1;
            
            // Play sound
            PlaySound(LoadSound("resources/paddle_hit.wav"));
        }
        
        // Scoring
        if (gameState.ballPosition.x < 0) {
            // Right player scores
            gameState.rightScore++;
            PlaySound(LoadSound("resources/score.wav"));
            
            // Reset ball with level-based speed
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            
            // Level-specific initial ball speed (much more pronounced differences)
            float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;  // Level 1: 1.0x, Level 2: 1.5x, Level 3: 2.0x
            gameState.ballVelocity.x = INITIAL_BALL_SPEED * levelSpeedMultiplier;
            gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 
                                      (0.6f + ((float)GetRandomValue(0, 40) / 100.0f)) * levelSpeedMultiplier;
            
            // Check for game over
            if (gameState.rightScore >= MAX_SCORE) {
                gameState.gameOver = true;
            }
        }
        
        if (gameState.ballPosition.x > SCREEN_WIDTH) {
            // Left player scores
            gameState.leftScore++;
            PlaySound(LoadSound("resources/score.wav"));
            
            // Reset ball with level-based speed
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            
            // Level-specific initial ball speed (much more pronounced differences)
            float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;  // Level 1: 1.0x, Level 2: 1.5x, Level 3: 2.0x
            gameState.ballVelocity.x = -INITIAL_BALL_SPEED * levelSpeedMultiplier;
            gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 
                                      (0.6f + ((float)GetRandomValue(0, 40) / 100.0f)) * levelSpeedMultiplier;
            
            // Check for game over
            if (gameState.leftScore >= MAX_SCORE) {
                gameState.gameOver = true;
            }
        }
        
        pthread_mutex_unlock(&gameState.stateMutex);
    }
    
    return NULL;
}

// Thread function for AI (right paddle)
void* aiThreadFunc(void* arg) {
    while (1) {
        // Sleep to control update rate and add some "thinking time"
        usleep(16000); // ~60 updates per second
        
        pthread_mutex_lock(&gameState.stateMutex);
        
        if (gameState.gameOver || gameState.gamePaused) {
            pthread_mutex_unlock(&gameState.stateMutex);
            continue;
        }
        
        // Adjust AI difficulty based on level - Much more dramatic differences
        float difficultyFactor = 0.4f + (gameState.level * 0.25f); // Level 1: 0.65, Level 2: 0.9, Level 3: 1.15
        
        // Basic AI: move toward the ball with some prediction and reaction time
        if (gameState.ballVelocity.x > 0) { // Only move if ball is coming toward AI
            // Target position is where the ball will be
            float targetY = gameState.ballPosition.y;
            
            // Add prediction capability based on level
            if (gameState.level > 1) {
                // Calculate where ball will be when it reaches the paddle
                float timeToReach = (SCREEN_WIDTH - PADDLE_WIDTH - gameState.ballPosition.x) / gameState.ballVelocity.x;
                float predictedY = gameState.ballPosition.y + (gameState.ballVelocity.y * timeToReach);
                
                // Bounce calculation for prediction
                while (predictedY < 0 || predictedY > SCREEN_HEIGHT) {
                    if (predictedY < 0) predictedY = -predictedY;
                    if (predictedY > SCREEN_HEIGHT) predictedY = 2 * SCREEN_HEIGHT - predictedY;
                }
                
                // Adjust target with prediction - higher levels have more accurate prediction
                targetY = predictedY * difficultyFactor + gameState.ballPosition.y * (1 - difficultyFactor);
            }
            
            // Introduce error based on level - much more error at lower levels
            float errorRange = (4.0f - gameState.level) * 25.0f;  // Level 1: 75, Level 2: 50, Level 3: 25
            targetY += (GetRandomValue(-100, 100) / 100.0f) * errorRange;
            
            // Make the paddle move toward the target with level-appropriate speed
            float aiSpeed = PADDLE_SPEED * (0.8f + gameState.level * 0.2f);  // Level-based speed
            
            if (targetY < gameState.rightPaddleY + PADDLE_HEIGHT/2 - 10) {
                gameState.rightPaddleY -= aiSpeed * difficultyFactor;
            } else if (targetY > gameState.rightPaddleY + PADDLE_HEIGHT/2 + 10) {
                gameState.rightPaddleY += aiSpeed * difficultyFactor;
            }
        } else {
            // If ball moving away, move toward center with less urgency
            if (gameState.rightPaddleY + PADDLE_HEIGHT/2 < SCREEN_HEIGHT/2 - 20) {
                gameState.rightPaddleY += PADDLE_SPEED * 0.5f;
            } else if (gameState.rightPaddleY + PADDLE_HEIGHT/2 > SCREEN_HEIGHT/2 + 20) {
                gameState.rightPaddleY -= PADDLE_SPEED * 0.5f;
            }
        }
        
        // Ensure paddle stays within screen bounds
        if (gameState.rightPaddleY < 0) gameState.rightPaddleY = 0;
        if (gameState.rightPaddleY > SCREEN_HEIGHT - PADDLE_HEIGHT) gameState.rightPaddleY = SCREEN_HEIGHT - PADDLE_HEIGHT;
        
        pthread_mutex_unlock(&gameState.stateMutex);
        
        // Sleep based on AI reaction time and difficulty
        // Level 1: slow reactions, Level 3: quick reactions
        int reactionTimeMs = 80 - (gameState.level * 25);  // Level 1: 55ms, Level 2: 30ms, Level 3: 5ms
        usleep(reactionTimeMs * 1000);
    }
    
    return NULL;
}

// Initialize game state
void initializeGame() {
    gameState.leftPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    gameState.rightPaddleY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    
    gameState.ballPosition.x = SCREEN_WIDTH / 2;
    gameState.ballPosition.y = SCREEN_HEIGHT / 2;
    
    // Initial ball velocity with level-based speed
    float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;
    gameState.ballVelocity.x = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * levelSpeedMultiplier;
    gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 
                              (0.6f + ((float)GetRandomValue(0, 40) / 100.0f)) * levelSpeedMultiplier;
    
    gameState.leftScore = 0;
    gameState.rightScore = 0;
    
    gameState.gameOver = false;
    gameState.gamePaused = false;
    
    gameState.level = 1; // Start at level 1
    
    pthread_mutex_init(&gameState.stateMutex, NULL);
}

// Draw game
void drawGame() {
    BeginDrawing();
    
    // Background color changes based on level - more dramatic differences
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
    
    // Draw center line
    for (int y = 0; y < SCREEN_HEIGHT; y += 20) {
        DrawRectangle(SCREEN_WIDTH/2 - 5, y, 10, 10, Fade(WHITE, 0.5f));
    }
    
    // Level-specific paddle colors
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
    
    // Draw paddles with level-specific colors
    DrawRectangleRounded((Rectangle){0, gameState.leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT}, 0.3f, 8, paddleColor);
    DrawRectangleRounded((Rectangle){SCREEN_WIDTH - PADDLE_WIDTH, gameState.rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT}, 0.3f, 8, paddleColor);
    
    // Level-specific ball color
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
    
    // Draw ball with trail effect - longer trails for higher levels
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
    
    // Draw scores
    char scoreText[32];
    sprintf(scoreText, "%d", gameState.leftScore);
    DrawText(scoreText, SCREEN_WIDTH/4, 30, 60, WHITE);
    
    sprintf(scoreText, "%d", gameState.rightScore);
    DrawText(scoreText, 3*SCREEN_WIDTH/4 - 20, 30, 60, WHITE);
    
    // Draw level indicator with more prominence
    char levelText[32];
    sprintf(levelText, "Level: %d", gameState.level);
    
    // Level-specific colors for the level text
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
    
    // Draw game over message
    if (gameState.gameOver) {
        const char* gameOverText = "GAME OVER";
        const char* winnerText = (gameState.leftScore > gameState.rightScore) ? "PLAYER WINS!" : "CPU WINS!";
        const char* restartText = "Press R to Restart";
        
        DrawRectangle(0, SCREEN_HEIGHT/2 - 60, SCREEN_WIDTH, 120, Fade(BLACK, 0.8f));
        DrawText(gameOverText, SCREEN_WIDTH/2 - MeasureText(gameOverText, 40)/2, SCREEN_HEIGHT/2 - 40, 40, WHITE);
        DrawText(winnerText, SCREEN_WIDTH/2 - MeasureText(winnerText, 30)/2, SCREEN_HEIGHT/2, 30, YELLOW);
        DrawText(restartText, SCREEN_WIDTH/2 - MeasureText(restartText, 20)/2, SCREEN_HEIGHT/2 + 40, 20, GREEN);
    }
    
    // Draw pause message
    if (gameState.gamePaused && !gameState.gameOver) {
        const char* pausedText = "GAME PAUSED";
        const char* resumeText = "Press P to Resume";
        
        DrawRectangle(0, SCREEN_HEIGHT/2 - 60, SCREEN_WIDTH, 120, Fade(BLACK, 0.8f));
        DrawText(pausedText, SCREEN_WIDTH/2 - MeasureText(pausedText, 40)/2, SCREEN_HEIGHT/2 - 40, 40, WHITE);
        DrawText(resumeText, SCREEN_WIDTH/2 - MeasureText(resumeText, 20)/2, SCREEN_HEIGHT/2 + 20, 20, GREEN);
    }
    
    // Draw controls help
    if (!gameState.gameOver && !gameState.gamePaused) {
        DrawText("W/S - Move Paddle", 10, SCREEN_HEIGHT - 60, 20, Fade(WHITE, 0.7f));
        DrawText("P - Pause", 10, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
        DrawText("L - Change Level", SCREEN_WIDTH - MeasureText("L - Change Level", 20) - 10, SCREEN_HEIGHT - 30, 20, Fade(WHITE, 0.7f));
    }
    
    pthread_mutex_unlock(&gameState.stateMutex);
    
    EndDrawing();
}

// Main function
int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize raylib
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "FAST-NU Pong Game - Multithreaded");
    InitAudioDevice();
    SetTargetFPS(60);
    
    // Initialize game state
    initializeGame();
    
    // Create threads
    pthread_t ballThread, aiThread;
    pthread_create(&ballThread, NULL, ballThreadFunc, NULL);
    pthread_create(&aiThread, NULL, aiThreadFunc, NULL);
    
    // Load sounds
    Sound paddleHitSound = LoadSound("resources/paddle_hit.wav");
    Sound wallHitSound = LoadSound("resources/wall_hit.wav");
    Sound scoreSound = LoadSound("resources/score.wav");
    
    // Main game loop
    while (!WindowShouldClose()) {
        pthread_mutex_lock(&gameState.stateMutex);
        
        // Process input
        if (IsKeyPressed(KEY_P)) {
            gameState.gamePaused = !gameState.gamePaused;
        }
        
        if (IsKeyPressed(KEY_L) && !gameState.gameOver && !gameState.gamePaused) {
            gameState.level = (gameState.level % 3) + 1;
        }
        
        if (IsKeyPressed(KEY_R) && gameState.gameOver) {
            // Reset game
            gameState.leftScore = 0;
            gameState.rightScore = 0;
            gameState.gameOver = false;
            gameState.ballPosition.x = SCREEN_WIDTH / 2;
            gameState.ballPosition.y = SCREEN_HEIGHT / 2;
            
            // Reset with level-appropriate speed
            float levelSpeedMultiplier = 1.0f + (gameState.level - 1) * 0.5f;
            gameState.ballVelocity.x = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * levelSpeedMultiplier;
            gameState.ballVelocity.y = INITIAL_BALL_SPEED * (GetRandomValue(0, 1) ? 1 : -1) * 0.8f * levelSpeedMultiplier;
        }
        
        // Player paddle control
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
        }
        
        pthread_mutex_unlock(&gameState.stateMutex);
        
        // Draw game
        drawGame();
    }
    
    // Cleanup
    pthread_mutex_destroy(&gameState.stateMutex);
    CloseWindow();
    
    return 0;
}