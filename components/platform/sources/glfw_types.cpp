//
// Created by stuka on 16.11.2026.
//

#include "sgf/platform/glfw/glfw_types.h"

#include "GLFW/glfw3.h"

int sgf::platform::toGlfwKey(KeyboardKey key) noexcept
{
    switch(key)
    {
        case KeyboardKey::kUnknown: return GLFW_KEY_UNKNOWN;
        case KeyboardKey::kSpace: return GLFW_KEY_SPACE;
        case KeyboardKey::kApostrophe: return GLFW_KEY_APOSTROPHE;
        case KeyboardKey::kComma: return GLFW_KEY_COMMA;
        case KeyboardKey::kMinus: return GLFW_KEY_MINUS;
        case KeyboardKey::kPeriod: return GLFW_KEY_PERIOD;
        case KeyboardKey::kSlash: return GLFW_KEY_SLASH;
        case KeyboardKey::k0: return GLFW_KEY_0;
        case KeyboardKey::k1: return GLFW_KEY_1;
        case KeyboardKey::k2: return GLFW_KEY_2;
        case KeyboardKey::k3: return GLFW_KEY_3;
        case KeyboardKey::k4: return GLFW_KEY_4;
        case KeyboardKey::k5: return GLFW_KEY_5;
        case KeyboardKey::k6: return GLFW_KEY_6;
        case KeyboardKey::k7: return GLFW_KEY_7;
        case KeyboardKey::k8: return GLFW_KEY_8;
        case KeyboardKey::k9: return GLFW_KEY_9;
        case KeyboardKey::kSemicolon: return GLFW_KEY_SEMICOLON;
        case KeyboardKey::kEqual: return GLFW_KEY_EQUAL;
        case KeyboardKey::kA: return GLFW_KEY_A;
        case KeyboardKey::kB: return GLFW_KEY_B;
        case KeyboardKey::kC: return GLFW_KEY_C;
        case KeyboardKey::kD: return GLFW_KEY_D;
        case KeyboardKey::kE: return GLFW_KEY_E;
        case KeyboardKey::kF: return GLFW_KEY_F;
        case KeyboardKey::kG: return GLFW_KEY_G;
        case KeyboardKey::kH: return GLFW_KEY_H;
        case KeyboardKey::kI: return GLFW_KEY_I;
        case KeyboardKey::kJ: return GLFW_KEY_J;
        case KeyboardKey::kK: return GLFW_KEY_K;
        case KeyboardKey::kL: return GLFW_KEY_L;
        case KeyboardKey::kM: return GLFW_KEY_M;
        case KeyboardKey::kN: return GLFW_KEY_N;
        case KeyboardKey::kO: return GLFW_KEY_O;
        case KeyboardKey::kP: return GLFW_KEY_P;
        case KeyboardKey::kQ: return GLFW_KEY_Q;
        case KeyboardKey::kR: return GLFW_KEY_R;
        case KeyboardKey::kS: return GLFW_KEY_S;
        case KeyboardKey::kT: return GLFW_KEY_T;
        case KeyboardKey::kU: return GLFW_KEY_U;
        case KeyboardKey::kV: return GLFW_KEY_V;
        case KeyboardKey::kW: return GLFW_KEY_W;
        case KeyboardKey::kX: return GLFW_KEY_X;
        case KeyboardKey::kY: return GLFW_KEY_Y;
        case KeyboardKey::kZ: return GLFW_KEY_Z;
        case KeyboardKey::kLeftBracket: return GLFW_KEY_LEFT_BRACKET;
        case KeyboardKey::kBackslash: return GLFW_KEY_BACKSLASH;
        case KeyboardKey::kRightBracket: return GLFW_KEY_RIGHT_BRACKET;
        case KeyboardKey::kGraveAccent: return GLFW_KEY_GRAVE_ACCENT;
        case KeyboardKey::kWorld1: return GLFW_KEY_WORLD_1;
        case KeyboardKey::kWorld2: return GLFW_KEY_WORLD_2;
        case KeyboardKey::kEscape: return GLFW_KEY_ESCAPE;
        case KeyboardKey::kEnter: return GLFW_KEY_ENTER;
        case KeyboardKey::kTab: return GLFW_KEY_TAB;
        case KeyboardKey::kBackspace: return GLFW_KEY_BACKSPACE;
        case KeyboardKey::kInsert: return GLFW_KEY_INSERT;
        case KeyboardKey::kDelete: return GLFW_KEY_DELETE;
        case KeyboardKey::kRight: return GLFW_KEY_RIGHT;
        case KeyboardKey::kLeft: return GLFW_KEY_LEFT;
        case KeyboardKey::kDown: return GLFW_KEY_DOWN;
        case KeyboardKey::kUp: return GLFW_KEY_UP;
        case KeyboardKey::kPageUp: return GLFW_KEY_PAGE_UP;
        case KeyboardKey::kPageDown: return GLFW_KEY_PAGE_DOWN;
        case KeyboardKey::kHome: return GLFW_KEY_HOME;
        case KeyboardKey::kEnd: return GLFW_KEY_END;
        case KeyboardKey::kCapsLock: return GLFW_KEY_CAPS_LOCK;
        case KeyboardKey::kScrollLock: return GLFW_KEY_SCROLL_LOCK;
        case KeyboardKey::kNumLock: return GLFW_KEY_NUM_LOCK;
        case KeyboardKey::kPrintScreen: return GLFW_KEY_PRINT_SCREEN;
        case KeyboardKey::kPause: return GLFW_KEY_PAUSE;
        case KeyboardKey::kF1: return GLFW_KEY_F1;
        case KeyboardKey::kF2: return GLFW_KEY_F2;
        case KeyboardKey::kF3: return GLFW_KEY_F3;
        case KeyboardKey::kF4: return GLFW_KEY_F4;
        case KeyboardKey::kF5: return GLFW_KEY_F5;
        case KeyboardKey::kF6: return GLFW_KEY_F6;
        case KeyboardKey::kF7: return GLFW_KEY_F7;
        case KeyboardKey::kF8: return GLFW_KEY_F8;
        case KeyboardKey::kF9: return GLFW_KEY_F9;
        case KeyboardKey::kF10: return GLFW_KEY_F10;
        case KeyboardKey::kF11: return GLFW_KEY_F11;
        case KeyboardKey::kF12: return GLFW_KEY_F12;
        case KeyboardKey::kF13: return GLFW_KEY_F13;
        case KeyboardKey::kF14: return GLFW_KEY_F14;
        case KeyboardKey::kF15: return GLFW_KEY_F15;
        case KeyboardKey::kF16: return GLFW_KEY_F16;
        case KeyboardKey::kF17: return GLFW_KEY_F17;
        case KeyboardKey::kF18: return GLFW_KEY_F18;
        case KeyboardKey::kF19: return GLFW_KEY_F19;
        case KeyboardKey::kF20: return GLFW_KEY_F20;
        case KeyboardKey::kF21: return GLFW_KEY_F21;
        case KeyboardKey::kF22: return GLFW_KEY_F22;
        case KeyboardKey::kF23: return GLFW_KEY_F23;
        case KeyboardKey::kF24: return GLFW_KEY_F24;
        case KeyboardKey::kF25: return GLFW_KEY_F25;
        case KeyboardKey::kKeypad0: return GLFW_KEY_KP_0;
        case KeyboardKey::kKeypad1: return GLFW_KEY_KP_1;
        case KeyboardKey::kKeypad2: return GLFW_KEY_KP_2;
        case KeyboardKey::kKeypad3: return GLFW_KEY_KP_3;
        case KeyboardKey::kKeypad4: return GLFW_KEY_KP_4;
        case KeyboardKey::kKeypad5: return GLFW_KEY_KP_5;
        case KeyboardKey::kKeypad6: return GLFW_KEY_KP_6;
        case KeyboardKey::kKeypad7: return GLFW_KEY_KP_7;
        case KeyboardKey::kKeypad8: return GLFW_KEY_KP_8;
        case KeyboardKey::kKeypad9: return GLFW_KEY_KP_9;
        case KeyboardKey::kKeypadDecimal: return GLFW_KEY_KP_DECIMAL;
        case KeyboardKey::kKeypadDivide: return GLFW_KEY_KP_DIVIDE;
        case KeyboardKey::kKeypadMultiply: return GLFW_KEY_KP_MULTIPLY;
        case KeyboardKey::kKeypadSubtract: return GLFW_KEY_KP_SUBTRACT;
        case KeyboardKey::kKeypadAdd: return GLFW_KEY_KP_ADD;
        case KeyboardKey::kKeypadEnter: return GLFW_KEY_KP_ENTER;
        case KeyboardKey::kKeypadEqual: return GLFW_KEY_KP_EQUAL;
        case KeyboardKey::kLeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyboardKey::kLeftControl: return GLFW_KEY_LEFT_CONTROL;
        case KeyboardKey::kLeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyboardKey::kLeftSuper: return GLFW_KEY_LEFT_SUPER;
        case KeyboardKey::kRightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyboardKey::kRightControl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyboardKey::kRightAlt: return GLFW_KEY_RIGHT_ALT;
        case KeyboardKey::kRightSuper: return GLFW_KEY_RIGHT_SUPER;
        case KeyboardKey::kMenu: return GLFW_KEY_MENU;
        case KeyboardKey::kCount: return GLFW_KEY_UNKNOWN;
    }

    return GLFW_KEY_UNKNOWN;
}

