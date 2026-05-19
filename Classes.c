/* ============================================================
   RPG Class Manager  —  raylib 5.0+
   ============================================================ */
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

/* ── Constants ──────────────────────────────────────────────── */
#define SW          1100
#define SH          700
#define SIDEBAR_W   282
#define HEADER_H     62
#define MAX_CLASSES  32
#define MAX_LEVELS   20
#define MAX_NAME     64
#define SAVE_FILE   "rpg_save.dat"

/* ── Colour Palette (dark RPG / parchment-gold theme) ───────── */
static const Color C_BG      = {10,  10,  20,  255};
static const Color C_PANEL   = {18,  18,  34,  255};
static const Color C_PANEL2  = {24,  24,  44,  255};
static const Color C_PANEL3  = {33,  33,  57,  255};
static const Color C_GOLD    = {178, 138, 42,  255};
static const Color C_GOLD_L  = {232, 188, 72,  255};
static const Color C_TEXT    = {224, 218, 196, 255};
static const Color C_MUTED   = {118, 113, 98,  255};
static const Color C_GREEN   = {62,  148, 58,  255};
static const Color C_GREEN_L = {175, 238, 170, 255};
static const Color C_RED     = {182, 56,  56,  255};
static const Color C_BLUE    = {62,  112, 192, 255};
static const Color C_BORDER  = {40,  40,  68,  255};
static const Color C_INPUT   = {12,  12,  24,  255};

/* ── Types ──────────────────────────────────────────────────── */
typedef struct {
    char name[MAX_NAME];
    int  levelCap;
    int  allocatedXP;
    int  levelReqs[MAX_LEVELS]; /* -1 = not yet set by user */
} Class;

typedef struct {
    char buf[128];
    bool active;
} TBox;

/* ── Global State ───────────────────────────────────────────── */
static int   totalXP      = 0;
static Class classes[MAX_CLASSES];
static int   classCount   = 0;
static int   selectedCls  = -1;
static bool  showModal    = false;

static TBox  tbNewName  = {{0}, false};
static int   newCap     = 10;
static TBox  tbTotalXP  = {{0}, false};
static TBox  tbDistXP   = {{0}, false};

static float sideScroll   = 0.f;
static float detailScroll = 0.f;

static char  notifMsg[220] = {0};
static float notifTimer    = 0.f;

/* ── Utility ────────────────────────────────────────────────── */
static void Notif(const char *msg)
{
    strncpy(notifMsg, msg, sizeof(notifMsg) - 1);
    notifTimer = 3.2f;
}

static Color Brighten(Color c, float f)
{
    return (Color){
        (unsigned char)fminf(255.f, (float)c.r * f),
        (unsigned char)fminf(255.f, (float)c.g * f),
        (unsigned char)fminf(255.f, (float)c.b * f),
        c.a
    };
}

static int GetLevel(const Class *c)
{
    int lv = 0;
    for (int i = 0; i < c->levelCap; i++)
        if (c->levelReqs[i] >= 0 && c->levelReqs[i] <= c->allocatedXP)
            lv = i + 1;
    return lv;
}

static int TotalAllocated(void)
{
    int t = 0;
    for (int i = 0; i < classCount; i++) t += classes[i].allocatedXP;
    return t;
}

static int AvailXP(void) { return totalXP - TotalAllocated(); }

/* Build the txt filename from a class name (spaces → underscores, lowercase). */
static void ClassFname(const Class *c, char *out, int n)
{
    snprintf(out, n, "%s_levels.txt", c->name);
    for (int i = 0; out[i]; i++) {
        if (out[i] == ' ') out[i] = '_';
        out[i] = (char)tolower((unsigned char)out[i]);
    }
}

