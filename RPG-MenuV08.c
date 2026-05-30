#include "raylib.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#define REGISTER_SKILL_COUNT  6
#define SKILL_COUNT          22
#define ATTRIBUTES_COUNT      6
#define MAX_LEVEL           101
#define TAB_COUNT            10
#define STATS_COUNT          12

#define IDX_MINING        0
#define IDX_COMBAT        1
#define IDX_CASTING      10
#define IDX_EXPLORATION  15
#define IDX_DUNGEONEERING 20
#define IDX_QUESTING     21

#define MAX_QUESTS       200
#define MAX_QNAME         64
#define MAX_QDESC        300
#define MAX_QNUM           8
#define QUEST_SAVE_FILE  "quest_save.txt"
#define QUEST_ROW_H       58

#define MAX_CLASSES          32
#define MAX_CLASS_LEVELS     20
#define MAX_CLASS_NAME       64
#define CLASS_SAVE_FILE     "rpg_save.dat"

// Colours
static const Color C_BG      = { 10,  10,  20, 255};
static const Color C_PANEL   = { 18,  18,  34, 255};
static const Color C_PANEL2  = { 24,  24,  44, 255};
static const Color C_PANEL3  = { 33,  33,  57, 255};
static const Color C_GOLD    = {178, 138,  42, 255};
static const Color C_GOLD_L  = {232, 188,  72, 255};
static const Color C_GOLD_D  = { 90,  66,  14, 255};
static const Color C_TEXT    = {224, 218, 196, 255};
static const Color C_MUTED   = {118, 113,  98, 255};
static const Color C_GREEN   = { 62, 148,  58, 255};
static const Color C_GREEN_L = {175, 238, 170, 255};
static const Color C_RED     = {182,  56,  56, 255};
static const Color C_RED_L   = {240, 140, 140, 255};
static const Color C_BLUE    = { 62, 112, 192, 255};
static const Color C_BLUE_L  = {140, 185, 240, 255};
static const Color C_BORDER  = { 40,  40,  68, 255};
static const Color C_HEADER  = { 14,  14,  28, 255};
static const Color C_INPUT   = { 12,  12,  24, 255};
static const Color Q_HOVER   = { 35,  27,  65, 255};
static const Color Q_BORDER2 = { 80,  60, 130, 255};
static const Color Q_XP      = { 80, 175, 255, 255};
static const Color Q_XP_DIM  = { 20,  55,  95, 255};
static const Color Q_CP      = {255, 200,  50, 255};
static const Color Q_CP_DIM  = { 85,  62,  10, 255};
static const Color Q_INBG    = {  7,   5,  14, 255};
static const Color Q_OVERLAY = {  3,   2,  10, 230};
static const Color Q_DONEBG  = { 10,  35,  18, 255};
static const Color Q_NOTIF   = { 40, 170,  80, 255};
static const Color Q_TDIM    = {110, 100,  80, 255};
static const Color Q_TDARK   = { 50,  45,  35, 255};
static const Color Q_GPALE   = {235, 200, 110, 255};
static const Color Q_GDIM    = { 90,  72,  15, 255};

// Data Tables
static int register_indices[REGISTER_SKILL_COUNT] = {
    IDX_MINING, IDX_COMBAT, IDX_CASTING, IDX_EXPLORATION, IDX_DUNGEONEERING, IDX_QUESTING};
static const char *stats_names[STATS_COUNT] = {
    "HEALTH","DEFENSE","STRENGTH","ATTACK SPEED","CRIT DAMAGE","SPEED","MANA","REGENERATION","RANGE","FORTUNE","BREAKING POWER","LABOR SPEED"};
static const char *stats_units[STATS_COUNT] = {
    "HP","DEF","STR","%","%","SPD","MP","HP/min","m","LUCK","BP","LSPD"};
static const char *attributes_names[ATTRIBUTES_COUNT] = {
    "STRENGTH","AGILITY","VITALITY","INTELLECT","RESOLVE","KARMA"};
static const char *skill_names[SKILL_COUNT] = {
    "Mining","Combat","Trading","Farming","Crafting","Building","Forestry","Enchanting","Alchemy","Fishing","Casting","Cooking","Taming","Stealth","Music","Exploration","Excavation","Archery","Defense","Agility","Dungeoneering","Questing"};
static const char *register_skill_names[REGISTER_SKILL_COUNT] = {
    "Mining","Combat","Casting","Exploration","Dungeoneering","Questing"};
static const char *tab_names[TAB_COUNT] = {
    "Classes","Stats","Attributes","Skills","Quests","Guild","Abilities","Gear","Enhancements","Register"};

// Types
typedef struct {
    char name[MAX_QNAME]; char desc[MAX_QDESC]; int cp, xp; bool done; int type;
} Quest;

typedef struct {
    char name[MAX_CLASS_NAME]; int levelCap; int allocatedXP; int levelReqs[MAX_CLASS_LEVELS];
} Class;

typedef struct {
    char buf[128]; bool active;
} TBox;

// Quest state
static Quest quests[MAX_QUESTS];
static int   questCount = 0;
static int   earnedCP   = 0, earnedXP = 0;
static int   totalCP    = 0, totalXP  = 0;

static int   quest_tab    = 0;
static int   quest_scroll = 0;
static int   quest_view   = -1;
static bool  quest_create = false;
static char  q_name[MAX_QNAME] = {0};
static char  q_desc[MAX_QDESC] = {0};
static char  q_cp  [MAX_QNUM]  = {0};
static char  q_xp  [MAX_QNUM]  = {0};
static int   q_focus    = 0;
static float q_mscale   = 0.f;
static float q_bs       = 0.f;

// Class state
static int   cls_totalXP  = 0;
static Class classes[MAX_CLASSES];
static int   classCount   = 0;
static int   selectedCls  = -1;
static bool  showModal    = false;

static TBox  tbNewName  = {{0}, false};
static int   newCap     = 10;
static TBox  tbDistXP   = {{0}, false};

static float sideScroll   = 0.f;
static float detailScroll = 0.f;

// Notification toast state
static char  notifMsg[128] = {0};
static float notifTimer    = 0.f;

static void ShowNotif(const char *msg)
{
    strncpy(notifMsg, msg, sizeof(notifMsg) - 1);
    notifMsg[sizeof(notifMsg)-1] = '\0';
    notifTimer = 2.5f;
}

// Save character data to files
static void SaveData(int *attrs, int *skills, int *base_stats)
{
    FILE *fw = fopen("skills.txt","w");
    if(fw){ for(int i=0;i<SKILL_COUNT;i++) fprintf(fw,"%d ",skills[i]); fclose(fw); }
    fw = fopen("attributes.txt","w");
    if(fw){ for(int i=0;i<ATTRIBUTES_COUNT;i++) fprintf(fw,"%d ",attrs[i]); fclose(fw); }
    fw = fopen("stats.txt","w");
    if(fw){ for(int i=0;i<STATS_COUNT;i++) fprintf(fw,"%d ",base_stats[i]); fclose(fw); }
}

static void SavePlayerStats(int level, int xp, int cp)
{
    FILE *fw = fopen("player_stats.txt","w");
    if(fw){ fprintf(fw,"%d %d %d",level,xp,cp); fclose(fw); }
}

// Save/load quests
static void QuestSave(void)
{
    FILE *f = fopen(QUEST_SAVE_FILE,"w");
    if(!f){ ShowNotif("ERROR: Could not save quests!"); return; }
    fprintf(f,"# Aetheria Quest Journal\nearned_cp=%d\nearned_xp=%d\ntotal_cp=%d\n\n",
            earnedCP, earnedXP, totalCP);
    for(int i=0;i<questCount;i++)
        fprintf(f,"QUEST\nname=%s\ndesc=%s\ncp=%d\nxp=%d\ndone=%d\ntype=%d\n---\n",
                quests[i].name, quests[i].desc, quests[i].cp, quests[i].xp,
                quests[i].done?1:0, quests[i].type);
    fclose(f);
}

