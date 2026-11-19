#include "core/johnnyos/JohnnyApps.h"

#if defined(JOHNNYOS_STICKS3)
#include "core/main_menu.h"
#include "core/display.h"
#include "core/utils.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <globals.h>
#include <vector>

namespace {
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return tft.color565(r,g,b); }
const uint16_t W = 0xffff;
void drain() { check(NextPress); check(PrevPress); check(SelPress); check(EscPress); }
void frame(const char *heading, const char *hint) {
    johnnyDrawWallpaper();
    tft.setTextColor(W);
    tft.setTextSize(1);
    tft.drawString(heading, 9, 7, 2);
    tft.drawFastHLine(9, 25, tftWidth - 18, rgb(72,94,144));
    tft.fillRoundRect(5, tftHeight - 16, tftWidth - 10, 13, 5, rgb(20,33,66));
    tft.setTextColor(rgb(186,207,240));
    tft.drawCentreString(hint, tftWidth/2, tftHeight-13, 1);
}
void alert(const String &message) {
    frame("JohnnyOS", "Hold Key 2 to return");
    tft.setTextColor(W);
    tft.drawCentreString(message.substring(0,37),120,68,1);
    drain();
    while (!check(EscPress)) delay(25);
    drain();
}
String trimmed(const String &raw, int maxLen) {
    return raw.length() > (unsigned)maxLen ? raw.substring(0,maxLen) : raw;
}
}

void johnnyCalculator() {
    // Classic 4x5 keypad: two taps navigate, hold Key 1 activates.
    const char* keys[] = {
        "7","8","9","/","4","5","6","*",
        "1","2","3","-","C","0",".","+",
        "DEL","+/-","=","BACK"
    };
    int sel = 0, old = -1;
    String number="0", left=""; char op=0;
    bool newNumber=false, full=true;
    auto paintKey=[&](int i) {
        int x=10+(i%4)*55, y=49+(i/4)*14;
        tft.fillRoundRect(x,y,51,13,4,i==sel?rgb(70,116,210):rgb(28,43,79));
        tft.setTextColor(i==sel?W:rgb(188,205,244));
        tft.drawCentreString(keys[i],x+25,y+2,1);
    };
    drain();
    while(true){
        if(check(EscPress))break;
        if(check(NextPress))sel=(sel+1)%20;
        if(check(PrevPress))sel=(sel+19)%20;
        if(check(SelPress)){
            String k=keys[sel];
            if(k=="BACK") break;
            if(k=="C"){number="0";left="";op=0;newNumber=false;}
            else if(k=="DEL"){number=number.length()>1?number.substring(0,number.length()-1):"0";}
            else if(k=="+/-"){if(number!="0")number=number.startsWith("-")?number.substring(1):"-"+number;}
            else if(k=="+"||k=="-"||k=="*"||k=="/"){left=number;op=k[0];newNumber=true;}
            else if(k=="="){
                if(op){
                    double a=left.toDouble(), b=number.toDouble(), v=0;
                    if(op=='+')v=a+b;else if(op=='-')v=a-b;else if(op=='*')v=a*b;
                    else if(b!=0)v=a/b;
                    if(op=='/'&&b==0) number="Error";
                    else number=String(v,4);
                    if(number.indexOf('.')>=0){while(number.endsWith("0"))number.remove(number.length()-1);if(number.endsWith("."))number.remove(number.length()-1);}
                    op=0;left="";newNumber=true;
                }
            }else if(k=="."){if(newNumber){number="0";newNumber=false;}if(number.indexOf('.')<0)number+=".";}
            else {
                if(newNumber||number=="0"||number=="Error"){number=k;newNumber=false;}
                else if(number.length()<14)number+=k;
            }
            full=true;
        }
        if(full){
            full=false;frame("Calculator","Tap: key  Hold 1: press  Hold 2: exit");
            tft.fillRoundRect(9,30,222,16,5,rgb(13,25,49));
            tft.setTextColor(W);tft.drawRightString(trimmed(number,17),223,33,2);
            for(int i=0;i<20;++i)paintKey(i);
        }else if(sel!=old){if(old>=0)paintKey(old);paintKey(sel);}
        old=sel;
        delay(20);
    }
    drain();
}

