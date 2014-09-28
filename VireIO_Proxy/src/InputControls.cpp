#include "InputControls.h"

bool xButtonsStatus[16] = {false, false, false, false,
	false, false, false, false,
	false, false, false, false,
	false, false, false, false};

InputControls::InputControls()
{
	for (int i = 0; i < 16; i++ )
		xButtonsStatus[i] = false;
}

void InputControls::UpdateXInputs()
{
	// Zeroise the XInput state
	ZeroMemory(&xInputState, sizeof(XINPUT_STATE));

	// Get the XInput state
	DWORD Result = XInputGetState(NULL, &xInputState);

	if(Result == ERROR_SUCCESS)
	{
		// set buttons by flags
		for(DWORD i = 0; i < 16;   ++i) 
			if((xInputState.Gamepad.wButtons >> i) & 1) 
				xButtonsStatus[i] = true;
			else 
				xButtonsStatus[i] = false;
	}
}

std::string InputControls::GetKeyName(int keyCode)
{
	if (keyCode < KeyNameList.size() && keyCode >= 0)
	{
		return KeyNameList[keyCode];
	}
	else
	{
		return "-";
	}
}

std::array<std::string, 256> InputControls::GetKeyNameList()
{
	std::array<std::string, 256> keyNameList;
	for (int i = 0; i < 256; i++)
		keyNameList[i] = "-";
	keyNameList[0x01] = "Left mouse button";
	keyNameList[0x02] = "Right mouse button";
	keyNameList[0x03] = "Control-break processing";
	keyNameList[0x04] = "Middle mouse button (three-button mouse)";
	keyNameList[0x08] = "Backspace";
	keyNameList[0x09] = "Tab";
	keyNameList[0x0C] = "CLEAR";
	keyNameList[0x0D] = "Enter";
	keyNameList[0x10] = "Shift";
	keyNameList[0x11] = "Ctrl";
	keyNameList[0x12] = "ALT";
	keyNameList[0x13] = "PAUSE";
	keyNameList[0x14] = "Caps";
	keyNameList[0x1B] = "Esc";
	keyNameList[0x20] = "Space";
	keyNameList[0x21] = "PageUp";
	keyNameList[0x22] = "PageDn";
	keyNameList[0x23] = "End";
	keyNameList[0x24] = "Home";
	keyNameList[0x25] = "Left";
	keyNameList[0x26] = "Up";
	keyNameList[0x27] = "RRight";
	keyNameList[0x28] = "Down";
	keyNameList[0x29] = "SELECT";
	keyNameList[0x2A] = "PRINT";
	keyNameList[0x2B] = "EXECUTE";
	keyNameList[0x2C] = "PRINT SCREEN";
	keyNameList[0x2D] = "INS";
	keyNameList[0x2E] = "DEL";
	keyNameList[0x2F] = "HELP";
	keyNameList[0x30] = "0";
	keyNameList[0x31] = "1";
	keyNameList[0x32] = "2";
	keyNameList[0x33] = "3";
	keyNameList[0x34] = "4";
	keyNameList[0x35] = "5";
	keyNameList[0x36] = "6";
	keyNameList[0x37] = "7";
	keyNameList[0x38] = "8";
	keyNameList[0x39] = "9";
	keyNameList[0x41] = "A";
	keyNameList[0x42] = "B";
	keyNameList[0x43] = "C";
	keyNameList[0x44] = "D";
	keyNameList[0x45] = "E";
	keyNameList[0x46] = "F";
	keyNameList[0x47] = "G";
	keyNameList[0x48] = "H";
	keyNameList[0x49] = "I";
	keyNameList[0x4A] = "J";
	keyNameList[0x4B] = "K";
	keyNameList[0x4C] = "L";
	keyNameList[0x4D] = "M";
	keyNameList[0x4E] = "N";
	keyNameList[0x4F] = "O";
	keyNameList[0x50] = "P";
	keyNameList[0x51] = "Q";
	keyNameList[0x52] = "R";
	keyNameList[0x53] = "S";
	keyNameList[0x54] = "T";
	keyNameList[0x55] = "U";
	keyNameList[0x56] = "V";
	keyNameList[0x57] = "W";
	keyNameList[0x58] = "X";
	keyNameList[0x59] = "Y";
	keyNameList[0x5A] = "Z";
	keyNameList[0x60] = "NUM0";
	keyNameList[0x61] = "NUM1";
	keyNameList[0x62] = "NUM2";
	keyNameList[0x63] = "NUM3";
	keyNameList[0x64] = "NUM4";
	keyNameList[0x65] = "NUM5";
	keyNameList[0x66] = "NUM6";
	keyNameList[0x67] = "NUM7";
	keyNameList[0x68] = "NUM8";
	keyNameList[0x69] = "NUM9";
	keyNameList[0x6C] = "separator";
	keyNameList[0x6D] = "NUM-";
	keyNameList[0x6E] = "NUM.";
	keyNameList[0x6F] = "NUM/";
	keyNameList[0x70] = "F1";
	keyNameList[0x71] = "F2";
	keyNameList[0x72] = "F3";
	keyNameList[0x73] = "F4";
	keyNameList[0x74] = "F5";
	keyNameList[0x75] = "F6";
	keyNameList[0x76] = "F7";
	keyNameList[0x77] = "F8";
	keyNameList[0x78] = "F9";
	keyNameList[0x79] = "F10";
	keyNameList[0x7A] = "F11";
	keyNameList[0x7B] = "F12";
	keyNameList[0x7C] = "F13";
	keyNameList[0x7D] = "F14";
	keyNameList[0x7E] = "F15";
	keyNameList[0x7F] = "F16";
	keyNameList[0x80] = "F17";
	keyNameList[0x81] = "F18";
	keyNameList[0x82] = "F19";
	keyNameList[0x83] = "F20";
	keyNameList[0x84] = "F21";
	keyNameList[0x85] = "F22";
	keyNameList[0x86] = "F23";
	keyNameList[0x87] = "F24";
	keyNameList[0x90] = "NUM LOCK";
	keyNameList[0x91] = "SCROLL LOCK";
	keyNameList[0xA0] = "LShift";
	keyNameList[0xA1] = "RShift";
	keyNameList[0xA2] = "LCtrl";
	keyNameList[0xA3] = "RCtrl";
	keyNameList[0xA4] = "Left MENU";
	keyNameList[0xA5] = "Right MENU";
	/// XInput hotkeys from 0xD0 to 0xDF
	keyNameList[0xD0] = "DPAD UP";
	keyNameList[0xD1] = "DPAD DOWN";
	keyNameList[0xD2] = "DPAD LEFT";
	keyNameList[0xD3] = "DPAD RIGHT";
	keyNameList[0xD4] = "START";
	keyNameList[0xD5] = "BACK";
	keyNameList[0xD6] = "LEFT THUMB";
	keyNameList[0xD7] = "RIGHT THUMB";
	keyNameList[0xD8] = "LEFT SHOULDER";
	keyNameList[0xD9] = "RIGHT SHOULDER";
	keyNameList[0xDC] = "Button A";
	keyNameList[0xDD] = "Button B";
	keyNameList[0xDE] = "Button X";
	keyNameList[0xDF] = "Button Y";
	/// end of XInput hotkeys
	keyNameList[0xFA] = "Play";
	keyNameList[0xFB] = "Zoom";
	return keyNameList;
}

bool InputControls::Key_Down( int virtualKeyCode )
{
	return (((GetAsyncKeyState(virtualKeyCode) & 0x8000) ? 1 : 0) || 
		((virtualKeyCode >= 0xD0) && (virtualKeyCode <= 0xDF) && (xButtonsStatus[virtualKeyCode % 0x10])));
}

bool InputControls::Key_Up( int virtualKeyCode )
{
	return ((GetAsyncKeyState(virtualKeyCode) & 0x8000) ? 0 : 1); ///TODO Should we be checking if xButtonStatus is false as well?
}

std::array<std::string, 256> InputControls::KeyNameList = InputControls::GetKeyNameList();