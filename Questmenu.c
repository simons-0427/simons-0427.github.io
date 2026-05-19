#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define SW          1200
#define SH          720
#define TOPBAR_H    70
#define TABAREA_H   120
#define ROW_H       82

#define MAX_QUESTS  200
#define MAX_NAME    64
#define MAX_DESC    300
#define MAX_NUM     8
#define SAVE_FILE   "quest_save.txt"

#define NODE_W      220
#define NODE_H      92

#define C_BG         (Color){10,  8,  20, 255}
#define C_PANEL      (Color){18, 14,  35, 255}
#define C_PANEL2     (Color){22, 17,  42, 255}
#define C_BORDER     (Color){55, 40,  90, 255}
#define C_BORDER2    (Color){80, 60, 130, 255}
#define C_HOVER      (Color){35, 27,  65, 255}
#define C_SEL        (Color){50, 35,  90, 255}
#define C_GOLD       (Color){201,162,  39, 255}
#define C_GOLD_DIM   (Color){ 90, 72,  15, 255}
#define C_GOLD_PALE  (Color){235,200, 110, 255}
#define C_TEXT       (Color){220,210, 190, 255}
#define C_TEXT_DIM   (Color){110,100,  80, 255}
#define C_TEXT_DARK  (Color){ 50, 45,  35, 255}
#define C_XP         (Color){ 80,175, 255, 255}
#define C_XP_DIM     (Color){ 20, 55,  95, 255}
#define C_CP         (Color){255,200,  50, 255}
#define C_CP_DIM     (Color){ 85, 62,  10, 255}
#define C_GREEN      (Color){ 50,185,  85, 255}
#define C_GREEN_DIM  (Color){ 15, 60,  28, 255}
#define C_RED        (Color){195, 50,  50, 255}
#define C_PURPLE     (Color){130, 70, 210, 255}
#define C_INPUT_BG   (Color){  7,  5,  14, 255}
#define C_OVERLAY    (Color){  3,  2,  10, 230}
#define C_DONE_BG    (Color){ 10, 35,  18, 255}
#define C_NOTIF      (Color){ 40,170,  80, 255}

typedef struct {
    char  name[MAX_NAME];
    char  desc[MAX_DESC];
    int   cp, xp;
    bool  done;
    int   type;      /* 0=Quest 1=Story 2=Chain */
    int   prereq;    /* index of quest that unlocks this chain-quest, -1 = none */
} Quest;

Quest quests[MAX_QUESTS];
int   questCount = 0;
int   earnedCP   = 0, earnedXP  = 0;
int   totalCP    = 0, totalXP   = 0;

int   activeTab    = 0;   /* 0=Quests 1=Story 2=Chain 3=Tree */
int   scrollOff    = 0;
int   hoveredQuest = -1;
int   viewQuest    = -1;

bool  showCreate   = false;
char  mName[MAX_NAME]= {0};
char  mDesc[MAX_DESC]= {0};
char  mCP  [MAX_NUM] = {0};
char  mXP  [MAX_NUM] = {0};
int   focusField   = 0;
float modalScale   = 0.0f;

char  notifMsg[80] = {0};
float notifTimer   = 0.0f;

static float bsTimer = 0.0f;
static int chainLinkSource = -1;

