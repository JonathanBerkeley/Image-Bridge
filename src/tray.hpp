// Windows system tray integration
#pragma once

#include "win32.hpp"


bool tray_create(HINSTANCE hInst);
void tray_destroy();
