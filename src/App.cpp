#include "History.h"
#include "HistoryWindow.h"
#include "Protocol.h"
#include <Application.h>
#include <Alert.h>
#include <Clipboard.h>
#include <Deskbar.h>
#include <Entry.h>
#include <File.h>
#include <Input.h>
#include <MessageRunner.h>
#include <Path.h>
#include <Roster.h>
#include <WindowInfo.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
namespace clipper {
class App : public BApplication {
public:
    App():BApplication(kSignature) {}
    void ArgvReceived(int32 argc,char** argv) override {
        for(int32 i=1;i<argc;++i) {
            if(strcmp(argv[i],"--history")==0) { wantsHistory=true; if(window) Show(false); }
            else if(strcmp(argv[i],"--quit")==0) PostMessage(B_QUIT_REQUESTED);
        }
    }
    void ReadyToRun() override {
        BFile file(SettingsPath("settings").String(),B_READ_ONLY); BMessage settings;
        if(settings.Unflatten(&file)==B_OK) {
            history.limit=std::max(int32(10),std::min(int32(1000),settings.GetInt32("limit",100)));
            persist=settings.GetBool("persist",true); autoPaste=settings.GetBool("auto_paste",true);
        }
        if(persist) {
            status_t loaded=history.Load(SettingsPath("history").String());
            if(loaded!=B_OK && loaded!=B_ENTRY_NOT_FOUND) notice="Saved history could not be read.";
        }
        window=new HistoryWindow; window->Run();
        be_clipboard->StartWatching(be_app_messenger);
        Capture(); Refresh();
        app_info info; GetAppInfo(&info); BDeskbar deskbar;
        if(deskbar.HasItem("Clipper")) deskbar.RemoveItem("Clipper");
        status_t status=deskbar.AddItem(&info.ref);
        if(status!=B_OK) { notice="Could not add the Deskbar icon."; Refresh(); }
        if(wantsHistory) Show(false);
    }
    void MessageReceived(BMessage* msg) override {
        switch(msg->what) {
            case B_CLIPBOARD_CHANGED: Capture(); break;
            case kShow: if(window) Show(msg->GetBool("toggle",false)); else wantsHistory=true; break;
            case kHide: if(window && window->Lock()) { window->Hide(); window->Unlock(); } break;
            case kChoose: Choose(msg->GetInt64("id",0),autoPaste); break;
            case kForcePaste: Choose(msg->GetInt64("id",0),true); break;
            case kCopyOnly: Choose(msg->GetInt64("id",0),false); break;
            case kPin: {
                auto clip=history.Find(msg->GetInt64("id",0)); if(clip) { clip->pinned=!clip->pinned; history.Trim(); Changed(); } break;
            }
            case kDelete: if(history.Remove(msg->GetInt64("id",0))) Changed(); break;
            case kClear: {
                bool all=msg->GetBool("all",false);
                history.clips.erase(std::remove_if(history.clips.begin(),history.clips.end(),[all](const Clip& c){ return all||!c.pinned; }),history.clips.end());
                Changed(); break;
            }
            case kPause: paused=!paused; Refresh(); break;
            case kSave: saveQueued=false; Save(); break;
            case kPasteReady: PasteReady(msg); break;
            case kSettings: {
                BMessenger existing("application/x-vnd.airOS-Clipper");
                // Settings is a transient native window; one at a time.
                for(int32 i=0;auto candidate=WindowAt(i);++i) if(strcmp(candidate->Title(),"Clipper settings")==0) {
                    if(candidate->Lock()) { candidate->Activate(); candidate->Unlock(); } return;
                }
                (new SettingsWindow(history.limit,persist,autoPaste))->Show(); break;
            }
            case kSaveSettings: {
                history.limit=std::max(int32(10),std::min(int32(1000),msg->GetInt32("limit",100))); persist=msg->GetBool("persist",true); autoPaste=msg->GetBool("auto_paste",true);
                history.Trim(); SaveSettings();
                if(!persist) unlink(SettingsPath("history").String());
                Changed(); break;
            }
            default: BApplication::MessageReceived(msg);
        }
    }
    bool QuitRequested() override {
        be_clipboard->StopWatching(be_app_messenger); Save(); BDeskbar().RemoveItem("Clipper");
        if(window && window->Lock()) { window->Quit(); window=nullptr; }
        return true;
    }
private:
    void Refresh() {
        if(window && window->Lock()) { window->Update(history,paused,notice.String()); window->Unlock(); }
    }
    void Changed() {
        Refresh();
        if(persist && !saveQueued) { saveQueued=true; BMessage save(kSave); BMessageRunner::StartSending(be_app_messenger,&save,500000,1); }
    }
    void Save() {
        if(persist && history.Save(SettingsPath("history").String())!=B_OK) { notice="History could not be saved."; Refresh(); }
    }
    void SaveSettings() {
        BMessage settings; settings.AddInt32("limit",history.limit); settings.AddBool("persist",persist); settings.AddBool("auto_paste",autoPaste);
        BString path=SettingsPath("settings"),temp(path); temp<<".new";
        BFile file(temp.String(),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
        if(settings.Flatten(&file)==B_OK && file.Sync()==B_OK) { chmod(temp.String(),0600); rename(temp.String(),path.String()); }
    }
    void Capture() {
        if(paused || !be_clipboard->Lock()) return;
        BMessage data(*be_clipboard->Data()); uint32 count=be_clipboard->LocalCount();
        team_id sourceTeam=be_clipboard->DataSource().Team(); be_clipboard->Unlock();
        if(count==selfCommit) { selfCommit=0; return; }
        app_info source; BString name("Application");
        if(be_roster->GetRunningAppInfo(sourceTeam,&source)==B_OK) name=source.ref.name;
        if(history.Capture(data,name.String())) { notice=""; Changed(); }
        else if(!history.notice.IsEmpty()) { notice=history.notice; Refresh(); }
    }
    void RememberTarget() {
        targetTeam=-1; targetToken=-1;
        app_info active; if(be_roster->GetActiveAppInfo(&active)==B_OK && active.team!=Team()
            && strcmp(active.signature,"application/x-vnd.Be-TSKB")!=0) targetTeam=active.team;
        int32* tokens=nullptr,count=0;
        if(BPrivate::get_window_order(current_workspace(),&tokens,&count)==B_OK) {
            for(int32 i=0;i<count;++i) {
                auto info=get_window_info(tokens[i]); if(!info) continue;
                bool use=info->team!=Team() && info->feel==B_NORMAL_WINDOW_FEEL && info->show_hide_level==0
                    && !info->is_mini && !(info->flags&B_AVOID_FOCUS);
                if(use && (targetTeam < 0 || info->team == targetTeam)) { targetTeam=info->team; targetToken=tokens[i]; free(info); break; }
                free(info);
            } free(tokens);
        }
    }
    void Show(bool toggle) {
        if(getenv("CLIPPER_TRACE")) { printf("show toggle=%d hidden=%d\n",toggle,window->IsHidden()); fflush(stdout); }
        if(window->Lock()) {
            if(toggle && !window->IsHidden()) window->Hide();
            else { if(window->IsHidden()) RememberTarget(); window->Open(); be_roster->ActivateApp(Team()); }
            window->Unlock();
        }
    }
    void Choose(int64 id,bool paste) {
        Clip* clip=history.Find(id); if(!clip) return;
        if(getenv("CLIPPER_TRACE")) { printf("choose id=%lld paste=%d target=%d token=%d\n",(long long)id,paste,(int)targetTeam,(int)targetToken); fflush(stdout); }
        if(!be_clipboard->Lock()) { notice="Clipboard is busy; try again."; Refresh(); return; }
        be_clipboard->Clear(); *be_clipboard->Data()=clip->data;
        status_t result=be_clipboard->Commit(); selfCommit=be_clipboard->LocalCount(); be_clipboard->Unlock();
        if(result!=B_OK) { notice="Could not restore the clipboard."; Refresh(); return; }
        notice="";
        if(window->Lock()) { window->Hide(); window->Unlock(); }
        if(!paste) return;
        if(targetTeam<0 || targetTeam==Team()) { notice="Copied. Switch to your app and press Alt+V."; Refresh(); return; }
        if(targetToken>=0) do_window_action(targetToken,B_BRING_TO_FRONT,BRect(),false);
        be_roster->ActivateApp(targetTeam);
        ++pasteGeneration; BMessage next(kPasteReady); next.AddInt64("generation",pasteGeneration); next.AddInt32("attempt",0);
        BMessageRunner::StartSending(be_app_messenger,&next,80000,1);
    }
    void PasteReady(BMessage* msg) {
        if(msg->GetInt64("generation",0)!=pasteGeneration) return;
        app_info active;
        if(be_roster->GetActiveAppInfo(&active)!=B_OK || active.team!=targetTeam) {
            notice="Copied. Paste cancelled because the destination changed."; Refresh(); return;
        }
        key_info keys; bool held=get_key_info(&keys)==B_OK && (keys.modifiers&(B_COMMAND_KEY|B_CONTROL_KEY|B_OPTION_KEY|B_SHIFT_KEY));
        status_t result=B_BUSY;
        if(!held) {
            BInputDevice* device=find_input_device(kDevice);
            if(device) { BMessage request(kPaste); result=device->Control(kPaste,&request); delete device; }
            else result=B_NAME_NOT_FOUND;
        }
        int32 attempt=msg->GetInt32("attempt",0);
        if(result==B_BUSY && attempt<30) { msg->ReplaceInt32("attempt",attempt+1); BMessageRunner::StartSending(be_app_messenger,msg,50000,1); return; }
        if(getenv("CLIPPER_TRACE")) { printf("paste result=%d\n",(int)result); fflush(stdout); }
        if(result!=B_OK) { notice="Copied. Press Alt+V in your app to paste (paste bridge unavailable or keys held)."; Refresh(); }
    }
    History history; HistoryWindow* window=nullptr;
    bool paused=false,persist=true,autoPaste=true,wantsHistory=false,saveQueued=false;
    uint32 selfCommit=0; BString notice; team_id targetTeam=-1; int32 targetToken=-1; int64 pasteGeneration=0;
};
}
int main() { clipper::App app; app.Run(); return 0; }
