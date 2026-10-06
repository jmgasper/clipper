#include "Protocol.h"
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Deskbar.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <image.h>
#include <cmath>
namespace clipper {
class DeskbarView : public BView {
public:
    DeskbarView(BRect frame) : BView(frame,"Clipper",B_FOLLOW_NONE,B_WILL_DRAW) { InitIcon(); }
    DeskbarView(BMessage* archive) : BView(archive) { InitIcon(); }
    ~DeskbarView() override { delete icon; }
    static BArchivable* Instantiate(BMessage* archive);
    status_t Archive(BMessage* archive,bool deep=true) const override {
        status_t status=BView::Archive(archive,deep);
        if(status==B_OK) status=archive->AddString("add_on",kSignature);
        if(status==B_OK) status=archive->AddString("class","clipper::DeskbarView");
        return status;
    }
    void AttachedToWindow() override { AdoptParentColors(); }
    void Draw(BRect) override {
        if (!icon) return;
        PushState();
        SetDrawingMode(B_OP_ALPHA);
        SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
        DrawBitmap(icon, BPoint(0, 0));
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
private:
    void InitIcon() {
        SetToolTip("Clipper — clipboard history\nHold Alt+V or press Shift+Alt+V");
        // This view runs inside Deskbar: load the resource from the image that
        // contains our code, not Deskbar's own application resources.
        image_info image;
        int32 cookie = 0;
        addr_t address = reinterpret_cast<addr_t>(&DeskbarView::Instantiate);
        while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
            if (address < reinterpret_cast<addr_t>(image.text)
                || address >= reinterpret_cast<addr_t>(image.text) + image.text_size) continue;
            BFile file(image.name, B_READ_ONLY);
            BAppFileInfo info(&file);
            int32 size = Bounds().IntegerHeight() + 1;
            icon = new BBitmap(BRect(0, 0, size-1, size-1), B_RGBA32);
            if (icon->InitCheck() != B_OK || info.GetIcon(icon, static_cast<icon_size>(size)) != B_OK) {
                delete icon; icon = nullptr;
            }
            break;
        }
    }
    BBitmap* icon = nullptr;
};
BArchivable* DeskbarView::Instantiate(BMessage* archive) {
    return validate_instantiation(archive,"clipper::DeskbarView") ? new DeskbarView(archive) : nullptr;
}
}
extern "C" _EXPORT BView* instantiate_deskbar_item(float,float maxHeight) {
    float size=floorf(maxHeight>0?maxHeight:20); return new clipper::DeskbarView(BRect(0,0,size-1,size-1));
}
