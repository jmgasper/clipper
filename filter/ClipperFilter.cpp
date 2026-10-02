// Never sends IPC or launches applications on the input thread.
#include "Protocol.h"
#include <add-ons/input_server/InputServerFilter.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Locker.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Roster.h>
#include <atomic>
using namespace clipper;
namespace {
constexpr uint32 kHold = 'clHL';
class Dispatcher : public BLooper {
public:
    Dispatcher() : BLooper("Clipper shortcut dispatcher") { Run(); }
    BLocker guard;
    bool down = false, shown = false;
    int64 generation = 0;
    void MessageReceived(BMessage* msg) override {
        if (msg->what == kHold) {
            guard.Lock();
            bool show = down && generation == msg->GetInt64("generation", -1);
            if (show) shown = true;
            guard.Unlock();
            if (show) Show();
        } else if (msg->what == kShow) Show();
        else BLooper::MessageReceived(msg);
    }
    void Show() {
        BMessage message(kShow); message.AddBool("toggle", true);
        BMessenger app(kSignature);
        if (app.IsValid()) app.SendMessage(&message, static_cast<BHandler*>(nullptr), 100000);
        else { char* args[] = {const_cast<char*>("--history")}; be_roster->Launch(kSignature, 1, args); }
    }
};
class ClipperFilter : public BInputServerFilter {
public:
    ClipperFilter() : dispatcher(new Dispatcher) {}
    status_t InitCheck() override { return B_OK; }
    ~ClipperFilter() override { if (dispatcher->Lock()) dispatcher->Quit(); }
    filter_result Filter(BMessage* message, BList* out) override {
        if (message->GetBool("clipper:injected", false)) return B_DISPATCH_MESSAGE;
        int32 key = message->GetInt32("key", -1);
        uint32 mods = message->GetInt32("modifiers", 0);
        // Haiku maps the physical Alt key to B_COMMAND_KEY. raw_char follows the keymap.
        int32 rawChar = message->GetInt32("raw_char", -1);
        bool isV = rawChar == 'v' || rawChar == 'V' || (rawChar < 0 && key == 0x56);
        if (message->what == B_KEY_DOWN && isV && (mods & B_COMMAND_KEY)
            && !(mods & (B_CONTROL_KEY | B_OPTION_KEY))) {
            dispatcher->guard.Lock();
            if (dispatcher->down) { dispatcher->guard.Unlock(); return B_SKIP_MESSAGE; }
            dispatcher->down = true; dispatcher->shown = false;
            ++dispatcher->generation; saved = *message; savedKey = key;
            BMessage hold(kHold); hold.AddInt64("generation", dispatcher->generation);
            dispatcher->guard.Unlock();
            // Shift+Alt+V opens immediately, useful for accessibility and remote input.
            if (mods & B_SHIFT_KEY) {
                dispatcher->guard.Lock(); dispatcher->shown = true; dispatcher->guard.Unlock();
                dispatcher->PostMessage(kShow);
            } else BMessageRunner::StartSending(BMessenger(dispatcher), &hold, 350000, 1);
            return B_SKIP_MESSAGE;
        }
        if (message->what == B_KEY_UP && key == savedKey) {
            dispatcher->guard.Lock();
            bool down = dispatcher->down, shown = dispatcher->shown;
            dispatcher->down = false; ++dispatcher->generation; savedKey = -1;
            dispatcher->guard.Unlock();
            if (down && !shown) {
                // Replay one ordinary paste on a tap. Other filters and app_server receive it.
                auto press = new BMessage(saved); press->ReplaceInt64("when", system_time());
                out->AddItem(press); out->AddItem(new BMessage(*message));
                return B_DISPATCH_MESSAGE;
            }
            if (down) return B_SKIP_MESSAGE;
        }
        return B_DISPATCH_MESSAGE;
    }
private:
    Dispatcher* dispatcher; BMessage saved; int32 savedKey = -1;
};
}
extern "C" _EXPORT BInputServerFilter* instantiate_input_filter() { return new ClipperFilter; }
