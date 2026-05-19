#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ─── CONSTANTS ────────────────────────────────────────────────────────────────
#define SW        1280
#define SH        720
#define CELL      32
#define MAP_ROWS  55
#define MAP_COLS  80
#define MAX_ENE   8
#define MAX_COI   32
#define MAX_ITM   4
#define MAX_PRT   2
#define MAX_PAR   256
#define INV_SZ    3

#define CLAMP255(v) ((unsigned char)((v)<0?0:((v)>255?255:(v))))

// ─── TYPES ────────────────────────────────────────────────────────────────────
typedef unsigned char uchar;
typedef enum { GS_MENU, GS_PLAY, GS_DEAD, GS_NEXT } GState;
typedef enum { IK_SPD=0, IK_SHD, IK_BMB, IK_NONE } IKind;

static const char* IK_NAME[] = { "SPD", "SHD", "BMB" };
// item colours (RGBA)
static const Color IK_COL[] = {
    { 80,220, 80,255 },   // SPD  green
    { 80,150,255,255 },   // SHD  blue
    {255,140, 40,255 }    // BMB  orange
};

typedef struct { float x,y,vx,vy,life,ml; Color c; float sz; } Par;
typedef struct { float x,y,mt,cd; bool on; } Ene;
typedef struct { int x,y; bool on; } CoinObj;
typedef struct { int x,y; bool on; IKind k; } ItmObj;
typedef struct { int x,y; bool on; } PrtObj;

// ─── GLOBALS ──────────────────────────────────────────────────────────────────
static GState    gs        = GS_MENU;
static char      map[MAP_ROWS][MAP_COLS];
static int       mw, mh;

// Player
static int    px, py;
static float  pmt, pmd;
static bool   pdead, pshield;
static float  pspd;  // speed boost timer
static IKind  pinv[INV_SZ];
static int    pinvn;

// World objects
static Ene    enes[MAX_ENE];
static CoinObj coins[MAX_COI];
static ItmObj  itms[MAX_ITM];
static PrtObj  prts[MAX_PRT];
static Par     pars[MAX_PAR];

// Stats / flags
static int   roundn, totcoi, tottrs;
static bool  eneon, wasportal;
static float sttime;   // state overlay timer
static Camera2D cam;

// ─── HIGHSCORES & PERSISTENCE ─────────────────────────────────────────────────
#define STATS_FILE "dungeons_stats.txt"
static int hs_round = 0;
static int hs_trs = 0;
static int hs_coi = 0;
static int global_totcoi = 0;

static void LoadStats(void) {
    FILE *f = fopen(STATS_FILE, "r");
    if (f) {
        fscanf(f, "%d %d %d %d", &hs_round, &hs_trs, &hs_coi, &global_totcoi);
        fclose(f);
    }
}

static void SaveStats(void) {
    FILE *f = fopen(STATS_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d\n", hs_round, hs_trs, hs_coi, global_totcoi);
        fclose(f);
    }
}

static void UpdateAndSaveStats(void) {
    if (roundn > hs_round) hs_round = roundn;
    if (tottrs > hs_trs) hs_trs = tottrs;
    if (totcoi > hs_coi) hs_coi = totcoi;
    SaveStats();
}

// ─── COLOUR PALETTE ───────────────────────────────────────────────────────────
#define CB     ((Color){12,10,18,255})       // void / bg
#define CF     ((Color){28,24,40,255})       // floor
#define CW     ((Color){50,45,66,255})       // wall face
#define CWH    ((Color){72,65,92,255})       // wall highlight
#define CWD    ((Color){18,15,28,255})       // wall shadow
#define CTRAP  ((Color){195,40,40,255})      // trap blood
#define CTRES  ((Color){255,200,50,255})     // treasure
#define CPL    ((Color){90,220,120,255})     // player
#define CPD    ((Color){230,55,55,255})      // player dead
#define CCOIN  ((Color){255,215,0,255})      // coin
#define CPORT  ((Color){80,160,255,255})     // portal
#define CENE   ((Color){220,65,65,255})      // enemy
#define CSHD   ((Color){80,150,255,255})     // shield aura