static void QuestLoad(void)
{
    FILE *f = fopen(QUEST_SAVE_FILE,"r");
    if(!f) return;
    char line[512]; Quest *cur=NULL;
    while(fgets(line,sizeof(line),f)){
        int n=(int)strlen(line);
        while(n>0&&(line[n-1]=='\n'||line[n-1]=='\r')) line[--n]='\0';
        if     (strcmp(line,"QUEST")==0){ if(questCount<MAX_QUESTS){ cur=&quests[questCount++]; memset(cur,0,sizeof(Quest)); } }
        else if(cur&&strncmp(line,"name=",5)==0) strncpy(cur->name,line+5,MAX_QNAME-1);
        else if(cur&&strncmp(line,"desc=",5)==0) strncpy(cur->desc,line+5,MAX_QDESC-1);
        else if(cur&&strncmp(line,"cp=",3)==0)   cur->cp=atoi(line+3);
        else if(cur&&strncmp(line,"xp=",3)==0)   cur->xp=atoi(line+3);
        else if(cur&&strncmp(line,"done=",5)==0) cur->done=(atoi(line+5)==1);
        else if(cur&&strncmp(line,"type=",5)==0) cur->type=atoi(line+5);
        else if(strcmp(line,"---")==0) cur=NULL;
    }
    fclose(f);
}

// Save/load class state
static void ClassFname(const Class *c, char *out, int n)
{
    snprintf(out, n, "%s_levels.txt", c->name);
    for(int i=0; out[i]; i++){
        if(out[i]==' ') out[i]='_';
        out[i]=(char)tolower((unsigned char)out[i]);
    }
}

static void CreateClassFile(const Class *c)
{
    char fn[256]; ClassFname(c, fn, sizeof(fn));
    FILE *f = fopen(fn, "w");
    if(!f) return;
    fprintf(f, "# %s  -  Level XP Requirements\n", c->name);
    fprintf(f, "# Edit the numbers after each colon.\n");
    fprintf(f, "# Level 1 is usually 0 XP (starting level).\n");
    fprintf(f, "# After saving this file, click [Reload] in the app.\n#\n");
    for(int i=0; i<c->levelCap; i++) fprintf(f, "Level %d: 0\n", i+1);
    fclose(f);
}

static void LoadClassFile(Class *c)
{
    char fn[256]; ClassFname(c, fn, sizeof(fn));
    FILE *f = fopen(fn, "r");
    if(!f) return;
    char line[256];
    while(fgets(line, sizeof(line), f)){
        if(line[0]=='#'||line[0]=='\n') continue;
        int lv, xp;
        if(sscanf(line, "Level %d: %d", &lv, &xp)==2 && lv>=1 && lv<=c->levelCap)
            c->levelReqs[lv-1] = xp;
    }
    fclose(f);
}

static void SaveClassState(void)
{
    FILE *f = fopen(CLASS_SAVE_FILE, "w");
    if(!f) return;
    fprintf(f, "CLASS_COUNT %d\n", classCount);
    for(int i=0; i<classCount; i++)
        fprintf(f, "CLASS %s|%d|%d\n",
                classes[i].name, classes[i].levelCap, classes[i].allocatedXP);
    fclose(f);
}

static void LoadClassState(void)
{
    FILE *f = fopen(CLASS_SAVE_FILE, "r");
    if(!f) return;
    char line[256];
    classCount = 0;
    while(fgets(line, sizeof(line), f)){
        if(strncmp(line,"TOTAL_XP",8)==0){
            // Legacy line — cls_totalXP is now auto-calculated, ignore
        } else if(strncmp(line,"CLASS ",6)==0 && classCount<MAX_CLASSES){
            Class *c = &classes[classCount++];
            memset(c, 0, sizeof(*c));
            for(int j=0; j<MAX_CLASS_LEVELS; j++) c->levelReqs[j]=-1;
            char *p=line+6, *p1=strchr(p,'|'), *p2=p1?strchr(p1+1,'|'):NULL;
            if(p1&&p2){
                int nl=(int)(p1-p); if(nl>=MAX_CLASS_NAME) nl=MAX_CLASS_NAME-1;
                strncpy(c->name,p,nl); c->name[nl]='\0';
                c->levelCap=atoi(p1+1); c->allocatedXP=atoi(p2+1);
            }
            LoadClassFile(c);
        }
    }
    fclose(f);
}

// Class helpers
static int GetLevel(const Class *c)
{
    int lv=0;
    for(int i=0; i<c->levelCap; i++)
        if(c->levelReqs[i]>=0 && c->levelReqs[i]<=c->allocatedXP) lv=i+1;
    return lv;
}

static int TotalAllocated(void)
{
    int t=0;
    for(int i=0; i<classCount; i++) t+=classes[i].allocatedXP;
    return t;
}

static int AvailXP(void) { return cls_totalXP - TotalAllocated(); }

// Auto-calculate cls_totalXP
static void UpdateTotalXP(int level, int xp, int *tnl)
{
    cls_totalXP = xp;
    for(int i = 0; i < level; i++)
        cls_totalXP += tnl[i];
}

// TBox input widget
static void TBoxTick(TBox *t, Rectangle r)
{
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        t->active = CheckCollisionPointRec(GetMousePosition(), r);
    if(!t->active) return;
    for(int k=GetCharPressed(); k; k=GetCharPressed()){
        int n=(int)strlen(t->buf);
        if(k>=32 && n<(int)sizeof(t->buf)-1){ t->buf[n]=(char)k; t->buf[n+1]='\0'; }
    }
    if(IsKeyPressed(KEY_BACKSPACE) && strlen(t->buf)>0)
        t->buf[strlen(t->buf)-1]='\0';
    if(IsKeyPressed(KEY_ESCAPE)) t->active=false;
}

static void TBoxDraw(const TBox *t, Rectangle r, const char *ph)
{
    DrawRectangleRounded(r, 0.12f, 8, C_INPUT);
    DrawRectangleRoundedLinesEx(r, 0.12f, 8, 1.5f, t->active?C_BLUE:C_BORDER);
    bool has=(strlen(t->buf)>0);
    DrawText(has?t->buf:ph, (int)r.x+10, (int)(r.y+(r.height-18.f)/2.f), 18, has?C_TEXT:C_MUTED);
    if(t->active && has && (int)(GetTime()*2.0)%2==0){
        int tw=MeasureText(t->buf,18);
        DrawRectangle((int)r.x+11+tw, (int)(r.y+(r.height-18.f)/2.f), 2, 18, C_TEXT);
    }
}

// Quest text input with backspace repeat
static void TypeInto(char *buf, int maxLen, bool numeric, float dt)
{
    int key=GetCharPressed();
    while(key>0){
        bool ok = numeric?(key>='0'&&key<='9'):(key>=32&&key<126);
        if(ok&&(int)strlen(buf)<maxLen-1){ int l=(int)strlen(buf); buf[l]=(char)key; buf[l+1]='\0'; }
        key=GetCharPressed();
    }
    if     (IsKeyPressed(KEY_BACKSPACE)){ int l=(int)strlen(buf); if(l>0)buf[l-1]='\0'; q_bs=0; }
    else if(IsKeyDown(KEY_BACKSPACE))   { q_bs+=dt; if(q_bs>0.5f){ q_bs=0.42f; int l=(int)strlen(buf); if(l>0)buf[l-1]='\0'; } }
    else q_bs=0;
}

// Drawing primitives
static Color Brighten(Color c, float f)
{
    return (Color){
        (unsigned char)fminf(255.f,c.r*f),
        (unsigned char)fminf(255.f,c.g*f),
        (unsigned char)fminf(255.f,c.b*f),
        c.a
    };
}

static void Panel(Rectangle r, Color bg, Color border)
{
    DrawRectangleRounded(r,0.06f,8,bg);
    DrawRectangleRoundedLinesEx(r,0.06f,8,1.5f,border);
}

// Small badge pill font-12, used by the tab bar
static void Badge(int x, int y, const char *txt, Color bg, Color fg)
{
    int tw=MeasureText(txt,12);
    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)(tw+16),20.f},0.5f,8,bg);
    DrawText(txt,x+8,y+4,12,fg);
}

// Small badge pill font-13, used by the Classes panel
static void ClsBadge(int x, int y, const char *txt, Color bg, Color fg)
{
    int tw=MeasureText(txt,13);
    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)(tw+16),22.f},0.5f,8,bg);
    DrawText(txt,x+8,y+4,13,fg);
}

