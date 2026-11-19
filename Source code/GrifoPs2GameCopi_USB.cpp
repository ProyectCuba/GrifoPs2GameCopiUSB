// ============================================================================
//  GrifoPs2GameCopi  -  Copiador de juegos PS2 a USB (OPL)
//  Creador: xyvenqorix
//  Compilar con Dev-C++ (MinGW). NO necesita librerias ni flags extra.
// ============================================================================
#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <shlobj.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <commdlg.h>

// ---------------------------- DATOS DEL CREADOR -----------------------------
#define CREATOR_NAME   L"xyvenqorix"
#define CREATOR_PHONE  L"+53 56639178"
#define CREATOR_WEB    L"https://xyvenqorix.vercel.app"
#define CREATOR_WEBTXT L"xyvenqorix.vercel.app"
// >>> PEGA AQUI TU DIRECCION USDT (POLYGON) <<<
#define USDT_ADDR      L"PEGA_AQUI_TU_DIRECCION_USDT_POLYGON"
#define APP_VERSION    L"1.0"
// ----------------------------------------------------------------------------


// ---------- gdi32 se carga en tiempo de ejecucion (no hace falta enlazar nada) ----------
typedef HBRUSH (WINAPI *F_CSB)(COLORREF);
typedef HPEN (WINAPI *F_CP)(int, int, COLORREF);
typedef HGDIOBJ (WINAPI *F_SO)(HDC, HGDIOBJ);
typedef BOOL (WINAPI *F_DO)(HGDIOBJ);
typedef BOOL (WINAPI *F_RR)(HDC, int, int, int, int, int, int);
typedef BOOL (WINAPI *F_PG)(HDC, const POINT*, int);
typedef int (WINAPI *F_SBM)(HDC, int);
typedef COLORREF (WINAPI *F_SC)(HDC, COLORREF);
typedef BOOL (WINAPI *F_TO)(HDC, int, int, LPCWSTR, int);
typedef BOOL (WINAPI *F_MT)(HDC, int, int, LPPOINT);
typedef BOOL (WINAPI *F_LT)(HDC, int, int);
typedef HFONT (WINAPI *F_CF)(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);
static F_CSB pCreateSolidBrush; static F_CP pCreatePen; static F_SO pSelectObject; static F_DO pDeleteObject;
static F_RR pRoundRect; static F_PG pPolygon; static F_SBM pSetBkMode; static F_SC pSetTextColor, pSetBkColor;
static F_TO pTextOutW; static F_MT pMoveToEx; static F_LT pLineTo; static F_CF pCreateFontW;
#define CreateSolidBrush pCreateSolidBrush
#define CreatePen pCreatePen
#define SelectObject pSelectObject
#define DeleteObject pDeleteObject
#define RoundRect pRoundRect
#define Polygon pPolygon
#define SetBkMode pSetBkMode
#define SetTextColor pSetTextColor
#define SetBkColor pSetBkColor
#define TextOutW pTextOutW
#define MoveToEx pMoveToEx
#define LineTo pLineTo
#define CreateFontW pCreateFontW
static bool LoadGdi()
{
    HMODULE g = LoadLibraryW(L"gdi32.dll");
    if (!g) return false;
    pCreateSolidBrush = (F_CSB)GetProcAddress(g, "CreateSolidBrush");
    pCreatePen = (F_CP)GetProcAddress(g, "CreatePen");
    pSelectObject = (F_SO)GetProcAddress(g, "SelectObject");
    pDeleteObject = (F_DO)GetProcAddress(g, "DeleteObject");
    pRoundRect = (F_RR)GetProcAddress(g, "RoundRect");
    pPolygon = (F_PG)GetProcAddress(g, "Polygon");
    pSetBkMode = (F_SBM)GetProcAddress(g, "SetBkMode");
    pSetTextColor = (F_SC)GetProcAddress(g, "SetTextColor");
    pSetBkColor = (F_SC)GetProcAddress(g, "SetBkColor");
    pTextOutW = (F_TO)GetProcAddress(g, "TextOutW");
    pMoveToEx = (F_MT)GetProcAddress(g, "MoveToEx");
    pLineTo = (F_LT)GetProcAddress(g, "LineTo");
    pCreateFontW = (F_CF)GetProcAddress(g, "CreateFontW");
    return pCreateSolidBrush && pCreatePen && pSelectObject && pDeleteObject && pRoundRect && pPolygon &&
           pSetBkMode && pSetTextColor && pSetBkColor && pTextOutW && pMoveToEx && pLineTo && pCreateFontW;
}

#define IDI_MAIN 100
#define IOCTL_STORQ 0x2D1400  // IOCTL_STORAGE_QUERY_PROPERTY
#define IOCTL_LEN   0x7405C   // IOCTL_DISK_GET_LENGTH_INFO
#define IOCTL_PARTX 0x70048   // IOCTL_DISK_GET_PARTITION_INFO_EX

static HINSTANCE g_inst;
static HWND g_hwnd;
static HFONT fUI, fLabel, fTitle, fSub, fMono, fBig;
static HBRUSH brBg, brPanel;
static HICON g_icoBig, g_icoSmall;
static const COLORREF cBg = RGB(13, 15, 26), cPanel = RGB(24, 27, 46),
                      cGold = RGB(212, 175, 55), cText = RGB(232, 226, 208),
                      cMuted = RGB(150, 146, 130);

static void DrawProg(DRAWITEMSTRUCT* d, int pos)
{
    RECT r = d->rcItem;
    HBRUSH b1 = CreateSolidBrush(cPanel);
    FillRect(d->hDC, &r, b1);
    DeleteObject(b1);
    r.right = r.left + (r.right - r.left) * pos / 100;
    HBRUSH b2 = CreateSolidBrush(cGold);
    FillRect(d->hDC, &r, b2);
    DeleteObject(b2);
}
static HICON LoadAppIcon(int size)
{
    HICON h = (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(IDI_MAIN), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    if (!h) h = (HICON)LoadImageW(NULL, L"grifo.ico", IMAGE_ICON, size, size, LR_LOADFROMFILE);
    return h;
}

static void DrawBtn(DRAWITEMSTRUCT* d, bool primary)
{
    HDC dc = d->hDC;
    RECT r = d->rcItem;
    bool dis = (d->itemState & ODS_DISABLED) != 0, sel = (d->itemState & ODS_SELECTED) != 0;
    COLORREF fill = primary ? (sel ? RGB(170, 138, 38) : cGold) : (sel ? RGB(44, 48, 78) : cPanel);
    COLORREF border = cGold, tx = primary ? cBg : cGold;
    if (dis) { fill = RGB(34, 36, 48); border = RGB(80, 80, 92); tx = RGB(110, 110, 124); }
    FillRect(dc, &r, brBg);
    HBRUSH b = CreateSolidBrush(fill);
    HPEN pn = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pn);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 12, 12);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(b); DeleteObject(pn);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, tx);
    HGDIOBJ of = SelectObject(dc, fLabel);
    wchar_t t[80];
    GetWindowTextW(d->hwndItem, t, 80);
    DrawTextW(dc, t, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of);
}

// ============================ JUEGOS PS2 (OPL) ===============================
// USBExtreme / OPL: ul.cfg (registros binarios de 64 bytes) + partes ul.* en la raiz.
// Cada parte tiene como maximo 1 GiB. Compatible con el formato UL clasico de OPL.
#define WM_P_PROG (WM_APP + 4)
#define WM_P_STAT (WM_APP + 5)
#define WM_P_DONE (WM_APP + 6)
enum { IDP_DRIVE = 301, IDP_REFRESH, IDP_LIST, IDP_NOTE, IDP_ADD, IDP_DEL, IDP_PROG, IDP_STATUS, IDP_CLOSE, IDP_ABOUT };
enum { IDA_WEB = 401, IDA_PHONE, IDA_USDT, IDA_CLOSE };