static bool mOver(Rectangle r){
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static int questTypeForActiveTab(void){
    if(activeTab == 1) return 1;
    if(activeTab == 2) return 2;
    return 0;
}

static bool isValidQuestIndex(int idx){
    return idx >= 0 && idx < questCount;
}

static bool isChainQuestUnlocked(int idx){
    if(!isValidQuestIndex(idx)) return false;
    if(quests[idx].type != 2) return true;
    if(quests[idx].prereq < 0) return true;
    if(!isValidQuestIndex(quests[idx].prereq)) return true;
    return quests[quests[idx].prereq].done;
}

static bool canLinkChainQuest(int from, int to){
    if(!isValidQuestIndex(from) || !isValidQuestIndex(to)) return false;
    if(from == to) return false;
    if(quests[from].type != 2 || quests[to].type != 2) return false;

    int cur = from;
    while(isValidQuestIndex(cur) && quests[cur].prereq >= 0){
        if(quests[cur].prereq == to) return false;
        cur = quests[cur].prereq;
    }
    return true;
}

static void recalc(void){
    earnedCP = earnedXP = totalCP = totalXP = 0;
    for(int i=0;i<questCount;i++){
        totalCP += quests[i].cp;
        totalXP += quests[i].xp;
        if(quests[i].done){ earnedCP += quests[i].cp; earnedXP += quests[i].xp; }
    }
}

static void showNotif(const char* msg){
    strncpy(notifMsg, msg, 79);
    notifMsg[79] = '\0';
    notifTimer = 2.5f;
}

static int getTabQuestCount(int type){
    int cnt = 0;
    for(int i=0;i<questCount;i++) if(quests[i].type == type) cnt++;
    return cnt;
}

static int getTabDoneCount(int type){
    int done = 0;
    for(int i=0;i<questCount;i++) if(quests[i].type == type && quests[i].done) done++;
    return done;
}

static void remapQuestLinksAfterDelete(int deletedIdx){
    for(int i=0;i<questCount;i++){
        if(quests[i].prereq == deletedIdx){
            quests[i].prereq = -1;
        } else if(quests[i].prereq > deletedIdx){
            quests[i].prereq--;
        }
    }

    if(chainLinkSource == deletedIdx) chainLinkSource = -1;
    else if(chainLinkSource > deletedIdx) chainLinkSource--;

    if(viewQuest == deletedIdx) viewQuest = -1;
    else if(viewQuest > deletedIdx) viewQuest--;
}

static void resetChainDependents(int idx){
    for(int i=0;i<questCount;i++){
        if(quests[i].type == 2 && quests[i].prereq == idx){
            if(quests[i].done) quests[i].done = false;
            resetChainDependents(i);
        }
    }
}

static void saveData(void){
    recalc();
    FILE* f = fopen(SAVE_FILE,"w");
    if(!f){ showNotif("ERROR: Datei konnte nicht gespeichert werden!"); return; }

    fprintf(f,"# RPG Quest Journal - Speicherdatei\n");
    fprintf(f,"earned_cp=%d\n", earnedCP);
    fprintf(f,"earned_xp=%d\n", earnedXP);
    fprintf(f,"total_cp=%d\n",  totalCP);
    fprintf(f,"total_xp=%d\n\n",totalXP);

    for(int i=0;i<questCount;i++){
        fprintf(f,"QUEST\n");
        fprintf(f,"name=%s\n", quests[i].name);
        fprintf(f,"desc=%s\n", quests[i].desc);
        fprintf(f,"cp=%d\n",   quests[i].cp);
        fprintf(f,"xp=%d\n",   quests[i].xp);
        fprintf(f,"done=%d\n", quests[i].done?1:0);
        fprintf(f,"type=%d\n", quests[i].type);
        fprintf(f,"prereq=%d\n", quests[i].prereq);
        fprintf(f,"---\n");
    }
    fclose(f);
}

static void loadData(void){
    FILE* f = fopen(SAVE_FILE,"r");
    if(!f) return;

    char line[512];
    Quest* cur = NULL;

    while(fgets(line,sizeof(line),f)){
        int len = (int)strlen(line);
        while(len>0 && (line[len-1]=='\n' || line[len-1]=='\r')) line[--len]='\0';

        if(strcmp(line,"QUEST")==0){
            if(questCount<MAX_QUESTS){
                cur=&quests[questCount++];
                memset(cur,0,sizeof(Quest));
                cur->prereq = -1;
            }
        } else if(cur && strncmp(line,"name=",5)==0){
            strncpy(cur->name,line+5,MAX_NAME-1);
        } else if(cur && strncmp(line,"desc=",5)==0){
            strncpy(cur->desc,line+5,MAX_DESC-1);
        } else if(cur && strncmp(line,"cp=",3)==0){
            cur->cp = atoi(line+3);
        } else if(cur && strncmp(line,"xp=",3)==0){
            cur->xp = atoi(line+3);
        } else if(cur && strncmp(line,"done=",5)==0){
            cur->done = (atoi(line+5)==1);
        } else if(cur && strncmp(line,"type=",5)==0){
            cur->type = atoi(line+5);
        } else if(cur && strncmp(line,"prereq=",7)==0){
            cur->prereq = atoi(line+7);
        } else if(strcmp(line,"---")==0){
            cur = NULL;
        }
    }

    fclose(f);
    recalc();
}

static void deleteQuestAt(int idx){
    if(!isValidQuestIndex(idx)) return;
    for(int k=idx;k<questCount-1;k++) quests[k]=quests[k+1];
    questCount--;
    remapQuestLinksAfterDelete(idx);
    recalc();
    saveData();
}

static bool setQuestDoneState(int idx, bool done){
    if(!isValidQuestIndex(idx)) return false;
    Quest* q = &quests[idx];

    if(done && q->type == 2 && !isChainQuestUnlocked(idx)){
        showNotif("Chain-Quest is not unlocked yet");
        return false;
    }

    q->done = done;
    if(!done){
        resetChainDependents(idx);
    }
    recalc();
    saveData();
    return true;
}

static void typeInto(char* buf, int maxLen, bool numeric, float dt){
    int key = GetCharPressed();
    while(key>0){
        bool ok = numeric ? (key>='0'&&key<='9') : (key>=32&&key<126);
        if(ok && (int)strlen(buf)<maxLen-1){
            int l = (int)strlen(buf);
            buf[l] = (char)key;
            buf[l+1] = '\0';
        }
        key = GetCharPressed();
    }

    if(IsKeyPressed(KEY_BACKSPACE)){
        int l = (int)strlen(buf);
        if(l>0) buf[l-1] = '\0';
        bsTimer = 0;
    } else if(IsKeyDown(KEY_BACKSPACE)){
        bsTimer += dt;
        if(bsTimer>0.5f){
            bsTimer = 0.42f;
            int l = (int)strlen(buf);
            if(l>0) buf[l-1] = '\0';
        }
    } else {
        bsTimer = 0;
    }
}

static void drawBadge(int x,int y,const char* lbl,int val,Color bg,Color fg){
    char buf[32];
    snprintf(buf,32,"%s %d",lbl,val);
    int w = MeasureText(buf,13)+14;
    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)w,22},0.45f,6,bg);
    DrawRectangleRoundedLines((Rectangle){(float)x,(float)y,(float)w,22},0.45f,6,fg);
    DrawText(buf,x+7,y+4,13,fg);
}

static void drawButton(Rectangle r, const char* txt, Color bg, Color fg, bool hover){
    Color bg2 = hover ? (Color){bg.r+20,bg.g+15,bg.b+30,bg.a} : bg;
    DrawRectangleRounded(r,0.35f,8,bg2);
    DrawRectangleRoundedLines(r,0.35f,8,fg);
    int tw = MeasureText(txt,15);
    DrawText(txt, (int)(r.x+r.width/2-tw/2), (int)(r.y+r.height/2-8), 15, fg);
}

