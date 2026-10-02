// Native integration fixture: standard editable text plus a paste payload log.
#include "History.h"
#include "Protocol.h"
#include <Application.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <Deskbar.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <TextView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <Window.h>
#include <cstdio>
#include <cstring>
using namespace clipper;
class Pad : public BTextView {
public:
    Pad():BTextView("pad") { SetText("Clipper integration test pad\n"); }
    void Paste(BClipboard* clipboard) override {
        if(clipboard->Lock()) {
            BFile file("/tmp/clipper-last-paste",B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
            clipboard->Data()->Flatten(&file); file.Sync(); clipboard->Unlock();
        }
        BTextView::Paste(clipboard);
        BFile text("/tmp/clipper-pad-text",B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE); text.Write(Text(),TextLength());
    }
};
int main(int argc,char** argv) {
    BApplication app("application/x-vnd.airOS-Clipper-Fixture");
    if(argc>2 && strcmp(argv[1],"--control")==0) {
        uint32 what=strcmp(argv[2],"pin")==0?kPin:strcmp(argv[2],"delete")==0?kDelete:
            strcmp(argv[2],"clear")==0?kClear:strcmp(argv[2],"pause")==0?kPause:
            strcmp(argv[2],"settings")==0?kSaveSettings:0;
        if(!what) return 2;
        BMessage request(what);
        if(what==kPin || what==kDelete) { if(argc<4)return 2;request.AddInt64("id",strtoll(argv[3],nullptr,10)); }
        if(what==kClear)request.AddBool("all",argc>3 && strcmp(argv[3],"all")==0);
        if(what==kSaveSettings) {if(argc<6)return 2;request.AddInt32("limit",atoi(argv[3]));request.AddBool("persist",atoi(argv[4]));request.AddBool("auto_paste",atoi(argv[5]));}
        return BMessenger(kSignature).SendMessage(&request)==B_OK?0:1;
    }
    if(argc>1 && strcmp(argv[1],"--hide")==0) { BMessage hide(kHide); BMessenger(kSignature).SendMessage(&hide); return 0; }
    if(argc>1 && strcmp(argv[1],"--set-text")==0 && argc>2) {
        be_clipboard->Lock(); be_clipboard->Clear(); be_clipboard->Data()->AddData("text/plain",B_MIME_TYPE,argv[2],strlen(argv[2]));
        be_clipboard->Commit(); be_clipboard->Unlock(); return 0;
    }
    if(argc>1 && strcmp(argv[1],"--set-image")==0 && argc>2) {
        auto bitmap=BTranslationUtils::GetBitmap(argv[2]); if(!bitmap) return 1;
        be_clipboard->Lock(); be_clipboard->Clear(); BMessage archive; bitmap->Archive(&archive);
        be_clipboard->Data()->AddMessage("image/bitmap",&archive); be_clipboard->Commit(); be_clipboard->Unlock(); delete bitmap; return 0;
    }
    if(argc>1 && strcmp(argv[1],"--history")==0) {
        History h; status_t result=h.Load(SettingsPath("history").String()); if(result!=B_OK) return 1;
        for(const auto& c:h.clips) printf("%lld %s %s %s\n",(long long)c.id,c.pinned?"pinned":"normal",c.kind.String(),c.text.String());
        return 0;
    }
    if(argc>1 && strcmp(argv[1],"--read-paste")==0) {
        BFile file("/tmp/clipper-last-paste",B_READ_ONLY); BMessage payload;
        if(payload.Unflatten(&file)!=B_OK) return 1;
        Clip c;c.data=payload;History::Describe(c);printf("%s: %s\n",c.kind.String(),c.text.String());
        BMessage bitmapArchive;if(payload.FindMessage("image/bitmap",&bitmapArchive)==B_OK){BBitmap b(&bitmapArchive);printf("bitmap %.0fx%.0f valid=%d\n",b.Bounds().Width()+1,b.Bounds().Height()+1,b.InitCheck()==B_OK);}
        return 0;
    }
    if(argc>1 && strcmp(argv[1],"--deskbar")==0) { printf("Deskbar Clipper=%d\n",BDeskbar().HasItem("Clipper")); return 0; }
    auto window=new BWindow(BRect(150,150,750,520),"Clipper test pad",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS|B_QUIT_ON_WINDOW_CLOSE);
    auto pad=new Pad; auto scroll=new BScrollView("scroll",pad,0,false,true);
    BLayoutBuilder::Group<>(window,B_VERTICAL,8).SetInsets(12).Add(new BStringView("help","Use normal Alt+C / Alt+X / Alt+V here. Paste events are logged under /tmp." )).Add(scroll);
    pad->MakeFocus();window->Show();app.Run();return 0;
}
