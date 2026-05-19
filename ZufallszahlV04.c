#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {

int number;
int guess;
int attempts;
int balance = 0;
int round = 0;
int continueing;

printf("You will need to guess a number between 1-100.\n");
printf("If you guess the number correctly, you will win 7$.\n");
printf("Each guess will cost you 1$.\n");
printf("After each guess you will be told if you guessed too high or too low.\n");
printf("If you want to play, enter 1! If not, press any other key!\n");

scanf("%d", &continueing);
while (1) {
if (continueing == 1) {
    round++;

while (1) {
attempts = 0;
srand(time(NULL));
number = rand() % 100 + 1;
printf("Round %d\n", round);
printf("Guess the number (1-100)\n");

while (1) {

scanf("%d", &guess);
attempts++;

if (guess > number) {
printf("Too high!\n");
balance --;}

else if (guess < number) {
printf("Too low!\n");
balance --;}

else {
printf("Correct! Attempts: %d\n", attempts);
balance += 7;
balance --;
printf("Your current balance is %d$\n", balance);
break;
}

}
printf("Do you want to play another game?\nIf yes, enter 1\nIf no, enter any other key\n");
scanf("%d", &continueing);
if (continueing == 1) {
    round++;
}
else {
    break;
}

}
}
else {
    break;
}
printf("You started with 0$\n");
printf("Now, your balance is %d$", balance);
}
return 0;
}