typedef BOOL (WINAPI *F_GOFN)(LPOPENFILENAMEW);

static HWND g_ps2 = NULL;
static volatile bool g_busy2 = false;
static int g_pos2 = 0;
static wchar_t g_gp[400][MAX_PATH];     // prefijos de archivos UL mostrados en la lista
static int g_gpn = 0;
static wchar_t g_addDrive;
static wchar_t g_addList[64][MAX_PATH]; // ISOs elegidos para copiar
static int g_addCount = 0;

static void PostTo(HWND h, UINT m, WPARAM w, const wchar_t* s)
{
    wchar_t* c = s ? _wcsdup(s) : NULL;
    PostMessageW(h, m, w, (LPARAM)c);
}

static bool ReadAtH(HANDLE h, ULONGLONG off, void* buf, DWORD len)
{
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)off;
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN)) return false;
    DWORD r = 0;
    return ReadFile(h, buf, len, &r, NULL) && r == len;
}
static DWORD Rd32(const BYTE* b) { return (DWORD)b[0] | ((DWORD)b[1] << 8) | ((DWORD)b[2] << 16) | ((DWORD)b[3] << 24); }

// "SLUS_200.62" -> true
static bool LooksLikeId(const wchar_t* s)
{
    for (int i = 0; i < 4; i++) if (!iswalpha(s[i])) return false;
    if (s[4] != L'_') return false;
    for (int i = 5; i < 8; i++) if (!iswdigit(s[i])) return false;
    if (s[8] != L'.') return false;
    return iswdigit(s[9]) && iswdigit(s[10]);
}

// Lee de SYSTEM.CNF la linea:  BOOT2 = cdrom0:\SLUS_200.62;1
static bool IdFromText(const char* t, char* id)
{
    const char* p = strstr(t, "BOOT2");
    if (!p) return false;
    const char* c = strstr(p, "cdrom0:");
    if (!c) return false;
    c += 7;
    while (*c == '\\' || *c == '/' || *c == ' ') c++;
    int k = 0;
    while (*c && *c != ';' && *c != '\r' && *c != '\n' && *c != ' ' && k < 15) id[k++] = (char)toupper((unsigned char)*c++);
    id[k] = 0;
    return k >= 8;
}

