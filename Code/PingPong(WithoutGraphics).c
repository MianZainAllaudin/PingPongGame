#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#define WIDTH 80
#define HEIGHT 24
#define PADDLE_HEIGHT 5
#define WINNING_SCORE 10
typedef struct {
    int ball_x;
    int ball_y;
    int ball_dx;
    int ball_dy;
    int ball_speed;
    int paddle1_y;
    int paddle2_y;
    int paddle_speed;
    int score1;
    int score2;
    int game_over;
    int pause;
    int power_up_active;
    int power_up_type;
    int power_up_x;
    int power_up_y;
    int power_up_timer;
    pthread_mutex_t mutex;     // Mutex for thread synchronization
} GameState;
GameState game;
void initGame() {
    if (pthread_mutex_init(&game.mutex, NULL) != 0) {
        printf("Mutex initialization failed\n");
        exit(1);
    }
    game.ball_x = WIDTH / 2;
    game.ball_y = HEIGHT / 2;
    srand(time(NULL));
    game.ball_dx = (rand() % 2) * 2 - 1;  // -1 or 1
    game.ball_dy = (rand() % 2) * 2 - 1;  // -1 or 1
    game.ball_speed = 1;
    game.paddle1_y = HEIGHT / 2 - PADDLE_HEIGHT / 2;     // Set initial paddle positions
    game.paddle2_y = HEIGHT / 2 - PADDLE_HEIGHT / 2;
    game.paddle_speed = 1;    
    game.score1 = 0; // Reset scores
    game.score2 = 0;
    game.game_over = 0;     // Game not over and not paused initially
    game.pause = 0;
    game.power_up_active = 0;     // Initialize power-up
    game.power_up_type = 0;
    game.power_up_timer = 0;
}
void resetBall() {
    pthread_mutex_lock(&game.mutex);
    game.ball_x = WIDTH / 2;     // Reset to center
    game.ball_y = HEIGHT / 2;
    game.ball_dx = (rand() % 2) * 2 - 1;  // -1 or 1
    game.ball_dy = (rand() % 2) * 2 - 1;  // -1 or 1
    game.ball_speed = 1;
    pthread_mutex_unlock(&game.mutex);
    usleep(500000);
}
void spawnPowerUp() {
    if (!game.power_up_active && rand() % 100 < 5) {  // 5% chance each update
        pthread_mutex_lock(&game.mutex);
        game.power_up_active = 1;
        game.power_up_type = rand() % 3;  // 0: faster ball, 1: larger paddle, 2: slower opponent
        game.power_up_x = WIDTH / 4 + rand() % (WIDTH / 2);  
        game.power_up_y = 2 + rand() % (HEIGHT - 4);        
        pthread_mutex_unlock(&game.mutex);
    }
}
void applyPowerUp(int player) {
    pthread_mutex_lock(&game.mutex);
    switch (game.power_up_type) {
        case 0:  // Faster ball
            game.ball_speed = 2;
            break;
        case 1:  // Larger paddle (temporary)
            if (player == 1) {
                game.paddle1_y -= 1;  // Extend paddle by 2 units
            } else {
                game.paddle2_y -= 1;
            }
            break;
        case 2:  // Slow opponent
            if (player == 1) {
                game.paddle_speed = 2;  // Player 2 slowed down
            } else {
                game.paddle_speed = 2;  // Player 1 slowed down
            }
            break;
    }
    game.power_up_active = 0;
    game.power_up_timer = 100;  // Effect lasts for 100 updates
    pthread_mutex_unlock(&game.mutex);
}
void* ballThread(void* arg) {
    while (!game.game_over) {
        if (game.pause) {
            usleep(100000);
            continue;
        }
        pthread_mutex_lock(&game.mutex);
        for (int i = 0; i < game.ball_speed; i++) {
            game.ball_x += game.ball_dx;
            game.ball_y += game.ball_dy;
            if (game.ball_y <= 0 || game.ball_y >= HEIGHT - 1) {             // Check for wall collisions (top and bottom)
                game.ball_dy = -game.ball_dy;
            }
            if (game.ball_x == 1 && 
                game.ball_y >= game.paddle1_y && 
                game.ball_y < game.paddle1_y + PADDLE_HEIGHT) {
                game.ball_dx = -game.ball_dx;
                if (rand() % 3 == 0) {                 // Add some randomness to bounce
                    game.ball_dy = (rand() % 2) * 2 - 1;
                }
            }
            if (game.ball_x == WIDTH - 2 && 
                game.ball_y >= game.paddle2_y && 
                game.ball_y < game.paddle2_y + PADDLE_HEIGHT) {
                game.ball_dx = -game.ball_dx;
                if (rand() % 3 == 0) {
                    game.ball_dy = (rand() % 2) * 2 - 1;
                }
            }
            if (game.power_up_active && 
                game.ball_x == game.power_up_x && 
                game.ball_y == game.power_up_y) {
                int player = (game.ball_dx > 0) ? 1 : 2;
                applyPowerUp(player);
            }
            if (game.ball_x <= 0) {
                game.score2++;
                pthread_mutex_unlock(&game.mutex);
                resetBall();
                pthread_mutex_lock(&game.mutex);
                if (game.score2 >= WINNING_SCORE) {
                    game.game_over = 1;
                }
                break;
            }
            if (game.ball_x >= WIDTH - 1) {
                game.score1++;
                pthread_mutex_unlock(&game.mutex);
                resetBall();
                pthread_mutex_lock(&game.mutex);
                if (game.score1 >= WINNING_SCORE) {
                    game.game_over = 1;
                }
                break;
            }
        }
        if (game.power_up_timer > 0) {
            game.power_up_timer--;
            if (game.power_up_timer == 0) {
                game.ball_speed = 1;
                game.paddle_speed = 1;
            }
        }
        pthread_mutex_unlock(&game.mutex);
        usleep(100000);  // 0.1 seconds
    }
    return NULL;
}
void* aiPaddleThread(void* arg) {
    while (!game.game_over) {
        if (game.pause) {
            usleep(100000);
            continue;
        }
        pthread_mutex_lock(&game.mutex);
        int target_y = game.ball_y - PADDLE_HEIGHT / 2;
        if (rand() % 10 < 2) {  // 20% chance of moving randomly
            target_y += (rand() % 5) - 2;
        }
        if (game.paddle2_y < target_y) {
            game.paddle2_y += game.paddle_speed;
        } else if (game.paddle2_y > target_y) {
            game.paddle2_y -= game.paddle_speed;
        }
        if (game.paddle2_y < 0) {
            game.paddle2_y = 0;
        } else if (game.paddle2_y > HEIGHT - PADDLE_HEIGHT) {
            game.paddle2_y = HEIGHT - PADDLE_HEIGHT;
        }
        pthread_mutex_unlock(&game.mutex);
        usleep(150000);  // 0.15 seconds (AI reaction time)
    } 
    return NULL;
}
void* powerUpThread(void* arg) {
    while (!game.game_over) {
        if (game.pause) {
            usleep(100000);
            continue;
        }
        spawnPowerUp();
        usleep(2000000);  // 2 seconds
    }
    return NULL;
}
void enableRawMode() { // Function to enable non-blocking keyboard input
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}
void disableRawMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    fcntl(STDIN_FILENO, F_SETFL, 0);
}
void* inputThread(void* arg) {
    char c;
    enableRawMode();
    while (!game.game_over) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            pthread_mutex_lock(&game.mutex);
            switch (c) {
                case 'w':
                case 'W':
                    game.paddle1_y -= game.paddle_speed;
                    if (game.paddle1_y < 0) {
                        game.paddle1_y = 0;
                    }
                    break;
                case 's':
                case 'S':
                    game.paddle1_y += game.paddle_speed;
                    if (game.paddle1_y > HEIGHT - PADDLE_HEIGHT) {
                        game.paddle1_y = HEIGHT - PADDLE_HEIGHT;
                    }
                    break;
                    
                case 'p':
                case 'P':
                    game.pause = !game.pause;
                    break;
                case 'q':
                case 'Q':
                    game.game_over = 1;
                    break;
            }
            pthread_mutex_unlock(&game.mutex);
        }
        usleep(10000);  // 0.01 seconds
    }
    disableRawMode();
    return NULL;
}
void renderGame() {
    printf("\033[2J\033[H"); // Clear screen (ANSI escape code)
    char display[HEIGHT][WIDTH + 1];
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            display[y][x] = ' ';
        }
        display[y][WIDTH] = '\0';
    }
    pthread_mutex_lock(&game.mutex);
    for (int x = 0; x < WIDTH; x++) {
        display[0][x] = '=';
        display[HEIGHT - 1][x] = '=';
    }
    for (int y = 1; y < HEIGHT - 1; y++) {     // Draw center line
        if (y % 2 == 0) {
            display[y][WIDTH / 2] = '|';
        }
    }
    sprintf(&display[1][WIDTH / 2 - 10], "Player: %d", game.score1);
    sprintf(&display[1][WIDTH / 2 + 4], "AI: %d", game.score2);
    if (game.ball_y >= 0 && game.ball_y < HEIGHT && 
        game.ball_x >= 0 && game.ball_x < WIDTH) {
        display[game.ball_y][game.ball_x] = 'O';
    }
    if (game.power_up_active) {
        char power_up_symbols[] = {'S', 'L', 'D'};  // Speed, Large, Debuff
        if (game.power_up_y >= 0 && game.power_up_y < HEIGHT && 
            game.power_up_x >= 0 && game.power_up_x < WIDTH) {
            display[game.power_up_y][game.power_up_x] = power_up_symbols[game.power_up_type];
        }
    }
    for (int y = 0; y < PADDLE_HEIGHT; y++) {
        if (game.paddle1_y + y >= 0 && game.paddle1_y + y < HEIGHT) {
            display[game.paddle1_y + y][1] = '|';
        }
    }
    for (int y = 0; y < PADDLE_HEIGHT; y++) {
        if (game.paddle2_y + y >= 0 && game.paddle2_y + y < HEIGHT) {
            display[game.paddle2_y + y][WIDTH - 2] = '|';
        }
    }
    if (game.pause) {
        const char* pause_msg = "GAME PAUSED - Press P to resume";
        int msg_len = strlen(pause_msg);
        int start_x = (WIDTH - msg_len) / 2;
        
        for (int i = 0; i < msg_len; i++) {
            display[HEIGHT / 2][start_x + i] = pause_msg[i];
        }
    }
    if (game.game_over) {
        const char* game_over_msg;
        if (game.score1 >= WINNING_SCORE) {
            game_over_msg = "GAME OVER - YOU WIN!";
        } else {
            game_over_msg = "GAME OVER - AI WINS!";
        }
        int msg_len = strlen(game_over_msg);
        int start_x = (WIDTH - msg_len) / 2;
        for (int i = 0; i < msg_len; i++) {
            display[HEIGHT / 2][start_x + i] = game_over_msg[i];
        }
    }
    pthread_mutex_unlock(&game.mutex);
    for (int y = 0; y < HEIGHT; y++) {
        printf("%s\n", display[y]);
    }
    printf("\nControls: W - Move Up, S - Move Down, P - Pause, Q - Quit\n");
    printf("Power-ups: S - Speed Boost, L - Larger Paddle, D - Slow Opponent\n");
}
void* renderThread(void* arg) {
    while (!game.game_over) {
        renderGame();
        usleep(50000);  // Render at 20 FPS
    }
    renderGame();     // Final render after game over
    return NULL;
}
void handleSignal(int signum) {
    disableRawMode();
    printf("\nGame terminated by signal %d\n", signum);
    exit(0);
}
int main() {
    signal(SIGINT, handleSignal); // Set up signal handler
    printf("=== Zain Allaudin_PING PONG ===\n");
    printf("Controls: W - Move Up, S - Move Down, P - Pause, Q - Quit\n");
    printf("First to score %d points wins!\n", WINNING_SCORE);
    printf("Special power-ups will appear during the game!\n");
    printf("Press Enter to start...");
    getchar();
    initGame();
    pthread_t ball_thread, ai_thread, input_thread, render_thread, power_up_thread;
    int ret;
    ret = pthread_create(&ball_thread, NULL, ballThread, NULL);
    if (ret != 0) {
        printf("Error creating ball thread: %d\n", ret);
        return 1;
    }
    ret = pthread_create(&ai_thread, NULL, aiPaddleThread, NULL);
    if (ret != 0) {
        printf("Error creating AI thread: %d\n", ret);
        return 1;
    }
    ret = pthread_create(&input_thread, NULL, inputThread, NULL);
    if (ret != 0) {
        printf("Error creating input thread: %d\n", ret);
        return 1;
    }
    ret = pthread_create(&render_thread, NULL, renderThread, NULL);
    if (ret != 0) {
        printf("Error creating render thread: %d\n", ret);
        return 1;
    }
    ret = pthread_create(&power_up_thread, NULL, powerUpThread, NULL);
    if (ret != 0) {
        printf("Error creating power-up thread: %d\n", ret);
        return 1;
    }
    pthread_join(ball_thread, NULL);
    pthread_join(ai_thread, NULL);
    pthread_join(input_thread, NULL);
    pthread_join(render_thread, NULL);
    pthread_join(power_up_thread, NULL);
    pthread_mutex_destroy(&game.mutex);
    printf("\nGame Over! Final Score: Player %d - AI %d\n", game.score1, game.score2);
    printf("Thanks for playing!\n");
    return 0;
}