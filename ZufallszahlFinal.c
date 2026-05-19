#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Definition der verschiedenen Spielbildschirme inklusive Bestätigungs-Screen
typedef enum GameScreen { 
    SCREEN_MENU, 
    SCREEN_TUTORIAL, 
    SCREEN_GAME, 
    SCREEN_CONFIRM_RESET 
} GameScreen;

// Funktionen zum Speichern und Laden des Kontostands
int LoadBalance() {
    int balance = 0;
    FILE *file = fopen("save.txt", "r");
    if (file != NULL) {
        fscanf(file, "%d", &balance);
        fclose(file);
    }
    return balance;
}

void SaveBalance(int balance) {
    FILE *file = fopen("save.txt", "w");
    if (file != NULL) {
        fprintf(file, "%d", balance);
        fclose(file);
    }
}

int main(void) {
    // Initialisierung des Fensters
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Zahlenraten Deluxe");
    SetTargetFPS(60);
    srand(time(NULL));

    // Spielvariablen
    GameScreen currentScreen = SCREEN_MENU;
    int balance = LoadBalance();
    int targetNumber = 0;
    int attempts = 0;
    int roundCount = 1;
    
    // Variablen für die Texteingabe
    char guessBuffer[4] = "\0"; 
    int letterCount = 0;
    
    // Feedback-Text für den Spieler
    char feedbackText[60] = "Gib eine Zahl ein und drücke ENTER!";
    Color feedbackColor = MAROON;
    bool gameWon = false;

    // Hauptspielschleife
    while (!WindowShouldClose()) {
        // --- LOGIK ---
        switch (currentScreen) {
            case SCREEN_MENU: {
                // Tastensteuerung im Hauptmenü
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_ENTER)) {
                    currentScreen = SCREEN_GAME;
                    targetNumber = rand() % 100 + 1;
                    attempts = 0;
                    gameWon = false;
                    letterCount = 0;
                    guessBuffer[0] = '\0';
                    TextCopy(feedbackText, "Gib eine Zahl ein und drücke ENTER!");
                    feedbackColor = LIGHTGRAY;
                }
                if (IsKeyPressed(KEY_T)) {
                    currentScreen = SCREEN_TUTORIAL;
                }
                if (IsKeyPressed(KEY_R)) {
                    currentScreen = SCREEN_CONFIRM_RESET; // Wechselt zur Bestätigung
                }
            } break;

            case SCREEN_CONFIRM_RESET: {
                // Bestätigungsmenü abfragen
                if (IsKeyPressed(KEY_Y)) { // Y für Ja (Yes)
                    balance = 0;
                    SaveBalance(balance);
                    currentScreen = SCREEN_MENU;
                }
                if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) { // N oder ESC für Nein
                    currentScreen = SCREEN_MENU;
                }
            } break;

            case SCREEN_TUTORIAL: {
                if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_ESCAPE)) {
                    currentScreen = SCREEN_MENU;
                }
            } break;

            case SCREEN_GAME: {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    currentScreen = SCREEN_MENU;
                }

                if (!gameWon) {
                    // Zeichen-Eingabe für die Zahl (nur Tasten 0-9)
                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= '0') && (key <= '9') && (letterCount < 3)) {
                            guessBuffer[letterCount] = (char)key;
                            guessBuffer[letterCount + 1] = '\0';
                            letterCount++;
                        }
                        key = GetCharPressed();
                    }

                    // Backspace zum Löschen einzelner Ziffern
                    if (IsKeyPressed(KEY_BACKSPACE)) {
                        letterCount--;
                        if (letterCount < 0) letterCount = 0;
                        guessBuffer[letterCount] = '\0';
                    }

                    // Auswertung bei ENTER-Druck
                    if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                        int currentGuess = atoi(guessBuffer);
                        attempts++;
                        balance--; // Jeder Versuch kostet 1$
                        
                        if (currentGuess > targetNumber) {
                            TextCopy(feedbackText, "Zu hoch! Versuche es noch einmal.");
                            feedbackColor = RED;
                        } else if (currentGuess < targetNumber) {
                            TextCopy(feedbackText, "Zu niedrig! Versuche es noch einmal.");
                            feedbackColor = RED;
                        } else {
                            // Gewonnen!
                            TextCopy(feedbackText, "Richtig erraten! Du erhältst +8$!");
                            feedbackColor = LIME;
                            balance += 8;
                            gameWon = true;
                            SaveBalance(balance); // Sofort in save.txt sichern
                        }
                        
                        // Buffer nach dem Tippen leeren
                        guessBuffer[0] = '\0';
                        letterCount = 0;
                    }
                } else {
                    // Wenn die Runde gewonnen wurde, startet ein erneuter ENTER-Druck das Spiel neu
                    if (IsKeyPressed(KEY_ENTER)) {
                        roundCount++;
                        targetNumber = rand() % 100 + 1;
                        attempts = 0;
                        gameWon = false;
                        TextCopy(feedbackText, "Neue Runde! Gib eine Zahl ein.");
                        feedbackColor = LIGHTGRAY;
                    }
                }
            } break;
        }

        // --- ZEICHNEN ---
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF)); // Schicker Dark-Mode Hintergrund

        // Permanenter Kontostand oben rechts (außer im Tutorial und Reset-Bildschirm)
        if (currentScreen != SCREEN_TUTORIAL && currentScreen != SCREEN_CONFIRM_RESET) {
            DrawText(TextFormat("Kontostand: %d$", balance), screenWidth - 220, 20, 24, GREEN);
        }

        switch (currentScreen) {
            case SCREEN_MENU: {
                DrawText("ZAHLENRATEN DELUXE", screenWidth/2 - MeasureText("ZAHLENRATEN DELUXE", 40)/2, 150, 40, GOLD);
                
                DrawText("[S] Spiel Starten", screenWidth/2 - MeasureText("[S] Spiel Starten", 20)/2, 280, 20, RAYWHITE);
                DrawText("[T] Tutorial anzeigen", screenWidth/2 - MeasureText("[T] Tutorial anzeigen", 20)/2, 330, 20, RAYWHITE);
                DrawText("[R] Kontostand zurücksetzen", screenWidth/2 - MeasureText("[R] Kontostand zurücksetzen", 16)/2, 450, 16, RED);
                
                DrawText("Drücke die entsprechende Taste", screenWidth/2 - MeasureText("Drücke die entsprechende Taste", 14)/2, 540, 14, GRAY);
            } break;

            case SCREEN_CONFIRM_RESET: {
                // Bestätigungsfenster (Hintergrund bleibt leicht sichtbar verdunkelt)
                DrawText("BIST DU SICHER?", screenWidth/2 - MeasureText("BIST DU SICHER?", 32)/2, 180, 32, RED);
                DrawText("Dein gesamtes gespartes Geld wird gelöscht!", screenWidth/2 - MeasureText("Dein gesamtes gespartes Geld wird gelöscht!", 20)/2, 240, 20, RAYWHITE);
                
                DrawText("[Y] JA, ALLES LÖSCHEN", screenWidth/2 - MeasureText("[Y] JA, ALLES LÖSCHEN", 20)/2, 340, 20, RED);
                DrawText("[N] NEIN, ABBRECHEN", screenWidth/2 - MeasureText("[N] NEIN, ABBRECHEN", 20)/2, 390, 20, LIME);
            } break;

            case SCREEN_TUTORIAL: {
                DrawText("ANLEITUNG", screenWidth/2 - MeasureText("ANLEITUNG", 30)/2, 80, 30, GOLD);
                
                int startY = 180;
                int spacing = 40;
                DrawText("- Computer wählt eine geheime Zahl zwischen 1 und 100.", 50, startY, 20, RAYWHITE);
                DrawText("- Jeder Tipp kostet dich genau 1$.", 50, startY + spacing, 20, RAYWHITE);
                DrawText("- Errätst du die Zahl richtig, gewinnst du 8$.", 50, startY + (spacing * 2), 20, RAYWHITE);
                DrawText("- Das Spiel zeigt dir, ob dein Tipp zu hoch oder zu niedrig war.", 50, startY + (spacing * 3), 20, RAYWHITE);
                DrawText("- Dein Kontostand wird automatisch permanent gespeichert!", 50, startY + (spacing * 4), 20, GREEN);

                DrawText("[B] Zurück zum Hauptmenü", screenWidth/2 - MeasureText("[B] Zurück zum Hauptmenü", 20)/2, 480, 20, GOLD);
            } break;

            case SCREEN_GAME: {
                DrawText(TextFormat("Runde: %d", roundCount), 40, 20, 24, BLUE);
                DrawText(TextFormat("Versuche: %d", attempts), 40, 50, 20, ORANGE);

                DrawText("Errate die Zahl (1-100):", screenWidth/2 - MeasureText("Errate die Zahl (1-100):", 24)/2, 180, 24, RAYWHITE);

                // Hier ist die korrigierte Raylib-Funktion für das Rechteck:
                DrawRectangleLines(screenWidth/2 - 60, 240, 120, 50, GRAY);
                
                if (letterCount == 0 && !gameWon) {
                    DrawText("???", screenWidth/2 - MeasureText("???", 28)/2, 250, 28, DARKGRAY);
                } else {
                    DrawText(guessBuffer, screenWidth/2 - MeasureText(guessBuffer, 28)/2, 250, 28, RAYWHITE);
                }

                // Feedback-Texte
                DrawText(feedbackText, screenWidth/2 - MeasureText(feedbackText, 20)/2, 340, 20, feedbackColor);

                if (gameWon) {
                    DrawText("Drücke ENTER für die nächste Runde!", screenWidth/2 - MeasureText("Drücke ENTER für die nächste Runde!", 18)/2, 420, 18, GOLD);
                }
                
                DrawText("[ESC] Zurück zum Menü", 40, screenHeight - 40, 16, GRAY);
            } break;
        }

        EndDrawing();
    }

    // Fenster schließen und Ressourcen freigeben
    CloseWindow();

    return 0;
}