int sgf::platform::toGlfwMouseButton(MouseButton button) noexcept
{
    switch(button)
    {
        case MouseButton::kLeft: return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::kRight: return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::kMiddle: return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::kButton4: return GLFW_MOUSE_BUTTON_4;
        case MouseButton::kButton5: return GLFW_MOUSE_BUTTON_5;
        case MouseButton::kButton6: return GLFW_MOUSE_BUTTON_6;
        case MouseButton::kButton7: return GLFW_MOUSE_BUTTON_7;
        case MouseButton::kButton8: return GLFW_MOUSE_BUTTON_8;
        case MouseButton::kCount: return -1;
    }

    return -1;
}

int sgf::platform::toGlfwKeyMod(KeyMod mods) noexcept
{
    int glfw_mods = 0;

    if((mods & KeyMod::kShift) == KeyMod::kShift) glfw_mods |= GLFW_MOD_SHIFT;
    if((mods & KeyMod::kControl) == KeyMod::kControl) glfw_mods |= GLFW_MOD_CONTROL;
    if((mods & KeyMod::kAlt) == KeyMod::kAlt) glfw_mods |= GLFW_MOD_ALT;
    if((mods & KeyMod::kSuper) == KeyMod::kSuper) glfw_mods |= GLFW_MOD_SUPER;
    if((mods & KeyMod::kCapsLock) == KeyMod::kCapsLock) glfw_mods |= GLFW_MOD_CAPS_LOCK;
    if((mods & KeyMod::kNumLock) == KeyMod::kNumLock) glfw_mods |= GLFW_MOD_NUM_LOCK;

    return glfw_mods;
}