/* ── File I/O ───────────────────────────────────────────────── */
static void CreateClassFile(const Class *c)
{
    char fn[256];
    ClassFname(c, fn, sizeof(fn));
    FILE *f = fopen(fn, "w");
    if (!f) return;

    fprintf(f, "# %s  -  Level XP Requirements\n", c->name);
    fprintf(f, "# ─────────────────────────────────────────────\n");
    fprintf(f, "# Edit the numbers after each colon.\n");
    fprintf(f, "# Level 1 is usually 0 XP (starting level).\n");
    fprintf(f, "# After saving this file, click [Reload] in the app.\n");
    fprintf(f, "#\n");
    for (int i = 0; i < c->levelCap; i++)
        fprintf(f, "Level %d: 0\n", i + 1);
    fclose(f);
}

static void LoadClassFile(Class *c)
{
    char fn[256];
    ClassFname(c, fn, sizeof(fn));
    FILE *f = fopen(fn, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        int lv, xp;
        if (sscanf(line, "Level %d: %d", &lv, &xp) == 2 &&
            lv >= 1 && lv <= c->levelCap)
            c->levelReqs[lv - 1] = xp;
    }
    fclose(f);
}

/*
 * Save format (pipe-separated to support spaces in class names):
 *   TOTAL_XP 5000
 *   CLASS_COUNT 2
 *   CLASS Fire Mage|15|2400
 */
static void SaveState(void)
{
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return;
    fprintf(f, "TOTAL_XP %d\nCLASS_COUNT %d\n", totalXP, classCount);
    for (int i = 0; i < classCount; i++)
        fprintf(f, "CLASS %s|%d|%d\n",
                classes[i].name, classes[i].levelCap, classes[i].allocatedXP);
    fclose(f);
}

static void LoadState(void)
{
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return;
    char line[256];
    classCount = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "TOTAL_XP", 8) == 0) {
            sscanf(line, "TOTAL_XP %d", &totalXP);
            snprintf(tbTotalXP.buf, sizeof(tbTotalXP.buf), "%d", totalXP);
        } else if (strncmp(line, "CLASS ", 6) == 0 && classCount < MAX_CLASSES) {
            Class *c = &classes[classCount++];
            memset(c, 0, sizeof(*c));
            for (int j = 0; j < MAX_LEVELS; j++) c->levelReqs[j] = -1;

            char *p     = line + 6;
            char *pipe1 = strchr(p, '|');
            char *pipe2 = pipe1 ? strchr(pipe1 + 1, '|') : NULL;
            if (pipe1 && pipe2) {
                int nl = (int)(pipe1 - p);
                if (nl >= MAX_NAME) nl = MAX_NAME - 1;
                strncpy(c->name, p, nl);
                c->name[nl]    = '\0';
                c->levelCap    = atoi(pipe1 + 1);
                c->allocatedXP = atoi(pipe2 + 1);
            }
            LoadClassFile(c);
        }
    }
    fclose(f);
}

/* ── Text Box ───────────────────────────────────────────────── */
static void TBoxTick(TBox *t, Rectangle r)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        t->active = CheckCollisionPointRec(GetMousePosition(), r);
    if (!t->active) return;
    for (int k = GetCharPressed(); k; k = GetCharPressed()) {
        int n = (int)strlen(t->buf);
        if (k >= 32 && n < (int)sizeof(t->buf) - 1)
        { t->buf[n] = (char)k; t->buf[n + 1] = '\0'; }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(t->buf) > 0)
        t->buf[strlen(t->buf) - 1] = '\0';
    if (IsKeyPressed(KEY_ESCAPE)) t->active = false;
}

static void TBoxDraw(const TBox *t, Rectangle r, const char *ph)
{
    DrawRectangleRounded(r, 0.12f, 8, C_INPUT);
    DrawRectangleRoundedLinesEx(r, 0.12f, 8, 1.5f,
                                t->active ? C_BLUE : C_BORDER);
    bool has = (strlen(t->buf) > 0);
    DrawText(has ? t->buf : ph,
             (int)r.x + 10,
             (int)(r.y + (r.height - 18.f) / 2.f),
             18, has ? C_TEXT : C_MUTED);
    if (t->active && has && (int)(GetTime() * 2.0) % 2 == 0) {
        int tw = MeasureText(t->buf, 18);
        DrawRectangle((int)r.x + 11 + tw,
                      (int)(r.y + (r.height - 18.f) / 2.f),
                      2, 18, C_TEXT);
    }
}