static void drawInputBox(Rectangle r, const char* label, const char* val,
                         bool focused, Color borderCol){
    DrawText(label,(int)r.x,(int)r.y-18,13,C_TEXT_DIM);
    DrawRectangleRec(r, C_INPUT_BG);

    Color bc = focused ? C_GOLD : borderCol;
    DrawRectangleLinesEx(r,1.5f,bc);

    if(focused){
        DrawRectangleLinesEx((Rectangle){r.x-1,r.y-1,r.width+2,r.height+2},1,
                             (Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,80});
    }

    int maxW = (int)r.width-16;
    char disp[256];
    strncpy(disp,val,255);
    disp[255] = '\0';

    while(MeasureText(disp,15)>maxW && strlen(disp)>0) disp[strlen(disp)-1]='\0';

    DrawText(disp,(int)r.x+8,(int)r.y+(int)(r.height/2)-8,15,C_TEXT);

    if(focused && (int)(GetTime()*2)%2==0){
        int cx=(int)r.x+8+MeasureText(disp,15);
        DrawLine(cx,(int)r.y+6,cx,(int)r.y+(int)r.height-6,C_GOLD);
    }
}

static int drawTextWrapped(const char* txt,int x,int y,int fontSize,int maxW,Color col){
    char word[64];
    char line[512] = {0};
    int lineH = fontSize+4, yy=y;
    const char* p = txt;

    while(*p){
        int wi=0;
        while(*p && *p!=' ' && *p!='\n' && wi<63) word[wi++]=*p++;
        word[wi]='\0';
        if(*p==' ') p++;

        char test[512];
        if(strlen(line)==0) snprintf(test,512,"%s",word);
        else snprintf(test,512,"%s %s",line,word);

        if(MeasureText(test,fontSize)>maxW && strlen(line)>0){
            DrawText(line,x,yy,fontSize,col);
            yy += lineH;
            strcpy(line,word);
        } else {
            strcpy(line,test);
        }

        if(*p=='\n' || *(p-1)=='\n'){
            DrawText(line,x,yy,fontSize,col);
            yy += lineH;
            line[0] = '\0';
            if(*p=='\n') p++;
        }
    }

    if(strlen(line)>0){
        DrawText(line,x,yy,fontSize,col);
        yy += lineH;
    }

    return yy-y;
}

static void drawTopBar(void){
    DrawRectangleGradientH(0,0,SW,TOPBAR_H,
        (Color){20,15,45,255},(Color){10,8,22,255});
    DrawLine(0,TOPBAR_H,SW,TOPBAR_H,C_BORDER);
    DrawRectangle(0,TOPBAR_H-2,SW,2,C_GOLD_DIM);

    DrawText("QUEST BOOK", 20, 20, 26, C_GOLD);

    char buf[64];
    snprintf(buf,64,"%d / %d",earnedCP,totalCP);
    int x=SW-520;
    DrawText("CP",x,12,13,(Color){C_CP.r,C_CP.g,C_CP.b,150});
    DrawText(buf,x,28,16,C_CP);

    snprintf(buf,64,"%d / %d",earnedXP,totalXP);
    x=SW-280;
    DrawText("XP",x,12,13,(Color){C_XP.r,C_XP.g,C_XP.b,150});
    DrawText(buf,x,28,16,C_XP);
}

static void drawTabCards(void){
    const char* tabs[] = {"Quests", "Story Quests", "Chain Quests", "Tree Quests"};
    int startX = 20;
    int gap = 16;
    int cardW = (SW - startX*2 - gap*3) / 4;
    int cardH = 78;
    int y = TOPBAR_H + 18;

    for(int i=0;i<4;i++){
        Rectangle r = {(float)(startX + i*(cardW + gap)), (float)y, (float)cardW, (float)cardH};
        bool hov = mOver(r);
        bool sel = (activeTab == i);

        Color fill = sel ? C_SEL : (hov ? C_HOVER : C_PANEL);
        Color border = sel ? C_GOLD : C_BORDER2;
        Color text = sel ? C_GOLD_PALE : (hov ? C_TEXT : C_TEXT_DIM);

        DrawRectangleRounded(r, 0.18f, 8, fill);
        DrawRectangleRoundedLines(r, 0.18f, 8, border);

        if(sel){
            DrawRectangleRounded((Rectangle){r.x, r.y, r.width, 5}, 0.8f, 8, C_GOLD);
        }

        int tw = MeasureText(tabs[i], 18);
        DrawText(tabs[i], (int)(r.x + r.width/2 - tw/2), (int)r.y + 16, 18, text);

        if(i < 3){
            int questType = (i == 0) ? 0 : i;
            int total = getTabQuestCount(questType);
            int done = getTabDoneCount(questType);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d/%d", done, total);
            Color badgeColor = (done == total && total > 0) ? C_GREEN : C_TEXT_DARK;
            int bw = MeasureText(buf, 14);
            DrawText(buf, (int)(r.x + r.width/2 - bw/2), (int)r.y + 45, 14, badgeColor);
        } else {
            const char* soon = "Coming Soon";
            int sw = MeasureText(soon, 14);
            DrawText(soon, (int)(r.x + r.width/2 - sw/2), (int)r.y + 45, 14, C_TEXT_DARK);
        }

        if(hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            activeTab = i;
            scrollOff = 0;
            hoveredQuest = -1;
            chainLinkSource = -1;
        }
    }
}