void johnnyFilesPlus() {
    // Read-only LittleFS explorer. Bruce Files still handles SD and writing.
    struct Entry {String name;size_t bytes;bool directory;};
    std::vector<Entry> files;
    File root=LittleFS.open("/");
    if(root && root.isDirectory()){
        File file=root.openNextFile();
        while(file && files.size()<64){
            files.push_back({String(file.name()), (size_t)file.size(), file.isDirectory()});
            file=root.openNextFile();
        }
    }
    int sel=0,old=-1;bool full=true;
    auto draw=[&](int j){
        int i=sel-(sel%4)+j;
        int y=31+j*20;
        bool active=i==sel;
        tft.fillRoundRect(9,y,222,18,6,active?rgb(69,103,188):rgb(25,38,70));
        if(i>=0&&i<(int)files.size()){
            tft.setTextColor(active?W:rgb(179,202,239));
            tft.drawString(trimmed(files[i].name,24),15,y+5,1);
            tft.drawRightString(files[i].directory?"DIR":String(files[i].bytes)+" B",223,y+5,1);
        }
    };
    drain();
    while(true){
        if(check(EscPress))break;
        if(check(NextPress)&&!files.empty())sel=(sel+1)%files.size();
        if(check(PrevPress)&&!files.empty())sel=(sel+files.size()-1)%files.size();
        if(check(SelPress)&&!files.empty()){
            File file=LittleFS.open(files[sel].name,"r");
            if(file && !file.isDirectory()) {
                frame("File Preview","Hold Key 2 to go back");
                tft.setTextColor(rgb(191,214,246));
                tft.drawString(trimmed(files[sel].name,32),10,34,1);
                String line;
                for(int row=0;row<5&&file.available();++row){
                    line=file.readStringUntil('\n');
                    tft.drawString(trimmed(line,35),10,52+row*12,1);
                }
                file.close();drain();while(!check(EscPress))delay(25);drain();full=true;
            } else alert("Directory preview unavailable");
            full=true;
        }
        if(full||sel/4!=old/4){
            full=false;frame("Files +","Read-only LittleFS browser");
            if(files.empty()){tft.drawCentreString("No files found",120,63,2);}
            else for(int j=0;j<4;++j)draw(j);
        }else if(sel!=old){draw(old%4);draw(sel%4);}
        old=sel;delay(25);
    }
    drain();
}

void johnnyPerformance() {
    drain();int lastHeap=-1,lastBat=-1;uint32_t next=0;
    frame("Performance","Live readings  Hold Key 2: back");
    while(!check(EscPress)){
        uint32_t now=millis();
        if(now>=next){
            next=now+1000;
            int heap=ESP.getFreeHeap()/1024,bat=getBattery();
            if(heap!=lastHeap||bat!=lastBat){
                tft.fillRoundRect(10,33,220,80,10,rgb(24,37,70));
                tft.setTextColor(W);
                tft.drawString("Battery",19,40,1);
                tft.drawRightString(String(bat)+"%",216,40,1);
                tft.drawString("Free heap",19,57,1);
                tft.drawRightString(String(heap)+" KB",216,57,1);
                tft.drawString("PSRAM",19,74,1);
                tft.drawRightString(String(ESP.getFreePsram()/1024)+" KB",216,74,1);
                tft.drawString("Wi-Fi",19,91,1);
                tft.drawRightString(WiFi.isConnected()?"Connected":"Offline",216,91,1);
            }
            lastHeap=heap;lastBat=bat;
        }
        delay(50);
    }
    drain();
}