/* ── Drawing Primitives ─────────────────────────────────────── */
static void Panel(Rectangle r, Color bg, Color border)
{
    DrawRectangleRounded(r, 0.06f, 8, bg);
    DrawRectangleRoundedLinesEx(r, 0.06f, 8, 1.5f, border);
}

static bool Btn(Rectangle r, const char *txt, Color bg, Color fg)
{
    bool hov  = CheckCollisionPointRec(GetMousePosition(), r);
    bool down = hov && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    Color c = down ? Brighten(bg, 0.78f) : hov ? Brighten(bg, 1.28f) : bg;
    DrawRectangleRounded(r, 0.18f, 8, c);
    int fs = 16, tw = MeasureText(txt, fs);
    DrawText(txt, (int)(r.x + (r.width  - (float)tw) / 2.f),
                  (int)(r.y + (r.height - (float)fs) / 2.f), fs, fg);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static bool SmallBtn(Rectangle r, const char *txt, Color bg, Color fg)
{
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    Color c  = hov ? Brighten(bg, 1.28f) : bg;
    DrawRectangleRounded(r, 0.20f, 8, c);
    int fs = 13, tw = MeasureText(txt, fs);
    DrawText(txt, (int)(r.x + (r.width  - (float)tw) / 2.f),
                  (int)(r.y + (r.height - (float)fs) / 2.f), fs, fg);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void ProgBar(Rectangle r, float pct, Color fill)
{
    DrawRectangleRounded(r, 0.5f, 8, C_PANEL3);
    if (pct > 0.001f) {
        Rectangle f = {r.x, r.y, r.width * fminf(pct, 1.f), r.height};
        DrawRectangleRounded(f, 0.5f, 8, fill);
    }
}

static void Badge(int x, int y, const char *txt, Color bg, Color fg)
{
    int tw = MeasureText(txt, 13);
    DrawRectangleRounded((Rectangle){(float)x, (float)y, (float)(tw + 16), 22.f},
                         0.5f, 8, bg);
    DrawText(txt, x + 8, y + 4, 13, fg);
}

/* ── Modal: Add Class ───────────────────────────────────────── */
static void DrawModal(void)
{
    DrawRectangle(0, 0, SW, SH, (Color){0, 0, 0, 155});

    Rectangle m = {(float)(SW / 2 - 242), (float)(SH / 2 - 218), 484.f, 436.f};
    Panel(m, C_PANEL, C_GOLD);

    DrawText("Add New Class", (int)m.x + 22, (int)m.y + 16, 22, C_GOLD_L);
    DrawLineEx((Vector2){m.x + 22, m.y + 52},
               (Vector2){m.x + m.width - 22, m.y + 52}, 1.f, C_BORDER);

    int px = (int)m.x + 24, py = (int)m.y + 64;

    /* Class name */
    DrawText("Class Name", px, py, 13, C_MUTED);
    py += 18;
    Rectangle nr = {(float)px, (float)py, m.width - 48.f, 40.f};
    TBoxTick(&tbNewName, nr);
    TBoxDraw(&tbNewName, nr, "e.g.  Warrior,  Mage,  Ranger ...");
    py += 56;

    /* Level cap slider */
    char capLabel[32];
    snprintf(capLabel, sizeof(capLabel), "Level Cap  —  %d", newCap);
    DrawText(capLabel, px, py, 13, C_MUTED);
    py += 20;

    float t = (newCap - 5) / 15.f;
    Rectangle track = {(float)px, (float)(py + 8), m.width - 48.f, 6.f};
    DrawRectangleRounded(track, 0.5f, 8, C_BORDER);

    /* Filled portion */
    if (t > 0.f) {
        Rectangle fill = {track.x, track.y, t * track.width, track.height};
        DrawRectangleRounded(fill, 0.5f, 8, C_GOLD);
    }

    /* Knob */
    float kx = track.x + t * track.width;
    DrawCircle((int)kx, (int)(track.y + 3.f), 11, C_PANEL3);
    DrawCircle((int)kx, (int)(track.y + 3.f),  9, C_GOLD_L);

    /* Drag */
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        if (mp.y >= track.y - 14.f && mp.y <= track.y + 26.f &&
            mp.x >= track.x - 12.f && mp.x <= track.x + track.width + 12.f) {
            float nt = (mp.x - track.x) / track.width;
            nt = fmaxf(0.f, fminf(1.f, nt));
            newCap = 5 + (int)roundf(nt * 15.f);
        }
    }

    DrawText("5",  px,               py + 24, 12, C_MUTED);
    DrawText("20", (int)(m.x + m.width - 36), py + 24, 12, C_MUTED);
    py += 56;

    /* Info text */
    DrawText("A .txt file is created automatically. Open it in any",
             px, py, 13, C_MUTED);
    py += 18;
    DrawText("text editor, fill in XP values, then click Reload.",
             px, py, 13, C_MUTED);
    py += 46;

    /* Buttons */
    Rectangle cancelR = {(float)px,                     (float)py, 200.f, 42.f};
    Rectangle addR    = {m.x + m.width - 224.f, (float)py, 200.f, 42.f};

    if (Btn(cancelR, "Cancel", C_PANEL3, C_TEXT)) {
        showModal = false;
        memset(tbNewName.buf, 0, sizeof(tbNewName.buf));
    }

    if (Btn(addR, "Add Class + Create File", C_GOLD, C_BG)) {
        if (strlen(tbNewName.buf) > 0 && classCount < MAX_CLASSES) {
            Class *c = &classes[classCount++];
            memset(c, 0, sizeof(*c));
            for (int j = 0; j < MAX_LEVELS; j++) c->levelReqs[j] = -1;
            strncpy(c->name, tbNewName.buf, MAX_NAME - 1);
            c->levelCap    = newCap;
            c->allocatedXP = 0;
            CreateClassFile(c);
            SaveState();
            char fn[256]; ClassFname(c, fn, sizeof(fn));
            Notif(TextFormat("'%s' added! Edit %s and click Reload.", c->name, fn));
            showModal = false;
            memset(tbNewName.buf, 0, sizeof(tbNewName.buf));
            newCap = 10;
        } else {
            Notif("Please enter a class name first.");
        }
    }
}

/* ── Detail Panel (right side) ──────────────────────────────── */
static void DrawDetail(int x, int y, int w, int h)
{
    if (selectedCls < 0 || selectedCls >= classCount) {
        const char *hint = "<-- Select a class or add a new one.";
        DrawText(hint,
                 x + (w - MeasureText(hint, 17)) / 2,
                 y + h / 2 - 9, 17, C_MUTED);
        return;
    }

    Class *c   = &classes[selectedCls];
    int    lv  = GetLevel(c);
    int    av  = AvailXP();

    /* Mouse-wheel scroll */
    if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){x, y, w, h}))
        detailScroll -= GetMouseWheelMove() * 52.f;
    if (detailScroll < 0.f) detailScroll = 0.f;

    BeginScissorMode(x, y, w, h);

    int px = x + 22;
    int pw = w - 44;
    int py = y + 18 - (int)detailScroll;

    /* ── Name + Level badge ── */
    int nw = MeasureText(c->name, 28);
    DrawText(c->name, px, py, 28, C_GOLD_L);
    char lvs[32];
    snprintf(lvs, sizeof(lvs), "Lv %d / %d", lv, c->levelCap);
    Badge(px + nw + 14, py + 6, lvs,
          lv == c->levelCap ? C_GREEN : C_PANEL3,
          lv == c->levelCap ? C_GREEN_L : C_MUTED);
    if (lv == c->levelCap)
        Badge(px + nw + 14 + MeasureText(lvs, 13) + 36, py + 6,
              "MAX LEVEL", C_GREEN, C_GREEN_L);
    py += 48;

    /* ── XP row + bar ── */
    DrawText(TextFormat("Allocated XP:  %d", c->allocatedXP),
             px, py, 16, C_TEXT);
    py += 26;
    ProgBar((Rectangle){(float)px, (float)py, (float)pw, 8},
             c->levelCap > 0 ? (float)lv / (float)c->levelCap : 0.f,
             C_GOLD);
    py += 22;

    /* ── Distribute XP ── */
    Panel((Rectangle){(float)px, (float)py, (float)pw, 100.f}, C_PANEL2, C_BORDER);
    DrawText("Distribute XP", px + 14, py + 11, 14, C_MUTED);

    Rectangle ir = {(float)(px + 14), (float)(py + 34), 192.f, 38.f};
    TBoxTick(&tbDistXP, ir);
    TBoxDraw(&tbDistXP, ir, "Amount...");

    DrawText(TextFormat("Pool: %d XP", av),
             px + 216, py + 42, 13, av > 0 ? C_GREEN : C_RED);

    Rectangle addB = {(float)(px + pw - 184), (float)(py + 28), 88.f, 40.f};
    Rectangle subB = {(float)(px + pw -  90), (float)(py + 28), 80.f, 40.f};

    if (Btn(addB, "+ Add", C_GOLD, C_BG)) {
        int amt = atoi(tbDistXP.buf);
        if (amt > 0) {
            int add = amt > av ? av : amt;
            c->allocatedXP += add;
            SaveState();
            Notif(TextFormat("Added %d XP to %s.", add, c->name));
        }
    }
    if (Btn(subB, "- Sub", C_PANEL3, C_TEXT)) {
        int amt = atoi(tbDistXP.buf);
        if (amt > 0) {
            c->allocatedXP -= amt;
            if (c->allocatedXP < 0) c->allocatedXP = 0;
            SaveState();
            Notif(TextFormat("Removed %d XP from %s.", amt, c->name));
        }
    }
    py += 116;

    /* ── Level Requirements Grid ── */
    DrawText("Level Requirements", px, py, 15, C_TEXT);
    char fn[256]; ClassFname(c, fn, sizeof(fn));
    DrawText(fn, px + MeasureText("Level Requirements", 15) + 14, py + 2,
             12, C_MUTED);

    if (SmallBtn((Rectangle){(float)(px + pw - 142), (float)(py - 2), 136.f, 26.f},
                 "Reload from file", C_PANEL3, C_MUTED)) {
        for (int i = 0; i < MAX_LEVELS; i++) c->levelReqs[i] = -1;
        LoadClassFile(c);
        Notif(TextFormat("Reloaded requirements for %s.", c->name));
    }
    py += 34;

    /* 5-column grid */
    int cols = 5;
    int cw   = pw / cols;
    int ch   = 58;
    for (int i = 0; i < c->levelCap; i++) {
        int col = i % cols;
        int row = i / cols;
        Rectangle cell = {
            (float)(px + col * cw),
            (float)(py + row * ch),
            (float)(cw - 6),
            (float)(ch - 6)
        };
        bool reached = (c->levelReqs[i] >= 0 && c->levelReqs[i] <= c->allocatedXP);
        bool cur     = (i + 1 == lv);

        Color bg = (reached || cur) ? (Color){26, 48, 24, 255} : C_PANEL2;
        Color bd = (reached || cur) ? C_GREEN : C_BORDER;

        DrawRectangleRounded(cell, 0.10f, 8, bg);
        DrawRectangleRoundedLinesEx(cell, 0.10f, 8, 1.5f, bd);

        char ll[16]; snprintf(ll, sizeof(ll), "Lv %d", i + 1);
        DrawText(ll, (int)(cell.x + 8), (int)(cell.y + 5), 12,
                 cur ? C_MUTED : C_MUTED);

        if (c->levelReqs[i] >= 0) {
            char rs[24]; snprintf(rs, sizeof(rs), "%d XP", c->levelReqs[i]);
            DrawText(rs, (int)(cell.x + 8), (int)(cell.y + 22), 14,
                     cur ? C_TEXT : C_TEXT);
        } else {
            DrawText("— XP", (int)(cell.x + 8), (int)(cell.y + 22), 14, C_MUTED);
        }
    }
    int rows = (c->levelCap + cols - 1) / cols;
    py += rows * ch + 24;

    /* ── Delete ── */
    if (Btn((Rectangle){(float)(px + pw - 160), (float)py, 152.f, 38.f},
            "Delete Class", C_RED, C_TEXT)) {
        for (int i = selectedCls; i < classCount - 1; i++)
            classes[i] = classes[i + 1];
        classCount--;
        selectedCls  = -1;
        detailScroll = 0.f;
        SaveState();
        Notif("Class deleted.");
    }

    /* Clamp scroll */
    int virtualBottom = py + (int)detailScroll + 60;
    float maxS = fmaxf(0.f, (float)(virtualBottom - (y + h)));
    if (detailScroll > maxS) detailScroll = maxS;

    EndScissorMode();
}