static void drawQuestList(int type){
    int cx = 20;
    int cy = TOPBAR_H + TABAREA_H;
    int cw = SW - 40;
    int ch = SH - cy - 80;

    int idxs[MAX_QUESTS];
    int n = 0;
    for(int i=0;i<questCount;i++) if(quests[i].type==type) idxs[n++] = i;

    int maxScroll = (n * ROW_H - ch);
    if(maxScroll < 0) maxScroll = 0;

    float wh = (float)GetMouseWheelMove();
    if(wh!=0 && !showCreate && viewQuest<0){
        scrollOff -= (int)(wh*30);
        if(scrollOff<0) scrollOff=0;
        if(scrollOff>maxScroll) scrollOff=maxScroll;
    }

    BeginScissorMode(cx, cy, cw, ch);

    if(n==0){
        DrawText("No active Quests", cx+20, cy+40, 18, C_TEXT_DARK);
    }

    for(int ii=0;ii<n;ii++){
        int qi = idxs[ii];
        Quest* q = &quests[qi];
        int ry = cy + ii*ROW_H - scrollOff;
        if(ry+ROW_H < cy || ry > cy+ch) continue;

        Rectangle row = {(float)cx, (float)ry, (float)cw, ROW_H-4};
        bool hov = mOver(row) && !showCreate && viewQuest<0;
        bool done = q->done;

        Color rowBg = done ? C_DONE_BG : (hov ? C_HOVER : C_PANEL);
        DrawRectangleRounded(row,0.07f,4,rowBg);

        Color borderC = done ? (Color){20,80,40,255} : (hov ? C_BORDER2 : C_BORDER);
        DrawRectangleRoundedLines(row,0.07f,4,borderC);

        Color barC = done ? C_GREEN : C_RED;
        DrawRectangleRounded((Rectangle){(float)cx,(float)ry,4,ROW_H-4},0.2f,4,barC);

        Rectangle cb = {(float)cx+18,(float)ry+28,24,24};
        DrawRectangleRec(cb,C_INPUT_BG);
        DrawRectangleLinesEx(cb,1.5f, done?C_GREEN:C_BORDER2);
        if(done) DrawText("X",(int)cb.x+7,(int)cb.y+4,18,C_GREEN);

        if(hov && mOver(cb) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(setQuestDoneState(qi, !q->done)){
                showNotif(q->done?"Quest finished!":"Quest reactivated");
            }
        }

        Color nameC = done ? C_TEXT_DIM : C_GOLD_PALE;
        DrawText(q->name, cx+54, ry+12, 18, nameC);

        char prev[80];
        strncpy(prev,q->desc,79);
        prev[79]='\0';
        if(strlen(prev)>55){
            prev[55]='.';
            prev[56]='.';
            prev[57]='.';
            prev[58]='\0';
        }
        DrawText(prev, cx+54, ry+36, 13, C_TEXT_DIM);

        int bx = cx+cw-200;
        drawBadge(bx,     ry+16, "CP", q->cp, C_CP_DIM, C_CP);
        drawBadge(bx+100, ry+16, "XP", q->xp, C_XP_DIM, C_XP);

        Rectangle vb={(float)(cx+cw-200),(float)(ry+42),90,24};
        bool vhov=mOver(vb)&&hov;
        drawButton(vb,"View",C_PANEL2,C_BORDER2,vhov);
        if(vhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) viewQuest=qi;

        Rectangle db={(float)(cx+cw-104),(float)(ry+42),90,24};
        bool dhov=mOver(db)&&hov;
        drawButton(db,"Delete",(Color){40,10,10,255},C_RED,dhov);
        if(dhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            deleteQuestAt(qi);
            showNotif("Quest deleted");
            if(scrollOff>0) scrollOff-=ROW_H;
        }
    }

    EndScissorMode();

    if(maxScroll>0){
        float ratio = (float)ch / (n*ROW_H);
        float barH  = ch*ratio;
        if(barH<30) barH=30;
        float barY  = cy + (float)scrollOff/(n*ROW_H)*ch;
        DrawRectangleRounded((Rectangle){(float)(cx+cw+4),(float)barY,6,barH},0.5f,4,C_BORDER2);
    }

    Rectangle nb={(float)cx,(float)(SH-68),(float)cw,44};
    bool nhov = mOver(nb)&&!showCreate&&viewQuest<0;
    DrawRectangleRounded(nb,0.2f,6,(Color){25,18,50,255});
    DrawRectangleRoundedLines(nb,0.2f,6, nhov?C_GOLD:C_BORDER2);

    const char* label = (type==0) ? "Create new Quest" : "Create new Story-Quest";
    int lw = MeasureText(label,16);
    DrawText(label,(int)(nb.x+nb.width/2-lw/2),(int)(nb.y+14),16,nhov?C_GOLD_PALE:C_TEXT_DIM);

    if(nhov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        showCreate = true;
        focusField = 0;
        memset(mName,0,MAX_NAME);
        memset(mDesc,0,MAX_DESC);
        memset(mCP,0,MAX_NUM);
        memset(mXP,0,MAX_NUM);
        modalScale = 0;
    }
}

static void drawArrow(Vector2 start, Vector2 end, Color col){
    DrawLineEx(start, end, 3.0f, col);

    Vector2 dir = {end.x - start.x, end.y - start.y};
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if(len < 1.0f) return;
    dir.x /= len;
    dir.y /= len;

    Vector2 tip = end;
    Vector2 left = {tip.x - dir.x*16.0f - dir.y*8.0f, tip.y - dir.y*16.0f + dir.x*8.0f};
    Vector2 right= {tip.x - dir.x*16.0f + dir.y*8.0f, tip.y - dir.y*16.0f - dir.x*8.0f};
    DrawTriangle(tip, left, right, col);
}