// ─── PARTICLE SYSTEM ──────────────────────────────────────────────────────────
static void SpawnPar(float x, float y, Color c,
                     float vx, float vy, float life, float sz) {
    for (int i = 0; i < MAX_PAR; i++) {
        if (pars[i].life <= 0) {
            pars[i] = (Par){x,y,vx,vy,life,life,c,sz};
            return;
        }
    }
}
static void Burst(float x, float y, Color c, int n, float spd) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / n * (2*PI) + GetRandomValue(0,100)*0.063f;
        float s = spd * (0.5f + GetRandomValue(0,100)*0.005f);
        float lf = 0.4f + GetRandomValue(0,60)*0.01f;
        float sz = (float)(2 + GetRandomValue(0,3));
        SpawnPar(x, y, c, cosf(a)*s, sinf(a)*s, lf, sz);
    }
}
static void UpdPars(float dt) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        p->x += p->vx * dt * 60;
        p->y += p->vy * dt * 60;
        p->vy += 0.05f * dt * 60;
        p->life -= dt;
    }
}
static void DrawPars(void) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        float a = p->life / p->ml;
        Color c = p->c; c.a = (uchar)(a * 255);
        DrawCircleV((Vector2){p->x, p->y}, p->sz * a + 1.0f, c);
    }
}

// ─── WORLD HELPERS ────────────────────────────────────────────────────────────
static inline float WX(int tx) { return tx * CELL + CELL * 0.5f; }
static inline float WY(int ty) { return ty * CELL + CELL * 0.5f; }

