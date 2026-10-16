/**
Copyright (c) 2026 tomix89

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to use,
copy, modify, and distribute the Software for non-commercial purposes only,
subject to the following conditions:

1. Attribution: All copies or substantial portions of the Software must
   retain this copyright notice and the original author information.

2. Open-Source Requirement: Any modified versions of the Software must be
   distributed under this same license and made publicly available in source
   form.

3. Non-Commercial Use: The Software may not be used for commercial purposes
   without explicit written permission from the author.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

typedef enum {
	USB_ACTION_EP,
	USB_ACTION_ENTT,
} UsbActionType_t;

void fngr_log_usb_command(UsbActionType_t type, uint8_t feature, uint8_t request);
void fngr_reset();
extern inline uint8_t fngr_is_android();

