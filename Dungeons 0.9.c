#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define RESET  "\x1b[0m"
#define FLOOR "\x1b[38;5;250m"
#define PLAYER "\x1b[38;5;46m"
#define WALL "\x1b[0m"
#define TRAP "\x1b[38;5;196m"
#define TREASURE "\x1b[38;5;214m"

int main() {
int round = 1;
int treasuresfound = 0;
while (1) {
    system("cls");
    srand(time(NULL));
    char input;
    char map[13 + 2*(round - 1)][23 + 2*(round - 1)];
    int height = 13 + (round - 1);
    int width = 23 + 2*(round - 1);
    int specialX = rand() % (width - 2) + 1;
    int specialY = rand() % (height - 2) + 1;
    int x = width / 2;
    int y = height / 2;
    if (specialX == x) specialX++;
    if (specialY == y) specialY++;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == 0 || row == height-1 || col == 0 || col == width-1) {
                map[row][col] = '#';
            } else {
                int r = rand() % 100;
                if (r < 8 + 0.5* round)
                    map[row][col] = '#';
                else if (r < 13 + round)
                    map[row][col] = '+';
                else
                    map[row][col] = '.';}
        }
    }
    map[specialY][specialX] = 'T';
    map[y][x] = '.';

while (1) {
printf("\033[H");
printf("Round %d\n", round);
printf("Treasures found: %d\n",treasuresfound);
printf("Enter w/a/s/d to move\n");
printf("Enter q to quit\n");
for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == y && col == x) {
    printf(PLAYER "X" RESET);
}
else if (map[row][col] == '#') {
    printf(WALL "#" RESET);
}
else if (map[row][col] == '+') {
    printf(TRAP "+" RESET);
}
else if (map[row][col] == 'T') {
    printf(TREASURE "T" RESET);
}
else {
    printf(FLOOR "." RESET);
}
        }

printf("\n");
}
input = _getch();
if (input == 'q') break;

int newX = x;
int newY= y;

if (input == 'w') newY--;
if (input == 'a') newX--;
if (input == 's') newY++;
if (input == 'd') newX++;

if (map[newY][newX]!= '#') {
    x = newX;
    y = newY;
}
Sleep(50);
if (map[newY][newX]== '+') {
printf("You lost\nGame Over\n");
round = 1;
treasuresfound = 0;
break;
}
if (map[newY][newX]== 'T') {
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
    break;}
}
printf("\nThanks for playing");

return 0;
}
