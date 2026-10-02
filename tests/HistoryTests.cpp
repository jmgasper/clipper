#include "History.h"
#include "Protocol.h"
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
using namespace clipper;
static BMessage Text(const char* text) { BMessage data; data.AddData("text/plain",B_MIME_TYPE,text,strlen(text)); return data; }
int main() {
    BApplication app("application/x-vnd.airOS-Clipper-Tests");
    History h; h.limit=3;
    assert(h.Capture(Text("first"),"Test")); int64 first=h.clips[0].id;
    assert(h.Capture(Text("second"),"Test"));
    assert(h.Capture(Text("first"),"Test")); assert(h.clips.size()==2 && h.clips[0].id==first);
    h.Find(first)->pinned=true;
    assert(h.Capture(Text("third"),"Test")); assert(h.Capture(Text("fourth"),"Test"));
    assert(h.clips.size()==3 && h.Find(first)); assert(h.clips[0].text=="fourth");
    assert(Matches("Clipboard Search Example","cbs ex")); assert(Matches("HaikuOS","HAIK")); assert(!Matches("image","xyz"));
    BMessage rich=Text("styled"); rich.AddData("text/html",B_MIME_TYPE,"<b>styled</b>",13);
    assert(h.Capture(rich,"Styled")); assert(h.clips[0].data.HasData("text/html",B_MIME_TYPE));
    BBitmap bitmap(BRect(0,0,7,7),B_RGBA32); memset(bitmap.Bits(),0x80,bitmap.BitsLength());
    BMessage archive,image; bitmap.Archive(&archive); image.AddMessage("image/bitmap",&archive);
    assert(h.Capture(image,"ImageTest")); assert(h.clips[0].kind.FindFirst("Image")==0);
    char path[]="/tmp/clipper-history-test-XXXXXX"; int fd=mkstemp(path); assert(fd>=0); close(fd);
    assert(h.Save(path)==B_OK); History restored; restored.limit=3; assert(restored.Load(path)==B_OK);
    assert(restored.clips.size()==h.clips.size()); assert(restored.Find(first)->pinned);
    BMessage restoredArchive; assert(restored.clips[0].data.FindMessage("image/bitmap",&restoredArchive)==B_OK);
    BBitmap restoredBitmap(&restoredArchive); assert(restoredBitmap.InitCheck()==B_OK);
    assert(restoredBitmap.BitsLength()==bitmap.BitsLength()); assert(memcmp(restoredBitmap.Bits(),bitmap.Bits(),bitmap.BitsLength())==0);
    BFile bad(path,B_WRITE_ONLY|B_ERASE_FILE); bad.Write("invalid",7); bad.Unset();
    assert(restored.Load(path)!=B_OK); unlink(path);
    assert(h.Remove(first)); assert(!h.Find(first));
    BMessage empty; assert(!h.Capture(empty,"Empty"));
    BMessage large; std::vector<char> huge(kMaxEntryBytes+1,'a'); large.AddData("text/plain",B_MIME_TYPE,huge.data(),huge.size());
    assert(!h.Capture(large,"Large")); assert(!h.notice.IsEmpty());
    puts("PASS: deduplication, ordering, pins, eviction, search, rich text, bitmap byte roundtrip, atomic persistence, corruption and size limits.");
}
