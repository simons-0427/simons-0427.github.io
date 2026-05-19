#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {

int x = 11;
int y = 6;
char input;
char map[13][23];
 for (int row = 0; row < 13; row++) {
        for (int col = 0; col < 23; col++) {
            if (row == 0 || row == 12 || col == 0 || col == 22) {
                map[row][col] = '#';
            } else {
                map[row][col] = '.';
            }
        }
    }

int height = sizeof(map) / sizeof(map[0]);
int width = sizeof(map[0]) / sizeof(map[0][0]);

while (1) {
system("cls");
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
scanf(" %c", &input);
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
}
printf("Thanks for playing");

return 0;
}