// ─── GENERATE ROUND ───────────────────────────────────────────────────────────
static void GenRound(void) {
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2*(roundn - 1); if (mw > 76) mw = 76;
    int tp = 8 + roundn;  if (tp > 34) tp = 34; 
    int xp = tp + 5;                              

    // ── Base map ──
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        int r = GetRandomValue(0,99);
        map[y][x] = (r < tp) ? '#' : (r < xp) ? '+' : '.';
    }

    // Player centre
    px = mw/2; py = mh/2; map[py][px] = '.';

    // Treasure – far from player
    for (;;) {
        int tx = GetRandomValue(1,mw-2), ty = GetRandomValue(1,mh-2);
        if (map[ty][tx]!='#' && !(tx==px&&ty==py)) { map[ty][tx]='T'; break; }
    }

    // Portals
    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;
    for (int i = 0; i < MAX_PRT; i++) {
        if (GetRandomValue(0,1)==0) {
            for (int t = 0; t < 300; t++) {
                int ox = GetRandomValue(1,mw-2), oy = GetRandomValue(1,mh-2);
                if (map[oy][ox]=='.') {
                    prts[i] = (PrtObj){ox, oy, true};
                    map[oy][ox] = 'O'; break;
                }
            }
        }
    }

    // Coins
    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    int nc = 8 + GetRandomValue(0,9); if (nc > MAX_COI) nc = MAX_COI;
    for (int placed=0, t=0; placed<nc && t<400; t++) {
        int cx = GetRandomValue(1,mw-2), cy = GetRandomValue(1,mh-2);
        if (map[cy][cx]=='.') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    // Items on map (2-3 per round)
    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    int ni = 2 + GetRandomValue(0,1); if (ni > MAX_ITM) ni = MAX_ITM;
    for (int placed=0, t=0; placed<ni && t<400; t++) {
        int ix = GetRandomValue(1,mw-2), iy = GetRandomValue(1,mh-2);
        if (map[iy][ix]=='.') {
            itms[placed++] = (ItmObj){ix, iy, true, (IKind)GetRandomValue(0,2)};
            map[iy][ix] = 'I';
        }
    }

    // Enemies (count grows with rounds)
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
    if (eneon) {
        int ne = 1 + roundn/2; if (ne > MAX_ENE) ne = MAX_ENE;
        for (int i=0, t=0; i<ne && t<400; t++) {
            int ex = GetRandomValue(1,mw-2), ey = GetRandomValue(1,mh-2);
            if (map[ey][ex]!='#' && abs(ex-px)+abs(ey-py) > 7) {
                enes[i++] = (Ene){(float)ex,(float)ey, 0,
                                  0.65f + GetRandomValue(0,35)*0.01f, true};
            }
        }
    }

    // Camera
    cam.offset   = (Vector2){SW/2.0f, SH/2.0f};
    cam.target   = (Vector2){WX(px), WY(py)};
    cam.zoom     = 1.0f;
    cam.rotation = 0;

    // Reset per-round state
    pdead  = false; pmt = 0; pmd = 0.13f;
    pspd   = 0; pshield = false;
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── USE ITEM ─────────────────────────────────────────────────────────────────
static void UseItm(int slot) {
    if (slot >= pinvn) return;
    IKind k = pinv[slot];
    switch (k) {
        case IK_SPD:
            pspd = 8.0f;
            Burst(WX(px), WY(py), IK_COL[IK_SPD], 16, 3);
            break;
        case IK_SHD:
            pshield = true;
            Burst(WX(px), WY(py), CSHD, 18, 3);
            break;
        case IK_BMB:
            for (int dy=-2; dy<=2; dy++)
            for (int dx=-2; dx<=2; dx++) {
                int nx=px+dx, ny=py+dy;
                if (nx>=0&&ny>=0&&nx<mw&&ny<mh && map[ny][nx]=='+') {
                    map[ny][nx]='.';
                    Burst(WX(nx), WY(ny), IK_COL[IK_BMB], 8, 3);
                }
            }
            break;
        default: break;
    }
    // Consume item – shift remaining
    for (int i = slot; i < pinvn-1; i++) pinv[i] = pinv[i+1];
    pinvn--;
}

// ─── PLAYER MOVEMENT ──────────────────────────────────────────────────────────
static void TryMove(int dx, int dy) {
    int nx = px+dx, ny = py+dy;
    if (nx<0||ny<0||nx>=mw||ny>=mh) return;
    char tg = map[ny][nx];
    if (tg=='#') return;

    // ── FIX: Kollision mit Gegnern beim Betreten ihrer Kachel ────────────────
    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            Ene *e = &enes[i]; if (!e->on) continue;
            if ((int)(e->x+0.5f)==nx && (int)(e->y+0.5f)==ny) {
                // Spieler läuft in Gegner → Tod
                px = nx; py = ny;  // Position kurz setzen für Partikel-Ort
                Burst(WX(px), WY(py), CPD, 32, 5);
                Burst(WX(px), WY(py), CENE, 16, 3);
                pdead = true; gs = GS_DEAD; sttime = 0;
                UpdateAndSaveStats();
                return;
            }
        }
    }

    px = nx; py = ny;

    // Trap
    if (tg=='+') {
        if (pshield) {
            pshield = false; map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Treasure
    if (tg=='T') {
        Burst(WX(px), WY(py), CTRES, 32, 5);
        tottrs++; wasportal = false; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
    // Coin
    if (tg=='$') {
        for (int i=0; i<MAX_COI; i++)
            if (coins[i].on && coins[i].x==px && coins[i].y==py)
                { coins[i].on=false; break; }
        map[py][px] = '.'; totcoi++;
        global_totcoi++;
        Burst(WX(px), WY(py), CCOIN, 10, 2);
        return;
    }
    // Item pickup
    if (tg=='I') {
        for (int i=0; i<MAX_ITM; i++) {
            if (itms[i].on && itms[i].x==px && itms[i].y==py) {
                if (pinvn < INV_SZ) {
                    IKind k = itms[i].k;
                    itms[i].on = false; map[py][px] = '.';
                    pinv[pinvn++] = k;
                    Burst(WX(px), WY(py), IK_COL[k], 16, 3);
                }
                break;
            }
        }
        return;
    }
    // Portal
    if (tg=='O') {
        Burst(WX(px), WY(py), CPORT, 32, 5);
        wasportal = true; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
}

// ─── ENEMY AI ─────────────────────────────────────────────────────────────────
static void UpdEnes(float dt) {
    if (!eneon) return;
    for (int i = 0; i < MAX_ENE; i++) {
        Ene *e = &enes[i]; if (!e->on) continue;
        e->mt += dt;
        if (e->mt < e->cd) continue;
        e->mt = 0;

        int ex = (int)(e->x + 0.5f), ey = (int)(e->y + 0.5f);
        int ddx = (px > ex) ? 1 : (px < ex) ? -1 : 0;
        int ddy = (py > ey) ? 1 : (py < ey) ? -1 : 0;

        // Randomly prefer axis to avoid predictable paths
        bool yFirst = (GetRandomValue(0,3) == 0);
        bool moved = false;

        if (!yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && map[ey][nx]!='#') { e->x=(float)nx; moved=true; }
        }
        if (!moved && ddy) {
            int ny = ey+ddy;
            if (ny>=0&&ny<mh && map[ny][ex]!='#') { e->y=(float)ny; moved=true; }
        }
        if (!moved && yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && map[ey][nx]!='#') { e->x=(float)nx; }
        }

        // Stepped on player?
        if ((int)(e->x+0.5f)==px && (int)(e->y+0.5f)==py) {
            Burst(WX(px), WY(py), CPD, 28, 5);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
    }
}

// ─── DRAW TILE ────────────────────────────────────────────────────────────────
static void DrawTile(int x, int y, float t) {
    int wx = x*CELL, wy = y*CELL;
    char c = map[y][x];
    
    char dc = c;

    // Gap / background
    DrawRectangle(wx, wy, CELL, CELL, CB);

    if (dc=='#') {
        // Wall body
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CW);
        // Bevel highlights
        DrawRectangle(wx+1, wy+1, CELL-2, 3, CWH);
        DrawRectangle(wx+1, wy+1, 3, CELL-2, CWH);
        // Shadow edge
        DrawRectangle(wx+1, wy+CELL-4, CELL-2, 3, CWD);
        DrawRectangle(wx+CELL-4, wy+1, 3, CELL-2, CWD);
    } else {
        // Floor
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CF);

        if (dc=='+') {
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){20, 15, 25, 255});
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){60, 50, 70, 255});
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    int spx = wx + 8 + i * 8;
                    int spy = wy + 8 + j * 8;
                    DrawCircle(spx, spy, 2, (Color){190, 190, 210, 255});
                    DrawRectangle(spx - 1, spy - 2, 2, 2, CTRAP);
                }
            }

        } else if (dc=='T') {
            float p = 0.5f + 0.5f*sinf(t * 3.0f);
            Color tc = CTRES; tc.a = (uchar)(200 + 55*p);
            DrawRectangle(wx+6,  wy+9,  CELL-12, CELL-14, tc);
            DrawRectangle(wx+6,  wy+9,  CELL-12, 4, (Color){255,240,180,230});
            DrawCircle(wx+CELL/2, wy+CELL/2+2, 3, (Color){200,150,30,255});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12 + 5*p), (Color){255,200,50,(int)(18+20*p)});

        } else if (dc=='$') {
            float p = sinf(t * 4.0f + x*0.5f + y*0.7f);
            int r = (int)(5 + p*1.5f);
            DrawCircle(wx+CELL/2+1, wy+CELL/2+2, r, (Color){160,120,0,80});
            DrawCircle(wx+CELL/2,   wy+CELL/2,   r, CCOIN);
            DrawCircle(wx+CELL/2-1, wy+CELL/2-1,
                       (int)(r*0.4f)+1, (Color){255,245,160,200});

        } else if (dc=='I') {
            IKind k = IK_NONE;
            for (int i=0;i<MAX_ITM;i++)
                if (itms[i].on&&itms[i].x==x&&itms[i].y==y) { k=itms[i].k; break; }
            if (k != IK_NONE) {
                Color ic = IK_COL[k];
                float p = 0.5f + 0.5f*sinf(t*3.0f + (float)(x+y)*0.3f);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL-4, wy+CELL/2},
                    (Vector2){wx+CELL/2, wy+CELL-4}, ic);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL/2, wy+CELL-4},
                    (Vector2){wx+4,      wy+CELL/2}, ic);
                Color gc = ic; gc.a = (uchar)(55 * p);
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(14+3*p), gc);
            }

        } else if (dc=='O') {
            float p = 0.5f + 0.5f*sinf(t*2.0f + (float)(x+y)*0.2f);
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12+5*p), (Color){80,160,255,(int)(65+50*p)});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(7+2*p),  (Color){160,210,255,(int)(140+60*p)});
            DrawCircleLines(wx+CELL/2, wy+CELL/2,
                            (int)(12+5*p), (Color){120,190,255,200});
        }
    }
}