int sgf::platform::toGlfwKeyAction(KeyAction action) noexcept
{
    switch(action)
    {
        case KeyAction::kRelease: return GLFW_RELEASE;
        case KeyAction::kPress: return GLFW_PRESS;
        case KeyAction::kRepeat: return GLFW_REPEAT;
    }

    return GLFW_RELEASE;
}

sgf::platform::KeyboardKey sgf::platform::fromGlfwKey(int glfw_key) noexcept
{
    switch(glfw_key)
    {
        case GLFW_KEY_SPACE: return KeyboardKey::kSpace;
        case GLFW_KEY_APOSTROPHE: return KeyboardKey::kApostrophe;
        case GLFW_KEY_COMMA: return KeyboardKey::kComma;
        case GLFW_KEY_MINUS: return KeyboardKey::kMinus;
        case GLFW_KEY_PERIOD: return KeyboardKey::kPeriod;
        case GLFW_KEY_SLASH: return KeyboardKey::kSlash;
        case GLFW_KEY_0: return KeyboardKey::k0;
        case GLFW_KEY_1: return KeyboardKey::k1;
        case GLFW_KEY_2: return KeyboardKey::k2;
        case GLFW_KEY_3: return KeyboardKey::k3;
        case GLFW_KEY_4: return KeyboardKey::k4;
        case GLFW_KEY_5: return KeyboardKey::k5;
        case GLFW_KEY_6: return KeyboardKey::k6;
        case GLFW_KEY_7: return KeyboardKey::k7;
        case GLFW_KEY_8: return KeyboardKey::k8;
        case GLFW_KEY_9: return KeyboardKey::k9;
        case GLFW_KEY_SEMICOLON: return KeyboardKey::kSemicolon;
        case GLFW_KEY_EQUAL: return KeyboardKey::kEqual;
        case GLFW_KEY_A: return KeyboardKey::kA;
        case GLFW_KEY_B: return KeyboardKey::kB;
        case GLFW_KEY_C: return KeyboardKey::kC;
        case GLFW_KEY_D: return KeyboardKey::kD;
        case GLFW_KEY_E: return KeyboardKey::kE;
        case GLFW_KEY_F: return KeyboardKey::kF;
        case GLFW_KEY_G: return KeyboardKey::kG;
        case GLFW_KEY_H: return KeyboardKey::kH;
        case GLFW_KEY_I: return KeyboardKey::kI;
        case GLFW_KEY_J: return KeyboardKey::kJ;
        case GLFW_KEY_K: return KeyboardKey::kK;
        case GLFW_KEY_L: return KeyboardKey::kL;
        case GLFW_KEY_M: return KeyboardKey::kM;
        case GLFW_KEY_N: return KeyboardKey::kN;
        case GLFW_KEY_O: return KeyboardKey::kO;
        case GLFW_KEY_P: return KeyboardKey::kP;
        case GLFW_KEY_Q: return KeyboardKey::kQ;
        case GLFW_KEY_R: return KeyboardKey::kR;
        case GLFW_KEY_S: return KeyboardKey::kS;
        case GLFW_KEY_T: return KeyboardKey::kT;
        case GLFW_KEY_U: return KeyboardKey::kU;
        case GLFW_KEY_V: return KeyboardKey::kV;
        case GLFW_KEY_W: return KeyboardKey::kW;
        case GLFW_KEY_X: return KeyboardKey::kX;
        case GLFW_KEY_Y: return KeyboardKey::kY;
        case GLFW_KEY_Z: return KeyboardKey::kZ;
        case GLFW_KEY_LEFT_BRACKET: return KeyboardKey::kLeftBracket;
        case GLFW_KEY_BACKSLASH: return KeyboardKey::kBackslash;
        case GLFW_KEY_RIGHT_BRACKET: return KeyboardKey::kRightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return KeyboardKey::kGraveAccent;
        case GLFW_KEY_WORLD_1: return KeyboardKey::kWorld1;
        case GLFW_KEY_WORLD_2: return KeyboardKey::kWorld2;
        case GLFW_KEY_ESCAPE: return KeyboardKey::kEscape;
        case GLFW_KEY_ENTER: return KeyboardKey::kEnter;
        case GLFW_KEY_TAB: return KeyboardKey::kTab;
        case GLFW_KEY_BACKSPACE: return KeyboardKey::kBackspace;
        case GLFW_KEY_INSERT: return KeyboardKey::kInsert;
        case GLFW_KEY_DELETE: return KeyboardKey::kDelete;
        case GLFW_KEY_RIGHT: return KeyboardKey::kRight;
        case GLFW_KEY_LEFT: return KeyboardKey::kLeft;
        case GLFW_KEY_DOWN: return KeyboardKey::kDown;
        case GLFW_KEY_UP: return KeyboardKey::kUp;
        case GLFW_KEY_PAGE_UP: return KeyboardKey::kPageUp;
        case GLFW_KEY_PAGE_DOWN: return KeyboardKey::kPageDown;
        case GLFW_KEY_HOME: return KeyboardKey::kHome;
        case GLFW_KEY_END: return KeyboardKey::kEnd;
        case GLFW_KEY_CAPS_LOCK: return KeyboardKey::kCapsLock;
        case GLFW_KEY_SCROLL_LOCK: return KeyboardKey::kScrollLock;
        case GLFW_KEY_NUM_LOCK: return KeyboardKey::kNumLock;
        case GLFW_KEY_PRINT_SCREEN: return KeyboardKey::kPrintScreen;
        case GLFW_KEY_PAUSE: return KeyboardKey::kPause;
        case GLFW_KEY_F1: return KeyboardKey::kF1;
        case GLFW_KEY_F2: return KeyboardKey::kF2;
        case GLFW_KEY_F3: return KeyboardKey::kF3;
        case GLFW_KEY_F4: return KeyboardKey::kF4;
        case GLFW_KEY_F5: return KeyboardKey::kF5;
        case GLFW_KEY_F6: return KeyboardKey::kF6;
        case GLFW_KEY_F7: return KeyboardKey::kF7;
        case GLFW_KEY_F8: return KeyboardKey::kF8;
        case GLFW_KEY_F9: return KeyboardKey::kF9;
        case GLFW_KEY_F10: return KeyboardKey::kF10;
        case GLFW_KEY_F11: return KeyboardKey::kF11;
        case GLFW_KEY_F12: return KeyboardKey::kF12;
        case GLFW_KEY_F13: return KeyboardKey::kF13;
        case GLFW_KEY_F14: return KeyboardKey::kF14;
        case GLFW_KEY_F15: return KeyboardKey::kF15;
        case GLFW_KEY_F16: return KeyboardKey::kF16;
        case GLFW_KEY_F17: return KeyboardKey::kF17;
        case GLFW_KEY_F18: return KeyboardKey::kF18;
        case GLFW_KEY_F19: return KeyboardKey::kF19;
        case GLFW_KEY_F20: return KeyboardKey::kF20;
        case GLFW_KEY_F21: return KeyboardKey::kF21;
        case GLFW_KEY_F22: return KeyboardKey::kF22;
        case GLFW_KEY_F23: return KeyboardKey::kF23;
        case GLFW_KEY_F24: return KeyboardKey::kF24;
        case GLFW_KEY_F25: return KeyboardKey::kF25;
        case GLFW_KEY_KP_0: return KeyboardKey::kKeypad0;
        case GLFW_KEY_KP_1: return KeyboardKey::kKeypad1;
        case GLFW_KEY_KP_2: return KeyboardKey::kKeypad2;
        case GLFW_KEY_KP_3: return KeyboardKey::kKeypad3;
        case GLFW_KEY_KP_4: return KeyboardKey::kKeypad4;
        case GLFW_KEY_KP_5: return KeyboardKey::kKeypad5;
        case GLFW_KEY_KP_6: return KeyboardKey::kKeypad6;
        case GLFW_KEY_KP_7: return KeyboardKey::kKeypad7;
        case GLFW_KEY_KP_8: return KeyboardKey::kKeypad8;
        case GLFW_KEY_KP_9: return KeyboardKey::kKeypad9;
        case GLFW_KEY_KP_DECIMAL: return KeyboardKey::kKeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return KeyboardKey::kKeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return KeyboardKey::kKeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return KeyboardKey::kKeypadSubtract;
        case GLFW_KEY_KP_ADD: return KeyboardKey::kKeypadAdd;
        case GLFW_KEY_KP_ENTER: return KeyboardKey::kKeypadEnter;
        case GLFW_KEY_KP_EQUAL: return KeyboardKey::kKeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return KeyboardKey::kLeftShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyboardKey::kLeftControl;
        case GLFW_KEY_LEFT_ALT: return KeyboardKey::kLeftAlt;
        case GLFW_KEY_LEFT_SUPER: return KeyboardKey::kLeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return KeyboardKey::kRightShift;
        case GLFW_KEY_RIGHT_CONTROL: return KeyboardKey::kRightControl;
        case GLFW_KEY_RIGHT_ALT: return KeyboardKey::kRightAlt;
        case GLFW_KEY_RIGHT_SUPER: return KeyboardKey::kRightSuper;
        case GLFW_KEY_MENU: return KeyboardKey::kMenu;
        case GLFW_KEY_UNKNOWN: return KeyboardKey::kUnknown;
        default: return KeyboardKey::kUnknown;
    }
}

