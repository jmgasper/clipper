#include "Protocol.h"
#include <add-ons/input_server/InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <cstring>
using namespace clipper;
class PasteDevice : public BInputServerDevice {
public:
    PasteDevice() {
        ref.name = const_cast<char*>(kDevice); ref.type = B_KEYBOARD_DEVICE; ref.cookie = this;
        input_device_ref* devices[] = {&ref, nullptr}; status = RegisterDevices(devices);
    }
    status_t InitCheck() override { return status; }
    status_t Start(const char*, void*) override { return B_OK; }
    status_t Stop(const char*, void*) override { return B_OK; }
    status_t Control(const char*, void*, uint32 code, BMessage*) override {
        if (code != kPaste) return B_BAD_VALUE;
        key_info info; status_t result = get_key_info(&info);
        if (result != B_OK) return result;
        // Don't synthesize a shortcut while any physical modifier is held.
        if (info.modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY | B_SHIFT_KEY)) return B_BUSY;
        for (int32 i = 0; i < 2; ++i) {
            auto event = new BMessage(i == 0 ? B_KEY_DOWN : B_KEY_UP);
            event->AddInt64("when", system_time()); event->AddInt32("key", 0x56);
            event->AddInt32("modifiers", i == 0 ? (info.modifiers | B_COMMAND_KEY | B_LEFT_COMMAND_KEY) : info.modifiers);
            event->AddInt32("raw_char", 'v'); event->AddString("bytes", "v");
            event->AddInt8("byte", 'v'); event->AddBool("clipper:injected", true);
            uint8 states[16]; memcpy(states, info.key_states, sizeof(states));
            if (i == 0) states[0x56 >> 3] |= 1 << (7 - (0x56 & 7));
            event->AddData("states", B_UINT8_TYPE, states, sizeof(states));
            result = EnqueueMessage(event);
            if (result != B_OK) { delete event; return result; }
        }
        return B_OK;
    }
private:
    input_device_ref ref; status_t status;
};
extern "C" _EXPORT BInputServerDevice* instantiate_input_device() { return new PasteDevice; }