// ─── DRAW HUD ─────────────────────────────────────────────────────────────────
static void DrawHUD(float t) {
    // Top bar (etwas höher für zweite Zeile)
    DrawRectangle(0, 0, SW, 76, (Color){8,6,14,235});
    DrawRectangle(0, 75, SW, 2, (Color){55,50,72,200});

    // ── Zeile 1: Hauptstats ─────────────────────────────────────────────────
    // Round
    DrawText(TextFormat("Round  %d", roundn), 16, 8, 26, (Color){210,205,230,255});

    // Coin icon + count
    DrawCircle(213, 21, 9, CCOIN);
    DrawCircle(210, 18, 4, (Color){255,245,160,200});
    DrawText(TextFormat("%d", totcoi), 228, 8, 26, CCOIN);

    // Treasures
    DrawRectangle(295, 10, 20, 15, CTRES);
    DrawRectangle(295, 10, 20, 4, (Color){255,240,180,200});
    DrawText(TextFormat("%d", tottrs), 323, 10, 20, (Color){200,180,80,255});

    // Enemy badge
    if (eneon) {
        DrawText("ENEMIES ON", SW/2 - MeasureText("ENEMIES ON",16)/2,
                 10, 16, (Color){220,80,80,160});
    } else {
        DrawText("ENEMIES OFF", SW/2 - MeasureText("ENEMIES OFF",16)/2,
                 10, 16, (Color){80,80,100,130});
    }

    // ── Zeile 2: Bestenwerte (links) ─────────────────────────────────────────
    // Trennlinie
    DrawLine(14, 38, 380, 38, (Color){45,40,62,180});

    // Beschriftung
    DrawText("HIGHSCORES:", 16, 42, 14, (Color){100,95,130,180});

    // Beste Runde
    bool newRound = (roundn > hs_round);
    Color rcol = newRound ? (Color){255,210,60,255} : (Color){160,155,185,200};
    DrawText(TextFormat("Round %d", newRound ? roundn : hs_round),
             16, 56, 14, rcol);

    // Beste Münzen (aktuell vs. Rekord)
    bool newCoi = (totcoi > hs_coi);
    Color ccol = newCoi ? (Color){255,215,0,255} : (Color){160,155,185,200};
    DrawCircle(108, 61, 5, newCoi ? CCOIN : (Color){130,110,0,180});
    DrawText(TextFormat("%d", newCoi ? totcoi : hs_coi), 118, 56, 14, ccol);

    // Beste Schätze
    bool newTrs = (tottrs > hs_trs);
    Color tcol = newTrs ? (Color){255,200,50,255} : (Color){160,155,185,200};
    DrawRectangle(176, 57, 10, 8, newTrs ? CTRES : (Color){130,100,0,160});
    DrawText(TextFormat("%d", newTrs ? tottrs : hs_trs), 190, 56, 14, tcol);

    // Lifetime-Coins
    DrawText(TextFormat("Total Coins: %d", global_totcoi),
             230, 56, 13, (Color){90,85,120,180});

    // ── Item inventory slots ──────────────────────────────────────────────────
    int sx = SW - 14 - INV_SZ * 68;
    DrawText("ITEMS", sx-52, 16, 15, (Color){130,125,160,190});
    for (int i = 0; i < INV_SZ; i++) {
        int bx = sx + i*68, by = 9;
        bool has = (i < pinvn);
        DrawRectangleRounded((Rectangle){bx,by,62,56}, 0.3f, 8,
                             has ? (Color){35,30,52,210} : (Color){20,18,32,120});
        Color bc = has ? IK_COL[pinv[i]] : (Color){50,46,68,140};
        DrawRectangleRoundedLines((Rectangle){bx,by,62,56}, 0.3f, 8, bc);
        if (has) {
            DrawText(IK_NAME[pinv[i]], bx+6, by+5, 14, IK_COL[pinv[i]]);
            DrawText(TextFormat("[%d]",i+1), bx+6, by+36, 12, (Color){130,125,155,200});
        } else {
            DrawText(TextFormat("[%d]",i+1), bx+20, by+22, 13, (Color){65,60,85,140});
        }
    }

    // ── Active effects (bottom-left) ─────────────────────────────────────────
    int efx = 14, efy = SH - 32;
    if (pspd > 0) {
        DrawText(TextFormat("SPEED  %.1fs", pspd), efx, efy, 18,
                 (Color){80,220,80,255}); efx += 155;
    }
    if (pshield) {
        DrawText("SHIELD", efx, efy, 18, (Color){80,150,255,255}); efx += 95;
    }

    // Key hints bottom right
    DrawText("WASD/Arrows:Move   1/2/3:Use Item",
             SW - MeasureText("WASD/Arrows:Move   1/2/3:Use Item",14) - 14,
             SH-22, 14, (Color){80,75,100,160});
}