void johnnyHomeLayout() {
    // Reorder all visible apps without remapping safety actions.
    std::vector<String> names;
    auto items=mainMenu.getItems();
    String oldOrder=johnnyHomeOrder();
    int at=0;
    while(at<(int)oldOrder.length()){
        int stop=oldOrder.indexOf('|',at);
        if(stop<0)stop=oldOrder.length();
        String item=oldOrder.substring(at,stop);
        if(item.length())names.push_back(item);
        at=stop+1;
    }
    for(auto* item:items)if(item){
        String name=item->getName();bool found=false;
        for(auto &n:names)if(n==name){found=true;break;}
        if(!found)names.push_back(name);
    }
    if(names.empty())return;
    int sel=0,old=-1,lastCommand=-1;bool full=true;
    enum Action { BROWSE, EDIT }; Action action=BROWSE;
    int command=0;
    auto row=[&](int j){
        int i=(sel/4)*4+j,y=30+j*20;
        tft.fillRoundRect(8,y,224,18,6,i==sel?rgb(65,100,189):rgb(23,39,75));
        if(i<(int)names.size()){
            tft.setTextColor(W);
            tft.drawString(trimmed(names[i],25),18,y+5,1);
            if(names[i]==johnnyFavorite())tft.drawRightString("PIN",221,y+5,1);
        }
    };
    drain();
    while(true){
        if(check(EscPress)){if(action==EDIT){action=BROWSE;full=true;}else break;}
        if(action==BROWSE){
            if(check(NextPress))sel=(sel+1)%names.size();
            if(check(PrevPress))sel=(sel+names.size()-1)%names.size();
            if(check(SelPress)){action=EDIT;command=0;full=true;}
        } else {
            if(check(NextPress))command=(command+1)%4;
            if(check(PrevPress))command=(command+3)%4;
            if(check(SelPress)){
                if(command==0)johnnySetFavorite(names[sel]);
                else if(command==1&&sel>0){String t=names[sel-1];names[sel-1]=names[sel];names[sel]=t;sel--;}
                else if(command==2&&sel+1<(int)names.size()){String t=names[sel+1];names[sel+1]=names[sel];names[sel]=t;sel++;}
                if(command==3){action=BROWSE;full=true;}
                else {
                    String out;for(auto &n:names){out+=n;out+='|';}
                    johnnySetHomeOrder(out);action=BROWSE;full=true;
                }
            }
        }
        if(full){
            full=false;
            frame("Home Layout",action==BROWSE?"Tap: app  Hold 1: edit  Hold 2: exit":"Tap: action  Hold 1: do  Hold 2: back");
            if(action==BROWSE)for(int j=0;j<4;++j)row(j);
        }
        if(action==EDIT && (full || command!=lastCommand)){
            const char* actions[]={"Pin as favorite","Move earlier","Move later","Done"};
            tft.fillRoundRect(8,29,224,82,9,rgb(23,39,74));
            tft.setTextColor(W);
            tft.drawCentreString(trimmed(names[sel],29),120,35,1);
            for(int j=0;j<4;++j){
                int y=50+j*15;
                tft.fillRoundRect(14,y,211,14,4,j==command?rgb(69,112,212):rgb(26,43,79));
                tft.setTextColor(W);tft.drawString(actions[j],22,y+3,1);
            }
        }else if(sel!=old && old>=0){if(sel/4!=old/4){full=true;}else{row(old%4);row(sel%4);}}
        old=sel;lastCommand=command;delay(25);
    }
    drain();
}