static int getChainDepth(int idx){
    int depth = 0;
    int cur = idx;
    while(isValidQuestIndex(cur) && quests[cur].type == 2 && quests[cur].prereq >= 0){
        depth++;
        cur = quests[cur].prereq;
        if(depth > MAX_QUESTS) break;
    }
    return depth;
}

static void trimCopy(const char* src, char* dst, int dstSize, int maxChars){
    int len = (int)strlen(src);
    if(len < maxChars){
        strncpy(dst, src, dstSize - 1);
        dst[dstSize - 1] = '\0';
        return;
    }

    int copyLen = maxChars;
    if(copyLen > dstSize - 4) copyLen = dstSize - 4;
    strncpy(dst, src, copyLen);
    dst[copyLen] = '\0';
    strcat(dst, "...");
}

static void drawChainQuests(void){
    int panelX = 20;
    int panelY = TOPBAR_H + TABAREA_H;
    int panelW = SW - 40;
    int panelH = SH - panelY - 80;

    Rectangle panel = {(float)panelX, (float)panelY, (float)panelW, (float)panelH};
    DrawRectangleRounded(panel, 0.04f, 8, C_PANEL);
    DrawRectangleRoundedLines(panel, 0.04f, 8, C_BORDER);

    DrawText("Chain editor: Create quests, then connect them in order.", panelX+20, panelY+16, 18, C_GOLD_PALE);
    DrawText("Connect: click 'Link From' on quest A, then 'Set Next' on quest B. B is unlocked after A is completed.", panelX+20, panelY+40, 14, C_TEXT_DIM);

    int chainIdxs[MAX_QUESTS];
    Rectangle nodeRects[MAX_QUESTS];
    int n = 0;
    int maxDepth = 0;

    for(int i=0;i<questCount;i++){
        if(quests[i].type == 2){
            chainIdxs[n++] = i;
            nodeRects[n-1] = (Rectangle){0};
            int d = getChainDepth(i);
            if(d > maxDepth) maxDepth = d;
        }
    }

    if(n == 0){
        DrawText("No Chain-Quests yet. Create your first chain quest below.", panelX+20, panelY+92, 18, C_TEXT_DARK);
    }

    int cols = maxDepth + 1;
    if(cols < 1) cols = 1;
    float colW = (float)(panelW - 60) / cols;
    int countsPerCol[MAX_QUESTS] = {0};
    int slotInCol[MAX_QUESTS] = {0};

    for(int i=0;i<n;i++){
        int depth = getChainDepth(chainIdxs[i]);
        slotInCol[i] = countsPerCol[depth];
        countsPerCol[depth]++;
    }

    for(int i=0;i<n;i++){
        int qi = chainIdxs[i];
        int depth = getChainDepth(qi);
        float x = panelX + 24 + depth*colW;
        float y = panelY + 86 + slotInCol[i]*(NODE_H + 32);
        nodeRects[i] = (Rectangle){x,y,NODE_W,NODE_H};
    }

    for(int i=0;i<n;i++){
        int qi = chainIdxs[i];
        if(quests[qi].prereq < 0 || !isValidQuestIndex(quests[qi].prereq)) continue;

        int fromIdx = -1;
        for(int k=0;k<n;k++) if(chainIdxs[k] == quests[qi].prereq) { fromIdx = k; break; }
        if(fromIdx < 0) continue;

        Vector2 start = {nodeRects[fromIdx].x + nodeRects[fromIdx].width, nodeRects[fromIdx].y + nodeRects[fromIdx].height/2};
        Vector2 end   = {nodeRects[i].x - 10, nodeRects[i].y + nodeRects[i].height/2};
        Color lineColor = quests[quests[qi].prereq].done ? C_GREEN : C_BORDER2;
        drawArrow(start, end, lineColor);
    }

    for(int i=0;i<n;i++){
        int qi = chainIdxs[i];
        Quest* q = &quests[qi];
        Rectangle r = nodeRects[i];
        bool hov = mOver(r) && !showCreate && viewQuest < 0;
        bool unlocked = isChainQuestUnlocked(qi);

        Color bg = q->done ? C_DONE_BG : (unlocked ? (hov ? C_HOVER : C_PANEL2) : (Color){26,22,32,255});
        Color border = q->done ? C_GREEN : (chainLinkSource == qi ? C_GOLD : (unlocked ? C_BORDER2 : C_TEXT_DARK));

        DrawRectangleRounded(r,0.08f,6,bg);
        DrawRectangleRoundedLines(r,0.08f,6,border);
        DrawRectangleRounded((Rectangle){r.x,r.y,5,r.height},0.2f,4,q->done ? C_GREEN : (unlocked ? C_RED : C_TEXT_DARK));

        Color nameColor = q->done ? C_TEXT_DIM : (unlocked ? C_GOLD_PALE : C_TEXT_DIM);
        DrawText(q->name, (int)r.x+14, (int)r.y+10, 18, nameColor);

        char descBuf[96];
        trimCopy(q->desc, descBuf, sizeof(descBuf), 28);
        DrawText(descBuf, (int)r.x+14, (int)r.y+36, 13, C_TEXT_DIM);

        if(q->prereq >= 0 && isValidQuestIndex(q->prereq)){
            char reqBuf[96];
            char reqDisp[96];
            snprintf(reqBuf, sizeof(reqBuf), "After: %s", quests[q->prereq].name);
            trimCopy(reqBuf, reqDisp, sizeof(reqDisp), 26);
            DrawText(reqDisp, (int)r.x+14, (int)r.y+55, 12, unlocked ? C_TEXT_DIM : C_RED);
        } else {
            DrawText("Start quest", (int)r.x+14, (int)r.y+55, 12, C_GREEN);
        }

        Rectangle doneBtn = {(float)(r.x + r.width - 86), (float)(r.y + 10), 72, 24};
        bool doneHov = mOver(doneBtn) && hov;
        const char* doneTxt = q->done ? "Undo" : "Finish";
        Color doneBg = q->done ? (Color){35,12,12,255} : (unlocked ? (Color){12,35,18,255} : (Color){30,30,30,255});
        Color doneFg = q->done ? C_RED : (unlocked ? C_GREEN : C_TEXT_DARK);
        drawButton(doneBtn, doneTxt, doneBg, doneFg, doneHov);
        if(doneHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(setQuestDoneState(qi, !q->done)){
                showNotif(q->done ? "Chain-Quest completed" : "Chain-Quest reactivated");
            }
        }

        Rectangle viewBtn = {(float)(r.x + 14), (float)(r.y + r.height - 28), 58, 18};
        bool viewHov = mOver(viewBtn) && hov;
        drawButton(viewBtn, "View", C_PANEL, C_BORDER2, viewHov);
        if(viewHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) viewQuest = qi;

        Rectangle linkBtn = {(float)(r.x + 80), (float)(r.y + r.height - 28), 64, 18};
        bool linkHov = mOver(linkBtn) && hov;
        drawButton(linkBtn, "Link From", C_PANEL, chainLinkSource == qi ? C_GOLD : C_BORDER2, linkHov);
        if(linkHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            chainLinkSource = qi;
            showNotif("Source selected");
        }

        Rectangle nextBtn = {(float)(r.x + 152), (float)(r.y + r.height - 28), 56, 18};
        bool nextHov = mOver(nextBtn) && hov;
        drawButton(nextBtn, "Set Next", C_PANEL, C_BORDER2, nextHov);
        if(nextHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(chainLinkSource < 0){
                showNotif("Select Link From first");
            } else if(!canLinkChainQuest(chainLinkSource, qi)){
                showNotif("Invalid chain link");
            } else {
                quests[qi].prereq = chainLinkSource;
                if(!isChainQuestUnlocked(qi)) quests[qi].done = false;
                saveData();
                showNotif("Chain connection created");
                chainLinkSource = -1;
            }
        }

        Rectangle rootBtn = {(float)(r.x + r.width - 72), (float)(r.y + r.height - 28), 58, 18};
        bool rootHov = mOver(rootBtn) && hov;
        drawButton(rootBtn, "Unlink", C_PANEL, C_RED, rootHov);
        if(rootHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            q->prereq = -1;
            saveData();
            showNotif("Connection removed");
        }
    }

    Rectangle addBtn={(float)panelX,(float)(SH-68),(float)panelW,44};
    bool addHov = mOver(addBtn)&&!showCreate&&viewQuest<0;
    DrawRectangleRounded(addBtn,0.2f,6,(Color){25,18,50,255});
    DrawRectangleRoundedLines(addBtn,0.2f,6, addHov?C_GOLD:C_BORDER2);
    const char* addLabel = "Create new Chain-Quest";
    int lw = MeasureText(addLabel,16);
    DrawText(addLabel,(int)(addBtn.x+addBtn.width/2-lw/2),(int)(addBtn.y+14),16,addHov?C_GOLD_PALE:C_TEXT_DIM);

    if(addHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        showCreate = true;
        focusField = 0;
        memset(mName,0,MAX_NAME);
        memset(mDesc,0,MAX_DESC);
        memset(mCP,0,MAX_NUM);
        memset(mXP,0,MAX_NUM);
        modalScale = 0;
    }
}