// Full-size button — returns true on click
static bool Btn(Rectangle r, const char *txt, Color bg, Color fg)
{
    bool hov  = CheckCollisionPointRec(GetMousePosition(), r);
    bool down = hov && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    Color c = down?Brighten(bg,0.78f):hov?Brighten(bg,1.28f):bg;
    DrawRectangleRounded(r,0.18f,8,c);
    int fs=16, tw=MeasureText(txt,fs);
    DrawText(txt,(int)(r.x+(r.width-(float)tw)/2.f),(int)(r.y+(r.height-(float)fs)/2.f),fs,fg);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// Smaller button — returns true on click
static bool SmallBtn(Rectangle r, const char *txt, Color bg, Color fg)
{
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    Color c  = hov?Brighten(bg,1.28f):bg;
    DrawRectangleRounded(r,0.20f,8,c);
    int fs=13, tw=MeasureText(txt,fs);
    DrawText(txt,(int)(r.x+(r.width-(float)tw)/2.f),(int)(r.y+(r.height-(float)fs)/2.f),fs,fg);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void ProgBar(Rectangle r, float pct, Color fill)
{
    DrawRectangleRounded(r,0.5f,8,C_PANEL3);
    if(pct>0.001f){
        Rectangle f={r.x,r.y,r.width*fminf(pct,1.f),r.height};
        DrawRectangleRounded(f,0.5f,8,fill);
    }
}

static void HDivider(int x, int y, int w)
{
    DrawLineEx((Vector2){(float)x,(float)y},(Vector2){(float)(x+w),(float)y},1.f,C_BORDER);
}

// Quest drawing helpers
static void QBadge(int x, int y, const char *lbl, int val, Color bg, Color fg)
{
    char buf[32]; snprintf(buf,32,"%s %d",lbl,val);
    int w=MeasureText(buf,11)+10;
    DrawRectangleRounded     ((Rectangle){(float)x,(float)y,(float)w,18},0.5f,6,bg);
    DrawRectangleRoundedLines((Rectangle){(float)x,(float)y,(float)w,18},0.5f,6,fg);
    DrawText(buf,x+5,y+3,11,fg);
}

static void QButton(Rectangle r, const char *txt, Color bg, Color fg, bool hov)
{
    Color b2=hov?(Color){bg.r+20,bg.g+15,bg.b+30,bg.a}:bg;
    DrawRectangleRounded(r,0.35f,6,b2);
    DrawRectangleRoundedLines(r,0.35f,6,fg);
    int tw=MeasureText(txt,12);
    DrawText(txt,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-6),12,fg);
}

static void QInputBox(Rectangle r, const char *label, const char *val, bool focused)
{
    DrawText(label,(int)r.x,(int)r.y-16,12,Q_TDIM);
    DrawRectangleRec(r,Q_INBG);
    DrawRectangleLinesEx(r,1.5f,focused?C_GOLD:C_BORDER);
    if(focused)
        DrawRectangleLinesEx((Rectangle){r.x-1,r.y-1,r.width+2,r.height+2},1,(Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,70});
    char disp[256]; strncpy(disp,val,255); disp[255]='\0';
    while(MeasureText(disp,14)>(int)r.width-12&&strlen(disp)>0) disp[strlen(disp)-1]='\0';
    DrawText(disp,(int)r.x+6,(int)(r.y+r.height/2-7),14,C_TEXT);
    if(focused&&(int)(GetTime()*2)%2==0){
        int cx=(int)r.x+6+MeasureText(disp,14);
        DrawLine(cx,(int)r.y+5,cx,(int)r.y+(int)r.height-5,C_GOLD);
    }
}

static int QTextWrap(const char *txt, int x, int y, int fs, int maxW, Color col)
{
    char word[64],line[512]={0};
    int lH=fs+4, yy=y;
    const char *p=txt;
    while(*p){
        int wi=0;
        while(*p&&*p!=' '&&*p!='\n'&&wi<63) word[wi++]=*p++;
        word[wi]='\0'; if(*p==' ')p++;
        char test[512];
        if(!strlen(line)) snprintf(test,512,"%s",word);
        else              snprintf(test,512,"%s %s",line,word);
        if(MeasureText(test,fs)>maxW&&strlen(line)>0){
            DrawText(line,x,yy,fs,col); yy+=lH; strcpy(line,word);
        } else strcpy(line,test);
        if(*p=='\n'||*(p-1)=='\n'){
            DrawText(line,x,yy,fs,col); yy+=lH; line[0]='\0'; if(*p=='\n')p++;
        }
    }
    if(strlen(line)){ DrawText(line,x,yy,fs,col); yy+=lH; }
    return yy-y;
}

// Tab bar, header, footer
static void DrawTabs(int current_tab, bool edit_mode, bool player_edit_mode)
{
    DrawRectangle(0,110,192,490,C_PANEL);                                       
    DrawLine(192,110,192,600,C_BORDER);                                         
    for(int i=0;i<TAB_COUNT;i++){
        int ty=118+i*46;
        bool active=(i==current_tab);
        bool hov=CheckCollisionPointRec(GetMousePosition(),(Rectangle){0,(float)ty,192,38});
        Color bg  = active?C_PANEL3:hov?C_PANEL2:C_PANEL;
        Color txt = active?C_GOLD_L:C_TEXT;
        DrawRectangle(0,ty,192,38,bg);                                           
        if(active){
            DrawRectangle(0,ty,4,38,C_GOLD);
            if(edit_mode||player_edit_mode) DrawRectangle(188,ty,4,38,C_GOLD);  
        }
        DrawText(tab_names[i],18,ty+10,18,txt);
        if(active&&edit_mode)
            Badge(MeasureText(tab_names[i],18)+26,ty+9,"EDIT",C_GOLD_D,C_GOLD_L);
    }
}

static void DrawHeader(int level, int xp, int cp, int tnl_cur, bool player_edit_mode, int sel_ps)
{
    DrawRectangle(0,0,900,108,C_HEADER);
    DrawLine(0,108,900,108,C_BORDER);
    DrawRectangle(0,0,4,108,C_GOLD);

    DrawText("NAME",22,14,28,C_GOLD_L);
    DrawText("\"Nickname\"",22,46,14,C_MUTED);

    Color c_lv=(player_edit_mode&&sel_ps==0)?C_GOLD_L:C_TEXT;
    char lvs[32]; snprintf(lvs,sizeof(lvs),"LEVEL  %d",level);
    Badge(200,18,lvs,player_edit_mode&&sel_ps==0?C_GOLD_D:C_PANEL3,c_lv);

    Color c_xp=(player_edit_mode&&sel_ps==1)?C_GOLD_L:C_TEXT;
    DrawText(TextFormat("XP  %d / %d",xp,tnl_cur>0?tnl_cur:0),200,46,14,c_xp);

    float prog=tnl_cur>0?(float)xp/(float)tnl_cur:0.f;
    if(prog>1.f)prog=1.f;
    ProgBar((Rectangle){200,68,340,8},prog,C_GOLD);

    Color c_cp=(player_edit_mode&&sel_ps==2)?C_GOLD_L:C_TEXT;
    char cps[32]; snprintf(cps,sizeof(cps),"CP  %d",cp);
    Badge(580,18,cps,player_edit_mode&&sel_ps==2?C_GOLD_D:C_PANEL3,c_cp);
}

static void DrawFooter(bool edit_mode, bool player_edit_mode)
{
    DrawRectangle(0,572,900,28,C_HEADER);
    DrawLine(0,572,900,572,C_BORDER);
    const char *info;
    if(player_edit_mode)   info="[ p ] TAB EDIT   |   [ O ] EXIT PLAYER EDIT";
    else if(edit_mode)     info="[ P ] EXIT TAB EDIT   |   [ O ] PLAYER EDIT";
    else                   info="[ P ] TAB EDIT   |   [ O ] PLAYER EDIT";
    DrawText(info,14,578,13,C_MUTED);
}

// Classes tab: Add-Class Modal
static void DrawClassModal(void)
{
    DrawRectangle(0, 0, 900, 600, (Color){0, 0, 0, 155});

    Rectangle m = {(float)(900/2 - 242), (float)(600/2 - 218), 484.f, 436.f};
    Panel(m, C_PANEL, C_GOLD);

    DrawText("Add New Class", (int)m.x+22, (int)m.y+16, 22, C_GOLD_L);
    DrawLineEx((Vector2){m.x+22, m.y+52},(Vector2){m.x+m.width-22, m.y+52}, 1.f, C_BORDER);

    int px=(int)m.x+24, py=(int)m.y+64;

    DrawText("Class Name", px, py, 13, C_MUTED);
    py+=18;
    Rectangle nr={(float)px,(float)py,m.width-48.f,40.f};
    TBoxTick(&tbNewName, nr);
    TBoxDraw(&tbNewName, nr, "e.g.  Warrior,  Mage,  Ranger ...");
    py+=56;

    char capLabel[32]; snprintf(capLabel,sizeof(capLabel),"Level Cap  —  %d",newCap);
    DrawText(capLabel, px, py, 13, C_MUTED);
    py+=20;

    float t=(newCap-5)/15.f;
    Rectangle track={(float)px,(float)(py+8),m.width-48.f,6.f};
    DrawRectangleRounded(track, 0.5f, 8, C_BORDER);
    if(t>0.f){ Rectangle fill={track.x,track.y,t*track.width,track.height}; DrawRectangleRounded(fill,0.5f,8,C_GOLD); }
    float kx=track.x+t*track.width;
    DrawCircle((int)kx,(int)(track.y+3.f),11,C_PANEL3);
    DrawCircle((int)kx,(int)(track.y+3.f), 9,C_GOLD_L);

    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
        Vector2 mp=GetMousePosition();
        if(mp.y>=track.y-14.f&&mp.y<=track.y+26.f&&mp.x>=track.x-12.f&&mp.x<=track.x+track.width+12.f){
            float nt=(mp.x-track.x)/track.width;
            nt=fmaxf(0.f,fminf(1.f,nt));
            newCap=5+(int)roundf(nt*15.f);
        }
    }
    DrawText("5",  px,                    py+24, 12, C_MUTED);
    DrawText("20", (int)(m.x+m.width-36), py+24, 12, C_MUTED);
    py+=56;

    DrawText("A .txt file is created automatically. Open it in any", px, py, 13, C_MUTED); py+=18;
    DrawText("text editor, fill in XP values, then click Reload.",   px, py, 13, C_MUTED); py+=46;

    Rectangle cancelR={(float)px,                  (float)py,200.f,42.f};
    Rectangle addR   ={m.x+m.width-224.f,(float)py,200.f,42.f};

    if(Btn(cancelR,"Cancel",C_PANEL3,C_TEXT)){
        showModal=false;
        memset(tbNewName.buf,0,sizeof(tbNewName.buf));
    }
    if(Btn(addR,"Add Class + Create File",C_GOLD,C_BG)){
        if(strlen(tbNewName.buf)>0 && classCount<MAX_CLASSES){
            Class *c=&classes[classCount++];
            memset(c,0,sizeof(*c));
            for(int j=0;j<MAX_CLASS_LEVELS;j++) c->levelReqs[j]=-1;
            strncpy(c->name,tbNewName.buf,MAX_CLASS_NAME-1);
            c->levelCap=newCap; c->allocatedXP=0;
            CreateClassFile(c); SaveClassState();
            char fn[256]; ClassFname(c,fn,sizeof(fn));
            ShowNotif(TextFormat("'%s' added! Edit %s and click Reload.",c->name,fn));
            showModal=false;
            memset(tbNewName.buf,0,sizeof(tbNewName.buf));
            newCap=10;
        } else ShowNotif("Please enter a class name first.");
    }
}