// Abre el ISO (ISO9660, sectores de 2048) y busca SYSTEM.CNF en la raiz
static bool IdFromIso(const wchar_t* path, char* id)
{
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool ok = false;
    BYTE pvd[2048];
    if (ReadAtH(h, 16ULL * 2048, pvd, 2048) && memcmp(pvd + 1, "CD001", 5) == 0) {
        DWORD lba = Rd32(pvd + 156 + 2), size = Rd32(pvd + 156 + 10);
        if (size > 65536) size = 65536;
        DWORD rd = (size + 2047) / 2048 * 2048;
        if (rd == 0) rd = 2048;
        BYTE* dir = (BYTE*)VirtualAlloc(NULL, rd, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (dir && ReadAtH(h, (ULONGLONG)lba * 2048, dir, rd)) {
            DWORD pos = 0;
            while (pos + 34 <= size) {
                BYTE len = dir[pos];
                if (len == 0) { pos = (pos / 2048 + 1) * 2048; continue; }
                BYTE nl = dir[pos + 32];
                if (pos + 33 + nl <= rd) {
                    char nm[40];
                    int n = nl < 39 ? nl : 39;
                    memcpy(nm, dir + pos + 33, n);
                    nm[n] = 0;
                    if (_stricmp(nm, "SYSTEM.CNF;1") == 0 || _stricmp(nm, "SYSTEM.CNF") == 0) {
                        DWORD fl = Rd32(dir + pos + 2), fs = Rd32(dir + pos + 10);
                        if (fs > 1023) fs = 1023;
                        BYTE tb[2049];
                        memset(tb, 0, sizeof(tb));
                        if (ReadAtH(h, (ULONGLONG)fl * 2048, tb, 2048)) {
                            tb[fs] = 0;
                            ok = IdFromText((char*)tb, id);
                        }
                        break;
                    }
                }
                pos += len;
            }
        }
        if (dir) VirtualFree(dir, 0, MEM_RELEASE);
    }
    CloseHandle(h);
    return ok;
}

// Nombre limpio para OPL: sin .iso, sin ID repetido, sin simbolos prohibidos, max 32 letras
static void CleanName(const wchar_t* file, wchar_t* out, int max)
{
    wchar_t tmp[MAX_PATH];
    wcsncpy(tmp, file, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = 0;
    wchar_t* dot = wcsrchr(tmp, L'.');
    if (dot && _wcsicmp(dot, L".iso") == 0) *dot = 0;
    wchar_t* s = tmp;
    if (LooksLikeId(s) && s[11] == L'.') s += 12;
    while (*s == L' ') s++;
    int n = 0;
    for (; *s && n < max - 1; s++) {
        if (wcschr(L"\\/:*?\"<>|", *s)) continue;
        out[n++] = *s;
    }
    out[n] = 0;
    while (n > 0 && (out[n - 1] == L' ' || out[n - 1] == L'.')) out[--n] = 0;
    if (!n) wcscpy(out, L"Juego");
}

static void Note(wchar_t* notes, size_t cap, const wchar_t* fn, const wchar_t* why)
{
    wchar_t line[400];
    _snwprintf(line, 400, L"\n• %ls: %ls", fn, why);
    line[399] = 0;
    if (wcslen(notes) + wcslen(line) + 1 < cap) wcscat(notes, line);
}


// ----------------------- FORMATO USBEXTREME / UL ------------------------------
#define UL_CHUNK_SIZE 1073741824ULL
#define UL_RECORD_SIZE 64

static DWORD OplCRC32(const char* str)
{
    DWORD table[256], crc;
    for (int t=0; t<256; ++t) {
        crc = ((DWORD)t) << 24;
        for (int b=0; b<8; ++b)
            crc = (crc & 0x80000000UL) ? (crc << 1) : ((crc << 1) ^ 0x04C11DB7UL);
        table[255-t] = crc;
    }
    // Mantiene el CRC calculado al terminar la tabla, igual que iso2opl de OPL.
    int i=0;
    do {
        BYTE ch = (BYTE)str[i++];
        crc = table[ch ^ ((crc >> 24) & 0xFF)] ^ (crc << 8);
    } while (str[i-1] != 0);
    return crc;
}

static void WideToAnsiSafe(const wchar_t* in, char* out, int cap)
{
    if (cap <= 0) return;
    out[0]=0;
    WideCharToMultiByte(CP_ACP, 0, in, -1, out, cap, NULL, NULL);
    out[cap-1]=0;
}

static bool ULBuildPrefix(wchar_t drive, const char* gameName, const char* gameId,
                          wchar_t* prefix, int cap)
{
    // OPL usa el ID completo (por ejemplo SLUS_123.45) en el nombre de cada parte.
    if (!gameId || strlen(gameId) < 4) return false;
    _snwprintf(prefix, cap, L"%lc:\\ul.%08X.%hs.", drive, OplCRC32(gameName), gameId);
    prefix[cap-1]=0;
    return true;
}

static bool ULAppendRecord(wchar_t drive, const char* gameName, const char* gameId,
                           int parts, bool cd)
{
    wchar_t cfg[MAX_PATH];
    _snwprintf(cfg, MAX_PATH, L"%lc:\\ul.cfg", drive);
    cfg[MAX_PATH-1]=0;

    BYTE rec[UL_RECORD_SIZE];
    memset(rec, 0, sizeof(rec));
    strncpy((char*)rec, gameName, 32);
    char image[16];
    _snprintf(image, sizeof(image), "ul.%s", gameId);
    image[15]=0;
    strncpy((char*)rec+32, image, 15);
    rec[47]=(BYTE)parts;
    rec[48]=(BYTE)(cd ? 0x12 : 0x14);
    rec[53]=0x08;

    HANDLE h=CreateFileW(cfg, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (h==INVALID_HANDLE_VALUE) return false;
    DWORD wr=0;
    bool ok=WriteFile(h, rec, UL_RECORD_SIZE, &wr, NULL) && wr==UL_RECORD_SIZE;
    FlushFileBuffers(h);
    CloseHandle(h);
    return ok;
}

static bool ULExists(wchar_t drive, const char* gameName, const char* gameId)
{
    wchar_t cfg[MAX_PATH];
    _snwprintf(cfg, MAX_PATH, L"%lc:\\ul.cfg", drive);
    cfg[MAX_PATH-1]=0;
    HANDLE h=CreateFileW(cfg, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h==INVALID_HANDLE_VALUE) return false;
    BYTE rec[UL_RECORD_SIZE]; DWORD rd=0;
    char image[16]; _snprintf(image,16,"ul.%s",gameId); image[15]=0;
    bool found=false;
    while (ReadFile(h,rec,UL_RECORD_SIZE,&rd,NULL) && rd==UL_RECORD_SIZE) {
        if (!_stricmp((char*)rec+32,image) || !_stricmp((char*)rec,gameName)) { found=true; break; }
    }
    CloseHandle(h);
    return found;
}

static bool ULWriteParts(const wchar_t* src, wchar_t drive, const char* gameName,
                         const char* gameId, ULONGLONG size, int parts,
                         BYTE* buf, HWND notify, int item, int total,
                         ULONGLONG* doneBytes, ULONGLONG grand, int* lastp)
{
    wchar_t prefix[MAX_PATH], dst[MAX_PATH], status[500];
    if (!ULBuildPrefix(drive,gameName,gameId,prefix,MAX_PATH)) return false;

    HANDLE hs=CreateFileW(src,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                          FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (hs==INVALID_HANDLE_VALUE) return false;

    bool ok=true;
    for (int part=0; part<parts && ok; ++part) {
        _snwprintf(dst,MAX_PATH,L"%ls%02d",prefix,part);
        dst[MAX_PATH-1]=0;
        HANDLE hd=CreateFileW(dst,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if (hd==INVALID_HANDLE_VALUE) { ok=false; break; }
        ULONGLONG left=size-(ULONGLONG)part*UL_CHUNK_SIZE;
        if (left>UL_CHUNK_SIZE) left=UL_CHUNK_SIZE;
        while (left) {
            DWORD want=left>(1u<<20)?(1u<<20):(DWORD)left, r=0, wr=0;
            if (!ReadFile(hs,buf,want,&r,NULL) || !r ||
                !WriteFile(hd,buf,r,&wr,NULL) || wr!=r) { ok=false; break; }
            left-=r; *doneBytes+=r;
            int p=grand?(int)(*doneBytes*100/grand):0; if(p>99)p=99;
            if(p!=*lastp) {
                *lastp=p; PostMessageW(notify,WM_P_PROG,(WPARAM)p,0);
                _snwprintf(status,500,L"(%d/%d) Convirtiendo a USBExtreme: %hs   %d%%",item,total,gameName,p);
                PostTo(notify,WM_P_STAT,0,status);
            }
        }
        FlushFileBuffers(hd);
        CloseHandle(hd);
        if (!ok) DeleteFileW(dst);
    }
    CloseHandle(hs);
    if (!ok) {
        // Borra cualquier parte parcial creada para este juego.
        for(int p=0;p<parts;p++){ _snwprintf(dst,MAX_PATH,L"%ls%02d",prefix,p); DeleteFileW(dst); }
    }
    return ok;
}

static DWORD WINAPI AddWorker(LPVOID)
{
    wchar_t notes[1300]=L"", msg[1500];
    int done=0;
    ULONGLONG grand=0,doneBytes=0;
    for(int i=0;i<g_addCount;i++){
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if(GetFileAttributesExW(g_addList[i],GetFileExInfoStandard,&fa))
            grand+=((ULONGLONG)fa.nFileSizeHigh<<32)|fa.nFileSizeLow;
    }
    BYTE* buf=(BYTE*)VirtualAlloc(NULL,1<<20,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!buf){PostTo(g_ps2,WM_P_DONE,0,L"Sin memoria.");return 0;}
    int lastp=-1;

    for(int i=0;i<g_addCount;i++){
        const wchar_t* src=g_addList[i];
        const wchar_t* fn=wcsrchr(src,L'\\'); fn=fn?fn+1:src;
        wchar_t m[500];
        _snwprintf(m,500,L"(%d/%d) Analizando: %ls",i+1,g_addCount,fn);
        PostTo(g_ps2,WM_P_STAT,0,m);

        WIN32_FILE_ATTRIBUTE_DATA fa;
        if(!GetFileAttributesExW(src,GetFileExInfoStandard,&fa)){Note(notes,1300,fn,L"no pude abrir el archivo");continue;}
        ULONGLONG size=((ULONGLONG)fa.nFileSizeHigh<<32)|fa.nFileSizeLow;
        char id[16]=""; wchar_t idw[16]=L"";
        bool haveId=IdFromIso(src,id);
        if(haveId) MultiByteToWideChar(CP_ACP,0,id,-1,idw,16);
        else {
            for(int k=0;fn[k];k++) if(LooksLikeId(fn+k)){wcsncpy(idw,fn+k,11);idw[11]=0;haveId=true;break;}
        }
        if(!haveId){Note(notes,1300,fn,L"no encontre el ID del juego en SYSTEM.CNF ni en el nombre");continue;}

        bool cd=size<=734003200ULL;
        wchar_t name[40]; CleanName(fn,name,33);
        char gameName[33], gameId[16];
        WideToAnsiSafe(name,gameName,33);
        WideToAnsiSafe(idw,gameId,16);
        if(strlen(gameId)<4){Note(notes,1300,fn,L"ID de juego no valido para USBExtreme");continue;}
        int parts=(int)(size/UL_CHUNK_SIZE+(size%UL_CHUNK_SIZE?1:0));
        if(parts<1)parts=1;
        if(parts>255){Note(notes,1300,fn,L"el juego necesita demasiadas partes");continue;}
        if(ULExists(g_addDrive,gameName,gameId)){Note(notes,1300,fn,L"ya existe un juego con ese nombre o ID en ul.cfg");continue;}

        wchar_t root[4]={g_addDrive,L':',L'\\',0};
        ULARGE_INTEGER avail,tot,fr;
        if(GetDiskFreeSpaceExW(root,&avail,&tot,&fr)&&avail.QuadPart<size+1048576ULL){
            Note(notes,1300,fn,L"no hay espacio suficiente en la memoria");continue;
        }

        if(!ULWriteParts(src,g_addDrive,gameName,gameId,size,parts,buf,g_ps2,
                         i+1,g_addCount,&doneBytes,grand,&lastp)){
            Note(notes,1300,fn,L"fallo al crear las partes UL (revisa espacio y USB)");continue;
        }
        if(!ULAppendRecord(g_addDrive,gameName,gameId,parts,cd)){
            wchar_t prefix[MAX_PATH],part[MAX_PATH];
            ULBuildPrefix(g_addDrive,gameName,gameId,prefix,MAX_PATH);
            for(int p=0;p<parts;p++){_snwprintf(part,MAX_PATH,L"%ls%02d",prefix,p);DeleteFileW(part);}
            Note(notes,1300,fn,L"se copiaron las partes pero no se pudo actualizar ul.cfg");continue;
        }
        done++;
    }
    VirtualFree(buf,0,MEM_RELEASE);
    if(done>0)_snwprintf(msg,1500,L"¡Listo! Se instalaron %d de %d juego(s) en formato USBExtreme (ul.cfg + partes).%ls",done,g_addCount,notes);
    else _snwprintf(msg,1500,L"No se instalo ningun juego.%ls",notes);
    msg[1499]=0;
    PostTo(g_ps2,WM_P_DONE,done>0?1:0,msg);
    return 0;
}

// ------------------- FORMATEADOR FAT32 PROPIO (igual que GrifoFormat) -------------------
static void Progress(int p) { PostMessageW(g_ps2, WM_P_PROG, (WPARAM)p, 0); }

static bool WriteAt(HANDLE h, ULONGLONG off, const void* buf, DWORD len)
{
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)off;
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN)) return false;
    DWORD w = 0;
    return WriteFile(h, buf, len, &w, NULL) && w == len;
}
static void P16(BYTE* b, int o, DWORD v) { b[o] = (BYTE)v; b[o + 1] = (BYTE)(v >> 8); }
static void P32(BYTE* b, int o, DWORD v) { P16(b, o, v & 0xFFFF); P16(b, o + 2, v >> 16); }

static ULONGLONG zwDone, zwTotal;
static bool ZeroRange(HANDLE h, DWORD bps, ULONGLONG startSec, ULONGLONG count)
{
    DWORD chunk = (1 << 20) / bps;
    if (!chunk) chunk = 1;
    BYTE* z = (BYTE*)VirtualAlloc(NULL, (SIZE_T)chunk * bps, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!z) return false;
    bool ok = true;
    int last = -1;
    while (count && ok) {
        DWORD n = count > chunk ? chunk : (DWORD)count;
        ok = WriteAt(h, startSec * bps, z, n * bps);
        startSec += n; count -= n; zwDone += n;
        int p = zwTotal ? (int)(zwDone * 100 / zwTotal) : 0;
        if (p > 99) p = 99;
        if (p != last) { last = p; Progress(p); }
    }
    VirtualFree(z, 0, MEM_RELEASE);
    return ok;
}

// ---- Formateador FAT32 propio (Windows se niega a hacer FAT32 en >32 GB) ----
static bool Fat32Format(wchar_t L, const wchar_t* label, bool quick, wchar_t* err)
{
    wchar_t p[16];
    _snwprintf(p, 16, L"\\\\.\\%lc:", L);
    HANDLE h = CreateFileW(p, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
    if (h == INVALID_HANDLE_VALUE) { wcscpy(err, L"No pude abrir la unidad. Cierra ventanas que la usen."); return false; }
    DWORD br;
    bool locked = false;
    for (int i = 0; i < 10 && !locked; i++) {
        locked = DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &br, NULL) != 0;
        if (!locked) Sleep(500);
    }
    if (!locked) { wcscpy(err, L"La unidad esta en uso por otro programa. Cierralo e intenta de nuevo."); CloseHandle(h); return false; }
    DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &br, NULL);

    DISK_GEOMETRY g;
    DWORD bps = 512;
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &g, sizeof(g), &br, NULL) && g.BytesPerSector) bps = g.BytesPerSector;
    BYTE pi[256];
    memset(pi, 0, sizeof(pi));
    LONGLONG start = 0, len = 0;
    if (DeviceIoControl(h, IOCTL_PARTX, NULL, 0, pi, sizeof(pi), &br, NULL)) {
        start = *(LONGLONG*)(pi + 8);
        len = *(LONGLONG*)(pi + 16);
    }
    if (len <= 0 && !DeviceIoControl(h, IOCTL_LEN, NULL, 0, &len, 8, &br, NULL)) len = 0;

    DWORD eps = bps / 4;
    ULONGLONG total = (ULONGLONG)len / bps;
    bool ok = false;
    BYTE* s = NULL;
    BYTE* f = NULL;
    if (total < 70000) { wcscpy(err, L"La unidad es muy pequena para FAT32 (minimo ~35 MB)."); goto fin; }
    if (total > 0xFFFFFFFFULL) { wcscpy(err, L"FAT32 no soporta unidades de mas de 2 TB. Usa exFAT o NTFS."); goto fin; }
    {
        ULONGLONG mb = (ULONGLONG)len >> 20;
        DWORD cb;
        if (mb <= 260) cb = 512; else if (mb <= 8192) cb = 4096; else if (mb <= 16384) cb = 8192;
        else if (mb <= 32768) cb = 16384; else cb = 32768;
        DWORD spc = cb / bps;
        if (!spc) spc = 1;
        const DWORD rsvd = 32;
        DWORD fatSz = 0;
        ULONGLONG clusters = 0;
        for (;;) {
            fatSz = (DWORD)(((total - rsvd) / spc + 2 + eps - 1) / eps);
            clusters = (total - rsvd - 2ULL * fatSz) / spc;
            if (clusters >= 65525 || spc == 1) break;
            spc /= 2;
        }
        if (clusters < 65525 || clusters > 0x0FFFFFF5ULL) { wcscpy(err, L"No se pudo calcular una estructura FAT32 valida para esta unidad."); goto fin; }

        s = (BYTE*)VirtualAlloc(NULL, bps, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        f = (BYTE*)VirtualAlloc(NULL, bps, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!s || !f) { wcscpy(err, L"Sin memoria."); goto fin; }

        char lab[12];
        memset(lab, ' ', 11); lab[11] = 0;
        bool hasLabel = false;
        for (int i = 0; i < 11 && label[i]; i++) {
            wchar_t c = label[i];
            lab[i] = (c < 128 && c >= 32) ? (char)towupper(c) : '_';
            hasLabel = true;
        }

        // sector de arranque
        memset(s, 0, bps);
        s[0] = 0xEB; s[1] = 0x58; s[2] = 0x90;
        memcpy(s + 3, "MSWIN4.1", 8);
        P16(s, 11, bps); s[13] = (BYTE)spc; P16(s, 14, rsvd); s[16] = 2; s[21] = 0xF8;
        P16(s, 24, 63); P16(s, 26, 255);
        P32(s, 28, (DWORD)((ULONGLONG)start / bps)); P32(s, 32, (DWORD)total); P32(s, 36, fatSz);
        P32(s, 44, 2); P16(s, 48, 1); P16(s, 50, 6);
        s[64] = 0x80; s[66] = 0x29; P32(s, 67, GetTickCount());
        memcpy(s + 71, hasLabel ? lab : "NO NAME    ", 11);
        memcpy(s + 82, "FAT32   ", 8);
        s[510] = 0x55; s[511] = 0xAA;

        // limpiar
        ULONGLONG head = (ULONGLONG)rsvd + 2ULL * fatSz + spc;
        zwDone = 0;
        zwTotal = quick ? head : total;
        if (!ZeroRange(h, bps, 0, zwTotal)) { wcscpy(err, L"Error de escritura al limpiar la unidad."); goto fin; }

        bool w = true;
        w &= WriteAt(h, 0, s, bps);
        w &= WriteAt(h, 6ULL * bps, s, bps);
        // FSInfo
        memset(f, 0, bps);
        P32(f, 0, 0x41615252); P32(f, 484, 0x61417272); P32(f, 488, (DWORD)(clusters - 1)); P32(f, 492, 3);
        f[510] = 0x55; f[511] = 0xAA;
        w &= WriteAt(h, 1ULL * bps, f, bps);
        w &= WriteAt(h, 7ULL * bps, f, bps);
        // tablas FAT
        memset(f, 0, bps);
        P32(f, 0, 0x0FFFFFF8); P32(f, 4, 0xFFFFFFFF); P32(f, 8, 0x0FFFFFFF);
        w &= WriteAt(h, (ULONGLONG)rsvd * bps, f, bps);
        w &= WriteAt(h, ((ULONGLONG)rsvd + fatSz) * bps, f, bps);
        // etiqueta en el directorio raiz
        if (hasLabel) {
            memset(f, 0, bps);
            memcpy(f, lab, 11); f[11] = 0x08;
            w &= WriteAt(h, ((ULONGLONG)rsvd + 2ULL * fatSz) * bps, f, bps);
        }
        FlushFileBuffers(h);
        if (!w) { wcscpy(err, L"Error de escritura al crear el sistema FAT32."); goto fin; }
        ok = true;
    }
fin:
    DeviceIoControl(h, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &br, NULL);
    CloseHandle(h);
    if (s) VirtualFree(s, 0, MEM_RELEASE);
    if (f) VirtualFree(f, 0, MEM_RELEASE);
    if (ok) {
        Progress(100);
        wchar_t root[4] = { L, L':', L'\\', 0 };
        SHChangeNotify(SHCNE_MEDIAINSERTED, SHCNF_PATH, root, NULL);
    }
    return ok;
}


