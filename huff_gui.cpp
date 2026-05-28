#include <windows.h>
#include <map>
#include <queue>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
using namespace std;

// HNode is the building block of the Huffman Tree.
// Each node stores a character and its frequency.
struct HNode {
    char ch; int freq;
    HNode *left, *right;
    HNode(char c,int f,HNode*l=NULL,HNode*r=NULL):ch(c),freq(f),left(l),right(r){}
};

// Cmp is a comparator that makes the priority queue work as a Min-Heap 
// — lowest frequency comes first.
struct Cmp { bool operator()(HNode*a,HNode*b){return a->freq>b->freq;} };

//Global variables for window, drawing, and Huffman data
HWND hwnd; HDC memDC; HBITMAP memBmp;
int W , H ;

HNode* treeRoot=NULL;
map<char,string> huffCode;
map<char,int>    freqMap;
string inputText="", encodedStr="";
int origBits=0, compBits=0;
bool built=false, inputFocused=false;
string statusMsg="Type your text in the input box below, then click BUILD";

//encode() function
// This is a recursive function that traverses the tree.
// When it reaches a leaf node it assigns the binary code formed by the path taken (0s and 1s).
void encode(HNode* root,string s,map<char,string>&hc){
    if(!root)return;
    if(!root->left&&!root->right)hc[root->ch]=s;
    encode(root->left,s+"0",hc);
    encode(root->right,s+"1",hc);
}

//buildHuffman() function -  This is the heart of the program. It runs the complete Huffman algorithm —
// frequency count → priority queue → tree building → code generation → encoding.

// This function builds the Huffman Tree and generates the codes based on the input text.
void buildHuffman(){
    if(inputText.empty()){statusMsg="Please enter some text first!";return;}
    freqMap.clear();huffCode.clear();encodedStr="";

    // Step 1: Count frequencies
    for(char c:inputText)freqMap[c]++;

    // Step 2: Insert all characters into Min-Heap
    priority_queue<HNode*,vector<HNode*>,Cmp>pq;
    for(auto&p:freqMap)pq.push(new HNode(p.first,p.second));
    if(pq.size()==1){HNode*o=pq.top();pq.pop();pq.push(new HNode('\0',o->freq,o,NULL));}
    
    // Step 3: Merge two lowest nodes repeatedly
    while(pq.size()>1){
        HNode*l=pq.top();pq.pop(); // Lowest freq
        HNode*r=pq.top();pq.pop(); // Second lowest freq
        pq.push(new HNode('\0',l->freq+r->freq,l,r));
    }

    // Step 4: Root of tree
    treeRoot=pq.top();


    // Step 5: Generate codes
    encode(treeRoot,"",huffCode);

    // Step 6: Encode the input text
    for(char c:inputText)encodedStr+=huffCode[c];
    origBits=inputText.length()*8;
    compBits=encodedStr.length();
    built=true;
    ostringstream oss;
    oss<<"Built! Original:"<<origBits<<" bits  Compressed:"<<compBits<<" bits  Saved:"
       <<fixed<<setprecision(1)<<(100.0f-(float)compBits/origBits*100)<<"%";
    statusMsg=oss.str();
}

// Drawing Helper Functions

// Fills a rectangle with a solid color. 
// Used to draw backgrounds, panels, rows.
void fillR(HDC dc,int x,int y,int w,int h,COLORREF c){
    HBRUSH b=CreateSolidBrush(c);RECT r={x,y,x+w,y+h};FillRect(dc,&r,b);DeleteObject(b);
}

// Draws a rounded rectangle. Used for buttons and stat boxes.
void rrect(HDC dc,int x1,int y1,int x2,int y2,COLORREF fill,COLORREF border,int rn=10){
    HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,2,border);
    HBRUSH ob=(HBRUSH)SelectObject(dc,b);HPEN op=(HPEN)SelectObject(dc,p);
    RoundRect(dc,x1,y1,x2,y2,rn,rn);
    SelectObject(dc,ob);DeleteObject(b);SelectObject(dc,op);DeleteObject(p);
}