static void drawComingSoon(const char* tabName){
    int cx = 0;
    int cy = TOPBAR_H + TABAREA_H;
    int cw = SW;
    int ch = SH - cy;

    for(int xx=20; xx<SW; xx+=60) DrawLine(xx,cy,xx,SH,(Color){30,22,60,40});
    for(int yy=cy; yy<SH; yy+=60) DrawLine(20,yy,SW-20,yy,(Color){30,22,60,40});

    int w=480,h=220;
    Rectangle card={(float)(cx+(cw-w)/2),(float)(cy+(ch-h)/2),(float)w,(float)h};
    DrawRectangleRounded(card,0.12f,8,(Color){18,14,35,255});
    DrawRectangleRoundedLines(card,0.12f,8,C_BORDER2);

    for(int r=4;r>0;r--){
        DrawRectangleRoundedLines(
            (Rectangle){card.x-r*2,card.y-r*2,card.width+r*4,card.height+r*4},
            0.12f,8,(Color){C_PURPLE.r,C_PURPLE.g,C_PURPLE.b,(unsigned char)(30-r*6)});
    }

    int tw=MeasureText(tabName,26);
    DrawText(tabName,(int)(card.x+(w-tw)/2),(int)(card.y+44),26,C_GOLD);

    const char* sub="Coming Soon  . . .";
    int sw2=MeasureText(sub,20);
    DrawText(sub,(int)(card.x+(w-sw2)/2),(int)(card.y+100),20,C_TEXT_DIM);

    const char* hint="This feature is being developed.";
    int hw=MeasureText(hint,14);
    DrawText(hint,(int)(card.x+(w-hw)/2),(int)(card.y+140),14,(Color){70,60,50,255});
}