// Classes tab: right-side detail panel
static void DrawClassDetail(int x, int y, int w, int h)
{
    if(selectedCls<0||selectedCls>=classCount){
        const char *hint="<-- Select a class or add a new one.";
        DrawText(hint, x+(w-MeasureText(hint,14))/2, y+h/2-8, 14, C_MUTED);
        return;
    }

    Class *c  = &classes[selectedCls];
    int    lv = GetLevel(c);
    int    av = AvailXP();

    if(CheckCollisionPointRec(GetMousePosition(),(Rectangle){x,y,w,h}))
        detailScroll -= GetMouseWheelMove()*52.f;
    if(detailScroll<0.f) detailScroll=0.f;

    BeginScissorMode(x, y, w, h);

    int px=x+22, pw=w-44;
    int py=y+18-(int)detailScroll;

    int nw=MeasureText(c->name,28);
    DrawText(c->name, px, py, 28, C_GOLD_L);
    char lvs[32]; snprintf(lvs,sizeof(lvs),"Lv %d / %d",lv,c->levelCap);
    ClsBadge(px+nw+14, py+6, lvs, lv==c->levelCap?C_GREEN:C_PANEL3, lv==c->levelCap?C_GREEN_L:C_MUTED);
    if(lv==c->levelCap)
        ClsBadge(px+nw+14+MeasureText(lvs,13)+36, py+6, "MAX LEVEL", C_GREEN, C_GREEN_L);
    py+=48;

    DrawText(TextFormat("Allocated XP:  %d", c->allocatedXP), px, py, 16, C_TEXT);
    py+=26;
    ProgBar((Rectangle){(float)px,(float)py,(float)pw,8}, c->levelCap>0?(float)lv/(float)c->levelCap:0.f, C_GOLD);
    py+=22;

    Panel((Rectangle){(float)px,(float)py,(float)pw,100.f}, C_PANEL2, C_BORDER);
    DrawText("Distribute XP", px+14, py+11, 14, C_MUTED);

    Rectangle ir={(float)(px+14),(float)(py+34),172.f,38.f};
    TBoxTick(&tbDistXP, ir);
    TBoxDraw(&tbDistXP, ir, "Amount...");

    Rectangle addB={(float)(px+pw-184),(float)(py+34),88.f,38.f};
    Rectangle subB={(float)(px+pw- 90),(float)(py+34),80.f,38.f};

    if(Btn(addB,"+ Add",C_GOLD,C_BG)){
        int amt=atoi(tbDistXP.buf);
        if(amt>0){ int add=amt>av?av:amt; c->allocatedXP+=add; SaveClassState();
            ShowNotif(TextFormat("Added %d XP to %s.",add,c->name)); }
    }
    if(Btn(subB,"- Sub",C_PANEL3,C_TEXT)){
        int amt=atoi(tbDistXP.buf);
        if(amt>0){ c->allocatedXP-=amt; if(c->allocatedXP<0) c->allocatedXP=0;
            SaveClassState(); ShowNotif(TextFormat("Removed %d XP from %s.",amt,c->name)); }
    }
    py+=116;

    DrawText("Level Requirements", px, py, 15, C_TEXT);

    if(SmallBtn((Rectangle){(float)(px+pw-142),(float)(py-2),136.f,26.f},"Reload from file",C_PANEL3,C_MUTED)){
        for(int i=0;i<MAX_CLASS_LEVELS;i++) c->levelReqs[i]=-1;
        LoadClassFile(c);
        ShowNotif(TextFormat("Reloaded requirements for %s.",c->name));
    }
    py+=34;

    int cols=4, cw=pw/cols, ch=64;                                              // CHANGED: cols 5->4, ch 58->64
    for(int i=0; i<c->levelCap; i++){
        int col=i%cols, row=i/cols;
        Rectangle cell={(float)(px+col*cw),(float)(py+row*ch),(float)(cw-6),(float)(ch-6)};
        bool reached=(c->levelReqs[i]>=0&&c->levelReqs[i]<=c->allocatedXP);
        bool cur=(i+1==lv);

        Color cbg=(reached||cur)?(Color){26,48,24,255}:C_PANEL2;
        Color cbd=(reached||cur)?C_GREEN:C_BORDER;
        DrawRectangleRounded(cell,0.10f,8,cbg);
        DrawRectangleRoundedLinesEx(cell,0.10f,8,1.5f,cbd);

        char ll[16]; snprintf(ll,sizeof(ll),"Lv %d",i+1);
        DrawText(ll,(int)(cell.x+8),(int)(cell.y+6),12,C_MUTED);

        if(c->levelReqs[i]>=0){
            char rs[24]; snprintf(rs,sizeof(rs),"%d XP",c->levelReqs[i]);
            DrawText(rs,(int)(cell.x+8),(int)(cell.y+24),14,C_TEXT);
        } else DrawText("— XP",(int)(cell.x+8),(int)(cell.y+24),14,C_MUTED);
    }
    int rows=(c->levelCap+cols-1)/cols;
    py+=rows*ch+24;

    if(Btn((Rectangle){(float)(px+pw-160),(float)py,152.f,38.f},"Delete Class",C_RED,C_TEXT)){
        for(int i=selectedCls;i<classCount-1;i++) classes[i]=classes[i+1];
        classCount--; selectedCls=-1; detailScroll=0.f;
        SaveClassState(); ShowNotif("Class deleted.");
    }

    int virtualBottom=py+(int)detailScroll+60;
    float maxS=fmaxf(0.f,(float)(virtualBottom-(y+h)));
    if(detailScroll>maxS) detailScroll=maxS;

    EndScissorMode();
}