// ─── DRAW MENU ────────────────────────────────────────────────────────────────
static void DrawMenu(float t) {
    ClearBackground(CB);

    // Animated dungeon-tile background
    for (int y=0; y<=SH/CELL; y++)
    for (int x=0; x<=SW/CELL; x++) {
        float f = 0.35f + 0.25f*sinf(x*0.38f + y*0.47f + t*0.45f);
        DrawRectangle(x*CELL, y*CELL, CELL-1, CELL-1,
                      (Color){(uchar)(16*f),(uchar)(13*f),(uchar)(24*f),255});
    }

    // Centre panel
    int pw=640, ph=430, pox=SW/2-pw/2, poy=SH/2-ph/2;
    DrawRectangleRounded((Rectangle){pox,poy,pw,ph}, 0.06f, 8,
                         (Color){14,11,22,230});
    DrawRectangleRoundedLines((Rectangle){pox,poy,pw,ph}, 0.06f, 8, (Color){65,58,88,200});

    // Title
    const char *TT = "DUNGEON CRAWLER";
    int ts = 56, tw = MeasureText(TT, ts);
    DrawText(TT, SW/2-tw/2+3, poy+32+3, ts, (Color){0,0,0,140});
    DrawText(TT, SW/2-tw/2,   poy+32,   ts, (Color){215,210,235,255});

    DrawLine(pox+40, poy+104, pox+pw-40, poy+104, (Color){65,58,88,180});

    float pp = 0.5f + 0.5f*sinf(t * 2.2f);
    Color sc = {(uchar)(70+(int)(140*pp)), (uchar)(195+(int)(25*pp)), 110, 255};
    const char *ST = "PRESS  ENTER  TO  START";
    DrawText(ST, SW/2 - MeasureText(ST,30)/2, poy+122, 30, sc);

    const char *ET = eneon
        ? "Enemies :  ON    [E to toggle]"
        : "Enemies :  OFF   [E to toggle]";
    Color ec = eneon ? (Color){220,70,70,255} : (Color){100,95,140,200};
    DrawText(ET, SW/2 - MeasureText(ET,22)/2, poy+172, 22, ec);

    DrawLine(pox+40, poy+210, pox+pw-40, poy+210, (Color){55,50,75,120});

    struct { const char* sym; const char* desc; Color col; } leg[] = {
        { "T", "Treasure | Advance round",        CTRES  },
        { "O", "Portal | Skip to next round",     CPORT  },
        { "+", "Trap | Deadly spike pit!",        (Color){190,190,210,255}},
        { "$", "Coin | Collect for score",        CCOIN  },
        { "I", "Item | Pick up & use with 1/2/3", IK_COL[IK_SPD] },
        { "X", "Enemy | Avoid! (toggleable)",     CENE  },
    };
    int ly = poy+222, lx = pox+48;
    for (int i=0; i<6; i++) {
        DrawText(leg[i].sym, lx,        ly+i*24, 18, leg[i].col);
        DrawText("–",        lx+22,     ly+i*24, 18, (Color){70,65,90,180});
        DrawText(leg[i].desc,lx+38,     ly+i*24, 16, (Color){160,155,185,220});
    }
    
    const char *ileg[] = {
        "SPD - 2x speed 8s",
        "SHD - block next trap",
        "BMB - blast 5x5 traps"
    };
    int ilx = pox+360;
    for (int i=0;i<3;i++) {
        DrawRectangle(ilx-4, ly+i*24, 4, 18, IK_COL[i]);
        DrawText(ileg[i], ilx+6, ly+i*24, 14, (Color){155,150,180,210});
    }

    int hsy = ly + 85;
    DrawText("LIFETIME STATS", ilx-4, hsy, 16, (Color){200,180,80,255});
    DrawText(TextFormat("Total Coins: %d", global_totcoi), ilx-4, hsy+24, 14, CCOIN);
    DrawText(TextFormat("Best Round: %d", hs_round), ilx-4, hsy+44, 14, (Color){160,155,185,220});
    DrawText(TextFormat("Most Treasures: %d", hs_trs), ilx-4, hsy+64, 14, (Color){160,155,185,220});
    DrawText(TextFormat("Most Coins: %d", hs_coi), ilx-4, hsy+84, 14, (Color){160,155,185,220});

    DrawText("v1.0", pox+pw-44, poy+ph-22, 13, (Color){55,50,75,180});
}

