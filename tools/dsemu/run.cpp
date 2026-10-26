// Headless melonDS runner: run.cpp rom.nds frames out.ppm [touchx touchy]
// Builds against melonDS core (libcore.a). NDS mode, FreeBIOS.
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <vector>
#include "NDS.h"
#include "NDSCart.h"
#include "GPU.h"
#include "Platform.h"
#include "Args.h"
using namespace melonDS;
namespace melonDS::Platform {
struct FileHandle { FILE* f; };
void SignalStop(StopReason, void*) {}
std::string GetLocalFilePath(const std::string& n) { return n; }
FileHandle* OpenFile(const std::string& p, FileMode m) {
  const char* md = (m & FileMode::Write) ? ((m & FileMode::Read) ? "r+b" : "wb") : "rb";
  FILE* f = fopen(p.c_str(), md); if (!f && (m & FileMode::Write) && (m & FileMode::Read)) f = fopen(p.c_str(), "w+b");
  return f ? new FileHandle{f} : nullptr; }
FileHandle* OpenLocalFile(const std::string& p, FileMode m) { return OpenFile(p, m); }
bool FileExists(const std::string& n) { FILE* f = fopen(n.c_str(), "rb"); if (f) fclose(f); return f; }
bool LocalFileExists(const std::string& n) { return FileExists(n); }
bool CheckFileWritable(const std::string&) { return true; }
bool CheckLocalFileWritable(const std::string&) { return true; }
bool CloseFile(FileHandle* h) { fclose(h->f); delete h; return true; }
bool IsEndOfFile(FileHandle* h) { return feof(h->f); }
bool FileReadLine(char* s, int c, FileHandle* h) { return fgets(s, c, h->f); }
u64 FilePosition(FileHandle* h) { return ftell(h->f); }
bool FileSeek(FileHandle* h, s64 o, FileSeekOrigin w) { return !fseek(h->f, o, w == FileSeekOrigin::Start ? SEEK_SET : w == FileSeekOrigin::Current ? SEEK_CUR : SEEK_END); }
void FileRewind(FileHandle* h) { rewind(h->f); }
u64 FileRead(void* d, u64 s, u64 c, FileHandle* h) { return fread(d, s, c, h->f); }
bool FileFlush(FileHandle* h) { return !fflush(h->f); }
u64 FileWrite(const void* d, u64 s, u64 c, FileHandle* h) { return fwrite(d, s, c, h->f); }
u64 FileWriteFormatted(FileHandle* h, const char* fmt, ...) { va_list a; va_start(a, fmt); int r = vfprintf(h->f, fmt, a); va_end(a); return r; }
u64 FileLength(FileHandle* h) { long p = ftell(h->f); fseek(h->f, 0, SEEK_END); long l = ftell(h->f); fseek(h->f, p, SEEK_SET); return l; }
void Log(LogLevel, const char* fmt, ...) { if (getenv("DSLOG")) { va_list a; va_start(a, fmt); vfprintf(stderr, fmt, a); va_end(a); } }
struct Thread { std::thread t; };
Thread* Thread_Create(std::function<void()> f) { return new Thread{std::thread(f)}; }
void Thread_Free(Thread* t) { delete t; }
void Thread_Wait(Thread* t) { t->t.join(); }
struct Semaphore { std::mutex m; std::condition_variable c; int n = 0; };
Semaphore* Semaphore_Create() { return new Semaphore; }
void Semaphore_Free(Semaphore* s) { delete s; }
void Semaphore_Reset(Semaphore* s) { std::lock_guard<std::mutex> l(s->m); s->n = 0; }
void Semaphore_Wait(Semaphore* s) { std::unique_lock<std::mutex> l(s->m); s->c.wait(l, [&]{ return s->n > 0; }); s->n--; }
bool Semaphore_TryWait(Semaphore* s, int ms) { std::unique_lock<std::mutex> l(s->m); if (!s->c.wait_for(l, std::chrono::milliseconds(ms), [&]{ return s->n > 0; })) return false; s->n--; return true; }
void Semaphore_Post(Semaphore* s, int c) { { std::lock_guard<std::mutex> l(s->m); s->n += c; } s->c.notify_all(); }
struct Mutex { std::mutex m; };
Mutex* Mutex_Create() { return new Mutex; }
void Mutex_Free(Mutex* m) { delete m; }
void Mutex_Lock(Mutex* m) { m->m.lock(); }
void Mutex_Unlock(Mutex* m) { m->m.unlock(); }
bool Mutex_TryLock(Mutex* m) { return m->m.try_lock(); }
void Sleep(u64 us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
u64 GetMSCount() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
u64 GetUSCount() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void WriteNDSSave(const u8*, u32, u32, u32, void*) {}
void WriteGBASave(const u8*, u32, u32, u32, void*) {}
void WriteFirmware(const Firmware&, u32, u32, void*) {}
void WriteDateTime(int, int, int, int, int, int, void*) {}
void MP_Begin(void*) {} void MP_End(void*) {}
int MP_SendPacket(u8*, int, u64, void*) { return 0; }
int MP_RecvPacket(u8*, u64*, void*) { return 0; }
int MP_SendCmd(u8*, int, u64, void*) { return 0; }
int MP_SendReply(u8*, int, u64, u16, void*) { return 0; }
int MP_SendAck(u8*, int, u64, void*) { return 0; }
int MP_RecvHostPacket(u8*, u64*, void*) { return 0; }
u16 MP_RecvReplies(u8*, u64, u16, void*) { return 0; }
int Net_SendPacket(u8*, int, void*) { return 0; }
int Net_RecvPacket(u8*, void*) { return 0; }
void Camera_Start(int, void*) {} void Camera_Stop(int, void*) {}
void Camera_CaptureFrame(int, u32*, int, int, bool, void*) {}
void Mic_Start(void*) {} void Mic_Stop(void*) {}
int Mic_ReadInput(s16*, int, void*) { return 0; }
AACDecoder* AAC_Init() { return nullptr; }
void AAC_DeInit(AACDecoder*) {}
bool AAC_Configure(AACDecoder*, int, int) { return false; }
bool AAC_DecodeFrame(AACDecoder*, const void*, int, void*, int) { return false; }
bool Addon_KeyDown(KeyType, void*) { return false; }
void Addon_RumbleStart(u32, void*) {} void Addon_RumbleStop(void*) {}
float Addon_MotionQuery(MotionQueryType, void*) { return 0; }
DynamicLibrary* DynamicLibrary_Load(const char*) { return nullptr; }
void DynamicLibrary_Unload(DynamicLibrary*) {}
void* DynamicLibrary_LoadFunction(DynamicLibrary*, const char*) { return nullptr; }
}
static void dump(NDS& nds, const char* path) {
  void *t, *b; nds.GPU.GetFramebuffers(&t, &b);
  FILE* f = fopen(path, "wb"); fprintf(f, "P6\n256 384\n255\n");
  for (int s = 0; s < 2; s++) { u32* p = (u32*)(s ? b : t); if (!p) { for (int i = 0; i < 256*192; i++) fputc(0, f), fputc(0, f), fputc(0, f); continue; }
    for (int i = 0; i < 256*192; i++) { u32 c = p[i]; fputc((c>>16)&255, f); fputc((c>>8)&255, f); fputc(c&255, f); } }
  fclose(f);
}
int main(int argc, char** argv) {
  if (argc < 4) return 1;
  std::ifstream in(argv[1], std::ios::binary); std::vector<u8> rom((std::istreambuf_iterator<char>(in)), {});
  NDSArgs args; args.JIT = std::nullopt;
  NDS& nds = *new NDS(std::move(args));
  auto cart = NDSCart::ParseROM(rom.data(), rom.size());
  if (!cart) { fprintf(stderr, "bad rom\n"); return 2; }
  nds.SetNDSCart(std::move(cart));
  nds.Reset(); nds.SetupDirectBoot(argv[1]); nds.Start();
  int n = atoi(argv[2]);
  // optional script: argv[4] = "frame:keymask:tx:ty,..." (keymask bits: 1=A... as melon: 0=A 1=B 2=sel 3=start 4=R 5=L 6=U 7=D 8=R 9=L ; bit set = pressed)
  // script (argv[4]): "a-b:keys:tx,ty;..." frames a..b hold key bits (hex; 1=A 2=B 4=SEL 8=START 10=RIGHT 20=LEFT 40=UP 80=DOWN 100=R 200=L, 400=X 800=Y) and a touch at tx,ty (tx<0: none)
  // extra screenshots: argv[5] = "frame,frame,..." writes out.ppm with the frame number inserted before .ppm
  struct Ev { int a, b; unsigned keys; int tx, ty; }; std::vector<Ev> evs;
  if (argc > 4) { char* p = argv[4]; while (*p) { Ev e{0,0,0,-1,-1}; int c = 0; sscanf(p, "%d-%d:%x:%d,%d%n", &e.a, &e.b, &e.keys, &e.tx, &e.ty, &c); if (!c) break; evs.push_back(e); p += c; if (*p == ';') p++; } }
  std::vector<int> shots; if (argc > 5) { char* p = argv[5]; while (*p) { shots.push_back(atoi(p)); while (*p && *p != ',') p++; if (*p == ',') p++; } }
  for (int i = 0; i < n; i++) {
    unsigned keys = 0; int tx = -1, ty = -1;
    for (auto& e : evs) if (i >= e.a && i <= e.b) { keys |= e.keys; if (e.tx >= 0) { tx = e.tx; ty = e.ty; } }
    nds.SetKeyMask(0xFFF & ~keys);
    if (tx >= 0) { nds.TouchScreen(tx, ty); nds.ARM9Write16(0x02FFF004, tx); nds.ARM9Write16(0x02FFF006, ty); nds.ARM9Write16(0x02FFF00C, 0x3F); nds.ARM9Write16(0x02FFF008, 100); nds.ARM9Write16(0x02FFF00A, 100); }
    else { nds.ReleaseScreen(); nds.ARM9Write16(0x02FFF00C, 0x7F); }
    nds.RunFrame();
    for (int sf : shots) if (sf == i) { std::string nm = argv[3]; size_t d = nm.rfind(".ppm"); nm = nm.substr(0, d) + "_" + std::to_string(i) + ".ppm"; dump(nds, nm.c_str()); }
  }
  if (getenv("DBG")) fprintf(stderr, "dispcnt %08x bg0cnt %04x bg3cnt %04x vramD@0 %04x %04x bg(10,10)=%04x vramcnt D %02x\n", nds.ARM9Read32(0x04000000), nds.ARM9Read16(0x04000008), nds.ARM9Read16(0x0400000E), nds.ARM9Read16(0x06000000+(10*256+10)*2), nds.ARM9Read16(0x06000000+(4*256+4)*2), nds.ARM9Read16(0x06000000+(170*256+100)*2), nds.ARM9Read8(0x04000243));
  dump(nds, argv[3]);
  return 0;
}