// Classes tab: left-side sidebar
static void DrawClassSidebar(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, C_PANEL);
    DrawLine(x+w-1, y, x+w-1, y+h, C_BORDER);

    int px=x, py=y, pw=w-28;

    // XP Summary Card
    Panel((Rectangle){(float)px,(float)py,(float)pw,118.f}, C_PANEL2, C_BORDER);
    DrawText("TOTAL XP",  px+12, py+10, 11, C_MUTED);
    DrawText(TextFormat("%d", cls_totalXP), px+12, py+26, 26, C_GOLD_L);
    DrawLine(px+12, py+62, px+pw-12, py+62, C_BORDER);
    DrawText("AVAILABLE", px+12, py+70, 11, C_MUTED);
    int av=AvailXP();
    DrawText(TextFormat("%d",av), px+12, py+86, 22, av>0?C_GREEN:C_RED);
    py+=132;

    // Add class button
    if(Btn((Rectangle){(float)px,(float)py,(float)pw,40.f},"+ Add New Class",C_GOLD,C_BG))
        showModal=true;
    py+=48;

    DrawText(TextFormat("CLASSES  (%d)",classCount), px, py, 11, C_MUTED);
    py+=13;

    // Scrollable class list
    int listH=h-(py-y)-10;
    int itemH=62;

    if(CheckCollisionPointRec(GetMousePosition(),(Rectangle){(float)x,(float)py,(float)w,(float)listH})){
        sideScroll -= GetMouseWheelMove()*60.f;
        float maxS=fmaxf(0.f,(float)(classCount*itemH-listH));
        if(sideScroll<0.f) sideScroll=0.f;
        if(sideScroll>maxS) sideScroll=maxS;
    }

    BeginScissorMode(x, py, w, listH);
    int ly=py-(int)sideScroll;

    for(int i=0; i<classCount; i++){
        Class *c=&classes[i];
        int lv=GetLevel(c);
        bool sel=(i==selectedCls);

        Rectangle ir={(float)px,(float)ly,(float)pw,(float)(itemH-6)};
        bool hov=CheckCollisionPointRec(GetMousePosition(),ir);
        Color bg=sel?C_PANEL3:hov?C_PANEL2:C_PANEL;
        Color bd=sel?C_GOLD:C_BORDER;

        DrawRectangleRounded(ir,0.08f,8,bg);
        DrawRectangleRoundedLinesEx(ir,0.08f,8,1.5f,bd);
        DrawText(c->name,(int)(ir.x+10),(int)(ir.y+7),16,sel?C_GOLD_L:C_TEXT);

        char lb[28]; snprintf(lb,sizeof(lb),"Lv %d/%d",lv,c->levelCap);
        ClsBadge((int)(ir.x+10),(int)(ir.y+30),lb,
                 lv==c->levelCap?C_GREEN:C_PANEL3, lv==c->levelCap?C_GREEN_L:C_MUTED);

        if(hov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            selectedCls=i; detailScroll=0.f;
            memset(tbDistXP.buf,0,sizeof(tbDistXP.buf));
        }
        ly+=itemH;
    }
    EndScissorMode();
}

// Classes tab entry point
static void DrawTabClasses(int cX, int cY)
{
    int sideW   = 200;                  
    int h       = 572 - cY;
    int detailX = cX + sideW;      
    int detailW = 900 - detailX;       

    DrawClassSidebar(cX, cY, sideW, h);
    DrawClassDetail (detailX, cY, detailW, h);
}

// Stats tab
static void DrawTabStats(int cX, int cY, int *base_stats, int *gear_stats, int selected_stat, bool edit_mode)
{
    DrawText("BASE STATS",cX, cY,22,C_GOLD_L);
    DrawText("GEAR STATS",cX+360,cY,22,C_BLUE_L);
    HDivider(cX,cY+28,650);
    for(int i=0;i<STATS_COUNT;i++){
        int y=cY+38+i*30;
        bool sel=(i==selected_stat&&edit_mode);
        Color bg=i%2==0?C_PANEL2:C_PANEL;
        DrawRectangle(cX,y,340,26,bg);
        if(sel){
            DrawRectangleRoundedLinesEx((Rectangle){(float)cX,(float)y,340.f,26.f},0.08f,8,1.5f,C_GOLD);
            DrawRectangle(cX,y,3,26,C_GOLD);
        }
        DrawText(stats_names[i],cX+10,y+5,14,sel?C_GOLD_L:C_TEXT);
        Color vc=base_stats[i]>0?C_GREEN_L:base_stats[i]<0?C_RED_L:C_MUTED;
        DrawText(TextFormat("%d %s",base_stats[i],stats_units[i]),
                 cX+340-MeasureText(TextFormat("%d %s",base_stats[i],stats_units[i]),14)-10,y+5,14,vc);
        Color bg2=i%2==0?(Color){20,24,48,255}:C_PANEL;
        DrawRectangle(cX+360,y,290,26,bg2);
        DrawText(TextFormat("%d %s",gear_stats[i],stats_units[i]),cX+360+10,y+5,14,C_BLUE_L);
    }
}

// Attributes tab
static void DrawTabAttributes(int cX, int cY, int *attributes, int selected_attr, bool edit_mode)
{
    DrawText("ATTRIBUTES",cX,cY,22,C_GOLD_L);
    HDivider(cX,cY+28,640);
    for(int i=0;i<ATTRIBUTES_COUNT;i++){
        int y=cY+40+i*48;
        bool sel=(i==selected_attr&&edit_mode);
        Panel((Rectangle){(float)cX,(float)y,460.f,40.f},sel?C_PANEL3:C_PANEL2,sel?C_GOLD:C_BORDER);
        if(sel) DrawRectangle(cX,y,4,40,C_GOLD);
        DrawText(attributes_names[i],cX+16,y+11,17,sel?C_GOLD_L:C_TEXT);
        char vs[16]; snprintf(vs,sizeof(vs),"%d",attributes[i]);
        DrawText(vs,cX+460-MeasureText(vs,20)-18,y+10,20,
                 attributes[i]>0?C_GREEN_L:attributes[i]<0?C_RED_L:C_MUTED);
        float bp=fmaxf(0.f,fminf(1.f,attributes[i]/100.f));
        ProgBar((Rectangle){(float)(cX+480),(float)(y+12),140.f,8.f},bp,C_GOLD);
    }
}

// Skills tab
static void DrawTabSkills(int cX, int cY, int *skills, int selected_skill, bool edit_mode)
{
    DrawText("SKILLS",cX,cY,22,C_GOLD_L);
    HDivider(cX,cY+28,640);
    int col_w=310;
    for(int i=0;i<SKILL_COUNT;i++){
        int col=i<11?0:1, row=i%11;
        int x=cX+col*(col_w+20), y=cY+38+row*28;
        bool sel=(i==selected_skill&&edit_mode);
        Color bg=row%2==0?C_PANEL2:C_PANEL;
        DrawRectangle(x,y,col_w,24,bg);
        if(sel){
            DrawRectangleRoundedLinesEx((Rectangle){(float)x,(float)y,(float)col_w,24.f},0.1f,8,1.5f,C_GOLD);
            DrawRectangle(x,y,3,24,C_GOLD);
        }
        DrawText(skill_names[i],x+8,y+4,14,sel?C_GOLD_L:C_TEXT);
        char vs[16]; snprintf(vs,sizeof(vs),"%d",skills[i]);
        Color vc=skills[i]>0?C_GREEN_L:skills[i]<0?C_RED_L:C_MUTED;
        DrawText(vs,x+col_w-MeasureText(vs,14)-8,y+4,14,vc);
    }
}

// Register tab
static void DrawTabRegister(int cX, int cY, int *skills)
{
    DrawText("REGISTER",cX,cY,22,C_GOLD_L);
    HDivider(cX,cY+28,640);
    int sum=0;
    for(int i=0;i<REGISTER_SKILL_COUNT;i++) sum+=skills[register_indices[i]];
    int register_level=sum/REGISTER_SKILL_COUNT;
    Panel((Rectangle){(float)cX,(float)(cY+40),320.f,100.f},C_PANEL2,C_BORDER);
    DrawText("S_Lord",cX+16,cY+52,24,C_GOLD_L);
    Badge(cX+16, cY+84,"Rank: Gold",C_PANEL3,C_MUTED);
    char rls[32]; snprintf(rls,sizeof(rls),"Level %d",register_level);
    Badge(cX+130,cY+84,rls,C_PANEL3,C_BLUE_L);
    HDivider(cX,cY+156,640);
    DrawText("REGISTERED SKILLS",cX,cY+164,13,C_MUTED);
    for(int i=0;i<REGISTER_SKILL_COUNT;i++){
        int y=cY+186+i*40, val=skills[register_indices[i]];
        Panel((Rectangle){(float)cX,(float)y,460.f,32.f},C_PANEL2,C_BORDER);
        DrawText(register_skill_names[i],cX+14,y+8,16,C_TEXT);
        char vs[16]; snprintf(vs,sizeof(vs),"%d",val);
        DrawText(vs,cX+460-MeasureText(vs,16)-14,y+8,16,val>0?C_GREEN_L:val<0?C_RED_L:C_MUTED);
    }
}

