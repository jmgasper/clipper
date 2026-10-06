#include "HistoryWindow.h"
#include "Protocol.h"
#include <Application.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <CardLayout.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <DataIO.h>
#include <Screen.h>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <cstdlib>
namespace clipper {
static BBitmap* Decode(const BMessage& data) {
    BMessage archive;
    if (data.FindMessage("image/bitmap", &archive) == B_OK) {
        auto bitmap = new BBitmap(&archive);
        if (bitmap->InitCheck() == B_OK) return bitmap;
        delete bitmap;
    }
    char* name; type_code type; int32 count;
    for (int32 i=0; data.GetInfo(B_ANY_TYPE, i, &name, &type, &count)==B_OK; ++i) {
        if (strncmp(name, "image/", 6) != 0) continue;
        const void* bytes; ssize_t length;
        if (data.FindData(name, type, &bytes, &length) != B_OK) continue;
        BMemoryIO stream(bytes, length);
        if (auto bitmap = BTranslationUtils::GetBitmap(&stream)) return bitmap;
    }
    return nullptr;
}
class ImageView : public BView {
public:
    ImageView() : BView("image preview", B_WILL_DRAW) {
        SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR); SetExplicitMinSize(BSize(80, 145));
    }
    ~ImageView() override { delete bitmap; }
    void Set(BBitmap* value) { delete bitmap; bitmap = value; Invalidate(); }
    void Draw(BRect) override {
        if (!bitmap) {
            SetHighUIColor(B_DOCUMENT_TEXT_COLOR); DrawString("Image preview", BPoint(12, 25)); return;
        }
        BRect source = bitmap->Bounds(); BRect target = Bounds().InsetByCopy(10, 10);
        if (!target.IsValid()) return;
        float scale = std::min((target.Width()+1) / (source.Width()+1),
            (target.Height()+1) / (source.Height()+1));
        float w = (source.Width()+1) * scale, h = (source.Height()+1) * scale;
        BRect dest(target.left + (target.Width()+1-w)/2, target.top + (target.Height()+1-h)/2, 0, 0);
        dest.right = dest.left+w-1; dest.bottom = dest.top+h-1;
        SetDrawingMode(B_OP_ALPHA); SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
        DrawBitmap(bitmap, source, dest, B_FILTER_BITMAP_BILINEAR); SetDrawingMode(B_OP_COPY);
    }
private: BBitmap* bitmap = nullptr;
};
class ClipItem : public BListItem {
public:
    ClipItem(const Clip& c) : id(c.id), pinned(c.pinned), title(c.text), subtitle(c.source) {
        title.ReplaceAll("\r", " "); title.ReplaceAll("\n", "  "); title.ReplaceAll("\t", " ");
        if (title.Length() > 600) title.Truncate(600);
        time_t stamp = c.time / 1000000; tm local; localtime_r(&stamp, &local);
        char date[80]; strftime(date, sizeof(date), "%b %d, %H:%M", &local);
        subtitle << "  ·  " << c.kind << "  ·  " << date;
        if (c.pinned) subtitle.Prepend("Pinned  ·  ");
        if (c.kind.FindFirst("Image") == 0) {
            BBitmap* original = Decode(c.data);
            if (original) {
                // Cache small thumbnails so search doesn't retain full-size bitmaps.
                thumbnail = new BBitmap(BRect(0,0,47,47), B_RGBA32, true);
                if (thumbnail->InitCheck() == B_OK && thumbnail->Lock()) {
                    auto canvas = new BView(thumbnail->Bounds(), "thumb", B_FOLLOW_NONE, B_WILL_DRAW);
                    thumbnail->AddChild(canvas); canvas->SetHighColor(240,240,240); canvas->FillRect(canvas->Bounds());
                    BRect b = original->Bounds(); float s = std::min(46/(b.Width()+1),46/(b.Height()+1));
                    float w=(b.Width()+1)*s,h=(b.Height()+1)*s;
                    canvas->SetDrawingMode(B_OP_ALPHA); canvas->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
                    canvas->DrawBitmap(original,b,BRect((48-w)/2,(48-h)/2,(48+w)/2-1,(48+h)/2-1),B_FILTER_BITMAP_BILINEAR);
                    canvas->Sync(); thumbnail->RemoveChild(canvas); delete canvas; thumbnail->Unlock();
                } else { delete thumbnail; thumbnail=nullptr; }
                delete original;
            }
        }
    }
    ~ClipItem() override { delete thumbnail; }
    void Update(BView* view, const BFont* font) override {
        BListItem::Update(view,font); font_height h; font->GetHeight(&h);
        SetHeight(std::max(64.0f, 2*(h.ascent+h.descent+h.leading)+18));
    }
    void DrawItem(BView* owner, BRect frame, bool) override {
        owner->SetHighUIColor(IsSelected()?B_LIST_SELECTED_BACKGROUND_COLOR:B_LIST_BACKGROUND_COLOR);
        owner->FillRect(frame);
        owner->SetHighUIColor(IsSelected()?B_LIST_SELECTED_ITEM_TEXT_COLOR:B_LIST_ITEM_TEXT_COLOR);
        float x=frame.left+12;
        if (thumbnail) {
            owner->SetDrawingMode(B_OP_ALPHA); owner->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
            owner->DrawBitmap(thumbnail,BPoint(x,frame.top+8)); owner->SetDrawingMode(B_OP_COPY); x+=60;
        }
        BString label(title); owner->TruncateString(&label,B_TRUNCATE_END,frame.right-x-12);
        font_height h; owner->GetFontHeight(&h);
        owner->DrawString(label.String(),BPoint(x,frame.top+10+h.ascent));
        BFont small; owner->GetFont(&small); small.SetSize(small.Size()*0.84f); owner->SetFont(&small);
        label=subtitle; owner->TruncateString(&label,B_TRUNCATE_END,frame.right-x-12);
        owner->DrawString(label.String(),BPoint(x,frame.top+14+2*h.ascent+h.descent));
        owner->SetFont(be_plain_font);
        owner->SetHighUIColor(B_PANEL_BACKGROUND_COLOR); owner->StrokeLine(frame.LeftBottom(),frame.RightBottom());
    }
    int64 id; bool pinned;
private: BString title,subtitle; BBitmap* thumbnail = nullptr;
};
HistoryWindow::HistoryWindow() : BWindow(BRect(160,100,720,770), "Clipper", B_TITLED_WINDOW,
    B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE) {
    search = new BTextControl("search", "Search:", "", new BMessage(kSearch));
    search->SetModificationMessage(new BMessage(kSearch)); search->TextView()->SetExplicitMinSize(BSize(250, B_SIZE_UNSET));
    search->TextView()->SetToolTip("Search clipboard text, type, or source. Fuzzy matching is supported.");
    auto heading=new BStringView("heading","Clipboard history"); heading->SetFont(be_bold_font);
    auto help = new BStringView("help", "Tap Alt+V to paste · hold Alt+V for history");
    list = new BListView("clips", B_SINGLE_SELECTION_LIST); list->SetSelectionMessage(new BMessage(kSelected));
    list->SetInvocationMessage(new BMessage(kChoose)); list->SetExplicitMinSize(BSize(440,280));
    auto scroll=new BScrollView("history",list,0,false,true,B_FANCY_BORDER);
    text = new BTextView("text preview"); text->MakeEditable(false); text->SetWordWrap(true);
    text->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR); text->SetHighUIColor(B_DOCUMENT_TEXT_COLOR);
    auto preview = new BScrollView("preview",text,0,false,true,B_FANCY_BORDER);
    preview->SetExplicitMinSize(BSize(100,120)); preview->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,160));
    image = new ImageView();
    auto deck = new BView("preview deck", 0); previewLayout = new BCardLayout(); deck->SetLayout(previewLayout);
    previewLayout->AddView(preview); previewLayout->AddView(image);
    deck->SetExplicitMinSize(BSize(100,145)); deck->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,160));
    paste=new BButton("paste","Paste",new BMessage(kForcePaste));
    copy=new BButton("copy","Copy",new BMessage(kCopyOnly));
    pin=new BButton("pin","Pin",new BMessage(kPin));
    remove=new BButton("delete","Delete",new BMessage(kDelete));
    auto clear=new BButton("clear","Clear…",new BMessage(kClear));
    auto settings=new BButton("settings","Settings…",new BMessage(kSettings));
    pause=new BButton("pause","Pause recording",new BMessage(kPause));
    status=new BStringView("status","Copy something to get started.");
    status->SetTruncation(B_TRUNCATE_END); status->SetExplicitMinSize(BSize(200,B_SIZE_UNSET));
    status->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,B_SIZE_UNSET));
    BLayoutBuilder::Group<>(this,B_VERTICAL,10).SetInsets(16)
        .AddGroup(B_HORIZONTAL).Add(heading).AddGlue().Add(settings).End()
        .Add(help).Add(search).Add(scroll)
        .Add(deck)
        .AddGroup(B_HORIZONTAL,8).Add(paste).Add(copy).Add(pin).Add(remove).AddGlue().Add(clear).End()
        .AddGroup(B_HORIZONTAL,8).Add(status).AddGlue().Add(pause).End();
    SetDefaultButton(paste); CenterOnScreen();
}
void HistoryWindow::Dismiss() {
    // BWindow counts Hide/Show calls. Repeated dismissal of this popup must
    // not require multiple history shortcuts to make it visible again.
    if (!IsHidden()) Hide();
}
bool HistoryWindow::QuitRequested() { Dismiss(); return false; }
void HistoryWindow::Open() {
    search->SetText(""); Rebuild();
    if(!clips.empty()) for(int32 i=0;i<list->CountItems();++i) {
        auto item=static_cast<ClipItem*>(list->ItemAt(i));
        if(item->id==clips.front().id) { list->Select(i); break; }
    }
    if (IsHidden()) Show();
    Activate(true); search->MakeFocus(true);
}
void HistoryWindow::Update(const History& history, bool paused, const char* notice) {
    clips=history.clips;
    pause->SetLabel(paused ? "Resume recording" : "Pause recording");
    BString line; line.SetToFormat("%zu clips%s",clips.size(),paused?" · paused":"");
    if (notice && *notice) { line << " · " << notice; }
    status->SetText(line); status->SetToolTip(line.String());
    if(!IsHidden()) Rebuild();
}
void HistoryWindow::Rebuild() {
    int64 selected=0; if (auto item=dynamic_cast<ClipItem*>(list->ItemAt(list->CurrentSelection()))) selected=item->id;
    while (auto item=list->RemoveItem(int32(0))) delete item;
    BString query(search->Text());
    int32 index=-1;
    for (int group=0; group<2; ++group) for (const auto& clip:clips) {
        if (clip.pinned != (group==0)) continue;
        BString haystack(clip.text); haystack << " " << clip.source << " " << clip.kind;
        if (!Matches(haystack,query)) continue;
        if (clip.id==selected) index=list->CountItems();
        list->AddItem(new ClipItem(clip));
    }
    if (list->CountItems()>0) list->Select(index>=0?index:0);
    Selection();
}
void HistoryWindow::Selection() {
    auto item=dynamic_cast<ClipItem*>(list->ItemAt(list->CurrentSelection()));
    paste->SetEnabled(item); copy->SetEnabled(item); pin->SetEnabled(item); remove->SetEnabled(item);
    pin->SetLabel(item && item->pinned?"Unpin":"Pin");
    const Clip* clip=nullptr; if (item) for (const auto& c:clips) if(c.id==item->id) { clip=&c; break; }
    BBitmap* bitmap=clip && clip->kind.FindFirst("Image")==0?Decode(clip->data):nullptr;
    image->Set(bitmap);
    previewLayout->SetVisibleItem(bitmap ? 1 : 0);
    text->SetText(clip ? clip->text.String() : clips.empty()
        ? "Copy text or an image in any app to start your history."
        : "No matching clips. Try a different search.");
}
void HistoryWindow::Send(uint32 what) {
    auto item=dynamic_cast<ClipItem*>(list->ItemAt(list->CurrentSelection())); if (!item) return;
    BMessage message(what); message.AddInt64("id",item->id); be_app_messenger.SendMessage(&message);
}
void HistoryWindow::MessageReceived(BMessage* msg) {
    switch(msg->what) {
        case kSearch: Rebuild(); break;
        case kSelected: Selection(); break;
        case kChoose: case kForcePaste: case kCopyOnly: case kPin: case kDelete: Send(msg->what); break;
        case kClear: {
            auto alert=new BAlert("Clear history","Clear clipboard history? Pinned clips can be kept.","Cancel","Keep pinned","Clear all",B_WIDTH_AS_USUAL,B_WARNING_ALERT);
            int32 choice=alert->Go(); if(choice==0) break;
            BMessage request(kClear); request.AddBool("all",choice==2); be_app_messenger.SendMessage(&request); break;
        }
        case kPause: case kSettings: be_app_messenger.SendMessage(msg); break;
        default: BWindow::MessageReceived(msg);
    }
}
void HistoryWindow::DispatchMessage(BMessage* msg, BHandler* target) {
    if(msg->what==B_KEY_DOWN) {
        const char* bytes=msg->GetString("bytes","");
        if(bytes[0]==B_ESCAPE) { Dismiss(); return; }
        bool navigating = target==list || target==search || target==search->TextView();
        if(navigating && bytes[0]==B_ENTER) { Send(kChoose); return; }
        if(navigating && (bytes[0]==B_UP_ARROW || bytes[0]==B_DOWN_ARROW)) {
            int32 count=list->CountItems(); if(count>0) {
                int32 selected=list->CurrentSelection()+(bytes[0]==B_DOWN_ARROW?1:-1);
                list->Select(std::max(int32(0),std::min(count-1,selected))); list->ScrollToSelection();
            } return;
        }
        if(target==list && bytes[0]==B_DELETE) { Send(kDelete); return; }
        if(target==list && static_cast<unsigned char>(bytes[0])>=32
            && !(msg->GetInt32("modifiers",0)&(B_COMMAND_KEY|B_CONTROL_KEY))) {
            search->MakeFocus(true); search->TextView()->Insert(bytes); Rebuild(); return;
        }
    }
    BWindow::DispatchMessage(msg,target);
}
SettingsWindow::SettingsWindow(int32 cap,bool remember,bool autoValue):BWindow(BRect(0,0,430,220),"Clipper settings",B_TITLED_WINDOW,
    B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_CLOSE_ON_ESCAPE) {
    char number[16]; snprintf(number,sizeof(number),"%d",cap);
    limit=new BTextControl("limit","History limit (10–1000):",number,nullptr);
    persist=new BCheckBox("persist","Remember history between restarts",nullptr); persist->SetValue(remember);
    autoPaste=new BCheckBox("auto","Paste into the previous app when selecting a clip",nullptr); autoPaste->SetValue(autoValue);
    auto info=new BStringView("info","Images and text are saved locally, up to 128 MiB.");
    auto save=new BButton("save","Save",new BMessage(kSaveSettings));
    auto cancel=new BButton("cancel","Cancel",new BMessage(B_QUIT_REQUESTED));
    BLayoutBuilder::Group<>(this,B_VERTICAL,12).SetInsets(18).Add(limit).Add(persist).Add(autoPaste).Add(info)
        .AddGroup(B_HORIZONTAL).AddGlue().Add(cancel).Add(save).End(); SetDefaultButton(save); CenterOnScreen();
    limit->MakeFocus(true); limit->TextView()->SelectAll();
}
void SettingsWindow::MessageReceived(BMessage* msg) {
    if(msg->what!=kSaveSettings) { BWindow::MessageReceived(msg); return; }
    char* end; long cap=strtol(limit->Text(),&end,10);
    if(*end || cap<10 || cap>1000) { (new BAlert("limit","Enter a history limit from 10 to 1000.","OK"))->Go(); return; }
    BMessage settings(kSaveSettings); settings.AddInt32("limit",cap); settings.AddBool("persist",persist->Value());
    settings.AddBool("auto_paste",autoPaste->Value()); be_app_messenger.SendMessage(&settings); PostMessage(B_QUIT_REQUESTED);
}
}
