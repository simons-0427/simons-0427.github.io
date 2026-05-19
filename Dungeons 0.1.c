#include <stdio.h>
//Dies sind Ideen, die ich noch hinzufügen will
//eine richtige Karte anzeigen
//Kollisionen mit Wänden
//Items einsammeln
//ein kleines Dungeon-Spiel
int main() {

int x = 0;
int y = 0;
char input;

printf("Enter w/a/s/d to move\n");
printf("Enter q to quit\n");

while (1) {
printf("Your position is (%d|%d)\n", x, y);
scanf(" %c", &input);
if (input == 'q') break;
if (input == 'w') y++;
if (input == 'a') x--;
if (input == 's') y--;
if (input == 'd') x++;
}
printf("Thanks for playing");





return 0;
}