// Coming Soon placeholder
static void DrawComingSoon(int cX, int cY, int tabIdx)
{
    DrawText(tab_names[tabIdx],cX,cY,22,C_GOLD_L);
    HDivider(cX,cY+28,640);
    Panel((Rectangle){(float)cX,(float)(cY+50),640.f,380.f},C_PANEL2,C_BORDER);
    const char *cs="Coming soon...";
    DrawText(cs,cX+(640-MeasureText(cs,22))/2,cY+220,22,C_MUTED);
}

// Quest list renderer
static void DrawQuestList(int cx, int cy, int cw, int listH, int type)
{
    int idxs[MAX_QUESTS], n=0;
    for(int i=0;i<questCount;i++) if(quests[i].type==type) idxs[n++]=i;

    int totalH=n*QUEST_ROW_H;
    int maxScroll=totalH-listH;
    if(maxScroll<0) maxScroll=0;

    Vector2 mp=GetMousePosition();
    if(mp.x>cx&&mp.x<cx+cw&&mp.y>cy&&mp.y<cy+listH&&!quest_create&&quest_view<0){
        float wh=GetMouseWheelMove();
        if(wh!=0){
            quest_scroll-=(int)(wh*22);
            if(quest_scroll<0) quest_scroll=0;
            if(quest_scroll>maxScroll) quest_scroll=maxScroll;
        }
    }

    BeginScissorMode(cx,cy,cw,listH);

    if(n==0) DrawText("No quests yet — create one below.",cx+16,cy+20,14,Q_TDARK);

    for(int ii=0;ii<n;ii++){
        int qi=idxs[ii];
        Quest *q=&quests[qi];
        int ry=cy+ii*QUEST_ROW_H-quest_scroll;
        if(ry+QUEST_ROW_H<cy||ry>cy+listH) continue;

        Rectangle row={(float)cx,(float)ry,(float)cw,QUEST_ROW_H-4};
        bool hov=CheckCollisionPointRec(mp,row)&&!quest_create&&quest_view<0;
        bool done=q->done;

        DrawRectangleRounded(row,0.05f,4,done?Q_DONEBG:(hov?Q_HOVER:C_PANEL));
        DrawRectangleRoundedLinesEx(row,0.05f,4,1.f,done?(Color){20,80,40,255}:(hov?Q_BORDER2:C_BORDER));
        DrawRectangle(cx,ry,4,QUEST_ROW_H-4,done?C_GREEN:C_RED);

        int cbY=ry+(QUEST_ROW_H-4-20)/2;
        Rectangle cb={(float)(cx+12),(float)cbY,20,20};
        DrawRectangleRec(cb,Q_INBG);
        DrawRectangleLinesEx(cb,1.5f,done?C_GREEN:Q_BORDER2);
        if(done) DrawText("X",(int)cb.x+5,(int)cb.y+2,17,C_GREEN);

        if(hov&&CheckCollisionPointRec(mp,cb)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            q->done=!q->done; QuestSave();
            ShowNotif(q->done?"Quest completed!":"Quest reopened.");
        }

        DrawText(q->name,cx+40,ry+8,15,done?Q_TDIM:Q_GPALE);
        char prev[52]; strncpy(prev,q->desc,51); prev[51]='\0';
        if(strlen(prev)>44){ prev[44]='.'; prev[45]='.'; prev[46]='.'; prev[47]='\0'; }
        DrawText(prev,cx+40,ry+28,12,Q_TDIM);

        int bx=cx+cw-184;
        QBadge(bx,   ry+8,"CP",q->cp,Q_CP_DIM,Q_CP);
        QBadge(bx+90,ry+8,"XP",q->xp,Q_XP_DIM,Q_XP);

        Rectangle vb={(float)(cx+cw-184),(float)(ry+30),88,22};
        Rectangle db={(float)(cx+cw- 90),(float)(ry+30),86,22};
        bool vhov=hov&&CheckCollisionPointRec(mp,vb);
        bool dhov=hov&&CheckCollisionPointRec(mp,db);
        QButton(vb,"View",  C_PANEL2,            Q_BORDER2,vhov);
        QButton(db,"Delete",(Color){40,10,10,255},C_RED,    dhov);

        if(vhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) quest_view=qi;

        if(dhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            for(int k=qi;k<questCount-1;k++) quests[k]=quests[k+1];
            questCount--; QuestSave();
            ShowNotif("Quest deleted.");
            if(quest_scroll>QUEST_ROW_H) quest_scroll-=QUEST_ROW_H;
        }
    }
    EndScissorMode();

    if(maxScroll>0&&n>0){
        float ratio=(float)listH/(n*QUEST_ROW_H);
        float barH=listH*ratio; if(barH<24)barH=24;
        float barY=(float)cy+(float)quest_scroll/(n*QUEST_ROW_H)*listH;
        DrawRectangleRounded((Rectangle){(float)(cx+cw-6),(float)barY,5,barH},0.5f,4,Q_BORDER2);
    }
}

