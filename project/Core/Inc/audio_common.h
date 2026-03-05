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

// Windows is kind of confused with the real dB values.
// Windows usually splits the whole volume range to 0-100% in 2% steps
// on HW we have 102dB range with 0.5dB steps so make it 200
#define USB_MAX_VOLUME_PCT       200
#define USB_MIN_VOLUME_PCT         0
#define USB_VOLUME_STEP			   1
