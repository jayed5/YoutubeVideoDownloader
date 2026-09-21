#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <commctrl.h>
#include <windowsx.h>
#include <map>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <thread>
#include <functional>
namespace fs=std::filesystem;
struct Hit{size_t pos,end;std::string ext;std::vector<unsigned char> blob;WORD typeId=0;};
HWND hInput,hOutput,hList,hStatus,hScan,hExport,hProgress,hTitle,hSubtitle,hInputLabel,hOutputLabel,hInfo,hClose,hSettings; HFONT hUi,hUiBold; std::vector<unsigned char> data; enum Lang{EN,PL,ZH,DE,RU}; Lang language=EN;std::vector<Hit> results; bool ahkResourceDetected=false; HMODULE nativeModule=nullptr; std::vector<Hit>* nativeResults=nullptr; size_t nativeBase=0;
// ---- Video Downloader state ----
static std::atomic<int> gPostep{0};static std::atomic<bool> gPracuje{false};static std::atomic<bool> gAnuluj{false};
static std::vector<std::wstring> gUrls;static int gFormatIdx=0;static int gJakoscIdx=0;
static std::wstring gStatusText;

std::wstring Tr(int id){static const wchar_t* t[5][18]={
 {L"Video Downloader",L"Download videos with yt-dlp",L"Video URL:",L"Output folder:",L"Browse...",L"Download",L"Select all",L"Clear list",L"Cancel",L"Ready to download.",L"Add video URLs above (one per line), then click Download. One file with video+audio (mp4) or audio-only (mp3/m4a).",L"Ready to download.",L"Downloading...",L"Finished. Saved to: ",L"Tip: mp3/m4a requires ffmpeg.exe next to the program.",L"Settings",L"Format",L"Quality"},
 {L"Pobieracz Filmow",L"Pobieraj filmy z yt-dlp",L"Adres filmu:",L"Folder wyjsciowy:",L"Przegladaj...",L"Pobierz",L"Zaznacz wszystko",L"Wyczysc liste",L"Anuluj",L"Gotowy do pobierania.",L"Dodaj adresy filmow powyzej (jeden na linie), a nastepnie kliknij Pobierz. Jeden plik video+audio (mp4) lub tylko audio (mp3/m4a).",L"Gotowy do pobierania.",L"Pobieram...",L"Zakonczono. Zapisano w: ",L"Wskazowka: mp3/m4a wymaga ffmpeg.exe obok programu.",L"Ustawienia",L"Format",L"Jakosc"},
 {L"视频下载器",L"使用 yt-dlp 下载视频",L"视频网址：",L"输出文件夹：",L"浏览...",L"下载",L"全选",L"清空列表",L"取消",L"准备下载。",L"在上方添加视频网址（每行一个），然后点击下载。单文件包含视频+音频(mp4)或纯音频(mp3/m4a)。",L"准备下载。",L"正在下载...",L"完成。已保存到：",L"提示：mp3/m4a 需要程序旁的 ffmpeg.exe。",L"设置",L"格式",L"画质"},
 {L"Video Downloader",L"Videos mit yt-dlp laden",L"Video-URL:",L"Ausgabeordner:",L"Durchsuchen...",L"Herunterladen",L"Alle auswählen",L"Liste leeren",L"Abbrechen",L"Bereit zum Herunterladen.",L"Oben Video-URLs hinzufügen (eine pro Zeile), dann auf Herunterladen klicken. Eine Datei mit Video+Audio (mp4) oder nur Audio (mp3/m4a).",L"Bereit zum Herunterladen.",L"Wird geladen...",L"Fertig. Gespeichert in: ",L"Tipp: mp3/m4a benötigt ffmpeg.exe neben dem Programm.",L"Einstellungen",L"Format",L"Qualität"},
 {L"Загрузчик видео",L"Скачивайте видео через yt-dlp",L"Ссылка на видео:",L"Папка вывода:",L"Обзор...",L"Скачать",L"Выбрать всё",L"Очистить список",L"Отмена",L"Готов к загрузке.",L"Добавьте ссылки выше (по одной в строке) и нажмите Скачать. Один файл видео+аудио (mp4) или только аудио (mp3/m4a).",L"Готов к загрузке.",L"Загрузка...",L"Готово. Сохранено в: ",L"Подсказка: mp3/m4a требует ffmpeg.exe рядом с программой.",L"Настройки",L"Формат",L"Качество"}
};return t[(int)language][id];}
void SetLanguage(Lang l){language=l;SetWindowTextW(hTitle,Tr(0).c_str());SetWindowTextW(hSubtitle,Tr(1).c_str());SetWindowTextW(hInputLabel,Tr(2).c_str());SetWindowTextW(hOutputLabel,Tr(3).c_str());SetWindowTextW(GetDlgItem(GetParent(hOutput),2),Tr(4).c_str());SetWindowTextW(hScan,Tr(5).c_str());SetWindowTextW(GetDlgItem(GetParent(hScan),4),Tr(6).c_str());SetWindowTextW(GetDlgItem(GetParent(hScan),5),Tr(7).c_str());SetWindowTextW(hExport,Tr(8).c_str());SetWindowTextW(hInfo,Tr(10).c_str());SetWindowTextW(hStatus,Tr(11).c_str());SetWindowTextW(hSettings,Tr(15).c_str());}
void LanguageMenu(HWND w){HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING|((language==EN)?MF_CHECKED:0),20,L"English");AppendMenuW(m,MF_STRING|((language==PL)?MF_CHECKED:0),21,L"Polski");AppendMenuW(m,MF_STRING|((language==ZH)?MF_CHECKED:0),22,L"中文");AppendMenuW(m,MF_STRING|((language==DE)?MF_CHECKED:0),23,L"Deutsch");AppendMenuW(m,MF_STRING|((language==RU)?MF_CHECKED:0),24,L"Русский");POINT p;GetCursorPos(&p);int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,w,nullptr);DestroyMenu(m);if(id>=20&&id<=24)SetLanguage((Lang)(id-20));}