static wchar_t g_fmtDrive;
static DWORD WINAPI FmtWorker(LPVOID)
{
    wchar_t err[300] = L"", msg[400];
    PostTo(g_ps2, WM_P_STAT, 0, L"Formateando en FAT32... no desconectes la memoria.");
    bool ok = Fat32Format(g_fmtDrive, L"PS2", true, err);
    if (ok) _snwprintf(msg, 400, L"\u00a1Listo! La unidad %lc: ahora est\u00e1 en FAT32.\n\nPulsa AGREGAR JUEGOS para copiar tus ISO.", g_fmtDrive);
    else _snwprintf(msg, 400, L"No se pudo formatear: %ls", err);
    msg[399] = 0;
    PostTo(g_ps2, WM_P_DONE, ok ? 1 : 0, msg);
    return 0;
}

static HWND MkP(HWND par, const wchar_t* cls, const wchar_t* txt, DWORD style, int x, int y, int w, int h, int id, HFONT f, DWORD ex = 0)
{
    HWND c = CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, x, y, w, h, par, (HMENU)(INT_PTR)id, g_inst, NULL);
    if (f) SendMessageW(c, WM_SETFONT, (WPARAM)f, TRUE);
    return c;
}

static void P2Enable(HWND h, bool en)
{
    int ids[] = { IDP_DRIVE, IDP_REFRESH, IDP_LIST, IDP_ADD, IDP_DEL, IDP_ABOUT };
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) EnableWindow(GetDlgItem(h, ids[i]), en);
}