// ─── DRAW OVERLAY (death / next-round) ────────────────────────────────────────
static void DrawOverlay(float t) {
    DrawRectangle(0, 0, SW, SH, (Color){0,0,0,175});

    if (gs == GS_DEAD) {
        float pp = 0.5f + 0.5f*sinf(sttime * 5.0f);
        const char *M = "YOU  DIED";
        int ms = 64, mw2 = MeasureText(M, ms);
        DrawText(M, SW/2-mw2/2+2, SH/2-56+2, ms, (Color){0,0,0,140});
        DrawText(M, SW/2-mw2/2,   SH/2-56,   ms,
                 (Color){230,(uchar)(38+(int)(28*pp)),38,255});
        const char *S = TextFormat("Round %d   Coins %d   Treasures %d",
                                   roundn, totcoi, tottrs);
        DrawText(S, SW/2-MeasureText(S,22)/2, SH/2+22, 22,
                 (Color){185,180,205,215});
        DrawText("ENTER to restart   |   Q for menu",
                 SW/2-MeasureText("ENTER to restart   |   Q for menu",20)/2,
                 SH/2+62, 20, (Color){140,135,165,195});

    } else if (gs == GS_NEXT) {
        const char *M = wasportal ? "PORTAL  USED !" : "TREASURE  FOUND !";
        Color mc     = wasportal ? CPORT : CTRES;
        int ms = 52, mw2 = MeasureText(M, ms);
        DrawText(M, SW/2-mw2/2+2, SH/2-48+2, ms, (Color){0,0,0,140});
        DrawText(M, SW/2-mw2/2,   SH/2-48,   ms, mc);

        const char *C = TextFormat("Coins : %d", totcoi);
        DrawText(C, SW/2-MeasureText(C,28)/2, SH/2+22, 28, CCOIN);
        DrawText("ENTER for next round",
                 SW/2-MeasureText("ENTER for next round",20)/2,
                 SH/2+64, 20, (Color){140,135,165,195});
    }
}