std::wstring W(const std::string&s){return std::wstring(s.begin(),s.end());}
std::wstring Text(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,L'\0');GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
void SetText(HWND h,const std::wstring&s){SetWindowTextW(h,s.c_str());}
void DefaultOutFromInput(){std::wstring in=Text(hInput);if(Text(hOutput).empty()){size_t s=in.find_last_of(L"\\/");SetText(hOutput,s==std::wstring::npos?L".":in.substr(0,s));}}
void Log(const std::wstring&s){if(hInfo)SetText(hInfo,s);}
void ChooseFolder(){BROWSEINFOW b{};b.hwndOwner=GetActiveWindow();b.lpszTitle=L"Choose output folder";b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;auto id=SHBrowseForFolderW(&b);if(id){wchar_t p[MAX_PATH];if(SHGetPathFromIDListW(id,p))SetText(hOutput,p);CoTaskMemFree(id);}}
// ================== VIDEO DOWNLOADER (yt-dlp) ==================
static std::wstring ZnajdzNarzedzie(const wchar_t* nazwa){
    wchar_t buf[MAX_PATH];GetModuleFileNameW(nullptr,buf,MAX_PATH);
    std::wstring obok=(fs::path(buf).parent_path()/nazwa).wstring();
    if(fs::exists(obok))return L"\""+obok+L"\"";
    DWORD n=SearchPathW(nullptr,nazwa,nullptr,MAX_PATH,buf,nullptr);
    if(n>0&&n<MAX_PATH)return L"\""+std::wstring(buf)+L"\"";
    return L"";
}
static std::wstring ZbudujArgumenty(const std::wstring& url,const std::wstring& folder){
    std::wostringstream a;
    a<<L"--no-warnings --newline --no-playlist-reverse -o \""<<folder<<L"\\%(title)s.%(ext)s\" ";
    std::wstring ff=ZnajdzNarzedzie(L"ffmpeg.exe");
    if(!ff.empty())a<<L"--ffmpeg-location "<<ff<<L" ";
    const wchar_t* h[]={L"",L"1080",L"720",L"480",L"360"};
    if(gFormatIdx==0){
        if(gJakoscIdx==0)a<<L"-f \"bestvideo*+bestaudio/best\" --merge-output-format mp4 ";
        else a<<L"-f \"bestvideo*[height<="<<h[gJakoscIdx]<<L"]+bestaudio/best[height<="<<h[gJakoscIdx]<<L"]/best\" --merge-output-format mp4 ";
    }else a<<L"-x --audio-format "<<(gFormatIdx==1?L"mp3":L"m4a")<<L" --audio-quality 192 ";
    a<<L"\""<<url<<L"\"";
    return a.str();
}
static void UstawStatus(const std::wstring& s){gStatusText=s;SetText(hStatus,s);}
struct Pozycja{int idx;std::wstring url;};
static void WatekPobierania(std::vector<Pozycja> pozycje,std::wstring folder){
    std::wstring exe=ZnajdzNarzedzie(L"yt-dlp.exe");
    if(exe.empty()){UstawStatus(Tr(14));gPracuje=false;return;}
    for(auto&poz:pozycje){
        if(gAnuluj)break;
        gPostep=0;
        wchar_t st[64];swprintf(st,64,Tr(12).c_str(),(int)(&poz-&pozycje[0])+1,(int)pozycje.size());
        ListView_SetItemText(hList,poz.idx,3,st);
        SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
        HANDLE rRead=nullptr,rWrite=nullptr;
        if(!CreatePipe(&rRead,&rWrite,&sa,0)){ListView_SetItemText(hList,poz.idx,3,(LPWSTR)L"Error");continue;}
        SetHandleInformation(rRead,HANDLE_FLAG_INHERIT,0);
        STARTUPINFOW si{};si.cb=sizeof(si);
        si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
        si.hStdOutput=rWrite;si.hStdError=rWrite;si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
        si.wShowWindow=SW_HIDE;
        PROCESS_INFORMATION pi{};
        std::wstring cmd=exe+L" "+ZbudujArgumenty(poz.url,folder);
        std::vector<wchar_t> cmdBuf(cmd.begin(),cmd.end());cmdBuf.push_back(L'\0');
        if(!CreateProcessW(nullptr,cmdBuf.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi)){
            CloseHandle(rRead);CloseHandle(rWrite);
            ListView_SetItemText(hList,poz.idx,3,(LPWSTR)L"Error");continue;
        }
        CloseHandle(rWrite);
        std::string acc;char buf[4096];DWORD rd;
        std::wstring rozmiar;
        while(ReadFile(rRead,buf,sizeof(buf),&rd,nullptr)&&rd){
            acc.append(buf,rd);
            size_t nl;
            while((nl=acc.find('\n'))!=std::string::npos){
                std::string line=acc.substr(0,nl);acc.erase(0,nl+1);
                if(line.rfind("[download]",0)==0){
                    size_t p=line.find('%');
                    if(p!=std::string::npos&&p>10){
                        size_t s=line.rfind(' ',p);
                        if(s!=std::string::npos){try{gPostep=(int)std::stof(line.substr(s+1,p-s-1));}catch(...){}}
                    }
                    size_t of=line.find("of ");size_t at=line.find(" at ");
                    if(of!=std::string::npos&&at!=std::string::npos&&at>of+3){
                        std::string r=line.substr(of+3,at-of-3);
                        rozmiar=std::wstring(r.begin(),r.end());
                        ListView_SetItemText(hList,poz.idx,2,(LPWSTR)rozmiar.c_str());
                    }
                }
            }
            if(gAnuluj)break;
        }
        CloseHandle(rRead);
        WaitForSingleObject(pi.hProcess,15000);
        DWORD kod=0;GetExitCodeProcess(pi.hProcess,&kod);
        CloseHandle(pi.hProcess);CloseHandle(pi.hThread);
        if(gAnuluj){ListView_SetItemText(hList,poz.idx,3,(LPWSTR)L"Cancelled");break;}
        if(kod==0){gPostep=100;ListView_SetItemText(hList,poz.idx,3,(LPWSTR)L"Done");}
        else ListView_SetItemText(hList,poz.idx,3,(LPWSTR)L"Failed");
    }
    UstawStatus(gAnuluj?L"Cancelled.":Tr(13)+folder);
    gPracuje=false;
}
void PobierzZaznaczone(){
    if(gPracuje)return;
    std::wstring folder=Text(hOutput);
    if(folder.empty()){MessageBoxW(0,L"Choose an output folder first.",L"Missing folder",MB_ICONWARNING);return;}
    std::vector<Pozycja> wybrane;
    int n=ListView_GetItemCount(hList);
    for(int i=0;i<n;++i)if(ListView_GetCheckState(hList,i))wybrane.push_back({i,gUrls[i]});
    if(wybrane.empty()){MessageBoxW(0,L"Select at least one item.",L"Nothing selected",MB_OK|MB_ICONINFORMATION);return;}
    try{fs::create_directories(folder);}catch(...){}
    gAnuluj=false;gPracuje=true;gPostep=0;UstawStatus(Tr(12));
    static std::thread watek;
    if(watek.joinable())watek.join();
    watek=std::thread(WatekPobierania,std::move(wybrane),folder);
    watek.detach();
}
void DodajUrl(const std::wstring& url){
    if(url.empty())return;
    std::wstring u=url;u.erase(0,u.find_first_not_of(L" \t\r\n"));
    if(u.empty())return;
    gUrls.push_back(u);
    LVITEMW it{};it.mask=LVIF_TEXT;it.iItem=(int)gUrls.size()-1;it.pszText=(LPWSTR)u.c_str();
    int i=ListView_InsertItem(hList,&it);
    ListView_SetCheckState(hList,i,TRUE);
    ListView_SetItemText(hList,i,1,(LPWSTR)(gFormatIdx==0?L"mp4":(gFormatIdx==1?L"mp3":L"m4a")));
    ListView_SetItemText(hList,i,2,(LPWSTR)L"-");
    ListView_SetItemText(hList,i,3,(LPWSTR)L"Ready");
    SetText(hStatus,Tr(11));
}
void DodajZPola(){
    std::wstring t=Text(hInput);
    size_t s=0;
    while(s<t.size()){
        size_t e=t.find_first_of(L"\r\n",s);
        std::wstring linia=t.substr(s,e==std::wstring::npos?std::wstring::npos:e-s);
        size_t a=linia.find_first_not_of(L" \t");
        if(a!=std::wstring::npos){
            size_t b=linia.find_last_not_of(L" \t");
            DodajUrl(linia.substr(a,b-a+1));
        }
        if(e==std::wstring::npos)break;
        s=e+1;
    }
    SetText(hInput,L"");
}
void UstawieniaMenu(HWND w){
    HMENU m=CreatePopupMenu();
    HMENU mf=CreatePopupMenu();HMENU mq=CreatePopupMenu();HMENU ml=CreatePopupMenu();
    AppendMenuW(mf,MF_STRING|((gFormatIdx==0)?MF_CHECKED:0),30,L"video (mp4)");
    AppendMenuW(mf,MF_STRING|((gFormatIdx==1)?MF_CHECKED:0),31,L"audio (mp3)");
    AppendMenuW(mf,MF_STRING|((gFormatIdx==2)?MF_CHECKED:0),32,L"audio (m4a)");
    AppendMenuW(mq,MF_STRING|((gJakoscIdx==0)?MF_CHECKED:0),40,L"Best");
    AppendMenuW(mq,MF_STRING|((gJakoscIdx==1)?MF_CHECKED:0),41,L"1080p");
    AppendMenuW(mq,MF_STRING|((gJakoscIdx==2)?MF_CHECKED:0),42,L"720p");
    AppendMenuW(mq,MF_STRING|((gJakoscIdx==3)?MF_CHECKED:0),43,L"480p");
    AppendMenuW(mq,MF_STRING|((gJakoscIdx==4)?MF_CHECKED:0),44,L"360p");
    AppendMenuW(ml,MF_STRING|((language==EN)?MF_CHECKED:0),20,L"English");
    AppendMenuW(ml,MF_STRING|((language==PL)?MF_CHECKED:0),21,L"Polski");
    AppendMenuW(ml,MF_STRING|((language==ZH)?MF_CHECKED:0),22,L"中文");
    AppendMenuW(ml,MF_STRING|((language==DE)?MF_CHECKED:0),23,L"Deutsch");
    AppendMenuW(ml,MF_STRING|((language==RU)?MF_CHECKED:0),24,L"Русский");
    AppendMenuW(m,MF_POPUP,(UINT_PTR)mf,Tr(16).c_str());
    AppendMenuW(m,MF_POPUP,(UINT_PTR)mq,Tr(17).c_str());
    AppendMenuW(m,MF_POPUP,(UINT_PTR)ml,L"Language");
    POINT p;GetCursorPos(&p);
    int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,w,nullptr);
    DestroyMenu(m);
    if(id>=20&&id<=24)SetLanguage((Lang)(id-20));
    if(id>=30&&id<=32)gFormatIdx=id-30;
    if(id>=40&&id<=44)gJakoscIdx=id-40;
    int n=ListView_GetItemCount(hList);
    for(int i=0;i<n;++i)ListView_SetItemText(hList,i,1,(LPWSTR)(gFormatIdx==0?L"mp4":(gFormatIdx==1?L"mp3":L"m4a")));
}
void SelectAll(BOOL v){int n=ListView_GetItemCount(hList);for(int i=0;i<n;++i)ListView_SetCheckState(hList,i,v);}
void Font(HWND h,HFONT f){SendMessageW(h,WM_SETFONT,(WPARAM)f,TRUE);}
void Place(HWND h,int x,int y,int w,int z){SetWindowPos(h,nullptr,x,y,w,z,SWP_NOZORDER|SWP_NOACTIVATE);}
void PaintHeader(HWND w){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);r.bottom=58;HBRUSH b=CreateSolidBrush(RGB(24,38,62));FillRect(dc,&r,b);DeleteObject(b);EndPaint(w,&ps);}
LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(m==WM_CREATE){
  hUi=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,L"Segoe UI");hUiBold=CreateFontW(-16,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,L"Segoe UI");
  hInputLabel=CreateWindowW(L"STATIC",Tr(2).c_str(),WS_VISIBLE|WS_CHILD,20,20,110,22,w,0,0,0);hInput=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,135,17,430,26,w,(HMENU)9,0,0);CreateWindowW(L"BUTTON",L"Add",WS_VISIBLE|WS_CHILD,575,17,110,26,w,(HMENU)1,0,0);
  hOutputLabel=CreateWindowW(L"STATIC",Tr(3).c_str(),WS_VISIBLE|WS_CHILD,20,55,110,22,w,0,0,0);hOutput=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,135,52,430,26,w,0,0,0);CreateWindowW(L"BUTTON",Tr(4).c_str(),WS_VISIBLE|WS_CHILD,575,52,110,26,w,(HMENU)2,0,0);
  hScan=CreateWindowW(L"BUTTON",Tr(5).c_str(),WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,20,92,115,32,w,(HMENU)3,0,0);CreateWindowW(L"BUTTON",Tr(6).c_str(),WS_VISIBLE|WS_CHILD,145,92,155,32,w,(HMENU)4,0,0);CreateWindowW(L"BUTTON",Tr(7).c_str(),WS_VISIBLE|WS_CHILD,310,92,155,32,w,(HMENU)5,0,0);hExport=CreateWindowW(L"BUTTON",Tr(8).c_str(),WS_VISIBLE|WS_CHILD,520,92,220,32,w,(HMENU)6,0,0);hSettings=CreateWindowW(L"BUTTON",Tr(15).c_str(),WS_VISIBLE|WS_CHILD,750,92,130,32,w,(HMENU)7,0,0);
  hInfo=CreateWindowW(L"STATIC",Tr(10).c_str(),WS_VISIBLE|WS_CHILD,20,137,665,22,w,0,0,0);hStatus=CreateWindowW(L"STATIC",Tr(11).c_str(),WS_VISIBLE|WS_CHILD,20,165,665,22,w,0,0,0);hProgress=CreateWindowW(PROGRESS_CLASSW,L"",WS_VISIBLE|WS_CHILD,20,193,665,16,w,0,0,0);hList=CreateWindowW(WC_LISTVIEWW,L"",WS_VISIBLE|WS_CHILD|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS|WS_VSCROLL,20,220,665,260,w,0,0,0);ListView_SetExtendedListViewStyle(hList,LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.cx=400;c.pszText=(LPWSTR)L"Video (URL)";ListView_InsertColumn(hList,0,&c);c.cx=90;c.pszText=(LPWSTR)L"Format";ListView_InsertColumn(hList,1,&c);c.cx=120;c.pszText=(LPWSTR)L"Size";ListView_InsertColumn(hList,2,&c);c.cx=120;c.pszText=(LPWSTR)L"Status";ListView_InsertColumn(hList,3,&c);
  for(HWND q:{hInputLabel,hOutputLabel,hInfo,hStatus,hInput,hOutput,hScan,hExport,hSettings,hList})Font(q,hUi);Log(Tr(10));{wchar_t dl[MAX_PATH];SHGetFolderPathW(w,CSIDL_DESKTOPDIRECTORY,nullptr,0,dl);SetText(hOutput,std::wstring(dl)+L"\\Videos");}DragAcceptFiles(w,TRUE);SetTimer(w,1,120,nullptr);return 0;}
 if(m==WM_TIMER){if(gPracuje||gPostep==100){SendMessageW(hProgress,PBM_SETPOS,gPostep,0);}SetText(hStatus,gStatusText);return 0;}
 if(m==WM_DROPFILES){wchar_t path[MAX_PATH]={};UINT nf=DragQueryFileW((HDROP)b,0xFFFFFFFF,nullptr,0);for(UINT i=0;i<nf;++i){if(DragQueryFileW((HDROP)b,i,path,MAX_PATH))DodajUrl(path);}DragFinish((HDROP)b);return 0;}
 if(m==WM_SIZE){int Wd=LOWORD(b),Ht=HIWORD(b);if(Wd<700)Wd=700;if(Ht<500)Ht=500;Place(hInput,140,17,Wd-270,26);Place(hOutput,140,52,Wd-270,26);Place(GetDlgItem(w,1),Wd-125,17,110,26);Place(GetDlgItem(w,2),Wd-125,52,110,26);Place(hScan,20,92,125,32);Place(GetDlgItem(w,4),155,92,175,32);Place(GetDlgItem(w,5),340,92,175,32);Place(hExport,535,92,220,32);Place(hSettings,765,92,135,32);Place(hInfo,20,137,Wd-40,22);Place(hStatus,20,165,Wd-40,22);Place(hProgress,20,193,Wd-40,16);Place(hList,20,220,Wd-40,Ht-240);return 0;}
 if(m==WM_CTLCOLORSTATIC){HDC dc=(HDC)a;SetTextColor(dc,RGB(0,0,0));SetBkMode(dc,TRANSPARENT);static HBRUSH wb=CreateSolidBrush(RGB(255,255,255));return (LRESULT)wb;}  if(m==WM_COMMAND){switch(LOWORD(a)){case 9:if(HIWORD(a)==EN_SETFOCUS)PostMessageW(hInput,EM_SETSEL,0,-1);break;case 1:DodajZPola();break;case 2:ChooseFolder();break;case 3:PobierzZaznaczone();break;case 4:SelectAll(TRUE);break;case 5:if(!gPracuje){ListView_DeleteAllItems(hList);gUrls.clear();SetText(hStatus,Tr(11));}break;case 6:if(gPracuje)gAnuluj=true;break;case 7:UstawieniaMenu(w);break;case 20:SetLanguage(EN);break;case 21:SetLanguage(PL);break;case 22:SetLanguage(ZH);break;case 23:SetLanguage(DE);break;case 24:SetLanguage(RU);break;}return 0;}
 if(m==WM_DESTROY){KillTimer(w,1);if(hUi)DeleteObject(hUi);if(hUiBold)DeleteObject(hUiBold);PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,a,b);}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,LPWSTR,int n){INITCOMMONCONTROLSEX x{sizeof(x),ICC_PROGRESS_CLASS};InitCommonControlsEx(&x);CoInitialize(nullptr);WNDCLASSW c{};c.lpfnWndProc=Proc;c.hInstance=h;c.lpszClassName=L"VideoDownloaderClass";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);HWND w=CreateWindowW(c.lpszClassName,L"Video Downloader",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,100,100,920,600,nullptr,nullptr,h,nullptr);ShowWindow(w,n);MSG q;HACCEL hAccel;ACCEL acc[]={ {FCONTROL|FVIRTKEY,'S',3},{FCONTROL|FVIRTKEY,'E',6},{FCONTROL|FVIRTKEY,'O',2},{FCONTROL|FVIRTKEY,'L',4},{FCONTROL|FVIRTKEY,'K',5} };hAccel=CreateAcceleratorTableW(acc,5);while(GetMessageW(&q,nullptr,0,0)>0){if(!TranslateAcceleratorW(w,hAccel,&q)){TranslateMessage(&q);DispatchMessageW(&q);}}DestroyAcceleratorTable(hAccel);CoUninitialize();return 0;}
