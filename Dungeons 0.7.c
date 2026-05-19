#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

int main() {
int round = 1;
int treasuresfound = 0;
while (1) {
    system("cls");
    srand(time(NULL));
    int x = 11;
    int y = 6;
    char input;
    char map[13][23];
    int height = 13;
    int width = 23;
    int specialX = rand() % (width - 2) + 1;
    int specialY = rand() % (height - 2) + 1;
    if (specialX == x) specialX++;
    if (specialY == y) specialY++;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == 0 || row == height-1 || col == 0 || col == width-1) {
                map[row][col] = '#';
            } else {
                int r = rand() % 100;
                if (r < 8)
                    map[row][col] = '#';
                else if (r < 13)
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
                printf("X");}
            else {
                printf("%c", map[row][col]);}
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
round ++;
break;
}
if (map[newY][newX]== 'T') {
printf("You found the treasure\nYou won\nGame Over\n");
round ++;
treasuresfound ++;
break;
}
}
printf("Do you want to play again?(y/n)\n");
char anothergame;
anothergame = _getch();
if (anothergame != 'y') {
    break;}
}
printf("Thanks for playing");

return 0;
}
