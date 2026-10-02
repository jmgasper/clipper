#pragma once
#include <Window.h>
#include <Messenger.h>
#include "History.h"
class BCardLayout; class BListView; class BTextControl; class BStringView; class BTextView; class BButton; class BCheckBox;
namespace clipper {
class ImageView;
class HistoryWindow : public BWindow {
public:
    HistoryWindow();
    void Update(const History& history, bool paused, const char* notice = "");
    void Open();
    bool QuitRequested() override;
    void MessageReceived(BMessage* msg) override;
    void DispatchMessage(BMessage* msg, BHandler* target) override;
private:
    void Rebuild();
    void Selection();
    void Send(uint32 what);
    std::vector<Clip> clips;
    BTextControl* search; BListView* list; BStringView* status;
    BTextView* text; ImageView* image; BCardLayout* previewLayout;
    BButton* pin; BButton* paste; BButton* copy; BButton* remove;
};
class SettingsWindow : public BWindow {
public:
    SettingsWindow(int32 limit, bool persist, bool autoPaste);
    void MessageReceived(BMessage* msg) override;
private:
    BTextControl* limit; BCheckBox* persist; BCheckBox* autoPaste;
};
}
