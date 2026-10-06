#include "HistoryWindow.h"
#include "Protocol.h"
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <ListView.h>
#include <TextControl.h>
#include <TextView.h>
#include <atomic>
#include <cstdio>
#include <cstring>
using namespace clipper;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)
class TestWindow : public HistoryWindow {
public:
    std::atomic<bool> settingsInvoked{false};
    void MessageReceived(BMessage* message) override {
        if (message->what == kSettings) settingsInvoked = true;
        else HistoryWindow::MessageReceived(message);
    }
};
int main()
{
    BApplication app("application/x-vnd.airOS-Clipper-UITests");
    History history;
    for (const char* sample : {"UI review: first clip", "UI review: second clip"}) {
        BMessage data; data.AddData("text/plain", B_MIME_TYPE, sample, strlen(sample));
        history.Capture(data, "UI test");
    }
    auto* window = new TestWindow;
    window->Run();
    window->Lock();
    window->Update(history, false);
    window->Open(); window->Unlock(); snooze(100000); window->Lock();
    window->Dismiss(); window->Dismiss(); window->Open();
    CHECK(!window->IsHidden());
    auto* search = dynamic_cast<BTextControl*>(window->FindView("search"));
    auto* list = dynamic_cast<BListView*>(window->FindView("clips"));
    auto* settings = dynamic_cast<BButton*>(window->FindView("settings"));
    auto* pause = dynamic_cast<BButton*>(window->FindView("pause"));
    CHECK(search && list && settings && pause);
    if (search && list && settings && pause) {
        BMessage down(B_KEY_DOWN); char bytes[] = {B_DOWN_ARROW, 0}; down.AddString("bytes", bytes);
        window->DispatchMessage(&down, search->TextView());
        CHECK(list->CurrentSelection() == 1);
        BMessage enter(B_KEY_DOWN); enter.AddString("bytes", "\n");
        settings->MakeFocus();
        window->DispatchMessage(&enter, settings);
        window->Unlock(); snooze(100000); window->Lock();
        CHECK(window->settingsInvoked);
        window->Update(history, true);
        CHECK(strcmp(pause->Label(), "Resume recording") == 0);
        // Single-pixel dimensions must produce finite image preview geometry.
        BBitmap bitmap(BRect(0, 0, 0, 7), B_RGBA32);
        memset(bitmap.Bits(), 255, bitmap.BitsLength());
        BMessage archive, data; bitmap.Archive(&archive); data.AddMessage("image/bitmap", &archive);
        history.Capture(data, "UI test");
        window->Update(history, false);
        window->Open(); window->UpdateIfNeeded();
        CHECK(strcmp(pause->Label(), "Pause recording") == 0);
    }
    window->Quit();
    auto* preferences = new SettingsWindow(100, true, true);
    preferences->Show(); snooze(100000); preferences->Lock();
    CHECK(preferences->FindView("cancel") != nullptr);
    CHECK(preferences->CurrentFocus() == dynamic_cast<BTextControl*>(preferences->FindView("limit"))->TextView());
    preferences->Quit();
    std::printf("UI tests: %d failures\n", failures);
    return failures ? 1 : 0;
}