/* ── Sidebar ────────────────────────────────────────────────── */
static void DrawSidebar(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, C_PANEL);
    DrawLine(x + w - 1, y, x + w - 1, y + h, C_BORDER);

    int px = x + 14, py = y + 14, pw = w - 28;

    /* ── XP Summary Card ── */
    Panel((Rectangle){(float)px, (float)py, (float)pw, 118.f}, C_PANEL2, C_BORDER);
    DrawText("TOTAL XP",  px + 12, py + 10, 11, C_MUTED);
    DrawText(TextFormat("%d", totalXP), px + 12, py + 26, 26, C_GOLD_L);
    DrawLine(px + 12, py + 62, px + pw - 12, py + 62, C_BORDER);
    DrawText("AVAILABLE", px + 12, py + 70, 11, C_MUTED);
    int av = AvailXP();
    DrawText(TextFormat("%d", av), px + 12, py + 86, 22,
             av > 0 ? C_GREEN : C_RED);
    py += 132;

    /* ── Set Total XP ── */
    DrawText("Set Total XP", px, py, 12, C_MUTED);
    py += 16;
    Rectangle tr = {(float)px, (float)py, (float)pw, 36.f};
    TBoxTick(&tbTotalXP, tr);
    TBoxDraw(&tbTotalXP, tr, "Enter total XP...");
    py += 44;

    if (Btn((Rectangle){(float)px, (float)py, (float)pw, 36.f},
            "Confirm XP", C_PANEL3, C_TEXT)) {
        int n = atoi(tbTotalXP.buf);
        if (n >= 0) { totalXP = n; SaveState();
            Notif(TextFormat("Total XP set to %d.", n)); }
    }
    py += 52;

    DrawLine(px, py, px + pw, py, C_BORDER);
    py += 12;

    /* ── Add Class ── */
    if (Btn((Rectangle){(float)px, (float)py, (float)pw, 40.f},
            "+ Add New Class", C_GOLD, C_BG))
        showModal = true;
    py += 56;

    DrawText(TextFormat("CLASSES  (%d)", classCount), px, py, 11, C_MUTED);
    py += 18;

    /* ── Class List (scrollable) ── */
    int listH = h - (py - y) - 10;
    int itemH = 62;

    if (CheckCollisionPointRec(GetMousePosition(),
                               (Rectangle){(float)x, (float)py, (float)w, (float)listH})) {
        sideScroll -= GetMouseWheelMove() * 60.f;
        float maxS = fmaxf(0.f, (float)(classCount * itemH - listH));
        if (sideScroll < 0.f) sideScroll = 0.f;
        if (sideScroll > maxS) sideScroll = maxS;
    }

    BeginScissorMode(x, py, w, listH);
    int ly = py - (int)sideScroll;

    for (int i = 0; i < classCount; i++) {
        Class *c  = &classes[i];
        int    lv = GetLevel(c);
        bool   sel = (i == selectedCls);

        Rectangle ir = {(float)px, (float)ly, (float)pw, (float)(itemH - 6)};
        bool hov = CheckCollisionPointRec(GetMousePosition(), ir);
        Color bg = sel ? C_PANEL3 : hov ? C_PANEL2 : C_PANEL;
        Color bd = sel ? C_GOLD : C_BORDER;

        DrawRectangleRounded(ir, 0.08f, 8, bg);
        DrawRectangleRoundedLinesEx(ir, 0.08f, 8, 1.5f, bd);

        DrawText(c->name, (int)(ir.x + 10), (int)(ir.y + 7),
                 16, sel ? C_GOLD_L : C_TEXT);

        char lb[28]; snprintf(lb, sizeof(lb), "Lv %d/%d", lv, c->levelCap);
        Badge((int)(ir.x + 10), (int)(ir.y + 30), lb,
              lv == c->levelCap ? C_GREEN : C_PANEL3,
              lv == c->levelCap ? C_GREEN_L : C_MUTED);

        char xb[28]; snprintf(xb, sizeof(xb), "%d xp", c->allocatedXP);
        int xbw = MeasureText(xb, 13) + 16;
        Badge((int)(ir.x + ir.width - (float)xbw - 2),
              (int)(ir.y + 30), xb, C_PANEL2, C_MUTED);

        if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedCls  = i;
            detailScroll = 0.f;
            memset(tbDistXP.buf, 0, sizeof(tbDistXP.buf));
        }
        ly += itemH;
    }
    EndScissorMode();
}

