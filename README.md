# PingPongGame
## Ping Pong Game with Multithreading
This repository contains a multithreaded implementation of the classic Ping Pong game. The game is designed to demonstrate the use of multithreading, synchronization, and real-time rendering using the Raylib library. It includes multiple versions of the game with varying levels of graphical complexity and gameplay features.

## Features
Multithreading: The game uses multiple threads for ball movement, AI paddle control, and rendering to ensure smooth gameplay.
Graphics Options:

DarkGraphics.c: A visually enhanced version with dynamic backgrounds, level-based effects, and smooth animations.

LightGraphics.c: A lightweight graphical version for systems with lower performance.

PingPong.c: A full-featured version with advanced AI and level-based difficulty.

### Console Version:
PingPong(WithoutGraphics).c: A terminal-based version of the game for environments without graphical support.

Dynamic Difficulty: The game includes level-based difficulty adjustments for AI and ball speed.

Power-Ups (Console Version): Random power-ups like faster ball, larger paddles, and slower opponents

Single and Multiplayer Modes: Play against the AI or with a friend in two-player mode.

Customizable Gameplay: Change levels, pause the game, or restart at any time.

## Prerequisites

To run the game, ensure you have the following installed:

GCC Compiler: Required to compile the C code.

Raylib Library: For graphical versions of the game.

Pthread Library: For multithreading support.

Audio Resources: Ensure the resources folder contains the required sound files:

paddle_hit.wav

wall_hit.wav

score.wav

# Installation
Clone the repository.

Install Raylib: Follow the installation instructions for your platform from the Raylib official website.

Ensure the resources folder is in the same directory as the game files.

## How to Build and Run

Open a terminal in the project directory.

Build the game using the provided build.bash script:
```bash
bash build.bash
```

Run the game:
```bash
./a.out
```

## Controls
### General Controls

W/S: Move Player 1's paddle up/down.

Arrow Keys (Up/Down): Move Player 2's paddle (in multiplayer mode).

P: Pause/Resume the game.

L: Change the game level.

R: Restart the game after it ends.

M: Return to mode selection (after game over).

Q: Quit the game (console version).

### Console Version (PingPong(WithoutGraphics).c)
Power-Ups:

S: Speed Boost

L: Larger Paddle

D: Slow Opponent

### Game Modes
Single Player: Play against the AI.

Two Player: Play with a friend locally.

## Known Issues
Ensure the resources folder is present with the required sound files to avoid runtime errors.

The game may not run on systems without Raylib installed.

## License

Proprietary License

Copyright (c) 2025 Zain Allaudin

All rights reserved.

This software and associated documentation files (PingPongGame) are the exclusive property of Zain Allaudin. Unauthorized copying, modification, distribution, or use of the Software, in whole or in part, is strictly prohibited without prior written permission from the copyright holder.

The Software is provided "as is," without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement. In no event shall the copyright holder be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the Software or the use or other dealings in the Software.

For inquiries regarding licensing, please contact Zain Allaudin.

email: zainallaudin007@gmail.com