std::optional<sgf::platform::MouseButton> sgf::platform::fromGlfwMouseButton(int glfw_button) noexcept
{
    switch(glfw_button)
    {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::kLeft;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::kRight;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::kMiddle;
        case GLFW_MOUSE_BUTTON_4: return MouseButton::kButton4;
        case GLFW_MOUSE_BUTTON_5: return MouseButton::kButton5;
        case GLFW_MOUSE_BUTTON_6: return MouseButton::kButton6;
        case GLFW_MOUSE_BUTTON_7: return MouseButton::kButton7;
        case GLFW_MOUSE_BUTTON_8: return MouseButton::kButton8;
        default: return std::nullopt;
    }
}

sgf::platform::KeyMod sgf::platform::fromGlfwKeyMod(int glfw_mods) noexcept
{
    KeyMod mods = KeyMod::kNone;

    if((glfw_mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT) mods |= KeyMod::kShift;
    if((glfw_mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL) mods |= KeyMod::kControl;
    if((glfw_mods & GLFW_MOD_ALT) == GLFW_MOD_ALT) mods |= KeyMod::kAlt;
    if((glfw_mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER) mods |= KeyMod::kSuper;
    if((glfw_mods & GLFW_MOD_CAPS_LOCK) == GLFW_MOD_CAPS_LOCK) mods |= KeyMod::kCapsLock;
    if((glfw_mods & GLFW_MOD_NUM_LOCK) == GLFW_MOD_NUM_LOCK) mods |= KeyMod::kNumLock;

    return mods;
}

std::optional<sgf::platform::KeyAction> sgf::platform::fromGlfwKeyAction(int glfw_action) noexcept
{
    switch(glfw_action)
    {
        case GLFW_RELEASE: return KeyAction::kRelease;
        case GLFW_PRESS: return KeyAction::kPress;
        case GLFW_REPEAT: return KeyAction::kRepeat;
        default: return std::nullopt;
    }
}

sgf::core::LogLevel sgf::platform::fromGlfwError(int glfw_error) noexcept
{
    switch(glfw_error)
    {
        case GLFW_NO_ERROR: return core::LogLevel::kInfo;
        case GLFW_OUT_OF_MEMORY: return core::LogLevel::kFatal;
        case GLFW_FORMAT_UNAVAILABLE: return core::LogLevel::kWarn;
        case GLFW_NOT_INITIALIZED:
        case GLFW_NO_CURRENT_CONTEXT:
        case GLFW_INVALID_ENUM:
        case GLFW_INVALID_VALUE:
        case GLFW_API_UNAVAILABLE:
        case GLFW_VERSION_UNAVAILABLE:
        case GLFW_PLATFORM_ERROR:
        case GLFW_NO_WINDOW_CONTEXT:
            return core::LogLevel::kError;
        default: return core::LogLevel::kError;
    }
}
