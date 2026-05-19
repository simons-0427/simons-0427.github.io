#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define RESET  "\x1b[0m"
#define FLOOR "\x1b[38;5;250m"
#define PLAYER "\x1b[38;5;46m"
#define PLAYER_DEAD "\x1b[38;5;196m"
#define WALL "\x1b[38;5;240m"
#define TRAP "\x1b[38;5;196m"
#define TREASURE "\x1b[38;5;214m"

void draw(int height, int width, int x, int y, int round, int treasuresfound, char map[height][width], int dead) {
    printf("\x1b[H");
    printf("Round %d\n", round);
    printf("Treasures found: %d\n",treasuresfound);
    printf("Enter w/a/s/d to move\n");
    printf("Enter q to quit\n");

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == y && col == x) {
                if (dead)
                    printf(PLAYER_DEAD "X" RESET);
                else
                    printf(PLAYER "X" RESET);
            }
            else if (map[row][col] == '#') printf(WALL "#" RESET);
            else if (map[row][col] == '+') printf(TRAP "+" RESET);
            else if (map[row][col] == 'T') printf(TREASURE "T" RESET);
            else printf(FLOOR "." RESET);
        }
        printf("\n");
    }
}

int main() {
int round = 1;
int treasuresfound = 0;
srand(time(NULL));

while (1) {
    system("cls");
    int height = 13 + (round - 1);
    int width = 23 + 2*(round - 1);

    if (round <= 27) height = 13 + (round - 1);
    else height = 13 + 27;

    char map[height][width];

    char input;

    int specialX = rand() % (width - 2) + 1;
    int specialY = rand() % (height - 2) + 1;

    int x = width / 2;
    int y = height / 2;

    while (specialX == x && specialY == y) {
        specialX = rand() % (width - 2) + 1;
        specialY = rand() % (height - 2) + 1;
    }

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == 0 || row == height-1 || col == 0 || col == width-1) {
                map[row][col] = '#';
            } else {
                int r = rand() % 100;
                if (r < 8 + round)
                    map[row][col] = '#';
                else if (r < 13 + round)
                    map[row][col] = '+';
                else
                    map[row][col] = '.';
            }
        }
    }

    map[specialY][specialX] = 'T';
    map[y][x] = '.';

    while (1) {

        draw(height, width, x, y, round, treasuresfound, map, 0);

        input = _getch();

        int newX = x;
        int newY= y;

        if (input == 'q'){

        for (int i = 0; i < 3; i++) {
        draw(height, width, x, y, round, treasuresfound, map, 1);
        Sleep(120);
        printf("\x1b[H");
        draw(height, width, x, y, round, treasuresfound, map, 0);
        Sleep(120);
    }
    draw(height, width, x, y, round, treasuresfound, map, 1);
    printf("You lost\nGame Over\n");
    round = 1;
    treasuresfound = 0;
    break;
    }
        if (input == 'w') newY--;
        if (input == 'a') newX--;
        if (input == 's') newY++;
        if (input == 'd') newX++;

        char target = map[newY][newX];

        if (target != '#') {
            x = newX;
            y = newY;
        }

        Sleep(50);

        if (target == '+') {
            for (int i = 0; i < 3; i++) {
                draw(height, width, x, y, round, treasuresfound, map, 1);
                Sleep(120);
                printf("\x1b[H");
                draw(height, width, x, y, round, treasuresfound, map, 0);
                Sleep(120);
            }

            draw(height, width, x, y, round, treasuresfound, map, 1);
            printf("You lost\nGame Over\n");

            round = 1;
            treasuresfound = 0;
            break;
        }

        if (target == 'T') {
            for (int i = 0; i < 3; i++) {
                draw(height, width, x, y, round, treasuresfound, map, 0);
                printf("\x1b[38;5;226m");
                Sleep(120);
                draw(height, width, x, y, round, treasuresfound, map, 0);
                printf("\x1b[H");
                Sleep(120);
            }

            draw(height, width, x, y, round, treasuresfound, map, 0);
            printf("You found the treasure\nYou won\n");

            round ++;
            treasuresfound ++;
            break;
        }
    }

    printf("\nDo you want to play again?\nIf yes, press any key\nIf no, press q\n");
    Sleep(100);

    char anothergame;
    anothergame = _getch();
    if (anothergame == 'q') {
        break;
    }
}

printf("\nThanks for playing");

return 0;
}