// Draws text at a given position with given color, size, and bold setting using Consolas font.
void txt(HDC dc,int x,int y,int w,int h,const string&s,COLORREF c,int sz,bool bold,DWORD fmt=DT_CENTER|DT_VCENTER|DT_SINGLELINE){
    SetTextColor(dc,c);SetBkMode(dc,TRANSPARENT);
    HFONT f=CreateFontA(sz,0,0,0,bold?FW_BOLD:FW_NORMAL,0,0,0,0,0,0,0,0,"Consolas");
    HFONT of=(HFONT)SelectObject(dc,f);
    RECT r={x,y,x+w,y+h};DrawTextA(dc,s.c_str(),-1,&r,fmt);
    SelectObject(dc,of);DeleteObject(f);
}

// Draws a colored button with white text using rrect and txt.
void btn(HDC dc,int x1,int y1,int x2,int y2,const string&lbl,COLORREF col){
    rrect(dc,x1,y1,x2,y2,col,RGB(255,255,255),12);
    txt(dc,x1,y1,x2-x1,y2-y1,lbl,RGB(255,255,255),15,true);
}

//drawTreeHelper() Function

//This function recursively draws the Huffman tree on screen.
void drawTreeHelper(HDC dc,HNode*node,int x,int y,int offset){
    if(!node)return;
    HPEN pen=CreatePen(PS_SOLID,2,RGB(100,100,150));
    HPEN op=(HPEN)SelectObject(dc,pen);
    if(node->left) {MoveToEx(dc,x,y+22,NULL);LineTo(dc,x-offset,y+68);}
    if(node->right){MoveToEx(dc,x,y+22,NULL);LineTo(dc,x+offset,y+68);}
    SelectObject(dc,op);DeleteObject(pen);
    if(node->left) {int mx=(x+x-offset)/2-8,my=(y+22+y+68)/2-8;txt(dc,mx,my,16,16,"0",RGB(255,200,100),12,true);}
    if(node->right){int mx=(x+x+offset)/2,  my=(y+22+y+68)/2-8;txt(dc,mx,my,16,16,"1",RGB(100,200,255),12,true);}
    if(node->left) drawTreeHelper(dc,node->left, x-offset,y+90,max(offset/2,18));
    if(node->right)drawTreeHelper(dc,node->right,x+offset,y+90,max(offset/2,18));
    bool leaf=(!node->left&&!node->right);
    COLORREF fill=leaf?RGB(0,160,100):RGB(40,90,190);
    COLORREF bord=leaf?RGB(0,255,140):RGB(100,160,255);
    HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,2,bord);
    HBRUSH ob=(HBRUSH)SelectObject(dc,b);HPEN op2=(HPEN)SelectObject(dc,p);
    Ellipse(dc,x-22,y-22,x+22,y+22);
    SelectObject(dc,ob);DeleteObject(b);SelectObject(dc,op2);DeleteObject(p);
    if(node->ch!='\0'){
        string top=node->ch==' '?"SP":string(1,node->ch);
        txt(dc,x-20,y-20,40,18,top,RGB(255,255,255),12,true);
        txt(dc,x-20,y+2, 40,16,to_string(node->freq),RGB(200,255,200),10,false);
    } else {
        txt(dc,x-20,y-10,40,20,to_string(node->freq),RGB(255,255,255),13,true);
    }
}

