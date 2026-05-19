#include <stdio.h>
#include <stdlib.h>
//Dies sind Ideen, die ich noch hinzufügen will
//Kollisionen mit Wänden
//eine bessere Karte anzeigen
//Items einsammeln
//ein kleines Dungeon-Spiel

int main() {

int x = 2;
int y = 2;
char input;
char map[5][5] = {
{'.', '.', '.', '.', '.'},
{'.', '.', '.', '.', '.'},
{'.', '.', '.', '.', '.'},
{'.', '.', '.', '.', '.'},
{'.', '.', '.', '.', '.'}
};

while (1) {
system("cls");
printf("Enter w/a/s/d to move\n");
printf("Enter q to quit\n");
for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col ++) {
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
