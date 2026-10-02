#pragma once
#include <SupportDefs.h>
namespace clipper {
constexpr const char* kSignature = "application/x-vnd.airOS-Clipper";
constexpr const char* kDevice = "Clipper paste bridge";
constexpr uint32 kShow = 'clSH', kChanged = 'clCH', kChoose = 'clPS', kPin = 'clPN',
    kDelete = 'clDL', kClear = 'clCL', kPause = 'clPA', kSettings = 'clST',
    kSave = 'clSV', kPaste = 'clIN', kHide = 'clHD', kSearch = 'clSE', kSelected = 'clSL',
    kForcePaste = 'clFP', kCopyOnly = 'clCO', kSaveSettings = 'clSS', kPasteReady = 'clPR';
constexpr ssize_t kMaxEntryBytes = 32 * 1024 * 1024;
constexpr ssize_t kMaxHistoryBytes = 128 * 1024 * 1024;
}