static void createQuestFromModal(void){
    if(strlen(mName)==0){
        showNotif("Enter Quest-Name!");
        return;
    }
    if(questCount >= MAX_QUESTS) return;

    Quest* nq=&quests[questCount++];
    memset(nq,0,sizeof(Quest));
    strncpy(nq->name,mName,MAX_NAME-1);
    strncpy(nq->desc,mDesc,MAX_DESC-1);
    nq->cp = atoi(mCP);
    nq->xp = atoi(mXP);
    nq->done = false;
    nq->type = questTypeForActiveTab();
    nq->prereq = -1;
    recalc();
    saveData();
    showCreate=false;
    showNotif("Quest created successfully");
}

static void drawCreateModal(float dt){
    modalScale += dt*6.0f;
    if(modalScale>1.0f) modalScale=1.0f;
    float s=modalScale;
    s=s*s*(3-2*s);

    DrawRectangle(0,0,SW,SH,C_OVERLAY);

    int mw=580, mh=520;
    int mx=(SW-mw)/2, my=(SH-mh)/2;

    float hw=(mw*s)/2, hh=(mh*s)/2;
    int rx=(int)((SW/2)-hw), ry=(int)((SH/2)-hh);
    int rw=(int)(mw*s), rh=(int)(mh*s);
    if(rw<10||rh<10) return;

    Rectangle modal={(float)rx,(float)ry,(float)rw,(float)rh};
    DrawRectangleRounded(modal,0.06f,8,(Color){15,11,32,255});
    DrawRectangleRoundedLines(modal,0.06f,8,C_BORDER2);

    DrawRectangleRoundedLines(
        (Rectangle){modal.x-2,modal.y-2,modal.width+4,modal.height+4},
        0.06f,8,(Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,60});

    if(s<0.85f) return;

    DrawRectangleRounded((Rectangle){(float)mx,(float)my,(float)mw,52},0.06f,6,(Color){25,18,50,255});
    const char* ttl = (activeTab==1) ? "Create new Story-Quest" :
                      (activeTab==2) ? "Create new Chain-Quest" :
                                       "Create Quest";
    DrawText(ttl, mx+22, my+14, 20, C_GOLD_PALE);

    Rectangle closeBtn={(float)(mx+mw-44),(float)(my+8),34,34};
    bool cHov=mOver(closeBtn);
    DrawRectangleRounded(closeBtn,0.3f,4,cHov?(Color){60,20,20,255}:C_PANEL2);
    DrawText("X",(int)closeBtn.x+9,(int)closeBtn.y+7,18,C_RED);
    if(cHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) showCreate=false;

    int fy = my+78;

    Rectangle nameR={(float)(mx+22),(float)(fy+18),(float)(mw-44),38};
    if(mOver(nameR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focusField=0;
    drawInputBox(nameR,"Quest-Name",mName,focusField==0,C_BORDER);
    if(focusField==0) typeInto(mName,MAX_NAME,false,dt);

    fy += 80;

    Rectangle descR={(float)(mx+22),(float)(fy+18),(float)(mw-44),70};
    if(mOver(descR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focusField=1;
    drawInputBox(descR,"Description",mDesc,focusField==1,C_BORDER);
    if(focusField==1) typeInto(mDesc,MAX_DESC,false,dt);

    fy += 110;

    Rectangle cpR={(float)(mx+22),(float)(fy+18),(float)(mw/2-30),38};
    if(mOver(cpR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focusField=2;
    drawInputBox(cpR,"CP-Reward",mCP,focusField==2,C_BORDER);
    if(focusField==2) typeInto(mCP,MAX_NUM,true,dt);

    Rectangle xpR={(float)(mx+mw/2+8),(float)(fy+18),(float)(mw/2-30),38};
    if(mOver(xpR)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) focusField=3;
    drawInputBox(xpR,"XP-Reward",mXP,focusField==3,C_BORDER);
    if(focusField==3) typeInto(mXP,MAX_NUM,true,dt);

    if(activeTab==2){
        DrawText("Connections are created afterwards in the Chain-Quest view.", mx+22, fy+72, 14, C_TEXT_DIM);
    }

    if(IsKeyPressed(KEY_TAB)) focusField=(focusField+1)%4;
    if(IsKeyPressed(KEY_ESCAPE)) showCreate=false;

    Rectangle okBtn={(float)(mx+22),(float)(my+mh-62),(float)(mw-44)/2-8,44};
    Rectangle cnBtn={(float)(mx+22+(mw-44)/2+8),(float)(my+mh-62),(float)(mw-44)/2-8,44};

    bool okHov=mOver(okBtn);
    bool cnHov=mOver(cnBtn);
    drawButton(okBtn,"Create Quest",(Color){20,45,20,255},C_GREEN,okHov);
    drawButton(cnBtn,"Cancel",(Color){40,10,10,255},C_RED,cnHov);

    if(cnHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) showCreate=false;
    if(okHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) createQuestFromModal();

    if(IsKeyPressed(KEY_ENTER)&&focusField!=1) createQuestFromModal();
}

static void drawDescModal(void){
    if(viewQuest<0||viewQuest>=questCount) return;
    Quest* q=&quests[viewQuest];

    DrawRectangle(0,0,SW,SH,C_OVERLAY);
    int mw=560, mh=420;
    Rectangle modal={(float)((SW-mw)/2),(float)((SH-mh)/2),(float)mw,(float)mh};
    DrawRectangleRounded(modal,0.07f,8,(Color){14,11,30,255});
    DrawRectangleRoundedLines(modal,0.07f,8,C_GOLD_DIM);
    DrawRectangleRoundedLines(
        (Rectangle){modal.x-2,modal.y-2,modal.width+4,modal.height+4},
        0.07f,8,(Color){C_GOLD.r,C_GOLD.g,C_GOLD.b,40});

    int mx=(SW-mw)/2, my=(SH-mh)/2;

    DrawRectangleRounded((Rectangle){(float)mx,(float)my,(float)mw,54},0.07f,6,(Color){22,17,48,255});
    DrawText(q->name, mx+20, my+15, 20, q->done?C_TEXT_DIM:C_GOLD_PALE);

    Color sc=q->done?C_GREEN:C_RED;
    const char* sl=q->done?"Finished":"Active";
    DrawText(sl, mx+mw-MeasureText(sl,14)-20, my+19, 14, sc);

    drawBadge(mx+20,my+68,"CP",q->cp,C_CP_DIM,C_CP);
    drawBadge(mx+130,my+68,"XP",q->xp,C_XP_DIM,C_XP);

    DrawLine(mx+20,my+104,mx+mw-20,my+104,C_BORDER);
    DrawText("Description:",mx+20,my+116,13,C_TEXT_DIM);
    if(strlen(q->desc)==0) DrawText("No existing description",mx+20,my+136,15,C_TEXT_DARK);
    else drawTextWrapped(q->desc,mx+20,my+136,15,mw-50,C_TEXT);

    if(q->type == 2){
        const char* unlockLbl = "Unlock rule:";
        DrawText(unlockLbl, mx+20, my+250, 13, C_TEXT_DIM);
        if(q->prereq >= 0 && isValidQuestIndex(q->prereq)){
            char rule[160];
            snprintf(rule, sizeof(rule), "Can be completed after '%s' is finished.", quests[q->prereq].name);
            drawTextWrapped(rule, mx+20, my+268, 15, mw-50, isChainQuestUnlocked(viewQuest) ? C_TEXT : C_RED);
        } else {
            DrawText("This is a starting quest without prerequisite.", mx+20, my+268, 15, C_GREEN);
        }
    }

    Rectangle tb={(float)(mx+20),(float)(my+mh-66),(float)(mw/2-28),44};
    bool tHov=mOver(tb);
    Color tbg=q->done?(Color){35,12,12,255}:(Color){12,35,18,255};
    Color tfg=q->done?C_RED:(q->type == 2 && !isChainQuestUnlocked(viewQuest) ? C_TEXT_DARK : C_GREEN);
    const char* tl=q->done?"Mark as unfinished":"Mark as finished";
    drawButton(tb,tl,tbg,tfg,tHov);
    if(tHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        if(setQuestDoneState(viewQuest,!q->done)){
            showNotif(q->done?"Quest finished!":"Quest reactivated.");
        }
    }

    Rectangle cb={(float)(mx+mw/2+8),(float)(my+mh-66),(float)(mw/2-28),44};
    bool cHov=mOver(cb);
    drawButton(cb,"Close",(Color){30,22,55,255},C_BORDER2,cHov);
    if(cHov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) viewQuest=-1;
    if(IsKeyPressed(KEY_ESCAPE)) viewQuest=-1;
}

static void drawNotif(float dt){
    if(notifTimer<=0) return;

    notifTimer-=dt;
    float alpha=(notifTimer>0.4f)?1.0f:(notifTimer/0.4f);
    float slide=(notifTimer>2.2f)?(2.5f-notifTimer)/0.3f:1.0f;
    if(slide<0)slide=0;
    if(slide>1)slide=1;

    int nw=MeasureText(notifMsg,16)+30;
    int nx=SW-nw-20;
    int ny=(int)(SH-60-(slide*50));

    DrawRectangleRounded((Rectangle){(float)nx,(float)ny,(float)nw,38},0.35f,6,
        (Color){C_NOTIF.r,C_NOTIF.g,C_NOTIF.b,(unsigned char)(180*alpha)});
    DrawText(notifMsg,nx+15,ny+10,16,(Color){255,255,255,(unsigned char)(255*alpha)});
}

int main(void){
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_HIGHDPI);
    InitWindow(SW, SH, "RPG Quest Book");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    loadData();

    while(!WindowShouldClose()){
        float dt=GetFrameTime();

        BeginDrawing();
        ClearBackground(C_BG);

        for(int xx=0;xx<SW;xx+=50)
            DrawLine(xx,TOPBAR_H,xx,SH,(Color){20,15,45,30});
        for(int yy=TOPBAR_H;yy<SH;yy+=50)
            DrawLine(0,yy,SW,yy,(Color){20,15,45,30});

        drawTopBar();
        drawTabCards();

        switch(activeTab){
            case 0: drawQuestList(0); break;
            case 1: drawQuestList(1); break;
            case 2: drawChainQuests(); break;
            case 3: drawComingSoon("Tree-Quests");  break;
        }

        if(showCreate)   drawCreateModal(dt);
        if(viewQuest>=0) drawDescModal();
        drawNotif(dt);

        if(IsKeyPressed(KEY_ESCAPE)){
            if(showCreate) showCreate=false;
            else if(viewQuest>=0) viewQuest=-1;
            else if(chainLinkSource>=0) chainLinkSource=-1;
            else break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
