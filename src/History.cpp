#include "History.h"
#include "Protocol.h"
#include <Bitmap.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Directory.h>
#include <OS.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
namespace clipper {
BString SettingsPath(const char* leaf) {
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return BString();
    path.Append("Clipper");
    create_directory(path.Path(), 0700);
    chmod(path.Path(), 0700);
    path.Append(leaf);
    return BString(path.Path());
}
bool Matches(const BString& value, const BString& query) {
    BString text(value), needle(query); text.ToLower(); needle.ToLower();
    int32 at = 0;
    for (int32 i = 0; i < needle.Length(); ++i) {
        if (needle[i] == ' ') continue;
        at = text.FindFirst(needle[i], at);
        if (at < 0) return false;
        ++at;
    }
    return true;
}
bool History::Describe(Clip& clip) {
    const void* raw; ssize_t size;
    clip.text = ""; clip.kind = "";
    if (clip.data.FindData("text/plain", B_MIME_TYPE, &raw, &size) == B_OK && size > 0) {
        clip.text.SetTo(static_cast<const char*>(raw), std::min<ssize_t>(size, 1024 * 1024));
        clip.kind = "Text";
    }
    BMessage bitmap;
    if (clip.data.FindMessage("image/bitmap", &bitmap) == B_OK) {
        clip.kind = "Image";
        BRect bounds;
        if (bitmap.FindRect("_frame", &bounds) == B_OK) {
            clip.kind.SetToFormat("Image · %.0f × %.0f", bounds.Width()+1, bounds.Height()+1);
        }
        if (clip.text.IsEmpty()) clip.text = clip.kind;
        return true;
    }
    char* name; type_code type; int32 count;
    for (int32 i = 0; clip.data.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; ++i) {
        if (strncmp(name, "image/", 6) == 0 || strcmp(name, "text/html") == 0 || strcmp(name, "text/rtf") == 0) {
            clip.kind = strncmp(name, "image/", 6) == 0 ? "Image" : "Rich text";
            if (clip.text.IsEmpty()) clip.text = name;
            return true;
        }
    }
    if (!clip.kind.IsEmpty()) return true;
    entry_ref ref;
    if (clip.data.FindRef("refs", &ref) == B_OK) {
        clip.kind = "Files"; clip.text = ref.name; return true;
    }
    return false;
}
static bool Equal(const BMessage& a, const BMessage& b) {
    ssize_t size = a.FlattenedSize();
    if (size != b.FlattenedSize()) return false;
    std::vector<char> left(size), right(size);
    return a.Flatten(left.data(), size) == B_OK && b.Flatten(right.data(), size) == B_OK
        && left == right;
}
bool History::Capture(const BMessage& data, const char* source) {
    notice = "";
    if (data.FlattenedSize() > kMaxEntryBytes) {
        notice = "Clipboard item exceeds 32 MiB; it can still be pasted normally."; return false;
    }
    Clip clip; clip.data = data; clip.data.what = 0;
    if (!Describe(clip)) return false;
    for (auto it = clips.begin(); it != clips.end(); ++it) {
        if (!Equal(it->data, clip.data)) continue;
        clip.id = it->id; clip.pinned = it->pinned; clips.erase(it); break;
    }
    if (clip.id == 0) clip.id = real_time_clock_usecs();
    while (Find(clip.id)) ++clip.id;
    clip.time = real_time_clock_usecs(); clip.source = source ? source : "Application";
    clips.insert(clips.begin(), clip); Trim();
    return true;
}
Clip* History::Find(int64 id) {
    for (auto& clip : clips) if (clip.id == id) return &clip;
    return nullptr;
}
bool History::Remove(int64 id) {
    auto it = std::find_if(clips.begin(), clips.end(), [id](const Clip& c) { return c.id == id; });
    if (it == clips.end()) return false;
    clips.erase(it); return true;
}
void History::Trim() {
    ssize_t bytes = 0; for (const auto& c : clips) bytes += c.data.FlattenedSize();
    while (clips.size() > static_cast<size_t>(limit) || bytes > kMaxHistoryBytes) {
        auto it = std::find_if(clips.rbegin(), clips.rend(), [](const Clip& c) { return !c.pinned; });
        if (it == clips.rend()) break;
        bytes -= it->data.FlattenedSize(); clips.erase(std::next(it).base());
    }
}
status_t History::Save(const char* path) const {
    BMessage archive('clH1'); archive.AddInt32("version", 1);
    for (const auto& c : clips) {
        BMessage row; row.AddInt64("id", c.id); row.AddInt64("time", c.time);
        row.AddBool("pinned", c.pinned); row.AddString("source", c.source);
        row.AddMessage("data", &c.data); archive.AddMessage("clip", &row);
    }
    BString temp(path); temp << ".new";
    BFile file(temp.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    status_t status = file.InitCheck();
    if (status == B_OK) status = archive.Flatten(&file);
    if (status == B_OK) status = file.Sync();
    chmod(temp.String(), 0600);
    if (status == B_OK && rename(temp.String(), path) != 0) status = B_IO_ERROR;
    if (status != B_OK) unlink(temp.String());
    return status;
}
status_t History::Load(const char* path) {
    BFile file(path, B_READ_ONLY); status_t status = file.InitCheck();
    if (status != B_OK) return status;
    off_t size; if (file.GetSize(&size) != B_OK || size > kMaxHistoryBytes + 1024 * 1024) return B_BAD_DATA;
    BMessage archive; status = archive.Unflatten(&file);
    if (status != B_OK || archive.GetInt32("version", 0) != 1) return B_BAD_DATA;
    clips.clear();
    BMessage row;
    for (int32 i = 0; archive.FindMessage("clip", i, &row) == B_OK; ++i) {
        Clip c; c.id = row.GetInt64("id", 0); c.time = row.GetInt64("time", 0);
        c.pinned = row.GetBool("pinned", false); c.source = row.GetString("source", "Application");
        if (c.id <= 0 || Find(c.id) || row.FindMessage("data", &c.data) != B_OK
            || c.data.FlattenedSize() > kMaxEntryBytes || !Describe(c)) continue;
        clips.push_back(c);
    }
    Trim(); return B_OK;
}
}