void render(){
    fillR(memDC,0,0,W,H,RGB(18,18,30));
    fillR(memDC,0,0,W,50,RGB(25,25,50));
    txt(memDC,0,0,W,50,"CompressIQ - The Huffman Coding Visualizer",RGB(255,255,255),22,true);
    btn(memDC, 20,57,155,94,"BUILD",    RGB(33,150,243));
    btn(memDC,165,57,300,94,"CLEAR",    RGB(229,57,53));
    btn(memDC,310,57,445,94,"DECODE",   RGB(0,150,136));
    fillR(memDC,0,97,W,30,RGB(28,28,52));
    txt(memDC,10,97,W-20,30,statusMsg,RGB(100,220,255),14,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    HPEN lp=CreatePen(PS_SOLID,1,RGB(50,50,80));
    HPEN olp=(HPEN)SelectObject(memDC,lp);
    MoveToEx(memDC,0,128,NULL);LineTo(memDC,W,128);
    SelectObject(memDC,olp);DeleteObject(lp);

    if(!built){
        txt(memDC,0,300,W,40,"Enter text below and click BUILD to visualize Huffman Tree",RGB(70,70,110),18,false);
        txt(memDC,0,350,W,30,"Huffman Coding is used in ZIP, JPEG, MP3 file compression",RGB(50,50,80),15,false);
    } else {
        int tW=(int)(W*0.60);
        txt(memDC,0,130,tW,24,"HUFFMAN TREE  (green=leaf, blue=internal, 0=left, 1=right)",RGB(140,140,190),13,false,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(treeRoot)drawTreeHelper(memDC,treeRoot,tW/2,165,min(tW/4,170));
        int px=tW+8,pw=W-px-8;
        fillR(memDC,px,128,pw,24,RGB(30,30,55));
        txt(memDC,px,128,pw,24,"CODE TABLE",RGB(255,200,100),14,true);
        int ty=154;
        fillR(memDC,px,ty,pw,20,RGB(40,40,75));
        txt(memDC,px,   ty,45,20,"CH",  RGB(200,200,255),12,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        txt(memDC,px+45,ty,55,20,"FREQ",RGB(200,200,255),12,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        txt(memDC,px+100,ty,pw-140,20,"CODE",RGB(200,200,255),12,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        txt(memDC,px+pw-38,ty,36,20,"BITS",RGB(200,200,255),12,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        ty+=20;
        int row=0;
        for(auto&p:freqMap){
            if(ty>H-200)break;
            fillR(memDC,px,ty,pw,19,(row%2==0)?RGB(26,26,46):RGB(33,33,55));
            string ch=p.first==' '?"SP":string(1,p.first);
            txt(memDC,px,   ty,45,19,ch,                       RGB(0,230,118),12,true, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            txt(memDC,px+45,ty,55,19,to_string(p.second),      RGB(255,255,255),12,false,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            txt(memDC,px+100,ty,pw-140,19,huffCode[p.first],   RGB(255,200,100),12,false,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            txt(memDC,px+pw-38,ty,36,19,to_string(huffCode[p.first].length()),RGB(180,180,180),12,false,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            ty+=19;row++;
        }
        int sy=ty+8;
        if(sy<H-195){
            rrect(memDC,px,sy,px+pw,sy+125,RGB(20,38,20),RGB(0,160,70),8);
            txt(memDC,px,sy,pw,22,"COMPRESSION STATS",RGB(0,255,120),13,true);
            txt(memDC,px+5,sy+24,pw-10,18,"Original:   "+to_string(origBits)+" bits",RGB(200,200,200),12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            txt(memDC,px+5,sy+42,pw-10,18,"Compressed: "+to_string(compBits)+" bits",RGB(200,200,200),12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            ostringstream rs,ss;
            rs<<fixed<<setprecision(1)<<((float)compBits/origBits*100)<<"%";
            ss<<fixed<<setprecision(1)<<(100.0f-(float)compBits/origBits*100)<<"%";
            txt(memDC,px+5,sy+60,pw-10,18,"Ratio:      "+rs.str(),RGB(255,200,100),12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            txt(memDC,px+5,sy+78,pw-10,18,"Saved:      "+ss.str(),RGB(0,255,120),12,true,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            int bw=pw-20,bY=sy+100;
            fillR(memDC,px+5,bY,bw,14,RGB(50,50,50));
            int filled=(int)((float)compBits/origBits*bw);
            fillR(memDC,px+5,bY,filled,14,RGB(33,150,243));
            fillR(memDC,px+5+filled,bY,bw-filled,14,RGB(0,200,80));
        }
        // Encoded string
        txt(memDC,20,H-118,220,18,"ENCODED OUTPUT:",RGB(140,140,190),12,true,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        fillR(memDC,20,H-100,W-40,20,RGB(25,25,45));
        string enc=encodedStr.length()>140?encodedStr.substr(0,140)+"...":encodedStr;
        txt(memDC,22,H-100,W-44,20,enc,RGB(255,200,100),12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }

    // Input area
    fillR(memDC,0,H-68,W,68,RGB(20,20,40));
    HPEN ip=CreatePen(PS_SOLID,2,inputFocused?RGB(33,150,243):RGB(55,55,85));
    HPEN oip=(HPEN)SelectObject(memDC,ip);
    HBRUSH ib=CreateSolidBrush(RGB(28,28,52));
    HBRUSH oib=(HBRUSH)SelectObject(memDC,ib);
    RoundRect(memDC,15,H-58,W-215,H-12,8,8);
    SelectObject(memDC,oip);DeleteObject(ip);
    SelectObject(memDC,oib);DeleteObject(ib);
    string disp=inputText.empty()?"Click here and type your text...":inputText+(inputFocused?"|":"");
    COLORREF tc=inputText.empty()?RGB(70,70,100):RGB(255,255,255);
    txt(memDC,22,H-56,W-250,42,disp,tc,15,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    btn(memDC,W-205,H-58,W-15,H-12,"BUILD >>",RGB(33,150,243));

    HDC wdc=GetDC(hwnd);
    BitBlt(wdc,0,0,W,H,memDC,0,0,SRCCOPY);
    ReleaseDC(hwnd,wdc);
}

LRESULT CALLBACK WndProc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:{
       W = GetSystemMetrics(SM_CXSCREEN);
       H = GetSystemMetrics(SM_CYSCREEN);

        HDC wdc=GetDC(h);
        memDC=CreateCompatibleDC(wdc);
        memBmp=CreateCompatibleBitmap(wdc,W,H);
        SelectObject(memDC,memBmp);
        ReleaseDC(h,wdc);render();break;
    }
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);
        BitBlt(dc,0,0,W,H,memDC,0,0,SRCCOPY);
        EndPaint(h,&ps);break;
    }
    case WM_LBUTTONDOWN:{
        int x=LOWORD(lp),y=HIWORD(lp);
        if(x>=15&&x<=W-215&&y>=H-58&&y<=H-12){inputFocused=true;render();break;}
        inputFocused=false;
        if(y>=57&&y<=94){
            if(x>=20&&x<=155) buildHuffman();
            else if(x>=165&&x<=300){inputText="";built=false;treeRoot=NULL;huffCode.clear();freqMap.clear();encodedStr="";statusMsg="Cleared!";}
            else if(x>=310&&x<=445&&built){
                string decoded="";HNode*cur=treeRoot;
                for(char bit:encodedStr){
                    cur=(bit=='0')?cur->left:cur->right;
                    if(!cur->left&&!cur->right){decoded+=cur->ch;cur=treeRoot;}
                }
                statusMsg="Decoded: "+decoded+(decoded==inputText?" == Match!":" != Error!");
            }
        }
        if(x>=W-205&&x<=W-15&&y>=H-58&&y<=H-12) buildHuffman();
        render();break;
    }
    case WM_CHAR:{
        if(inputFocused){
            if(wp==VK_BACK){if(!inputText.empty())inputText.pop_back();}
            else if(wp=='\r') buildHuffman();
            else if(wp>=32&&wp<127)inputText+=(char)wp;
            render();
        }
        break;
    }
    case WM_KEYDOWN:{
        if(wp == VK_ESCAPE) PostQuitMessage(0);
        if(inputFocused&&wp==VK_BACK&&!inputText.empty()){inputText.pop_back();render();}
        break;
    }
    case WM_DESTROY:PostQuitMessage(0);break;
    
    }
    return DefWindowProc(h,msg,wp,lp);
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int){
    WNDCLASSA wc={};
    wc.lpfnWndProc=WndProc;wc.hInstance=hInst;
    wc.lpszClassName="HuffmanWin";
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    RegisterClassA(&wc);
    W  = GetSystemMetrics(SM_CXSCREEN);
    H = GetSystemMetrics(SM_CYSCREEN);
    hwnd=CreateWindowA("HuffmanWin"," CompressIQ -The Huffman Coding Visualizer",
    WS_POPUP,
    0,0,W,H,NULL,NULL,hInst,NULL);
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
    return 0;
}