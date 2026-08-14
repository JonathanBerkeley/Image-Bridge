#pragma once

#include "win32.hpp"

#include <string>


void send_ctrl_v();
void error_alert(HINSTANCE hInst, const std::wstring& text);