// 2048 is a real local puzzle game. Two taps choose movement direction;
// holding Select slides the tiles and spawns a 2 or 4.
void johnnyPuzzle2048() {
    int board[16]={},score=0,direction=0;bool redraw=true;
    auto spawn=[&](){
        int freeCells[16],n=0;
        for(int i=0;i<16;++i)if(board[i]==0)freeCells[n++]=i;
        if(n)board[freeCells[random(n)]]=random(10)?2:4;
        return n>0;
    };
    auto apply=[&](){
        bool moved=false;
        for(int line=0;line<4;++line){
            int old[4],out[4]={},n=0;
            for(int p=0;p<4;++p){
                int x=direction==0?3-p:direction==1?p:line;
                int y=direction==2?3-p:direction==3?p:line;
                if(direction<2)y=line;else x=line;
                old[p]=board[y*4+x];
                if(old[p]){
                    if(n>0&&out[n-1]==old[p]&&out[n-1]>0) {
                        out[n-1]*=2;score+=out[n-1];
                        // Mark a merge for this turn by preventing a second
                        // equal tile from merging into the same position.
                        out[n-1]=-out[n-1];
                    }else out[n++]=old[p];
                }
            }
            for(int p=0;p<4;++p){
                int x=direction==0?3-p:direction==1?p:line;
                int y=direction==2?3-p:direction==3?p:line;
                if(direction<2)y=line;else x=line;
                int value=abs(out[p]);
                if(board[y*4+x]!=value)moved=true;
                board[y*4+x]=value;
            }
        }
        if(moved)spawn();
    };
    drain();spawn();spawn();
    while(true){
        if(check(EscPress))break;
        if(check(NextPress)){direction=(direction+1)%4;redraw=true;}
        if(check(PrevPress)){direction=(direction+3)%4;redraw=true;}
        if(check(SelPress)){apply();redraw=true;}
        if(redraw){
            redraw=false;frame("2048 Puzzle","Tap: direction  Hold 1: slide  Hold 2: exit");
            const char* arrows[]={"RIGHT","LEFT","DOWN","UP"};
            tft.setTextColor(W);tft.drawString(String(score)+" pts",153,11,1);
            for(int j=0;j<4;++j)for(int i=0;i<4;++i){
                int val=board[j*4+i],x=24+i*49,y=29+j*21;
                tft.fillRoundRect(x,y,45,19,5,val?rgb(54+min(val,64),86,154):rgb(26,40,73));
                if(val){tft.setTextColor(W);tft.drawCentreString(String(val),x+22,y+5,1);}
            }
            tft.setTextColor(rgb(193,220,255));tft.drawCentreString(arrows[direction],120,114,1);
        }
        delay(25);
    }
    drain();
}

void johnnyQuickGame() {
    // Memory mini-game: repeat a sequence of Left/Right button taps.
    int round=1,pos=0,sequence[12]={};bool active=true;
    for(int i=0;i<12;++i)sequence[i]=random(2);
    drain();
    while(active) {
        frame("Memory Duel","Tap 1: Right  Tap 2: Left  Hold 2: exit");
        tft.setTextColor(W);
        tft.drawCentreString("ROUND "+String(round),120,37,2);
        for(int i=0;i<round;i++){
            tft.fillRoundRect(31+i*14,68,12,18,4,
                              sequence[i]?rgb(79,154,244):rgb(112,225,166));
            delay(320);
            tft.fillRoundRect(31+i*14,68,12,18,4,rgb(28,44,78));
            delay(130);
        }
        pos=0;drain();
        while(pos<round){
            if(check(EscPress)){active=false;break;}
            int val=-1;
            if(check(NextPress))val=1;
            if(check(PrevPress))val=0;
            if(val>=0){
                if(val!=sequence[pos]){active=false;break;}
                pos++;
                tft.fillRoundRect(40,94,160,13,5,rgb(45,95,147));
                tft.drawCentreString("Matched "+String(pos)+"/"+String(round),120,96,1);
            }
            delay(20);
        }
        if(!active)break;
        round++;
        if(round>10)break;
        delay(400);
    }
    frame("Memory Duel","Hold Key 2: exit");
    tft.setTextColor(W);tft.drawCentreString("Reached round "+String(round),120,66,2);
    drain();while(!check(EscPress))delay(25);drain();
}
#endif
