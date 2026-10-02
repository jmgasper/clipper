#include "Protocol.h"
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Deskbar.h>
#include <cmath>
namespace clipper {
class DeskbarView : public BView {
public:
    DeskbarView(BRect frame) : BView(frame,"Clipper",B_FOLLOW_NONE,B_WILL_DRAW) { SetToolTip("Clipper — clipboard history\nHold Alt+V or press Shift+Alt+V"); }
    DeskbarView(BMessage* archive) : BView(archive) { SetToolTip("Clipper — clipboard history"); }
    static BArchivable* Instantiate(BMessage* archive);
    status_t Archive(BMessage* archive,bool deep=true) const override {
        status_t status=BView::Archive(archive,deep);
        if(status==B_OK) status=archive->AddString("add_on",kSignature);
        if(status==B_OK) status=archive->AddString("class","clipper::DeskbarView");
        return status;
    }
    void AttachedToWindow() override { AdoptParentColors(); }
    void Draw(BRect) override {
        float scale=(Bounds().Height()+1)/20; PushState(); SetScale(scale);
        SetHighColor(31,104,120); FillRoundRect(BRect(3,3,16,18),2,2);
        SetHighColor(231,246,247); FillRoundRect(BRect(5,5,14,16),1,1);
        SetHighColor(25,78,91); FillRoundRect(BRect(7,1,12,6),1,1);
        SetHighColor(59,126,140); StrokeLine(BPoint(7,9),BPoint(12,9)); StrokeLine(BPoint(7,12),BPoint(12,12));
        PopState();
    }
    void MouseDown(BPoint point) override {
        uint32 buttons=Window()->CurrentMessage()->GetInt32("buttons",B_PRIMARY_MOUSE_BUTTON);
        if(buttons&B_SECONDARY_MOUSE_BUTTON) {
            BPopUpMenu menu("Clipper",false,false);
            menu.AddItem(new BMenuItem("Clipboard history",new BMessage(kShow)));
            menu.AddItem(new BMenuItem("Pause / resume recording",new BMessage(kPause)));
            menu.AddItem(new BMenuItem("Settings…",new BMessage(kSettings)));
            menu.AddSeparatorItem(); menu.AddItem(new BMenuItem("Quit Clipper",new BMessage(B_QUIT_REQUESTED)));
            auto item=menu.Go(ConvertToScreen(point)); if(item) Send(item->Message()->what);
        } else Send(kShow);
    }
    void Send(uint32 what) {
        BMessenger app(kSignature); BMessage msg(what);
        if(app.IsValid()) app.SendMessage(&msg);
        else be_roster->Launch(kSignature,&msg);
    }
};
BArchivable* DeskbarView::Instantiate(BMessage* archive) {
    return validate_instantiation(archive,"clipper::DeskbarView") ? new DeskbarView(archive) : nullptr;
}
}
extern "C" _EXPORT BView* instantiate_deskbar_item(float,float maxHeight) {
    float size=floorf(maxHeight>0?maxHeight:20); return new clipper::DeskbarView(BRect(0,0,size-1,size-1));
}