/* ── Main ───────────────────────────────────────────────────── */
int main(void)
{
    InitWindow(SW, SH, "RPG Class Manager");
    SetTargetFPS(60);

    /* Initialise level requirements to -1 (= not set) */
    for (int i = 0; i < MAX_CLASSES; i++)
        for (int j = 0; j < MAX_LEVELS; j++)
            classes[i].levelReqs[j] = -1;

    LoadState();

    while (!WindowShouldClose()) {
        notifTimer -= GetFrameTime();

        /* ESC closes modal */
        if (IsKeyPressed(KEY_ESCAPE) && showModal) {
            showModal = false;
            memset(tbNewName.buf, 0, sizeof(tbNewName.buf));
        }

        BeginDrawing();
        ClearBackground(C_BG);

        /* ── Header ── */
        DrawRectangle(0, 0, SW, HEADER_H, C_PANEL);
        DrawLine(0, HEADER_H, SW, HEADER_H, C_BORDER);
        DrawText("RPG Class Manager", 20, 16, 26, C_GOLD_L);

        char hi[160];
        snprintf(hi, sizeof(hi),
                 "Total: %d   |   Allocated: %d   |   Available: %d",
                 totalXP, TotalAllocated(), AvailXP());
        DrawText(hi, SW - MeasureText(hi, 14) - 18, 23, 14, C_MUTED);

        /* ── Panels ── */
        DrawSidebar(0, HEADER_H, SIDEBAR_W, SH - HEADER_H);
        DrawLine(SIDEBAR_W, HEADER_H, SIDEBAR_W, SH, C_BORDER);
        DrawDetail(SIDEBAR_W, HEADER_H, SW - SIDEBAR_W, SH - HEADER_H);

        /* ── Modal ── */
        if (showModal) DrawModal();

        /* ── Toast Notification ── */
        if (notifTimer > 0.f && notifMsg[0]) {
            float a = fminf(1.f, notifTimer);
            int   tw = MeasureText(notifMsg, 14);
            int   nx = SW / 2 - tw / 2 - 18;
            int   ny = SH - 56;
            DrawRectangleRounded(
                (Rectangle){(float)nx, (float)ny, (float)(tw + 36), 36.f},
                0.35f, 8,
                (Color){18, 18, 34, (unsigned char)(215 * a)});
            DrawRectangleRoundedLinesEx(
                (Rectangle){(float)nx, (float)ny, (float)(tw + 36), 36.f},
                0.35f, 8, 1.5f,
                (Color){178, 138, 42, (unsigned char)(200 * a)});
            DrawText(notifMsg, nx + 18, ny + 11, 14,
                     (Color){232, 188, 72, (unsigned char)(255 * a)});
        }

        EndDrawing();
    }

    SaveState();
    CloseWindow();
    return 0;
}