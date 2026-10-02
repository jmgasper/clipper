#pragma once
#include <Message.h>
#include <String.h>
#include <vector>
namespace clipper {
struct Clip {
    int64 id = 0;
    int64 time = 0;
    bool pinned = false;
    BString text, source, kind;
    BMessage data;
};
class History {
public:
    std::vector<Clip> clips;
    int32 limit = 100;
    BString notice;
    bool Capture(const BMessage& data, const char* source);
    Clip* Find(int64 id);
    bool Remove(int64 id);
    void Trim();
    status_t Load(const char* path);
    status_t Save(const char* path) const;
    static bool Describe(Clip& clip);
};
bool Matches(const BString& text, const BString& query);
BString SettingsPath(const char* leaf);
}
