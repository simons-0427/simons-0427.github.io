#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {

int x = 2;
int y = 2;
char input;
char map[10][20];
for (int row = 0; row < 10; row++) {
    for (int col = 0; col < 20; col++) {
        map[row][col] = '.' ;}
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
if (input == 'w') y--;
if (input == 'a') x--;
if (input == 's') y++;
if (input == 'd') x++;
}
printf("Thanks for playing");





return 0;
}