// Copia al combo de esta ventana las memorias USB que detecta la ventana principal
static void P2Refresh(HWND h)
{
    HWND lb=GetDlgItem(h,IDP_LIST), cb=GetDlgItem(h,IDP_DRIVE);
    SendMessageW(lb,LB_RESETCONTENT,0,0); g_gpn=0;
    int sel=(int)SendMessageW(cb,CB_GETCURSEL,0,0);
    if(sel<0){SetDlgItemTextW(h,IDP_STATUS,L"Conecta una memoria USB y pulsa Actualizar.");return;}
    wchar_t dl=(wchar_t)SendMessageW(cb,CB_GETITEMDATA,sel,0);
    wchar_t cfg[MAX_PATH]; _snwprintf(cfg,MAX_PATH,L"%lc:\\ul.cfg",dl); cfg[MAX_PATH-1]=0;
    HANDLE f=CreateFileW(cfg,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(f!=INVALID_HANDLE_VALUE){
        BYTE rec[64]; DWORD rd=0;
        while(g_gpn<400 && ReadFile(f,rec,64,&rd,NULL) && rd==64){
            char nm[33],im[16]; memcpy(nm,rec,32);nm[32]=0;memcpy(im,rec+32,15);im[15]=0;
            wchar_t wnm[80],wim[40],show[400];
            MultiByteToWideChar(CP_ACP,0,nm,-1,wnm,80);
            MultiByteToWideChar(CP_ACP,0,im,-1,wim,40);
            _snwprintf(show,400,L"[USBExtreme]  %ls   %ls   %u parte(s)",wnm,wim,(unsigned)rec[47]);
            int ix=(int)SendMessageW(lb,LB_ADDSTRING,0,(LPARAM)show);
            SendMessageW(lb,LB_SETITEMDATA,ix,g_gpn);
            // Se guarda el nombre UL base; el índice real del registro se mantiene por orden.
            wcsncpy(g_gp[g_gpn],wim,MAX_PATH-1);g_gp[g_gpn][MAX_PATH-1]=0;
            g_gpn++;
        }
        CloseHandle(f);
    }
    wchar_t root[4]={dl,L':',L'\\',0},fsn[16]=L"",st[400];
    GetVolumeInformationW(root,NULL,0,NULL,NULL,NULL,fsn,16);
    ULARGE_INTEGER avail,tot,fr; double freeGb=0;
    if(GetDiskFreeSpaceExW(root,&avail,&tot,&fr))freeGb=(double)avail.QuadPart/1073741824.0;
    _snwprintf(st,400,L"%d juego(s) UL  ·  %.1f GB libres  ·  %ls",g_gpn,freeGb,fsn[0]?fsn:L"USB");
    SetDlgItemTextW(h,IDP_STATUS,st);
}

static void P2Add(HWND h)
{
    if (g_busy2) return;
    HWND cb = GetDlgItem(h, IDP_DRIVE);
    int sel = (int)SendMessageW(cb, CB_GETCURSEL, 0, 0);
    if (sel < 0) { MessageBoxW(h, L"Primero conecta y elige una memoria USB.", L"GrifoFormat", MB_OK | MB_ICONINFORMATION); return; }
    wchar_t dl = (wchar_t)SendMessageW(cb, CB_GETITEMDATA, sel, 0);
    wchar_t root[4] = { dl, L':', L'\\', 0 }, fsn[16] = L"";
    GetVolumeInformationW(root, NULL, 0, NULL, NULL, NULL, fsn, 16);
    if (_wcsicmp(fsn, L"FAT32") != 0) {
        wchar_t item[256], q[800];
        SendMessageW(cb, CB_GETLBTEXT, sel, (LPARAM)item);
        _snwprintf(q, 800, L"Esta memoria NECESITA FORMATEAR.\n\nEst\u00e1 en %ls y los juegos USBExtreme necesitan FAT32.\n\n"
                           L"\u00bfFormatear ahora en FAT32? Se borrar\u00e1n TODOS los datos de:\n\n%ls",
                   fsn[0] ? fsn : L"un formato desconocido (sin formato)", item);
        q[799] = 0;
        if (MessageBoxW(h, q, L"GrifoPs2GameCopi - Necesita formatear", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
        g_fmtDrive = dl;
        g_busy2 = true;
        P2Enable(h, false);
        g_pos2 = 0;
        InvalidateRect(GetDlgItem(h, IDP_PROG), NULL, FALSE);
        HANDLE ft = CreateThread(NULL, 0, FmtWorker, NULL, 0, NULL);
        if (ft) CloseHandle(ft);
        else { g_busy2 = false; P2Enable(h, true); }
        return;
    }

    HMODULE cd = LoadLibraryW(L"comdlg32.dll");
    F_GOFN go = cd ? (F_GOFN)GetProcAddress(cd, "GetOpenFileNameW") : NULL;
    if (!go) { MessageBoxW(h, L"No pude abrir el selector de archivos.", L"GrifoFormat", MB_OK | MB_ICONERROR); return; }
    static wchar_t buf[32768];
    buf[0] = 0;
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = h;
    ofn.lpstrFilter = L"Juegos PS2 (*.iso)\0*.iso\0Todos los archivos\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = 32768;
    ofn.lpstrTitle = L"Elige los juegos de PS2 (ISO)";
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!go(&ofn)) return;

    g_addCount = 0;
    const wchar_t* next = buf + wcslen(buf) + 1;
    if (*next == 0) {                       // un solo archivo
        wcsncpy(g_addList[0], buf, MAX_PATH - 1);
        g_addList[0][MAX_PATH - 1] = 0;
        g_addCount = 1;
    } else {                                // carpeta + varios nombres
        while (*next && g_addCount < 64) {
            _snwprintf(g_addList[g_addCount], MAX_PATH, L"%ls\\%ls", buf, next);
            g_addList[g_addCount][MAX_PATH - 1] = 0;
            g_addCount++;
            next += wcslen(next) + 1;
        }
    }
    if (!g_addCount) return;

    g_addDrive = dl;
    g_busy2 = true;
    P2Enable(h, false);
    g_pos2 = 0;
    InvalidateRect(GetDlgItem(h, IDP_PROG), NULL, FALSE);
    SetDlgItemTextW(h, IDP_STATUS, L"Trabajando... no desconectes la memoria.");
    HANDLE t = CreateThread(NULL, 0, AddWorker, NULL, 0, NULL);
    if (t) CloseHandle(t);
    else { g_busy2 = false; P2Enable(h, true); }
}

static void P2Delete(HWND h)
{
    HWND lb=GetDlgItem(h,IDP_LIST);
    int cnt=(int)SendMessageW(lb,LB_GETSELCOUNT,0,0);
    if(cnt<=0){MessageBoxW(h,L"Selecciona en la lista el juego que quieres borrar.",L"GrifoFormat",MB_OK|MB_ICONINFORMATION);return;}
    if(MessageBoxW(h,L"¿Borrar de la memoria los juegos seleccionados y sus partes UL?",L"GrifoFormat - Borrar juegos",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES)return;
    int* sel=(int*)malloc(sizeof(int)*cnt); if(!sel)return;
    int got=(int)SendMessageW(lb,LB_GETSELITEMS,cnt,(LPARAM)sel);
    HWND cb=GetDlgItem(h,IDP_DRIVE);int cs=(int)SendMessageW(cb,CB_GETCURSEL,0,0);
    if(cs<0){free(sel);return;}
    wchar_t dl=(wchar_t)SendMessageW(cb,CB_GETITEMDATA,cs,0);
    wchar_t cfg[MAX_PATH],tmp[MAX_PATH];
    _snwprintf(cfg,MAX_PATH,L"%lc:\\ul.cfg",dl);cfg[MAX_PATH-1]=0;
    _snwprintf(tmp,MAX_PATH,L"%lc:\\ul.tmp",dl);tmp[MAX_PATH-1]=0;
    HANDLE in=CreateFileW(cfg,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(in==INVALID_HANDLE_VALUE){free(sel);P2Refresh(h);return;}
    HANDLE out=CreateFileW(tmp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(out==INVALID_HANDLE_VALUE){CloseHandle(in);free(sel);MessageBoxW(h,L"No pude actualizar ul.cfg.",L"GrifoFormat",MB_OK|MB_ICONERROR);return;}
    bool remove[400]={false};
    for(int i=0;i<got;i++){int ix=(int)SendMessageW(lb,LB_GETITEMDATA,sel[i],0);if(ix>=0&&ix<400)remove[ix]=true;}
    BYTE rec[64];DWORD rd=0,wr=0;int index=0,bad=0;
    while(ReadFile(in,rec,64,&rd,NULL)&&rd==64){
        if(index<400&&remove[index]){
            char nm[33],im[16];memcpy(nm,rec,32);nm[32]=0;memcpy(im,rec+32,15);im[15]=0;
            char* id=im+3; DWORD crc=OplCRC32(nm);
            wchar_t prefix[MAX_PATH],part[MAX_PATH];
            _snwprintf(prefix,MAX_PATH,L"%lc:\\%hs.%08X.%hs.",dl,"ul",crc,id);
            for(int p=0;p<rec[47];p++){_snwprintf(part,MAX_PATH,L"%ls%02d",prefix,p);if(!DeleteFileW(part)&&GetLastError()!=ERROR_FILE_NOT_FOUND)bad++;}
        }else{
            if(!WriteFile(out,rec,64,&wr,NULL)||wr!=64)bad++;
        }
        index++;
    }
    FlushFileBuffers(out);CloseHandle(in);CloseHandle(out);free(sel);
    if(!MoveFileExW(tmp,cfg,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        DeleteFileW(tmp);MessageBoxW(h,L"No pude reemplazar ul.cfg; no se completo el borrado.",L"GrifoFormat",MB_OK|MB_ICONERROR);return;
    }
    P2Refresh(h);
    if(bad)MessageBoxW(h,L"Se actualizo ul.cfg, pero algunos archivos de partes no se pudieron borrar.",L"GrifoFormat",MB_OK|MB_ICONWARNING);
}

static bool IsElevated()
{
    HANDLE t;
    DWORD e = 0, r = 0;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &t)) {
        GetTokenInformation(t, (TOKEN_INFORMATION_CLASS)20, &e, sizeof(e), &r);   // TokenElevation
        CloseHandle(t);
    }
    return e != 0;
}

static HFONT MkFont(const wchar_t* face, int h, int weight, bool italic)
{
    return CreateFontW(h, 0, 0, 0, weight, italic, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}

// ============================ DETECCION DE USB ===============================
// Se consulta con acceso 0.
static void DriveInfo(wchar_t dl, int* bus, wchar_t* model, int mlen)
{
    *bus = 0;
    model[0] = 0;
    wchar_t p[16];
    _snwprintf(p, 16, L"\\\\.\\%lc:", dl);
    HANDLE h = CreateFileW(p, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD br = 0;
    struct { DWORD id, qt; BYTE ap[4]; } q = { 0, 0, { 0, 0, 0, 0 } };
    BYTE buf[1024];
    memset(buf, 0, sizeof(buf));
    if (DeviceIoControl(h, IOCTL_STORQ, &q, sizeof(q), buf, sizeof(buf), &br, NULL)) {
        *bus = (int)*(DWORD*)(buf + 28);
        DWORD po = *(DWORD*)(buf + 16);
        if (po && po < sizeof(buf) - 2) {
            MultiByteToWideChar(CP_ACP, 0, (char*)buf + po, -1, model, mlen);
            int n = (int)wcslen(model);
            while (n > 0 && model[n - 1] == L' ') model[--n] = 0;
        }
    }
    CloseHandle(h);
}

static void FillDrives(HWND h)
{
    HWND cb = GetDlgItem(h, IDP_DRIVE);
    int old = (int)SendMessageW(cb, CB_GETCURSEL, 0, 0);
    wchar_t keep = 0;
    if (old >= 0) keep = (wchar_t)SendMessageW(cb, CB_GETITEMDATA, old, 0);
    SendMessageW(cb, CB_RESETCONTENT, 0, 0);

    wchar_t win[MAX_PATH];
    GetWindowsDirectoryW(win, MAX_PATH);
    DWORD mask = GetLogicalDrives();
    int idx = -1, count = 0;
    for (int i = 0; i < 26; i++) {
        if (!(mask & (1u << i))) continue;
        wchar_t dl = (wchar_t)(L'A' + i);
        if (dl == (wchar_t)towupper(win[0])) continue;           // nunca el disco de Windows
        wchar_t root[4] = { dl, L':', L'\\', 0 };
        UINT t = GetDriveTypeW(root);
        if (t != DRIVE_REMOVABLE && t != DRIVE_FIXED) continue;
        int bus;
        wchar_t model[64];
        DriveInfo(dl, &bus, model, 64);
        if (t != DRIVE_REMOVABLE && bus != 7) continue;          // solo USB / removibles
        ULARGE_INTEGER avail, tot, fr;
        if (!GetDiskFreeSpaceExW(root, &avail, &tot, &fr) || tot.QuadPart == 0) continue;
        wchar_t vol[64] = L"";
        GetVolumeInformationW(root, vol, 64, NULL, NULL, NULL, NULL, 0);
        wchar_t text[256];
        _snwprintf(text, 256, L"%lc:   %ls   %ls   (%.1f GB)", dl,
                   vol[0] ? vol : L"Sin nombre", model[0] ? model : L"USB", (double)tot.QuadPart / 1073741824.0);
        int ix = (int)SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)text);
        SendMessageW(cb, CB_SETITEMDATA, ix, (LPARAM)dl);
        if (dl == keep) idx = ix;
        count++;
    }
    if (count) SendMessageW(cb, CB_SETCURSEL, idx >= 0 ? idx : 0, 0);
}

static void RefreshAll(HWND h)
{
    FillDrives(h);
    P2Refresh(h);
}

// ----------------------------- Ventana "Acerca de" ---------------------------
static void CopyToClipboard(HWND owner, const wchar_t* t)
{
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    size_t n = (wcslen(t) + 1) * sizeof(wchar_t);
    HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, n);
    if (g) {
        memcpy(GlobalLock(g), t, n);
        GlobalUnlock(g);
        SetClipboardData(CF_UNICODETEXT, g);
    }
    CloseClipboard();
}

static LRESULT CALLBACK AboutProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE: {
        HWND b;
        b = CreateWindowExW(0, L"BUTTON", L"Abrir web", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 24, 352, 124, 32, h, (HMENU)IDA_WEB, g_inst, NULL);
        SendMessageW(b, WM_SETFONT, (WPARAM)fLabel, TRUE);
        b = CreateWindowExW(0, L"BUTTON", L"Copiar n\u00famero", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 158, 352, 124, 32, h, (HMENU)IDA_PHONE, g_inst, NULL);
        SendMessageW(b, WM_SETFONT, (WPARAM)fLabel, TRUE);
        b = CreateWindowExW(0, L"BUTTON", L"Copiar USDT", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 292, 352, 124, 32, h, (HMENU)IDA_USDT, g_inst, NULL);
        SendMessageW(b, WM_SETFONT, (WPARAM)fLabel, TRUE);
        b = CreateWindowExW(0, L"BUTTON", L"Cerrar", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 150, 398, 140, 34, h, (HMENU)IDA_CLOSE, g_inst, NULL);
        SendMessageW(b, WM_SETFONT, (WPARAM)fLabel, TRUE);
        SendMessageW(h, WM_SETICON, ICON_BIG, (LPARAM)g_icoBig);
        SendMessageW(h, WM_SETICON, ICON_SMALL, (LPARAM)g_icoSmall);
        return 0;
    }
    case WM_ERASEBKGND: {
        RECT r; GetClientRect(h, &r); FillRect((HDC)w, &r, brBg); return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        SetBkMode(dc, TRANSPARENT);
        if (g_icoBig) DrawIconEx(dc, 24, 18, g_icoBig, 64, 64, 0, NULL, DI_NORMAL);
        SelectObject(dc, fBig); SetTextColor(dc, cGold);
        TextOutW(dc, 104, 14, L"GrifoPs2GameCopi", 16);
        SelectObject(dc, fSub); SetTextColor(dc, cText);
        wchar_t v[64]; _snwprintf(v, 64, L"Versi\u00f3n %ls  \u00b7  Juegos de PS2 a tu USB", APP_VERSION);
        TextOutW(dc, 106, 56, v, (int)wcslen(v));

        RECT r = { 24, 100, 416, 168 };
        SelectObject(dc, fUI); SetTextColor(dc, cText);
        DrawTextW(dc, L"Copia tus juegos de PS2 (ISO) a tu memoria USB en formato USBExtreme, listo para OPL. "
                      L"Admite juegos de m\u00e1s de 4 GB.",
                  -1, &r, DT_LEFT | DT_WORDBREAK);

        SelectObject(dc, fLabel); SetTextColor(dc, cGold);
        TextOutW(dc, 24, 178, L"CREADOR", 7);
        SelectObject(dc, fBig); SetTextColor(dc, cText);
        TextOutW(dc, 24, 194, CREATOR_NAME, (int)wcslen(CREATOR_NAME));
        SelectObject(dc, fUI);
        wchar_t t[128];
        _snwprintf(t, 128, L"Contacto:  %ls", CREATOR_PHONE);
        TextOutW(dc, 24, 232, t, (int)wcslen(t));
        _snwprintf(t, 128, L"Web:  %ls", CREATOR_WEBTXT);
        TextOutW(dc, 24, 254, t, (int)wcslen(t));

        SelectObject(dc, fLabel); SetTextColor(dc, cGold);
        { const wchar_t* sb = L"DIRECCI\u00d3N USDT (POLYGON)"; TextOutW(dc, 24, 288, sb, (int)wcslen(sb)); }
        RECT ra = { 24, 308, 416, 346 };
        SelectObject(dc, fMono); SetTextColor(dc, cText);
        DrawTextW(dc, USDT_ADDR, -1, &ra, DT_LEFT | DT_WORDBREAK);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_DRAWITEM:
        DrawBtn((DRAWITEMSTRUCT*)l, ((DRAWITEMSTRUCT*)l)->CtlID == IDA_CLOSE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDA_WEB: ShellExecuteW(h, L"open", CREATOR_WEB, NULL, NULL, SW_SHOWNORMAL); break;
        case IDA_PHONE: CopyToClipboard(h, CREATOR_PHONE); SetDlgItemTextW(h, IDA_PHONE, L"\u00a1Copiado!"); break;
        case IDA_USDT: CopyToClipboard(h, USDT_ADDR); SetDlgItemTextW(h, IDA_USDT, L"\u00a1Copiado!"); break;
        case IDA_CLOSE: DestroyWindow(h); break;
        }
        return 0;
    case WM_CLOSE: DestroyWindow(h); return 0;
    case WM_DESTROY:
        EnableWindow(g_hwnd, TRUE);
        SetForegroundWindow(g_hwnd);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void ShowAbout()
{
    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = AboutProc; wc.hInstance = g_inst; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = brBg; wc.lpszClassName = L"GrifoPs2About";
        RegisterClassW(&wc);
        reg = true;
    }
    RECT rc = { 0, 0, 440, 450 };
    DWORD st = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&rc, st, FALSE);
    RECT pr; GetWindowRect(g_hwnd, &pr);
    int w = rc.right - rc.left, hh = rc.bottom - rc.top;
    int x = pr.left + ((pr.right - pr.left) - w) / 2, y = pr.top + ((pr.bottom - pr.top) - hh) / 2;
    if (y < 0) y = 0;
    EnableWindow(g_hwnd, FALSE);
    HWND a = CreateWindowExW(0, L"GrifoPs2About", L"Acerca de GrifoPs2GameCopi", st, x, y, w, hh, g_hwnd, NULL, g_inst, NULL);
    if (!a) { EnableWindow(g_hwnd, TRUE); return; }
    ShowWindow(a, SW_SHOW);
}

// ============================== VENTANA PRINCIPAL ============================
static LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        g_hwnd = g_ps2 = h;
        MkP(h, L"COMBOBOX", NULL, WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 24, 126, 396, 260, IDP_DRIVE, fUI);
        MkP(h, L"BUTTON", L"Actualizar", WS_TABSTOP | BS_OWNERDRAW, 430, 125, 66, 26, IDP_REFRESH, fLabel);
        MkP(h, L"LISTBOX", NULL, WS_TABSTOP | WS_VSCROLL | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
            24, 190, 472, 180, IDP_LIST, fUI, WS_EX_CLIENTEDGE);
        MkP(h, L"STATIC", L"Formato USBExtreme: ul.cfg + partes ul.* de hasta 1 GiB. Lee el ID desde SYSTEM.CNF y "
                          L"admite juegos de m\u00e1s de 4 GB. Se recomienda memoria FAT32.",
            SS_LEFT, 24, 380, 472, 54, IDP_NOTE, fLabel);
        MkP(h, L"BUTTON", L"AGREGAR JUEGOS ISO \u2192 UL", WS_TABSTOP | BS_OWNERDRAW, 24, 442, 288, 42, IDP_ADD, fLabel);
        MkP(h, L"BUTTON", L"BORRAR", WS_TABSTOP | BS_OWNERDRAW, 324, 442, 172, 42, IDP_DEL, fLabel);
        MkP(h, L"STATIC", NULL, SS_OWNERDRAW, 24, 496, 472, 14, IDP_PROG, NULL);
        MkP(h, L"STATIC", L"", SS_LEFT, 24, 518, 472, 40, IDP_STATUS, fUI);
        MkP(h, L"BUTTON", L"Acerca de", WS_TABSTOP | BS_OWNERDRAW, 24, 566, 120, 30, IDP_ABOUT, fLabel);
        SendMessageW(h, WM_SETICON, ICON_BIG, (LPARAM)g_icoBig);
        SendMessageW(h, WM_SETICON, ICON_SMALL, (LPARAM)g_icoSmall);
        RefreshAll(h);
        return 0;
    case WM_ERASEBKGND: {
        RECT r; GetClientRect(h, &r); FillRect((HDC)w, &r, brBg); return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        SetBkMode(dc, TRANSPARENT);
        if (g_icoBig) DrawIconEx(dc, 24, 14, g_icoBig, 56, 56, 0, NULL, DI_NORMAL);
        SelectObject(dc, fBig); SetTextColor(dc, cGold);
        TextOutW(dc, 92, 10, L"GrifoPs2GameCopi", 16);
        SelectObject(dc, fSub); SetTextColor(dc, cText);
        { const wchar_t* sb = L"Copia tus juegos de PS2 a la USB (OPL)"; TextOutW(dc, 94, 50, sb, (int)wcslen(sb)); }

        HPEN pn = CreatePen(PS_SOLID, 2, cGold);
        HGDIOBJ op = SelectObject(dc, pn);
        MoveToEx(dc, 24, 88, NULL); LineTo(dc, 236, 88);
        MoveToEx(dc, 284, 88, NULL); LineTo(dc, 496, 88);
        HBRUSH gb = CreateSolidBrush(cGold);
        HGDIOBJ ob = SelectObject(dc, gb);
        POINT dm[4] = { { 260, 80 }, { 268, 88 }, { 260, 96 }, { 252, 88 } };
        Polygon(dc, dm, 4);
        SelectObject(dc, ob); SelectObject(dc, op);
        DeleteObject(gb); DeleteObject(pn);

        SelectObject(dc, fLabel); SetTextColor(dc, cGold);
        TextOutW(dc, 24, 104, L"UNIDAD USB", 10);
        { const wchar_t* sb = L"JUEGOS EN LA MEMORIA"; TextOutW(dc, 24, 168, sb, (int)wcslen(sb)); }
        SetTextColor(dc, cMuted);
        { const wchar_t* sb = L"Hecho por xyvenqorix  ·  xyvenqorix.vercel.app"; TextOutW(dc, 160, 573, sb, (int)wcslen(sb)); }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)w;
        SetBkColor(dc, cBg);
        SetTextColor(dc, GetDlgCtrlID((HWND)l) == IDP_NOTE ? cMuted : cText);
        return (LRESULT)brBg;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)w;
        SetBkColor(dc, cPanel);
        SetTextColor(dc, cText);
        return (LRESULT)brPanel;
    }
    case WM_DRAWITEM:
        if (((DRAWITEMSTRUCT*)l)->CtlID == IDP_PROG) DrawProg((DRAWITEMSTRUCT*)l, g_pos2);
        else DrawBtn((DRAWITEMSTRUCT*)l, ((DRAWITEMSTRUCT*)l)->CtlID == IDP_ADD);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDP_REFRESH: if (!g_busy2) RefreshAll(h); break;
        case IDP_DRIVE: if (HIWORD(w) == CBN_SELCHANGE && !g_busy2) P2Refresh(h); break;
        case IDP_ADD: P2Add(h); break;
        case IDP_ABOUT: ShowAbout(); break;
        case IDP_DEL: if (!g_busy2) P2Delete(h); break;
        }
        return 0;
    case WM_DEVICECHANGE:
        if (!g_busy2 && (w == 0x8000 || w == 0x8004)) RefreshAll(h);   // conectar / quitar USB
        return TRUE;
    case WM_P_PROG:
        g_pos2 = (int)w;
        InvalidateRect(GetDlgItem(h, IDP_PROG), NULL, FALSE);
        return 0;
    case WM_P_STAT:
        if (l) { SetDlgItemTextW(h, IDP_STATUS, (wchar_t*)l); free((void*)l); }
        return 0;
    case WM_P_DONE: {
        wchar_t* msg = (wchar_t*)l;
        bool ok = w != 0;
        g_busy2 = false;
        P2Enable(h, true);
        g_pos2 = ok ? 100 : 0;
        InvalidateRect(GetDlgItem(h, IDP_PROG), NULL, FALSE);
        MessageBoxW(h, msg ? msg : L"", L"GrifoPs2GameCopi", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONWARNING));
        if (msg) free(msg);
        P2Refresh(h);
        return 0;
    }
    case WM_CLOSE:
        if (g_busy2) {
            MessageBoxW(h, L"Hay una copia en curso. Espera a que termine para no dañar la memoria.", L"GrifoPs2GameCopi", MB_OK | MB_ICONWARNING);
            return 0;
        }
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int)
{
    g_inst = hi;
    FreeConsole();   // oculta la ventana negra si el proyecto es de consola
    // Para formatear a FAT32 se necesitan permisos de administrador: se relanza solo.
    if (!IsElevated() && !wcsstr(GetCommandLineW(), L"/e")) {
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, MAX_PATH);
        if ((INT_PTR)ShellExecuteW(NULL, L"runas", exe, L"/e", NULL, SW_SHOWNORMAL) > 32) return 0;
    }
    if (!LoadGdi()) { MessageBoxW(NULL, L"No se pudo cargar gdi32.dll", L"GrifoPs2GameCopi", MB_OK | MB_ICONERROR); return 1; }

    brBg = CreateSolidBrush(cBg);
    brPanel = CreateSolidBrush(cPanel);
    fUI = MkFont(L"Segoe UI", -15, FW_NORMAL, false);
    fLabel = MkFont(L"Segoe UI", -13, FW_BOLD, false);
    fTitle = MkFont(L"Georgia", -40, FW_BOLD, false);
    fSub = MkFont(L"Georgia", -17, FW_NORMAL, true);
    fBig = MkFont(L"Georgia", -28, FW_BOLD, false);
    fMono = MkFont(L"Consolas", -14, FW_NORMAL, false);
    g_icoBig = LoadAppIcon(64);
    g_icoSmall = LoadAppIcon(16);

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainProc; wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = brBg;
    wc.lpszClassName = L"GrifoPs2GameCopiWnd";
    wc.hIcon = g_icoBig; wc.hIconSm = g_icoSmall;
    RegisterClassExW(&wc);

    RECT rc = { 0, 0, 520, 608 };
    DWORD st = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, st, FALSE);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    if (y < 0) y = 0;
    HWND wnd = CreateWindowExW(0, L"GrifoPs2GameCopiWnd", L"GrifoPs2GameCopi - Copiador de juegos PS2", st, x, y, w, h, NULL, NULL, hi, NULL);
    if (!wnd) return 1;
    ShowWindow(wnd, SW_SHOW);
    UpdateWindow(wnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