// Quest create modal with scale animation
static void DrawQuestCreateModal(int quest_type, float dt)
{
    q_mscale+=dt*6.f;
    if(q_mscale>1.f) q_mscale=1.f;
    float s=q_mscale; s=s*s*(3-2*s);

    DrawRectangle(0,0,900,600,Q_OVERLAY);

    int mw=520, mh=460;
    int rx=(int)(450-mw*s/2), ry=(int)(300-mh*s/2);
    int rw=(int)(mw*s), rh=(int)(mh*s);
    if(rw<10||rh<10) return;

    int mx=(900-mw)/2, my=(600-mh)/2;

    Rectangle modal={(float)rx,(float)ry,(float)rw,(float)rh};
    DrawRectangleRounded(modal,0.07f,8,(Color){15,11,32,255});
    DrawRectangleRoundedLinesEx(modal,0.07f,8,1.5f,Q_BORDER2);
    DrawRectangleRoundedLinesEx(
        (Rectangle){modal.x-2,modal.y-2,modal.width+4,modal.height+4},
        0.07f,8,1.f,(Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,50});

    if(s<0.8f) return;

    DrawRectangleRounded((Rectangle){(float)mx,(float)my,(float)mw,46},0.06f,6,(Color){25,18,50,255});
    const char *ttl=quest_type==1?"New Story-Quest":"New Quest";
    DrawText(ttl,mx+18,my+13,18,Q_GPALE);

    Rectangle closeR={(float)(mx+mw-40),(float)(my+8),30,30};
    bool cHov=CheckCollisionPointRec(GetMousePosition(),closeR);
    DrawRectangleRounded(closeR,0.3f,4,cHov?(Color){60,20,20,255}:C_PANEL2);
    DrawText("X",(int)closeR.x+10,(int)closeR.y+8,15,C_RED);
    if(cHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) quest_create=false;

    int fy=my+60;

    Rectangle nameR={(float)(mx+18),(float)(fy+18),(float)(mw-36),34};
    if(CheckCollisionPointRec(GetMousePosition(),nameR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) q_focus=0;
    QInputBox(nameR,"Quest Name",q_name,q_focus==0);
    if(q_focus==0) TypeInto(q_name,MAX_QNAME,false,dt);
    fy+=66;

    Rectangle descR={(float)(mx+18),(float)(fy+18),(float)(mw-36),58};
    if(CheckCollisionPointRec(GetMousePosition(),descR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) q_focus=1;
    QInputBox(descR,"Description",q_desc,q_focus==1);
    if(q_focus==1) TypeInto(q_desc,MAX_QDESC,false,dt);
    fy+=96;

    int hw2=(mw-36)/2-6;
    Rectangle cpR={(float)(mx+18),(float)(fy+18),(float)hw2,34};
    Rectangle xpR={(float)(mx+18+hw2+12),(float)(fy+18),(float)hw2,34};
    if(CheckCollisionPointRec(GetMousePosition(),cpR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) q_focus=2;
    if(CheckCollisionPointRec(GetMousePosition(),xpR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) q_focus=3;
    QInputBox(cpR,"CP Reward",q_cp,q_focus==2);
    QInputBox(xpR,"XP Reward",q_xp,q_focus==3);
    if(q_focus==2) TypeInto(q_cp,MAX_QNUM,true,dt);
    if(q_focus==3) TypeInto(q_xp,MAX_QNUM,true,dt);
    if(IsKeyPressed(KEY_TAB)) q_focus=(q_focus+1)%4;

    Rectangle okR={(float)(mx+18),(float)(my+mh-54),(float)(mw-36)/2-6,40};
    Rectangle cnR={(float)(mx+18+(mw-36)/2+6),(float)(my+mh-54),(float)(mw-36)/2-6,40};
    bool okH=CheckCollisionPointRec(GetMousePosition(),okR);
    bool cnH=CheckCollisionPointRec(GetMousePosition(),cnR);
    QButton(okR,"Create Quest",(Color){20,45,20,255},C_GREEN,okH);
    QButton(cnR,"Cancel",     (Color){40,10,10,255},C_RED,  cnH);
    if(cnH&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) quest_create=false;

    bool doCreate=(okH&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON))||(IsKeyPressed(KEY_ENTER)&&q_focus!=1);
    if(doCreate){
        if(!strlen(q_name)){ ShowNotif("Please enter a quest name!"); }
        else if(questCount<MAX_QUESTS){
            Quest *nq=&quests[questCount++];
            memset(nq,0,sizeof(Quest));
            strncpy(nq->name,q_name,MAX_QNAME-1);
            strncpy(nq->desc,q_desc,MAX_QDESC-1);
            nq->cp=atoi(q_cp); nq->xp=atoi(q_xp);
            nq->done=false; nq->type=quest_type;
            QuestSave();
            quest_create=false;
            ShowNotif("Quest created!");
        }
    }
}

// Quest view modal
static void DrawQuestViewModal(void)
{
    if(quest_view<0||quest_view>=questCount) return;
    Quest *q=&quests[quest_view];

    DrawRectangle(0,0,900,600,Q_OVERLAY);

    int mw=500, mh=380;
    int mx=(900-mw)/2, my=(600-mh)/2;
    Rectangle modal={(float)mx,(float)my,(float)mw,(float)mh};
    DrawRectangleRounded(modal,0.07f,8,(Color){14,11,30,255});
    DrawRectangleRoundedLinesEx(modal,0.07f,8,1.5f,Q_GDIM);
    DrawRectangleRoundedLinesEx(
        (Rectangle){modal.x-2,modal.y-2,modal.width+4,modal.height+4},
        0.07f,8,1.f,(Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,35});

    DrawRectangleRounded((Rectangle){(float)mx,(float)my,(float)mw,48},0.07f,6,(Color){22,17,48,255});
    DrawText(q->name,mx+18,my+14,17,q->done?Q_TDIM:Q_GPALE);
    Color sc=q->done?C_GREEN:C_RED;
    const char *sl=q->done?"Completed":"Active";
    DrawText(sl,mx+mw-MeasureText(sl,13)-16,my+17,13,sc);

    QBadge(mx+18, my+56,"CP",q->cp,Q_CP_DIM,Q_CP);
    QBadge(mx+120,my+56,"XP",q->xp,Q_XP_DIM,Q_XP);

    DrawLine(mx+18,my+86,mx+mw-18,my+86,C_BORDER);
    DrawText("Description:",mx+18,my+96,12,Q_TDIM);
    if(!strlen(q->desc)) DrawText("No description.",mx+18,my+114,14,Q_TDARK);
    else QTextWrap(q->desc,mx+18,my+114,14,mw-44,C_TEXT);

    int bW=(mw-48)/2;
    Rectangle tb={(float)(mx+18),(float)(my+mh-52),(float)bW,40};
    Rectangle cb={(float)(mx+24+bW),(float)(my+mh-52),(float)bW,40};
    bool tHov=CheckCollisionPointRec(GetMousePosition(),tb);
    bool cHov=CheckCollisionPointRec(GetMousePosition(),cb);
    Color tbg=q->done?(Color){35,12,12,255}:(Color){12,35,18,255};
    Color tfg=q->done?C_RED:C_GREEN;
    QButton(tb,q->done?"Mark unfinished":"Mark finished",tbg,tfg,tHov);
    QButton(cb,"Close",(Color){30,22,55,255},Q_BORDER2,cHov);

    if(tHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        q->done=!q->done; QuestSave();
        ShowNotif(q->done?"Quest completed":"Quest reopened");
    }
    if(cHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) quest_view=-1;
}

// Notification toast overlay — visible on all tabs
static void DrawNotifToast(float dt)
{
    if(notifTimer<=0) return;
    notifTimer-=dt;
    float alpha=notifTimer>0.4f?1.f:notifTimer/0.4f;
    float slide=notifTimer>2.2f?(2.5f-notifTimer)/0.3f:1.f;
    if(slide<0)slide=0; if(slide>1)slide=1;
    int nw=MeasureText(notifMsg,14)+26;
    int nx=900-nw-14;
    int ny=(int)(558-(slide*44));
    DrawRectangleRounded((Rectangle){(float)nx,(float)ny,(float)nw,34},0.35f,6,
        (Color){Q_NOTIF.r,Q_NOTIF.g,Q_NOTIF.b,(unsigned char)(165*alpha)});
    DrawText(notifMsg,nx+13,ny+9,14,(Color){255,255,255,(unsigned char)(255*alpha)});
}

// Quests tab entry point
static void DrawTabQuests(int cX, int cY, float dt)
{
    int cw=660;

    DrawText("QUESTS",cX,cY,22,C_GOLD_L);
    HDivider(cX,cY+34,cw);

    const char *qtabs[]={"Quests","Story Quests","Chain Quests","Tree Quests"};
    int tabW=(cw-12)/4;
    for(int i=0;i<4;i++){
        int tx=cX+i*(tabW+4), ty=cY+40;
        Rectangle tr={(float)tx,(float)ty,(float)tabW,26};
        bool active=(quest_tab==i);
        bool hov=CheckCollisionPointRec(GetMousePosition(),tr)&&!quest_create&&quest_view<0;
        DrawRectangleRounded(tr,0.25f,6,active?C_PANEL3:(hov?C_PANEL2:C_PANEL));
        DrawRectangleRoundedLinesEx(tr,0.25f,6,1.f,active?C_GOLD:C_BORDER);
        if(active) DrawRectangle(tx,ty,tabW,3,C_GOLD);
        int tw2=MeasureText(qtabs[i],12);
        DrawText(qtabs[i],tx+(tabW-tw2)/2,ty+7,12,active?C_GOLD_L:C_MUTED);
        if(hov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){ quest_tab=i; quest_scroll=0; }
    }

    int listY=cY+74;
    int createH=38;
    int createY=572-createH-6;
    int listH=createY-4-listY;

    if(quest_tab==0||quest_tab==1){
        DrawQuestList(cX,listY,cw,listH,quest_tab);

        Rectangle nb={(float)cX,(float)createY,(float)cw,(float)createH};
        bool nhov=CheckCollisionPointRec(GetMousePosition(),nb)&&!quest_create&&quest_view<0;
        DrawRectangleRounded(nb,0.2f,6,(Color){25,18,50,255});
        DrawRectangleRoundedLinesEx(nb,0.2f,6,1.f,nhov?C_GOLD:Q_BORDER2);
        const char *lbl=quest_tab==1?"+ New Story-Quest":"+ New Quest";
        int lw=MeasureText(lbl,15);
        DrawText(lbl,(int)(nb.x+nb.width/2-lw/2),(int)(nb.y+11),15,nhov?C_GOLD_L:Q_TDIM);

        if(nhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            quest_create=true; q_focus=0; q_mscale=0.f;
            memset(q_name,0,MAX_QNAME); memset(q_desc,0,MAX_QDESC);
            memset(q_cp,0,MAX_QNUM);    memset(q_xp,0,MAX_QNUM);
        }
    } else {
        Panel((Rectangle){(float)cX,(float)listY,(float)cw,(float)(listH+createH+8)},C_PANEL2,C_BORDER);
        const char *names[]={"Chain Quests","Tree Quests"};
        const char *nm=names[quest_tab-2];
        int nmH=listH+createH+8;
        DrawText(nm, cX+(cw-MeasureText(nm,18))/2, listY+nmH/2-20,18,C_GOLD_L);
        const char *cs="Coming soon...";
        DrawText(cs, cX+(cw-MeasureText(cs,14))/2, listY+nmH/2+8, 14,C_MUTED);
    }

    (void)dt;
}

// Main
int main(void)
{
    int skills[SKILL_COUNT]          = {0};
    int attributes[ATTRIBUTES_COUNT] = {0};
    int tnl[MAX_LEVEL]               = {0};
    int base_stats[STATS_COUNT]      = {0};
    int gear_stats[STATS_COUNT]      = {0};

    int xp=1673, level=4, cp=1650;
    int selected_stat=0, selected_skill=0, selected_attr=0;
    int edit_mode=0, player_edit_mode=0;
    int selected_player_stat=0;
    int current_tab=0;

    for(int i=0;i<MAX_CLASSES;i++)
        for(int j=0;j<MAX_CLASS_LEVELS;j++)
            classes[i].levelReqs[j]=-1;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(900,600,"RPG-Menu");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    // Load data from files
    FILE *lvlfile=fopen("levels.txt","r");
    if(lvlfile){ for(int i=0;i<MAX_LEVEL;i++) fscanf(lvlfile,"%d",&tnl[i]); fclose(lvlfile); }
    FILE *f=fopen("skills.txt","r");
    if(f){ for(int i=0;i<SKILL_COUNT;i++) fscanf(f,"%d",&skills[i]); fclose(f); }
    f=fopen("attributes.txt","r");
    if(f){ for(int i=0;i<ATTRIBUTES_COUNT;i++) fscanf(f,"%d",&attributes[i]); fclose(f); }
    FILE *fs=fopen("stats.txt","r");
    if(fs){ for(int i=0;i<STATS_COUNT;i++) fscanf(fs,"%d",&base_stats[i]); fclose(fs); }
    FILE *fps=fopen("player_stats.txt","r");
    if(fps){ fscanf(fps,"%d %d %d",&level,&xp,&cp); fclose(fps); }
    QuestLoad();
    LoadClassState();

    while(!WindowShouldClose()){
        float dt=GetFrameTime();
        notifTimer-=dt;

        for(int i=0;i<STATS_COUNT;i++) gear_stats[i]=base_stats[i];

        // Auto-calculate cls_totalXP every frame
        UpdateTotalXP(level, xp, tnl);

        bool quest_modal_open = (current_tab==4)&&(quest_create||quest_view>=0);
        bool class_modal_open = (current_tab==0)&&showModal;
        bool any_modal_open   = quest_modal_open||class_modal_open;

        // ESC closes any open modal
        if(IsKeyPressed(KEY_ESCAPE)){
            if(current_tab==0&&showModal)
                { showModal=false; memset(tbNewName.buf,0,sizeof(tbNewName.buf)); }
            else if(current_tab==4&&quest_create)   quest_create=false;
            else if(current_tab==4&&quest_view>=0)  quest_view=-1;
        }

        // Global shortcuts (blocked while a modal is open)
        if(!any_modal_open){
            if(IsKeyPressed(KEY_O)){
                player_edit_mode=!player_edit_mode;
                if(player_edit_mode) edit_mode=0;
            }
            if(IsKeyPressed(KEY_P)){
                if(current_tab==1||current_tab==2||current_tab==3){
                    edit_mode=!edit_mode;
                    if(edit_mode) player_edit_mode=0;
                }
            }
        }

        // Player-stats edit mode
        if(player_edit_mode&&!any_modal_open){
            bool p_changed=false;
            static float hold_timer=0.f;

            if(IsKeyPressed(KEY_UP))   selected_player_stat=selected_player_stat>0?selected_player_stat-1:2;
            if(IsKeyPressed(KEY_DOWN)) selected_player_stat=selected_player_stat<2?selected_player_stat+1:0;

            long long base_inc=1;
            if(IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))     base_inc=100;
            if(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL)) base_inc=10000;
            int dir=0;
            if(IsKeyDown(KEY_RIGHT)) dir= 1;
            if(IsKeyDown(KEY_LEFT))  dir=-1;
            if(dir!=0){
                hold_timer+=dt;
                if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_LEFT)){
                    if     (selected_player_stat==0) level=dir>0?(level<MAX_LEVEL-1?level+1:level):(level>0?level-1:0);
                    else if(selected_player_stat==1) xp+=(int)(base_inc*dir);
                    else if(selected_player_stat==2) cp+=(int)(base_inc*dir);
                    p_changed=true;
                }
                if(hold_timer>0.6f&&selected_player_stat!=0){
                    float acc=hold_timer>5.f?100.f:hold_timer>3.f?10.f:1.f;
                    long long amt=(long long)(base_inc*acc*dir);
                    if(selected_player_stat==1) xp+=(int)amt;
                    else if(selected_player_stat==2) cp+=(int)amt;
                    p_changed=true;
                }
            } else hold_timer=0.f;
            if(xp<0)xp=0; if(cp<0)cp=0;
            if(p_changed) SavePlayerStats(level,xp,cp);

        // Tab navigation and content editing
        } else if(!player_edit_mode&&!any_modal_open){
            if(IsKeyPressed(KEY_UP)){
                if(!edit_mode){
                    current_tab=current_tab>0?current_tab-1:TAB_COUNT-1;
                    if(current_tab!=4){ quest_create=false; quest_view=-1; }
                } else {
                    if(current_tab==1) selected_stat =selected_stat >0?selected_stat -1:STATS_COUNT-1;
                    if(current_tab==2) selected_attr =selected_attr >0?selected_attr -1:ATTRIBUTES_COUNT-1;
                    if(current_tab==3) selected_skill=selected_skill>0?selected_skill-1:SKILL_COUNT-1;
                }
            }
            if(IsKeyPressed(KEY_DOWN)){
                if(!edit_mode){
                    current_tab=current_tab<TAB_COUNT-1?current_tab+1:0;
                    if(current_tab!=4){ quest_create=false; quest_view=-1; }
                } else {
                    if(current_tab==1) selected_stat =selected_stat <STATS_COUNT-1     ?selected_stat +1:0;
                    if(current_tab==2) selected_attr =selected_attr <ATTRIBUTES_COUNT-1?selected_attr +1:0;
                    if(current_tab==3) selected_skill=selected_skill<SKILL_COUNT-1     ?selected_skill+1:0;
                }
            }
            if(edit_mode){
                bool changed=false;
                if(IsKeyPressed(KEY_RIGHT)){
                    if(current_tab==1) base_stats[selected_stat]++;
                    if(current_tab==2) attributes[selected_attr]++;
                    if(current_tab==3) skills[selected_skill]++;
                    changed=true;
                }
                if(IsKeyPressed(KEY_LEFT)){
                    if(current_tab==1) base_stats[selected_stat]--;
                    if(current_tab==2) attributes[selected_attr]--;
                    if(current_tab==3) skills[selected_skill]--;
                    changed=true;
                }
                if(changed) SaveData(attributes,skills,base_stats);
            }
        }

        // Draw frame
        BeginDrawing();
        ClearBackground(C_BG);

        DrawHeader(level,xp,cp, level<MAX_LEVEL?tnl[level]:0, player_edit_mode,selected_player_stat);
        DrawTabs(current_tab,edit_mode,player_edit_mode);

        DrawRectangle(193,109,707,463,C_PANEL);
        DrawLine(193,109,193,572,C_BORDER);
        DrawLine(193,572,900,572,C_BORDER);
        int cX=200, cY=122;

        switch(current_tab){
            case 0:  DrawTabClasses    (cX,cY);                                               break;
            case 1:  DrawTabStats      (cX,cY,base_stats,gear_stats,selected_stat,edit_mode); break;
            case 2:  DrawTabAttributes (cX,cY,attributes,selected_attr,edit_mode);            break;
            case 3:  DrawTabSkills     (cX,cY,skills,selected_skill,edit_mode);               break;
            case 4:  DrawTabQuests     (cX,cY,dt);                                            break;
            case 9:  DrawTabRegister   (cX,cY,skills);                                        break;
            default: DrawComingSoon    (cX,cY,current_tab);                                   break;
        }

        DrawFooter(edit_mode,player_edit_mode);

        // Overlays — drawn last so they appear on top
        if(current_tab==0 && showModal)     DrawClassModal();
        if(current_tab==4 && quest_create)  DrawQuestCreateModal(quest_tab==1?1:0,dt);
        if(current_tab==4 && quest_view>=0) DrawQuestViewModal();
        DrawNotifToast(dt);

        EndDrawing();
    }

    // Final save on exit
    SaveData(attributes,skills,base_stats);
    SavePlayerStats(level,xp,cp);
    SaveClassState();
    CloseWindow();
    return 0;
}