// ─── MAIN ─────────────────────────────────────────────────────────────────────
int main(void) {
    SetRandomSeed((unsigned)time(NULL));
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "Dungeon Crawler");
    SetTargetFPS(60);

    LoadStats();

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
    eneon = true; pinvn = 0;
    memset(pars, 0, sizeof(pars));

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float t  = GetTime();

        switch (gs) {

        case GS_MENU:
            if (IsKeyPressed(KEY_E)) eneon = !eneon;
            if (IsKeyPressed(KEY_ENTER)) {
                roundn=1; totcoi=0; tottrs=0; pinvn=0;
                GenRound(); gs = GS_PLAY;
            }
            break;

        case GS_PLAY:
            if (IsKeyPressed(KEY_ONE))   UseItm(0);
            if (IsKeyPressed(KEY_TWO))   UseItm(1);
            if (IsKeyPressed(KEY_THREE)) UseItm(2);

            if (pspd > 0) pspd -= dt;

            { float md = (pspd > 0) ? pmd * 0.5f : pmd;
              pmt += dt;
              if (pmt >= md) {
                  bool moved = false;
                  if      (IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))
                      { TryMove(0,-1); moved=true; }
                  else if (IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))
                      { TryMove(0,+1); moved=true; }
                  else if (IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))
                      { TryMove(-1,0); moved=true; }
                  else if (IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))
                      { TryMove(+1,0); moved=true; }
                  if (moved) pmt = 0;
              }
            }

            if (gs != GS_PLAY) break;

            UpdEnes(dt);
            UpdPars(dt);

            cam.target.x += (WX(px) - cam.target.x) * dt * 9.0f;
            cam.target.y += (WY(py) - cam.target.y) * dt * 9.0f;
            break;

        case GS_DEAD:
        case GS_NEXT:
            sttime += dt;
            UpdPars(dt);
            if (sttime > 0.45f) {
                if (gs == GS_DEAD) {
                    if (IsKeyPressed(KEY_ENTER))
                        { roundn=1;totcoi=0;tottrs=0;pinvn=0; GenRound(); gs=GS_PLAY; }
                    if (IsKeyPressed(KEY_Q)) gs = GS_MENU;
                } else {
                    if (IsKeyPressed(KEY_ENTER))
                        { roundn++; GenRound(); gs=GS_PLAY; }
                }
            }
            break;
        }

        BeginDrawing();
        ClearBackground(CB);

        if (gs == GS_MENU) {
            DrawMenu(t);
        } else {

            BeginMode2D(cam);

            for (int y=0; y<mh; y++)
            for (int x=0; x<mw; x++)
                DrawTile(x, y, t);

            if (eneon) {
                for (int i=0; i<MAX_ENE; i++) {
                    Ene *e = &enes[i]; if (!e->on) continue;
                    int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
                    float ewx = ex*CELL, ewy = ey*CELL;
                    float pp = 0.5f + 0.5f*sinf(t*4.0f + i);
                    DrawCircle((int)ewx+CELL/2, (int)ewy+CELL/2,
                               (int)(CELL/2+4+2*pp), (Color){200,40,40,(int)(28+18*pp)});
                    DrawCircle((int)ewx+CELL/2, (int)ewy+CELL/2, CELL/2-4, CENE);
                    DrawCircle((int)ewx+CELL/2-2,(int)ewy+CELL/2-3, CELL/4,
                               (Color){240,100,100,110});
                    DrawCircle((int)ewx+CELL/2-4,(int)ewy+CELL/2-2, 3, (Color){255,200,0,255});
                    DrawCircle((int)ewx+CELL/2+4,(int)ewy+CELL/2-2, 3, (Color){255,200,0,255});
                    DrawCircle((int)ewx+CELL/2-4,(int)ewy+CELL/2-2, 1, (Color){20,0,0,255});
                    DrawCircle((int)ewx+CELL/2+4,(int)ewy+CELL/2-2, 1, (Color){20,0,0,255});
                }
            }

            { int cx = px*CELL + CELL/2, cy = py*CELL + CELL/2;
              Color pc = pdead ? CPD : CPL;
              if (pshield) {
                  float pp = 0.5f + 0.5f*sinf(t*5.0f);
                  DrawCircle(cx, cy, CELL/2+6, (Color){80,150,255,(int)(38+28*pp)});
                  DrawCircleLines(cx, cy, CELL/2+6,(Color){80,150,255,(int)(155+65*pp)});
              }
              if (pspd > 0) {
                  DrawCircle(cx, cy, CELL/2+3, (Color){80,220,80,28});
              }
              DrawCircle(cx, cy, CELL/2-4, pc);
              DrawCircle(cx-2, cy-3, CELL/4,
                         (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),120});
              DrawCircle(cx-4, cy-2, 2, (Color){10,10,22,255});
              DrawCircle(cx+4, cy-2, 2, (Color){10,10,22,255});
            }

            DrawPars();
            EndMode2D();

            DrawHUD(t);

            if (gs == GS_DEAD || gs == GS_NEXT) DrawOverlay(t);
        }

        EndDrawing();
    }

    UpdateAndSaveStats();
    CloseWindow();
    return 0;
}
