#include "History.h"
#include "Protocol.h"
#include <Application.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
using namespace clipper;
constexpr uint32 kStep='tsST', kReceived='tsRC';
class Sink : public BTextView {
public:
    Sink():BTextView("paste sink") {}
    void Paste(BClipboard* clipboard) override {
        BMessage got(kReceived);
        if(clipboard->Lock()) { got.AddMessage("data",clipboard->Data()); clipboard->Unlock(); }
        be_app_messenger.SendMessage(&got);
    }
};
class TestApp : public BApplication {
public:
    TestApp():BApplication("application/x-vnd.airOS-Clipper-PasteIntegration") {}
    void ReadyToRun() override {
        window=new BWindow(BRect(140,140,620,360),"Clipper paste integration",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS);
        auto sink=new Sink;
        BLayoutBuilder::Group<>(window,B_VERTICAL,8).SetInsets(12).Add(new BStringView("label","Automatic Clipper test: this window receives injected paste.")).Add(sink);
        sink->MakeFocus(); window->Show(); Schedule(0);
    }
    void MessageReceived(BMessage* msg) override {
        if(msg->what==kReceived) {
            BMessage payload; msg->FindMessage("data",&payload);
            const void* bytes; ssize_t size;
            if(kind==0) {
                if(payload.FindData("text/plain",B_MIME_TYPE,&bytes,&size)!=B_OK || size!=static_cast<ssize_t>(strlen(sample)) || memcmp(bytes,sample,size)!=0
                    || !payload.HasData("text/html",B_MIME_TYPE)) Fail("rich text paste payload");
                else { puts("PASS: original target received injected paste with plain text and HTML intact."); ++generation; kind=1; Schedule(0); }
            } else {
                BMessage archive;
                if(payload.FindMessage("image/bitmap",&archive)!=B_OK) { Fail("image missing"); return; }
                BBitmap restored(&archive);
                if(restored.InitCheck()!=B_OK || restored.BitsLength()!=expected.size() || memcmp(restored.Bits(),expected.data(),expected.size())!=0) Fail("bitmap bytes");
                else { puts("PASS: original target received injected image paste with byte-identical bitmap."); PostMessage(B_QUIT_REQUESTED); }
            }
        } else if(msg->what==kStep) { if(msg->GetInt64("generation",-1)==generation) Step(msg->GetInt32("phase",0)); }
        else BApplication::MessageReceived(msg);
    }
    bool QuitRequested() override { if(window->Lock()) window->Quit(); return true; }
    bool passed=true;
private:
    void Schedule(int32 phase,bigtime_t delay=600000) { BMessage next(kStep);next.AddInt32("phase",phase);next.AddInt64("generation",generation);BMessageRunner::StartSending(be_app_messenger,&next,delay,1); }
    void Fail(const char* reason) { fprintf(stderr,"FAIL: %s\n",reason);passed=false;PostMessage(B_QUIT_REQUESTED); }
    void Step(int32 phase) {
        app_info active;be_roster->GetActiveAppInfo(&active);
        printf("phase=%d kind=%d own=%d active=%d %s\n",phase,kind,(int)Team(),(int)active.team,active.signature);fflush(stdout);
        if(phase==0) {
            BMessage hide(kHide);BMessenger(kSignature).SendMessage(&hide);
            if(window->Lock()) { window->Activate();window->Unlock(); }
            be_roster->ActivateApp(Team()); Schedule(1); return;
        }
        if(phase==1) {
            BMessage data;
            if(kind==0) {data.AddData("text/plain",B_MIME_TYPE,sample,strlen(sample));data.AddData("text/html",B_MIME_TYPE,"<b>integration</b>",18);}
            else {BBitmap bitmap(BRect(0,0,63,31),B_RGBA32);expected.resize(bitmap.BitsLength());for(size_t i=0;i<expected.size();++i)expected[i]=i%251;memcpy(bitmap.Bits(),expected.data(),expected.size());BMessage archive;bitmap.Archive(&archive);data.AddMessage("image/bitmap",&archive);}
            if(!be_clipboard->Lock()) { Fail("clipboard lock");return; }be_clipboard->Clear();*be_clipboard->Data()=data;be_clipboard->Commit();be_clipboard->Unlock();Schedule(2);return;
        }
        if(phase==2) {
            if(window->Lock()) {window->Activate();window->Unlock();}be_roster->ActivateApp(Team());
            Schedule(5,300000);return;
        }
        if(phase==5) {
            BMessage show(kShow); BMessenger app(kSignature);printf("show valid=%d status=%d\n",app.IsValid(),(int)app.SendMessage(&show));fflush(stdout);Schedule(3);return;
        }
        if(phase==3) {
            History history;if(history.Load(SettingsPath("history").String())!=B_OK) {Fail("history load");return;}
            int64 id=0;for(const auto& clip:history.clips) if((kind==0 && clip.text==sample)||(kind==1 && clip.kind=="Image · 64 × 32")){id=clip.id;break;}
            if(!id){Fail("captured clip missing");return;}
            // Recover focus if another workstation test activated an application.
            if(window->Lock()){window->Activate();window->Unlock();}be_roster->ActivateApp(Team());
            BMessage choose(kChoose);choose.AddInt64("id",id);BMessenger app(kSignature);printf("choose valid=%d status=%d\n",app.IsValid(),(int)app.SendMessage(&choose));fflush(stdout);Schedule(4,2000000);return;
        }
        if(phase==4) Fail("paste did not arrive at the original target");
    }
    BWindow* window=nullptr;int kind=0;int64 generation=0;std::vector<uint8> expected;const char* sample="Clipper integration: older rich-text item";
};
int main(){TestApp app;app.Run();return app.passed?0:1;}
