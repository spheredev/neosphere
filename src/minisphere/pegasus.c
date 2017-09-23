/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "minisphere.h"
#include "pegasus.h"

#include "api.h"
#include "async.h"
#include "audio.h"
#include "color.h"
#include "console.h"
#include "debugger.h"
#include "encoding.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "jsal.h"
#include "sockets.h"
#include "unicode.h"
#include "xoroshiro.h"

#define API_VERSION 2
#define API_LEVEL   0

enum file_op
{
	FILE_OP_READ,
	FILE_OP_WRITE,
	FILE_OP_UPDATE,
	FILE_OP_MAX,
};

static const
struct x11_color
{
	const char* name;
	uint8_t     r;
	uint8_t     g;
	uint8_t     b;
	uint8_t     a;
}
COLORS[] =
{
	{ "AliceBlue", 240, 248, 255, 255 },
	{ "AntiqueWhite", 250, 235, 215, 255 },
	{ "Aqua", 0, 255, 255, 255 },
	{ "Aquamarine", 127, 255, 212, 255 },
	{ "Azure", 240, 255, 255, 255 },
	{ "Beige", 245, 245, 220, 255 },
	{ "Bisque", 255, 228, 196, 255 },
	{ "Black", 0, 0, 0, 255 },
	{ "BlanchedAlmond", 255, 235, 205, 255 },
	{ "Blue", 0, 0, 255, 255 },
	{ "BlueViolet", 138, 43, 226, 255 },
	{ "Brown", 165, 42, 42, 255 },
	{ "BurlyWood", 222, 184, 135, 255 },
	{ "CadetBlue", 95, 158, 160, 255 },
	{ "Chartreuse", 127, 255, 0, 255 },
	{ "Chocolate", 210, 105, 30, 255 },
	{ "Coral", 255, 127, 80, 255 },
	{ "CornflowerBlue", 100, 149, 237, 255 },
	{ "Cornsilk", 255, 248, 220, 255 },
	{ "Crimson", 220, 20, 60, 255 },
	{ "Cyan", 0, 255, 255, 255 },
	{ "DarkBlue", 0, 0, 139, 255 },
	{ "DarkCyan", 0, 139, 139, 255 },
	{ "DarkGoldenrod", 184, 134, 11, 255 },
	{ "DarkGray", 169, 169, 169, 255 },
	{ "DarkGreen", 0, 100, 0, 255 },
	{ "DarkKhaki", 189, 183, 107, 255 },
	{ "DarkMagenta", 139, 0, 139, 255 },
	{ "DarkOliveGreen", 85, 107, 47, 255 },
	{ "DarkOrange", 255, 140, 0, 255 },
	{ "DarkOrchid", 153, 50, 204, 255 },
	{ "DarkRed", 139, 0, 0, 255 },
	{ "DarkSalmon", 233, 150, 122, 255 },
	{ "DarkSeaGreen", 143, 188, 143, 255 },
	{ "DarkSlateBlue", 72, 61, 139, 255 },
	{ "DarkSlateGray", 47, 79, 79, 255 },
	{ "DarkTurquoise", 0, 206, 209, 255 },
	{ "DarkViolet", 148, 0, 211, 255 },
	{ "DeepPink", 255, 20, 147, 255 },
	{ "DeepSkyBlue", 0, 191, 255, 255 },
	{ "DimGray", 105, 105, 105, 255 },
	{ "DodgerBlue", 30, 144, 255, 255 },
	{ "FireBrick", 178, 34, 34, 255 },
	{ "FloralWhite", 255, 250, 240, 255 },
	{ "ForestGreen", 34, 139, 34, 255 },
	{ "Fuchsia", 255, 0, 255, 255 },
	{ "Gainsboro", 220, 220, 220, 255 },
	{ "GhostWhite", 248, 248, 255, 255 },
	{ "Gold", 255, 215, 0, 255 },
	{ "Goldenrod", 218, 165, 32, 255 },
	{ "Gray", 128, 128, 128, 255 },
	{ "Green", 0, 128, 0, 255 },
	{ "GreenYellow", 173, 255, 47, 255 },
	{ "Honeydew", 240, 255, 240, 255 },
	{ "HotPink", 255, 105, 180, 255 },
	{ "IndianRed", 205, 92, 92, 255 },
	{ "Indigo", 75, 0, 130, 255 },
	{ "Ivory", 255, 255, 240, 255 },
	{ "Khaki", 240, 230, 140, 255 },
	{ "Lavender", 230, 230, 250, 255 },
	{ "LavenderBlush", 255, 240, 245, 255 },
	{ "LawnGreen", 124, 252, 0, 255 },
	{ "LemonChiffon", 255, 250, 205, 255 },
	{ "LightBlue", 173, 216, 230, 255 },
	{ "LightCoral", 240, 128, 128, 255 },
	{ "LightCyan", 224, 255, 255, 255 },
	{ "LightGoldenrodYellow", 250, 250, 210, 255 },
	{ "LightGray", 211, 211, 211, 255 },
	{ "LightGreen", 144, 238, 144, 255 },
	{ "LightPink", 255, 182, 193, 255 },
	{ "LightSalmon", 255, 160, 122, 255 },
	{ "LightSeaGreen", 32, 178, 170, 255 },
	{ "LightSkyBlue", 135, 206, 250, 255 },
	{ "LightSlateGray", 119, 136, 153, 255 },
	{ "LightSteelBlue", 176, 196, 222, 255 },
	{ "LightYellow", 255, 255, 224, 255 },
	{ "Lime", 0, 255, 0, 255 },
	{ "LimeGreen", 50, 205, 50, 255 },
	{ "Linen", 250, 240, 230, 255 },
	{ "Magenta", 255, 0, 255, 255 },
	{ "Maroon", 128, 0, 0, 255 },
	{ "MediumAquamarine", 102, 205, 170, 255 },
	{ "MediumBlue", 0, 0, 205, 255 },
	{ "MediumOrchid", 186, 85, 211, 255 },
	{ "MediumPurple", 147, 112, 219, 255 },
	{ "MediumSeaGreen", 60, 179, 113, 255 },
	{ "MediumSlateBlue", 123, 104, 238, 255 },
	{ "MediumSpringGreen", 0, 250, 154, 255 },
	{ "MediumTurquoise", 72, 209, 204, 255 },
	{ "MediumVioletRed", 199, 21, 133, 255 },
	{ "MidnightBlue", 25, 25, 112, 255 },
	{ "MintCream", 245, 255, 250, 255 },
	{ "MistyRose", 255, 228, 225, 255 },
	{ "Moccasin", 255, 228, 181, 255 },
	{ "NavajoWhite", 255, 222, 173, 255 },
	{ "Navy", 0, 0, 128, 255 },
	{ "OldLace", 253, 245, 230, 255 },
	{ "Olive", 128, 128, 0, 255 },
	{ "OliveDrab", 107, 142, 35, 255 },
	{ "Orange", 255, 165, 0, 255 },
	{ "OrangeRed", 255, 69, 0, 255 },
	{ "Orchid", 218, 112, 214, 255 },
	{ "PaleGoldenrod", 238, 232, 170, 255 },
	{ "PaleGreen", 152, 251, 152, 255 },
	{ "PaleTurquoise", 175, 238, 238, 255 },
	{ "PaleVioletRed", 219, 112, 147, 255 },
	{ "PapayaWhip", 225, 239, 213, 255 },
	{ "PeachPuff", 255, 218, 185, 255 },
	{ "Peru", 205, 133, 63, 255 },
	{ "Pink", 255, 192, 203, 255 },
	{ "Plum", 221, 160, 221, 255 },
	{ "PowderBlue", 176, 224, 230, 255 },
	{ "Purple", 128, 0, 128, 255 },
	{ "PurwaBlue", 155, 225, 255, 255 },
	{ "RebeccaPurple", 102, 51, 153, 255 },
	{ "Red", 255, 0, 0, 255 },
	{ "RosyBrown", 188, 143, 143, 255 },
	{ "RoyalBlue", 65, 105, 225, 255 },
	{ "SaddleBrown", 139, 69, 19, 255 },
	{ "Salmon", 250, 128, 114, 255 },
	{ "SandyBrown", 244, 164, 96, 255 },
	{ "SeaGreen", 46, 139, 87, 255 },
	{ "Seashell", 255, 245, 238, 255 },
	{ "Sienna", 160, 82, 45, 255 },
	{ "Silver", 192, 192, 192, 255 },
	{ "SkyBlue", 135, 206, 235, 255 },
	{ "SlateBlue", 106, 90, 205, 255 },
	{ "SlateGray", 112, 128, 144, 255 },
	{ "Snow", 255, 250, 250, 255 },
	{ "SpringGreen", 0, 255, 127, 255 },
	{ "SteelBlue", 70, 130, 180, 255 },
	{ "Tan", 210, 180, 140, 255 },
	{ "Teal", 0, 128, 128, 255 },
	{ "Thistle", 216, 191, 216, 255 },
	{ "Tomato", 255, 99, 71, 255 },
	{ "Transparent", 0, 0, 0, 0 },
	{ "Turquoise", 64, 224, 208, 255 },
	{ "Violet", 238, 130, 238, 255 },
	{ "Wheat", 245, 222, 179, 255 },
	{ "White", 255, 255, 255, 255 },
	{ "WhiteSmoke", 245, 245, 245, 255 },
	{ "Yellow", 255, 255, 0, 255 },
	{ "YellowGreen", 154, 205, 50, 255 },
	{ NULL, 0, 0, 0, 0 }
};

static bool js_require                       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_get_frameRate          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_get_frameSkip          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_get_fullScreen         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_set_frameRate          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_set_frameSkip          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_set_fullScreen         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_flip                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_now                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_screen_resize                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_APILevel           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Game               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Platform           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Version            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_abort                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_exit                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_restart                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_run                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sphere_sleep                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_Color               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_is                      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_mix                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_of                      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Color                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_name                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_r                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_g                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_b                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_get_a                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_set_r                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_set_g                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_set_b                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_set_a                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_clone                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Color_fade                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_DirectoryStream           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_dispose       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_fileCount (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_fileName  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_position  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_set_position  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_next          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_rewind        (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_cancel               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_cancelAll            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_later                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_now                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_onRender             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Dispatch_onUpdate             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_createDirectory            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_deleteFile                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_directoryExists            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_evaluateScript             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_fileExists                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_fullPath                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_readFile                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_relativePath               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_rename                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_removeDirectory            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FS_writeFile                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_FileStream                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_dispose            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_fileName       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_fileSize       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_position       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_set_position       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_read               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_FileStream_write              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_get_Default              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Font                      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_get_fileName             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_get_height               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_drawText                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_getTextSize              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Font_wordWrap                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_IndexList                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_Null             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_getDevices           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_name             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_numAxes          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_numButtons       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_getPosition          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Joystick_isPressed            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_Default          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_capsLock         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_numLock          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_scrollLock       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_clearQueue           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_getChar              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_getKey               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Keyboard_isPressed            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mixer_get_Default             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Mixer                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mixer_get_volume              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mixer_set_volume              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Model                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_get_shader              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_get_transform           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_set_shader              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_set_transform           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_draw                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setBoolean              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setColorVector          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setFloat                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setFloatArray           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setFloatVector          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setInt                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setIntArray             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setIntVector            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Model_setMatrix               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_Default             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_x                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_y                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_clearQueue              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_getEvent                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Mouse_isPressed               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_RNG_fromSeed                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_RNG_fromState                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_RNG                       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_RNG_get_state                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_RNG_set_state                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_RNG_next                      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SSj_log                       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SSj_trace                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Sample                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sample_get_fileName           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sample_play                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sample_stopAll                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Server                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Server_close                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Server_accept                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shader_get_Default            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Shader                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Shape                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_get_indexList           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_get_texture             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_get_vertexList          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_set_indexList           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_set_texture             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_set_vertexList          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Shape_draw                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Socket                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_get_bytesPending       (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_get_connected          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_get_remoteAddress      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_get_remotePort         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_close                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_connectTo              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_read                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Socket_write                  (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Sound                     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_fileName            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_length              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_pan                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_playing             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_position            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_repeat              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_speed               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_get_volume              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_set_pan                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_set_position            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_set_repeat              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_set_speed               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_set_volume              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_pause                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_play                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Sound_stop                    (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_SoundStream               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SoundStream_get_length        (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SoundStream_play              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SoundStream_pause             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SoundStream_stop              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_SoundStream_write             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Surface                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_get_height            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_get_transform         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_get_width             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_set_transform         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_clipTo                (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Surface_toTexture             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_TextDecoder               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_encoding      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_fatal         (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_ignoreBOM     (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_decode            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_TextEncoder               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextEncoder_get_encoding      (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_TextEncoder_encode            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Texture                   (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Texture_get_fileName          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Texture_get_height            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Texture_get_width             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_Transform                 (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_get_matrix          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_set_matrix          (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_compose             (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_identity            (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_project2D           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_project3D           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_rotate              (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_scale               (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_Transform_translate           (js_ref_t* me, int num_args, bool is_ctor, int magic);
static bool js_new_VertexList                (js_ref_t* me, int num_args, bool is_ctor, int magic);

static void js_Color_finalize           (void* host_ptr);
static void js_DirectoryStream_finalize (void* host_ptr);
static void js_FileStream_finalize      (void* host_ptr);
static void js_Font_finalize            (void* host_ptr);
static void js_IndexList_finalize       (void* host_ptr);
static void js_JobToken_finalize        (void* host_ptr);
static void js_Joystick_finalize        (void* host_ptr);
static void js_Mixer_finalize           (void* host_ptr);
static void js_Model_finalize           (void* host_ptr);
static void js_RNG_finalize             (void* host_ptr);
static void js_Sample_finalize          (void* host_ptr);
static void js_Server_finalize          (void* host_ptr);
static void js_Shader_finalize          (void* host_ptr);
static void js_Shape_finalize           (void* host_ptr);
static void js_Socket_finalize          (void* host_ptr);
static void js_Sound_finalize           (void* host_ptr);
static void js_SoundStream_finalize     (void* host_ptr);
static void js_Surface_finalize         (void* host_ptr);
static void js_TextDecoder_finalize     (void* host_ptr);
static void js_TextEncoder_finalize     (void* host_ptr);
static void js_Texture_finalize         (void* host_ptr);
static void js_Transform_finalize       (void* host_ptr);
static void js_VertexList_finalize      (void* host_ptr);

static void      on_import_module            (void);
static void      jsal_pegasus_push_color     (color_t color, bool in_ctor);
static void      jsal_pegasus_push_job_token (int64_t token);
static void      jsal_pegasus_push_require   (const char* module_id);
static color_t   jsal_pegasus_require_color  (int index);
static script_t* jsal_pegasus_require_script (int index);
static bool      jsal_safe_event_loop        (js_ref_t* me, int num_args, bool is_ctor, int magic);
static path_t*   find_module                 (const char* id, const char* origin, const char* sys_origin, bool es6_mode);
static void      load_joysticks              (void);
static path_t*   load_package_json           (const char* filename);

static mixer_t*  s_def_mixer;
static int       s_framerate = 60;
static js_ref_t* s_key_color;
static js_ref_t* s_key_x;
static js_ref_t* s_key_y;
static js_ref_t* s_key_z;
static js_ref_t* s_key_u;
static js_ref_t* s_key_v;
static int       s_next_module_id = 1;

void
initialize_pegasus_api(void)
{
	const struct x11_color* p;

	console_log(1, "initializing Sphere v%d L%d API", API_VERSION, API_LEVEL);

	s_def_mixer = mixer_new(44100, 16, 2);
	jsal_on_import_module(on_import_module);

	s_key_color = jsal_new_key("color");
	s_key_x = jsal_new_key("x");
	s_key_y = jsal_new_key("y");
	s_key_z = jsal_new_key("z");
	s_key_u = jsal_new_key("u");
	s_key_v = jsal_new_key("v");

	// initialize CommonJS cache and global require()
	jsal_push_hidden_stash();
	jsal_push_new_bare_object();
	jsal_put_prop_string(-2, "moduleCache");
	jsal_pop(1);

	jsal_push_global_object();
	jsal_push_eval("({ enumerable: false, writable: true, configurable: true })");
	jsal_pegasus_push_require(NULL);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "require");
	jsal_pop(1);

	// initialize the Sphere v2 API
	api_define_static_prop("Sphere", "APILevel", js_Sphere_get_APILevel, NULL);
	api_define_static_prop("Sphere", "Game", js_Sphere_get_Game, NULL);
	api_define_static_prop("Sphere", "Platform", js_Sphere_get_Platform, NULL);
	api_define_static_prop("Sphere", "Version", js_Sphere_get_Version, NULL);
	api_define_function("Sphere", "abort", js_Sphere_abort);
	api_define_function("Sphere", "exit", js_Sphere_exit);
	api_define_function("Sphere", "restart", js_Sphere_restart);
	api_define_function("Sphere", "run", js_Sphere_run);
	api_define_function("Sphere", "sleep", js_Sphere_sleep);
	api_define_class("Color", CLASS_COLOR, js_new_Color, js_Color_finalize);
	api_define_function("Color", "is", js_Color_is);
	api_define_function("Color", "mix", js_Color_mix);
	api_define_function("Color", "of", js_Color_of);
	api_define_property("Color", "name", false, js_Color_get_name, NULL);
	api_define_property("Color", "r", true, js_Color_get_r, js_Color_set_r);
	api_define_property("Color", "g", true, js_Color_get_g, js_Color_set_g);
	api_define_property("Color", "b", true, js_Color_get_b, js_Color_set_b);
	api_define_property("Color", "a", true, js_Color_get_a, js_Color_set_a);
	api_define_method("Color", "clone", js_Color_clone);
	api_define_method("Color", "fade", js_Color_fade);
	api_define_class("DirectoryStream", CLASS_DIR_STREAM, js_new_DirectoryStream, js_DirectoryStream_finalize);
	api_define_method("DirectoryStream", "dispose", js_DirectoryStream_dispose);
	api_define_property("DirectoryStream", "fileCount", false, js_DirectoryStream_get_fileCount, NULL);
	api_define_property("DirectoryStream", "fileName", false, js_DirectoryStream_get_fileName, NULL);
	api_define_property("DirectoryStream", "position", false, js_DirectoryStream_get_position, js_DirectoryStream_set_position);
	api_define_method("DirectoryStream", "next", js_DirectoryStream_next);
	api_define_method("DirectoryStream", "rewind", js_DirectoryStream_rewind);
	api_define_function("Dispatch", "cancel", js_Dispatch_cancel);
	api_define_function("Dispatch", "cancelAll", js_Dispatch_cancelAll);
	api_define_function("Dispatch", "later", js_Dispatch_later);
	api_define_function("Dispatch", "now", js_Dispatch_now);
	api_define_function("Dispatch", "onRender", js_Dispatch_onRender);
	api_define_function("Dispatch", "onUpdate", js_Dispatch_onUpdate);
	api_define_class("FileStream", CLASS_FILE_STREAM, js_new_FileStream, js_FileStream_finalize);
	api_define_method("FileStream", "dispose", js_FileStream_dispose);
	api_define_property("FileStream", "fileName", false, js_FileStream_get_fileName, NULL);
	api_define_property("FileStream", "fileSize", false, js_FileStream_get_fileSize, NULL);
	api_define_property("FileStream", "position", false, js_FileStream_get_position, js_FileStream_set_position);
	api_define_method("FileStream", "read", js_FileStream_read);
	api_define_method("FileStream", "write", js_FileStream_write);
	api_define_class("Font", CLASS_FONT, js_new_Font, js_Font_finalize);
	api_define_static_prop("Font", "Default", js_Font_get_Default, NULL);
	api_define_property("Font", "fileName", false, js_Font_get_fileName, NULL);
	api_define_property("Font", "height", false, js_Font_get_height, NULL);
	api_define_method("Font", "drawText", js_Font_drawText);
	api_define_method("Font", "getTextSize", js_Font_getTextSize);
	api_define_method("Font", "wordWrap", js_Font_wordWrap);
	api_define_function("FS", "createDirectory", js_FS_createDirectory);
	api_define_function("FS", "deleteFile", js_FS_deleteFile);
	api_define_function("FS", "directoryExists", js_FS_directoryExists);
	api_define_function("FS", "evaluateScript", js_FS_evaluateScript);
	api_define_function("FS", "fileExists", js_FS_fileExists);
	api_define_function("FS", "fullPath", js_FS_fullPath);
	api_define_function("FS", "readFile", js_FS_readFile);
	api_define_function("FS", "relativePath", js_FS_relativePath);
	api_define_function("FS", "removeDirectory", js_FS_removeDirectory);
	api_define_function("FS", "rename", js_FS_rename);
	api_define_function("FS", "writeFile", js_FS_writeFile);
	api_define_class("IndexList", CLASS_INDEX_LIST, js_new_IndexList, js_IndexList_finalize);
	api_define_class("JobToken", CLASS_JOB_TOKEN, NULL, js_JobToken_finalize);
	api_define_class("Joystick", CLASS_JOYSTICK, NULL, js_Joystick_finalize);
	api_define_static_prop("Joystick", "Null", js_Joystick_get_Null, NULL);
	api_define_function("Joystick", "getDevices", js_Joystick_getDevices);
	api_define_property("Joystick", "name", false, js_Joystick_get_name, NULL);
	api_define_property("Joystick", "numAxes", false, js_Joystick_get_numAxes, NULL);
	api_define_property("Joystick", "numButtons", false, js_Joystick_get_numButtons, NULL);
	api_define_method("Joystick", "getPosition", js_Joystick_getPosition);
	api_define_method("Joystick", "isPressed", js_Joystick_isPressed);
	api_define_class("Keyboard", CLASS_KEYBOARD, NULL, NULL);
	api_define_static_prop("Keyboard", "Default", js_Keyboard_get_Default, NULL);
	api_define_property("Keyboard", "capsLock", false, js_Keyboard_get_capsLock, NULL);
	api_define_property("Keyboard", "numLock", false, js_Keyboard_get_numLock, NULL);
	api_define_property("Keyboard", "scrollLock", false, js_Keyboard_get_scrollLock, NULL);
	api_define_method("Keyboard", "clearQueue", js_Keyboard_clearQueue);
	api_define_method("Keyboard", "getChar", js_Keyboard_getChar);
	api_define_method("Keyboard", "getKey", js_Keyboard_getKey);
	api_define_method("Keyboard", "isPressed", js_Keyboard_isPressed);
	api_define_class("Mixer", CLASS_MIXER, js_new_Mixer, js_Mixer_finalize);
	api_define_static_prop("Mixer", "Default", js_Mixer_get_Default, NULL);
	api_define_property("Mixer", "volume", false, js_Mixer_get_volume, js_Mixer_set_volume);
	api_define_class("Model", CLASS_MODEL, js_new_Model, js_Model_finalize);
	api_define_property("Model", "shader", false, js_Model_get_shader, js_Model_set_shader);
	api_define_property("Model", "transform", false, js_Model_get_transform, js_Model_set_transform);
	api_define_method("Model", "draw", js_Model_draw);
	api_define_method("Model", "setBoolean", js_Model_setBoolean);
	api_define_method("Model", "setColorVector", js_Model_setColorVector);
	api_define_method("Model", "setFloat", js_Model_setFloat);
	api_define_method("Model", "setFloatArray", js_Model_setFloatArray);
	api_define_method("Model", "setFloatVector", js_Model_setFloatVector);
	api_define_method("Model", "setInt", js_Model_setInt);
	api_define_method("Model", "setIntArray", js_Model_setIntArray);
	api_define_method("Model", "setIntVector", js_Model_setIntVector);
	api_define_method("Model", "setMatrix", js_Model_setMatrix);
	api_define_class("Mouse", CLASS_MOUSE, NULL, NULL);
	api_define_static_prop("Mouse", "Default", js_Mouse_get_Default, NULL);
	api_define_property("Mouse", "x", false, js_Mouse_get_x, NULL);
	api_define_property("Mouse", "y", false, js_Mouse_get_y, NULL);
	api_define_method("Mouse", "clearQueue", js_Mouse_clearQueue);
	api_define_method("Mouse", "getEvent", js_Mouse_getEvent);
	api_define_method("Mouse", "isPressed", js_Mouse_isPressed);
	api_define_class("RNG", CLASS_RNG, js_new_RNG, js_RNG_finalize);
	api_define_function("RNG", "fromSeed", js_RNG_fromSeed);
	api_define_function("RNG", "fromState", js_RNG_fromState);
	api_define_property("RNG", "state", false, js_RNG_get_state, js_RNG_set_state);
	api_define_method("RNG", "next", js_RNG_next);
	api_define_function("SSj", "log", js_SSj_log);
	api_define_function("SSj", "trace", js_SSj_trace);
	api_define_class("Sample", CLASS_SAMPLE, js_new_Sample, js_Sample_finalize);
	api_define_property("Sample", "fileName", false, js_Sample_get_fileName, NULL);
	api_define_method("Sample", "play", js_Sample_play);
	api_define_method("Sample", "stopAll", js_Sample_stopAll);
	api_define_class("Server", CLASS_SERVER, js_new_Server, js_Server_finalize);
	api_define_method("Server", "close", js_Server_close);
	api_define_method("Server", "accept", js_Server_accept);
	api_define_class("Shader", CLASS_SHADER, js_new_Shader, js_Shader_finalize);
	api_define_static_prop("Shader", "Default", js_Shader_get_Default, NULL);
	api_define_class("Shape", CLASS_SHAPE, js_new_Shape, js_Shape_finalize);
	api_define_property("Shape", "indexList", false, js_Shape_get_indexList, js_Shape_set_indexList);
	api_define_property("Shape", "texture", false, js_Shape_get_texture, js_Shape_set_texture);
	api_define_property("Shape", "vertexList", false, js_Shape_get_vertexList, js_Shape_set_vertexList);
	api_define_method("Shape", "draw", js_Shape_draw);
	api_define_class("Socket", CLASS_SOCKET, js_new_Socket, js_Socket_finalize);
	api_define_property("Socket", "bytesPending", false, js_Socket_get_bytesPending, NULL);
	api_define_property("Socket", "connected", false, js_Socket_get_connected, NULL);
	api_define_property("Socket", "remoteAddress", false, js_Socket_get_remoteAddress, NULL);
	api_define_property("Socket", "remotePort", false, js_Socket_get_remotePort, NULL);
	api_define_method("Socket", "close", js_Socket_close);
	api_define_method("Socket", "connectTo", js_Socket_connectTo);
	api_define_method("Socket", "read", js_Socket_read);
	api_define_method("Socket", "write", js_Socket_write);
	api_define_class("Sound", CLASS_SOUND, js_new_Sound, js_Sound_finalize);
	api_define_property("Sound", "fileName", false, js_Sound_get_fileName, NULL);
	api_define_property("Sound", "length", false, js_Sound_get_length, NULL);
	api_define_property("Sound", "pan", false, js_Sound_get_pan, js_Sound_set_pan);
	api_define_property("Sound", "playing", false, js_Sound_get_playing, NULL);
	api_define_property("Sound", "position", false, js_Sound_get_position, js_Sound_set_position);
	api_define_property("Sound", "repeat", false, js_Sound_get_repeat, js_Sound_set_repeat);
	api_define_property("Sound", "speed", false, js_Sound_get_speed, js_Sound_set_speed);
	api_define_property("Sound", "volume", false, js_Sound_get_volume, js_Sound_set_volume);
	api_define_method("Sound", "pause", js_Sound_pause);
	api_define_method("Sound", "play", js_Sound_play);
	api_define_method("Sound", "stop", js_Sound_stop);
	api_define_class("SoundStream", CLASS_SOUND_STREAM, js_new_SoundStream, js_SoundStream_finalize);
	api_define_property("SoundStream", "length", false, js_SoundStream_get_length, NULL);
	api_define_method("SoundStream", "pause", js_SoundStream_pause);
	api_define_method("SoundStream", "play", js_SoundStream_play);
	api_define_method("SoundStream", "stop", js_SoundStream_stop);
	api_define_method("SoundStream", "write", js_SoundStream_write);
	api_define_class("Surface", CLASS_SURFACE, js_new_Surface, js_Surface_finalize);
	api_define_property("Surface", "height", false, js_Surface_get_height, NULL);
	api_define_property("Surface", "transform", false, js_Surface_get_transform, js_Surface_set_transform);
	api_define_property("Surface", "width", false, js_Surface_get_width, NULL);
	api_define_method("Surface", "clipTo", js_Surface_clipTo);
	api_define_method("Surface", "toTexture", js_Surface_toTexture);
	api_define_class("TextDecoder", CLASS_TEXT_DEC, js_new_TextDecoder, js_TextDecoder_finalize);
	api_define_property("TextDecoder", "encoding", false, js_TextDecoder_get_encoding, NULL);
	api_define_property("TextDecoder", "fatal", false, js_TextDecoder_get_fatal, NULL);
	api_define_property("TextDecoder", "ignoreBOM", false, js_TextDecoder_get_ignoreBOM, NULL);
	api_define_method("TextDecoder", "decode", js_TextDecoder_decode);
	api_define_class("TextEncoder", CLASS_TEXT_ENC, js_new_TextEncoder, js_TextEncoder_finalize);
	api_define_property("TextEncoder", "encoding", false, js_TextEncoder_get_encoding, NULL);
	api_define_method("TextEncoder", "encode", js_TextEncoder_encode);
	api_define_class("Texture", CLASS_TEXTURE, js_new_Texture, js_Texture_finalize);
	api_define_property("Texture", "fileName", false, js_Texture_get_fileName, NULL);
	api_define_property("Texture", "height", false, js_Texture_get_height, NULL);
	api_define_property("Texture", "width", false, js_Texture_get_width, NULL);
	api_define_class("Transform", CLASS_TRANSFORM, js_new_Transform, js_Transform_finalize);
	api_define_property("Transform", "matrix", false, js_Transform_get_matrix, NULL);
	api_define_method("Transform", "compose", js_Transform_compose);
	api_define_method("Transform", "identity", js_Transform_identity);
	api_define_method("Transform", "project2D", js_Transform_project2D);
	api_define_method("Transform", "project3D", js_Transform_project3D);
	api_define_method("Transform", "rotate", js_Transform_rotate);
	api_define_method("Transform", "scale", js_Transform_scale);
	api_define_method("Transform", "translate", js_Transform_translate);
	api_define_class("VertexList", CLASS_VERTEX_LIST, js_new_VertexList, js_VertexList_finalize);

	api_define_object(NULL, "screen", CLASS_SURFACE, image_ref(screen_backbuffer(g_screen)));
	api_define_static_prop("screen", "frameRate", js_screen_get_frameRate, js_screen_set_frameRate);
	api_define_static_prop("screen", "frameSkip", js_screen_get_frameSkip, js_screen_set_frameSkip);
	api_define_static_prop("screen", "fullScreen", js_screen_get_fullScreen, js_screen_set_fullScreen);
	api_define_function("screen", "flip", js_screen_flip);
	api_define_function("screen", "now", js_screen_now);
	api_define_function("screen", "resize", js_screen_resize);

	api_define_const("FileOp", "Read", FILE_OP_READ);
	api_define_const("FileOp", "Write", FILE_OP_WRITE);
	api_define_const("FileOp", "Update", FILE_OP_UPDATE);
	api_define_const("Key", "Alt", ALLEGRO_KEY_ALT);
	api_define_const("Key", "AltGr", ALLEGRO_KEY_ALTGR);
	api_define_const("Key", "Apostrophe", ALLEGRO_KEY_QUOTE);
	api_define_const("Key", "Backslash", ALLEGRO_KEY_BACKSLASH);
	api_define_const("Key", "Backspace", ALLEGRO_KEY_BACKSPACE);
	api_define_const("Key", "CapsLock", ALLEGRO_KEY_CAPSLOCK);
	api_define_const("Key", "CloseBrace", ALLEGRO_KEY_CLOSEBRACE);
	api_define_const("Key", "Comma", ALLEGRO_KEY_COMMA);
	api_define_const("Key", "Delete", ALLEGRO_KEY_DELETE);
	api_define_const("Key", "Down", ALLEGRO_KEY_DOWN);
	api_define_const("Key", "End", ALLEGRO_KEY_END);
	api_define_const("Key", "Enter", ALLEGRO_KEY_ENTER);
	api_define_const("Key", "Equals", ALLEGRO_KEY_EQUALS);
	api_define_const("Key", "Escape", ALLEGRO_KEY_ESCAPE);
	api_define_const("Key", "F1", ALLEGRO_KEY_F1);
	api_define_const("Key", "F2", ALLEGRO_KEY_F2);
	api_define_const("Key", "F3", ALLEGRO_KEY_F3);
	api_define_const("Key", "F4", ALLEGRO_KEY_F4);
	api_define_const("Key", "F5", ALLEGRO_KEY_F5);
	api_define_const("Key", "F6", ALLEGRO_KEY_F6);
	api_define_const("Key", "F7", ALLEGRO_KEY_F7);
	api_define_const("Key", "F8", ALLEGRO_KEY_F8);
	api_define_const("Key", "F9", ALLEGRO_KEY_F9);
	api_define_const("Key", "F10", ALLEGRO_KEY_F10);
	api_define_const("Key", "F11", ALLEGRO_KEY_F11);
	api_define_const("Key", "F12", ALLEGRO_KEY_F12);
	api_define_const("Key", "Home", ALLEGRO_KEY_HOME);
	api_define_const("Key", "Hyphen", ALLEGRO_KEY_MINUS);
	api_define_const("Key", "Insert", ALLEGRO_KEY_INSERT);
	api_define_const("Key", "LCtrl", ALLEGRO_KEY_LCTRL);
	api_define_const("Key", "LShift", ALLEGRO_KEY_LSHIFT);
	api_define_const("Key", "Left", ALLEGRO_KEY_LEFT);
	api_define_const("Key", "NumLock", ALLEGRO_KEY_NUMLOCK);
	api_define_const("Key", "OpenBrace", ALLEGRO_KEY_OPENBRACE);
	api_define_const("Key", "PageDown", ALLEGRO_KEY_PGDN);
	api_define_const("Key", "PageUp", ALLEGRO_KEY_PGUP);
	api_define_const("Key", "Period", ALLEGRO_KEY_FULLSTOP);
	api_define_const("Key", "RCtrl", ALLEGRO_KEY_RCTRL);
	api_define_const("Key", "RShift", ALLEGRO_KEY_RSHIFT);
	api_define_const("Key", "Right", ALLEGRO_KEY_RIGHT);
	api_define_const("Key", "ScrollLock", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const("Key", "Semicolon", ALLEGRO_KEY_SEMICOLON);
	api_define_const("Key", "Slash", ALLEGRO_KEY_SLASH);
	api_define_const("Key", "Space", ALLEGRO_KEY_SPACE);
	api_define_const("Key", "Tab", ALLEGRO_KEY_TAB);
	api_define_const("Key", "Tilde", ALLEGRO_KEY_TILDE);
	api_define_const("Key", "Up", ALLEGRO_KEY_UP);
	api_define_const("Key", "A", ALLEGRO_KEY_A);
	api_define_const("Key", "B", ALLEGRO_KEY_B);
	api_define_const("Key", "C", ALLEGRO_KEY_C);
	api_define_const("Key", "D", ALLEGRO_KEY_D);
	api_define_const("Key", "E", ALLEGRO_KEY_E);
	api_define_const("Key", "F", ALLEGRO_KEY_F);
	api_define_const("Key", "G", ALLEGRO_KEY_G);
	api_define_const("Key", "H", ALLEGRO_KEY_H);
	api_define_const("Key", "I", ALLEGRO_KEY_I);
	api_define_const("Key", "J", ALLEGRO_KEY_J);
	api_define_const("Key", "K", ALLEGRO_KEY_K);
	api_define_const("Key", "L", ALLEGRO_KEY_L);
	api_define_const("Key", "M", ALLEGRO_KEY_M);
	api_define_const("Key", "N", ALLEGRO_KEY_N);
	api_define_const("Key", "O", ALLEGRO_KEY_O);
	api_define_const("Key", "P", ALLEGRO_KEY_P);
	api_define_const("Key", "Q", ALLEGRO_KEY_Q);
	api_define_const("Key", "R", ALLEGRO_KEY_R);
	api_define_const("Key", "S", ALLEGRO_KEY_S);
	api_define_const("Key", "T", ALLEGRO_KEY_T);
	api_define_const("Key", "U", ALLEGRO_KEY_U);
	api_define_const("Key", "V", ALLEGRO_KEY_V);
	api_define_const("Key", "W", ALLEGRO_KEY_W);
	api_define_const("Key", "X", ALLEGRO_KEY_X);
	api_define_const("Key", "Y", ALLEGRO_KEY_Y);
	api_define_const("Key", "Z", ALLEGRO_KEY_Z);
	api_define_const("Key", "D1", ALLEGRO_KEY_1);
	api_define_const("Key", "D2", ALLEGRO_KEY_2);
	api_define_const("Key", "D3", ALLEGRO_KEY_3);
	api_define_const("Key", "D4", ALLEGRO_KEY_4);
	api_define_const("Key", "D5", ALLEGRO_KEY_5);
	api_define_const("Key", "D6", ALLEGRO_KEY_6);
	api_define_const("Key", "D7", ALLEGRO_KEY_7);
	api_define_const("Key", "D8", ALLEGRO_KEY_8);
	api_define_const("Key", "D9", ALLEGRO_KEY_9);
	api_define_const("Key", "D0", ALLEGRO_KEY_0);
	api_define_const("Key", "NumPad1", ALLEGRO_KEY_PAD_1);
	api_define_const("Key", "NumPad2", ALLEGRO_KEY_PAD_2);
	api_define_const("Key", "NumPad3", ALLEGRO_KEY_PAD_3);
	api_define_const("Key", "NumPad4", ALLEGRO_KEY_PAD_4);
	api_define_const("Key", "NumPad5", ALLEGRO_KEY_PAD_5);
	api_define_const("Key", "NumPad6", ALLEGRO_KEY_PAD_6);
	api_define_const("Key", "NumPad7", ALLEGRO_KEY_PAD_7);
	api_define_const("Key", "NumPad8", ALLEGRO_KEY_PAD_8);
	api_define_const("Key", "NumPad9", ALLEGRO_KEY_PAD_9);
	api_define_const("Key", "NumPad0", ALLEGRO_KEY_PAD_0);
	api_define_const("Key", "NumPadEnter", ALLEGRO_KEY_PAD_ENTER);
	api_define_const("Key", "Add", ALLEGRO_KEY_PAD_PLUS);
	api_define_const("Key", "Decimal", ALLEGRO_KEY_PAD_DELETE);
	api_define_const("Key", "Divide", ALLEGRO_KEY_PAD_SLASH);
	api_define_const("Key", "Multiply", ALLEGRO_KEY_PAD_ASTERISK);
	api_define_const("Key", "Subtract", ALLEGRO_KEY_PAD_MINUS);
	api_define_const("MouseKey", "Left", MOUSE_KEY_LEFT);
	api_define_const("MouseKey", "Right", MOUSE_KEY_RIGHT);
	api_define_const("MouseKey", "Middle", MOUSE_KEY_MIDDLE);
	api_define_const("MouseKey", "WheelUp", MOUSE_KEY_WHEEL_UP);
	api_define_const("MouseKey", "WheelDown", MOUSE_KEY_WHEEL_DOWN);
	api_define_const("ShapeType", "Fan", SHAPE_TRI_FAN);
	api_define_const("ShapeType", "Lines", SHAPE_LINES);
	api_define_const("ShapeType", "LineLoop", SHAPE_LINE_LOOP);
	api_define_const("ShapeType", "LineStrip", SHAPE_LINE_STRIP);
	api_define_const("ShapeType", "Points", SHAPE_POINTS);
	api_define_const("ShapeType", "Triangles", SHAPE_TRIANGLES);
	api_define_const("ShapeType", "TriStrip", SHAPE_TRI_STRIP);

	// pre-create joystick objects for Joystick.getDevices().  this ensures that
	// multiple requests for the device list return the same Joystick object(s).
	load_joysticks();

	// register predefined X11 colors
	jsal_get_global_string("Color");
	p = COLORS;
	while (p->name != NULL) {
		jsal_push_eval("({ enumerable: false, configurable: true })");
		jsal_push_function(js_Color_get_Color, "get", 0, (int)(p - COLORS));
		jsal_put_prop_string(-2, "get");
		jsal_def_prop_string(-2, p->name);
		++p;
	}
	jsal_pop(1);
}

bool
pegasus_run(void)
{
	if (jsal_try(jsal_safe_event_loop, 0)) {
		jsal_pop(1);  // don't need return value
		return true;
	}
	else {
		// leave the error for the caller, don't pop it off
		return false;
	}
}

static void
on_import_module(void)
{
	const char* const PATHS[] =
	{
		"@/lib",
		"#/game_modules",
		"#/runtime",
	};

	char*       caller_id;
	const char* origin;
	path_t*     path;
	char*       source;
	size_t      source_len;
	char*       specifier;

	int i;

	// strdup() here because the JSAL-managed strings may get overwritten
	// in the course of a filename lookup.
	specifier = strdup(jsal_require_string(0));
	caller_id = strdup(jsal_require_string(1));

	// HACK: the way JSAL is currently designed, the same filename is used for
	//       module resolution as for display; this works around the limitation
	//       until more comprehensive refactoring can be done.
	origin = debugger_compiled_name(caller_id);

	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if (path = find_module(specifier, origin, PATHS[i], true))
			break;  // short-circuit
	}
	free(specifier);
	free(caller_id);

	if (path == NULL)
		jsal_error(JS_REF_ERROR, "couldn't find JS module '%s'", specifier);
	if (path_has_extension(path, ".mjs")) {
		source = game_read_file(g_game, path_cstr(path), &source_len);
		jsal_push_string(debugger_source_name(path_cstr(path)));
		jsal_push_lstring(source, source_len);
		free(source);
	}
	else {
		// ES module shim to allow 'import' of CommonJS modules
		jsal_push_sprintf("%%/moduleShim-%d.mjs", s_next_module_id++);
		jsal_push_sprintf("export default require(\"%s\");", path_cstr(path));
	}
}

bool
jsal_pegasus_eval_module(const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  Duktape's stack-based API is powerful, but gets
	// very messy very quickly when dealing with object properties.  I tried to add
	// comments to illuminate what's going on, but it's still likely to be confusing for
	// someone not familiar with Duktape code.  proceed with caution.

	// notes:
	//     - the final value of 'module.exports' is left on top of the Duktape value stack.
	//     - 'module.id' is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be in canonical form.
	//     - this is a protected call.  if the module being loaded throws, the error will be
	//       caught and left on top of the stack for the caller to deal with.

	lstring_t* code_string;
	path_t*    dir_path;
	path_t*    file_path;
	bool       is_module_loaded;
	char*      module_name;
	size_t     source_size;
	char*      source;

	file_path = path_new(filename);
	dir_path = path_strip(path_dup(file_path));

	// is the requested module already in the cache?
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	if (jsal_get_prop_string(-1, filename)) {
		jsal_remove(-2);
		jsal_remove(-2);
		goto have_module;
	}
	else {
		jsal_pop(3);
	}

	console_log(1, "initializing JS module '%s'", filename);

	source = game_read_file(g_game, filename, &source_size);
	code_string = lstr_from_utf8(source, source_size, true);
	free(source);

	// construct a module object for the new module
	jsal_push_new_object();  // module object
	jsal_push_new_object();
	jsal_put_prop_string(-2, "exports");  // module.exports = {}
	jsal_push_string(filename);
	jsal_put_prop_string(-2, "filename");  // module.filename
	jsal_push_string(filename);
	jsal_put_prop_string(-2, "id");  // module.id
	jsal_push_boolean(false);
	jsal_put_prop_string(-2, "loaded");  // module.loaded = false
	jsal_pegasus_push_require(filename);
	jsal_put_prop_string(-2, "require");  // module.require

	// evaluate .mjs scripts as ES6 modules
	if (path_has_extension(file_path, ".mjs")) {
		jsal_push_sprintf("import * as Module from \"%s\"; global.___exports = Module;",
			filename);
		module_name = strnewf("%%/moduleShim-%d.mjs", s_next_module_id++);
		is_module_loaded = jsal_try_eval_module(module_name);
		free(module_name);
		if (!is_module_loaded)
			goto on_error;
		jsal_pop(2);
		jsal_get_global_string("___exports");
		jsal_del_global_string("___exports");
		return true;
	}

	// cache the module object in advance
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_dup(-3);
	jsal_put_prop_string(-2, filename);
	jsal_pop(2);

	if (strcmp(path_extension(file_path), ".json") == 0) {
		// JSON file, decode to JavaScript object
		jsal_push_lstring_t(code_string);
		lstr_free(code_string);
		if (!jsal_try_parse(-1))
			goto on_error;
		jsal_put_prop_string(-2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the simplest way to
		// implement CommonJS semantics and matches the behavior of Node.js.
		jsal_push_sprintf("(function (exports, require, module, __filename, __dirname) {%s%s\n})",
			strncmp(lstr_cstr(code_string), "#!", 2) == 0 ? "//" : "",  // shebang?
			lstr_cstr(code_string));
		lstr_free(code_string);
		if (!jsal_try_compile(debugger_source_name(filename)))
			goto on_error;
		jsal_call(0);
		jsal_push_new_object();
		jsal_push_string("main");
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "name");
		
		// go, go, go!
		jsal_get_prop_string(-2, "exports");    // this = exports
		jsal_get_prop_string(-3, "exports");    // exports
		jsal_get_prop_string(-4, "require");    // require
		jsal_dup(-5);                           // module
		jsal_push_string(filename);             // __filename
		jsal_push_string(path_cstr(dir_path));  // __dirname
		if (!jsal_try_call_method(5))
			goto on_error;
		jsal_pop(1);
	}

	// module executed successfully, set 'module.loaded' to true
	jsal_push_boolean(true);
	jsal_put_prop_string(-2, "loaded");

have_module:
	// 'module` is on the stack, we need `module.exports'
	jsal_get_prop_string(-1, "exports");
	jsal_remove(-2);
	return true;

on_error:
	// note: it's assumed that at this point, the only things left in our portion of the
	//       value stack are the module object and the thrown error.
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_del_prop_string(-1, filename);
	jsal_pop(2);
	jsal_remove(-2);  // leave the error on the stack
	return false;
}

static void
jsal_pegasus_push_color(color_t color, bool in_ctor)
{
	color_t* color_ptr;
	
	color_ptr = malloc(sizeof(color_t));
	*color_ptr = color;
	jsal_push_class_obj(CLASS_COLOR, color_ptr, in_ctor);
}

static void
jsal_pegasus_push_job_token(int64_t token)
{
	int64_t* ptr;

	ptr = malloc(sizeof(int64_t));
	*ptr = token;
	jsal_push_class_obj(CLASS_JOB_TOKEN, ptr, false);
}

static void
jsal_pegasus_push_require(const char* module_id)
{
	jsal_push_function(js_require, "require", 1, 0);
	
	// assign 'require.cache'
	jsal_push_new_object();
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_remove(-2);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "cache");
	
	if (module_id != NULL) {
		jsal_push_new_object();
		jsal_push_string(module_id);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "id");  // require.id
	}
}

static color_t
jsal_pegasus_require_color(int index)
{
	color_t* color_ptr;

	color_ptr = jsal_require_class_obj(index, CLASS_COLOR);
	return *color_ptr;
}

static script_t*
jsal_pegasus_require_script(int index)
{
	return script_new_func(index);
}

static bool
jsal_safe_event_loop(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	while (async_busy()) {
		screen_flip(g_screen, s_framerate, true);
		image_set_scissor(screen_backbuffer(g_screen), screen_bounds(g_screen));
	}
	return false;
}

static path_t*
find_module(const char* id, const char* origin, const char* sys_origin, bool es6_mode)
{
	const char* const PATTERNS[] =
	{
		"%s",
		"%s.mjs",
		"%s.js",
		"%s.json",
		"%s/package.json",
		"%s/index.js",
		"%s/index.json",
	};

	path_t*      origin_path;
	char*        filename;
	path_t*      main_path;
	path_t*      path;

	int i;

	if (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0) {
		// resolve module relative to calling module
		origin_path = path_new(origin != NULL ? origin : "./");
	}
	else {
		// resolve module from designated module repository
		origin_path = path_new_dir(sys_origin);
	}

	for (i = 0; i < sizeof PATTERNS / sizeof PATTERNS[0]; ++i) {
		filename = strnewf(PATTERNS[i], id);
		if (strncmp(id, "@/", 2) == 0 || strncmp(id, "$/", 2) == 0 || strncmp(id, "~/", 2) == 0 || strncmp(id, "#/", 2) == 0) {
			path = game_full_path(g_game, filename, NULL, false);
		}
		else {
			path = path_dup(origin_path);
			path_strip(path);
			path_append(path, filename);
			path_collapse(path, true);
		}
		free(filename);
		if (game_file_exists(g_game, path_cstr(path))) {
			if (strcmp(path_filename(path), "package.json") != 0) {
				return path;
			}
			else {
				if (!(main_path = load_package_json(path_cstr(path))))
					goto next_filename;
				if (game_file_exists(g_game, path_cstr(main_path))) {
					path_free(path);
					return main_path;
				}
			}
		}

	next_filename:
		path_free(path);
	}

	return NULL;
}

static void
load_joysticks(void)
{
	int* device;
	int  num_devices;

	int i;

	jsal_push_hidden_stash();
	jsal_push_new_array();
	num_devices = joy_num_devices();
	for (i = 0; i < num_devices; ++i) {
		device = malloc(sizeof(int));
		*device = i;
		jsal_push_class_obj(CLASS_JOYSTICK, device, false);
		jsal_put_prop_index(-2, i);
	}
	jsal_put_prop_string(-2, "joystickObjects");
	jsal_pop(1);
}

static path_t*
load_package_json(const char* filename)
{
	int jsal_top;
	char*     json;
	size_t    json_size;
	path_t*   path;

	jsal_top = jsal_get_top();
	if (!(json = game_read_file(g_game, filename, &json_size)))
		goto on_error;
	jsal_push_lstring(json, json_size);
	free(json);
	if (!jsal_try_parse(-1))
		goto on_error;
	if (!jsal_is_object_coercible(-1))
		goto on_error;
	jsal_get_prop_string(-1, "main");
	if (!jsal_is_string(-1))
		goto on_error;
	path = path_strip(path_new(filename));
	path_append(path, jsal_get_string(-1));
	path_collapse(path, true);
	if (!game_file_exists(g_game, path_cstr(path)))
		goto on_error;
	return path;

on_error:
	jsal_set_top(jsal_top);
	return NULL;
}

static bool
js_require(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* const PATHS[] =
	{
		"@/lib",
		"#/game_modules",
		"#/runtime",
	};
	
	const char* id;
	const char* parent_id = NULL;
	path_t*     path;

	int i;

	id = jsal_require_string(0);

	jsal_push_ref(me);
	if (jsal_get_prop_string(-1, "id"))
		parent_id = jsal_get_string(-1);

	if (parent_id == NULL && (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0))
		jsal_error(JS_TYPE_ERROR, "require() outside of a CommonJS module must be absolute");
	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if (path = find_module(id, parent_id, PATHS[i], false))
			break;  // short-circuit
	}
	if (path == NULL)
		jsal_error(JS_REF_ERROR, "module not found '%s'", id);
	if (!jsal_pegasus_eval_module(path_cstr(path)))
		jsal_throw();
	return true;
}

static bool
js_screen_get_frameRate(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	// as far as Sphere v2 code is concerned, infinity, not 0, means "unthrottled".
	// that's stored as a zero internally though, so we need to translate.
	jsal_push_number(s_framerate > 0 ? s_framerate : INFINITY);
	return true;
}

static bool
js_screen_set_frameRate(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double framerate;

	framerate = jsal_require_number(0);

	if (framerate < 1.0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");
	if (framerate != INFINITY)
		s_framerate = framerate;
	else
		s_framerate = 0;  // unthrottled
	return false;
}

static bool
js_screen_get_frameSkip(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_number(screen_get_frameskip(g_screen));
	return true;
}

static bool
js_screen_set_frameSkip(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double max_skips;

	max_skips = jsal_require_number(0);

	if (max_skips < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid frameskip");
	screen_set_frameskip(g_screen, max_skips);
	return false;
}

static bool
js_screen_get_fullScreen(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(screen_get_fullscreen(g_screen));
	return true;
}

static bool
js_screen_set_fullScreen(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	bool fullscreen;

	fullscreen = jsal_require_boolean(0);

	screen_set_fullscreen(g_screen, fullscreen);
	return false;
}

static bool
js_screen_flip(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	screen_flip(g_screen, s_framerate, true);
	image_set_scissor(screen_backbuffer(g_screen), screen_bounds(g_screen));
	return false;
}

static bool
js_screen_now(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_number(screen_now(g_screen));
	return true;
}

static bool
js_screen_resize(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int  width;
	int  height;

	width = jsal_require_int(0);
	height = jsal_require_int(1);

	if (width < 0 || height < 0)
		jsal_error(JS_RANGE_ERROR, "invalid screen resolution");
	screen_resize(g_screen, width, height);
	return false;
}

static bool
js_Sphere_get_APILevel(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_int(API_LEVEL);
	return true;
}

static bool
js_Sphere_get_Game(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_lstring_t(game_manifest(g_game));
	jsal_parse(-1);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Game");

	return true;
}

static bool
js_Sphere_get_Platform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_sprintf("%s %s", SPHERE_ENGINE_NAME, SPHERE_VERSION);
	return true;
}

static bool
js_Sphere_get_Version(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_int(API_VERSION);
	return true;
}

static bool
js_Sphere_abort(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* filename;
	int         line_number;
	const char* message;
	char*       text;

	message = num_args >= 1
		? jsal_to_string(0)
		: "some type of eaty pig just ate your game\n\n\n...and you*munch*";

	/*jsal_inspect_callstack_entry(-2);
	jsal_get_prop_string(-1, "lineNumber");
	jsal_get_prop_string(-2, "function");
	jsal_get_prop_string(-1, "fileName");*/

	filename = "THE PIG ATE IT";
	line_number = 812;
	text = strnewf("%s:%d\nmanual abort\n\n%s", filename, line_number, message);
	sphere_abort(text);
}

static bool
js_Sphere_exit(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sphere_exit(false);
}

static bool
js_Sphere_restart(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sphere_restart();
}

static bool
js_Sphere_run(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sphere_run(true);
	jsal_push_boolean(true);
	return true;
}

static bool
js_Sphere_sleep(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double timeout;

	timeout = jsal_require_number(0);

	if (timeout < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid sleep timeout");
	sphere_sleep(timeout);
	return false;
}

static bool
js_Color_get_Color(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const struct x11_color* data;

	data = &COLORS[magic];
	jsal_pegasus_push_color(color_new(data->r, data->g, data->b, data->a), false);
	return true;
}

static bool
js_Color_is(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t color1;
	color_t color2;

	color1 = jsal_pegasus_require_color(0);
	color2 = jsal_pegasus_require_color(1);

	jsal_push_boolean(color1.r == color2.r
		&& color1.g == color2.g
		&& color1.b == color2.b);
	return true;
}

static bool
js_Color_mix(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t color1;
	color_t color2;
	float   w1 = 1.0;
	float   w2 = 1.0;

	color1 = jsal_pegasus_require_color(0);
	color2 = jsal_pegasus_require_color(1);
	if (num_args > 2) {
		w1 = jsal_require_number(2);
		w2 = jsal_require_number(3);
	}

	if (w1 < 0.0 || w2 < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid weight(s)", w1, w2);

	jsal_pegasus_push_color(color_mix(color1, color2, w1, w2), false);
	return true;
}

static bool
js_Color_of(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t     color;
	size_t      hex_length;
	const char* name;
	uint32_t    value;

	const struct x11_color* p;

	name = jsal_require_string(0);

	// check if caller gave an X11 color name
	p = &COLORS[0];
	while (p->name != NULL) {
		if (strcasecmp(name, p->name) == 0) {
			jsal_pegasus_push_color(color_new(p->r, p->g, p->b, p->a), false);
			return true;
		}
		++p;
	}

	// is 'name' an RGB or ARGB signature?
	if (name[0] != '#')
		jsal_error(JS_TYPE_ERROR, "unrecognized color name");
	hex_length = strspn(&name[1], "0123456789ABCDEFabcdef");
	if (hex_length != strlen(name) - 1 || (hex_length != 6 && hex_length != 8))
		jsal_error(JS_TYPE_ERROR, "invalid RGB signature");
	value = strtoul(&name[1], NULL, 16);
	color.a = hex_length == 8 ? (value >> 24) & 0xFF : 255;
	color.r = (value >> 16) & 0xFF;
	color.g = (value >> 8) & 0xFF;
	color.b = value & 0xFF;
	jsal_pegasus_push_color(color, false);
	return true;
}

static bool
js_new_Color(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float   a = 1.0;
	color_t color;
	float   r, g, b;

	r = jsal_require_number(0);
	g = jsal_require_number(1);
	b = jsal_require_number(2);
	if (num_args >= 4)
		a = jsal_require_number(3);

	// construct a Color object
	color = color_new(
		fmin(fmax(r, 0.0), 1.0) * 255,
		fmin(fmax(g, 0.0), 1.0) * 255,
		fmin(fmax(b, 0.0), 1.0) * 255,
		fmin(fmax(a, 0.0), 1.0) * 255);
	jsal_pegasus_push_color(color, true);
	return true;
}

static void
js_Color_finalize(void* host_ptr)
{
	free(host_ptr);
}

static bool
js_Color_get_name(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t color;

	const struct x11_color* p;

	jsal_push_this();
	color = jsal_pegasus_require_color(-1);

	p = &COLORS[0];
	while (p->name != NULL) {
		if (color.r == p->r && color.g == p->g && color.b == p->b && color.a == p->a) {
			jsal_push_string(p->name);
			return true;
		}
		++p;
	}

	jsal_push_sprintf("#%.2x%.2x%.2x%.2x", color.a, color.r, color.g, color.b);
	return true;
}

static bool
js_Color_get_r(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);

	jsal_push_number(color->r / 255.0);
	return true;
}

static bool
js_Color_get_g(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);

	jsal_push_number(color->g / 255.0);
	return true;
}

static bool
js_Color_get_b(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);

	jsal_push_number(color->b / 255.0);
	return true;
}

static bool
js_Color_get_a(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);

	jsal_push_number(color->a / 255.0);
	return true;
}

static bool
js_Color_set_r(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);
	value = jsal_require_number(0);

	color->r = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_g(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);
	value = jsal_require_number(0);

	color->g = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_b(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);
	value = jsal_require_number(0);

	color->b = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_a(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, CLASS_COLOR);
	value = jsal_require_number(0);

	color->a = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_clone(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t color;

	jsal_push_this();
	color = jsal_pegasus_require_color(-1);

	jsal_pegasus_push_color(color, false);
	return true;
}

static bool
js_Color_fade(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float   a;
	color_t color;

	jsal_push_this();
	color = jsal_pegasus_require_color(-1);
	a = jsal_require_number(0);

	color.a = fmin(fmax(color.a * a, 0), 255);
	jsal_pegasus_push_color(color, false);
	return true;
}

static bool
js_new_DirectoryStream(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char*  pathname;
	directory_t* stream;

	pathname = jsal_require_pathname(0, NULL, false, false);

	if (!(stream = directory_open(g_game, pathname)))
		jsal_error(JS_ERROR, "couldn't open directory");
	jsal_push_class_obj(CLASS_DIR_STREAM, stream, true);
	return true;
}

static void
js_DirectoryStream_finalize(void* host_ptr)
{
	directory_close(host_ptr);
}

static bool
js_DirectoryStream_dispose(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM);

	jsal_set_class_ptr(-1, NULL);
	directory_close(directory);
	return false;
}

static bool
js_DirectoryStream_get_fileCount(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");

	jsal_push_int(directory_num_files(directory));
	return true;
}

static bool
js_DirectoryStream_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");

	jsal_push_string(directory_pathname(directory));
	return true;
}

static bool
js_DirectoryStream_get_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");

	jsal_push_int(directory_position(directory));
	return true;
}

static bool
js_DirectoryStream_set_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;
	int          position;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");
	position = jsal_require_int(0);

	if (!directory_seek(directory, position))
		jsal_error(JS_ERROR, "couldn't set stream position");
	return false;
}

static bool
js_DirectoryStream_next(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t*  directory;
	const path_t* entry_path;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");

	entry_path = directory_next(directory);
	jsal_push_new_object();
	if (entry_path != NULL) {
		jsal_push_boolean(false);
		jsal_put_prop_string(-2, "done");
		jsal_push_new_object();
		if (path_is_file(entry_path))
			jsal_push_string(path_filename(entry_path));
		else
			jsal_push_sprintf("%s/", path_hop(entry_path, path_num_hops(entry_path) - 1));
		jsal_put_prop_string(-2, "fileName");
		jsal_push_string(path_cstr(entry_path));
		jsal_put_prop_string(-2, "fullPath");
		jsal_push_boolean(!path_is_file(entry_path));
		jsal_put_prop_string(-2, "isDirectory");
		jsal_put_prop_string(-2, "value");
	}
	else {
		jsal_push_boolean(true);
		jsal_put_prop_string(-2, "done");
	}
	return true;
}

static bool
js_DirectoryStream_rewind(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, CLASS_DIR_STREAM)))
		jsal_error(JS_ERROR, "object is disposed");

	directory_rewind(directory);
	return false;
}

static bool
js_Dispatch_cancel(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int64_t* token;

	token = jsal_require_class_obj(0, CLASS_JOB_TOKEN);

	async_cancel(*token);
	return false;
}

static bool
js_Dispatch_cancelAll(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	async_cancel_all(false);
	return false;
}

static bool
js_Dispatch_later(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	script_t* script;
	int       timeout;
	int64_t   token;

	timeout = jsal_require_int(0);
	script = jsal_pegasus_require_script(1);

	if (!(token = async_defer(script, timeout, ASYNC_UPDATE)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_now(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);

	if (!(token = async_defer(script, 0, ASYNC_TICK)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_onRender(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double    priority;
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);
	priority = num_args >= 2 ? jsal_require_number(1)
		: 0.0;

	if (!(token = async_recur(script, priority, ASYNC_RENDER)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_onUpdate(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double    priority;
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);
	priority = num_args >= 2 ? jsal_require_number(1)
		: 0.0;

	if (!(token = async_recur(script, priority, ASYNC_UPDATE)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_FS_createDirectory(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_mkdir(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't create directory '%s'", pathname);
	return false;
}

static bool
js_FS_deleteFile(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_unlink(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't delete file '%s'", pathname);
	return false;
}

static bool
js_FS_directoryExists(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);

	jsal_push_boolean(game_dir_exists(g_game, pathname));
	return true;
}

static bool
js_FS_evaluateScript(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!game_file_exists(g_game, filename))
		jsal_error(JS_ERROR, "script file not found '%s'", filename);
	if (!script_eval(filename, false))
		jsal_throw();
	return true;
}

static bool
js_FS_fileExists(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);

	jsal_push_boolean(game_file_exists(g_game, pathname));
	return true;
}

static bool
js_FS_fullPath(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* base_pathname = NULL;
	const char* pathname;

	if (num_args >= 2)
		base_pathname = jsal_require_pathname(1, NULL, false, false);
	pathname = jsal_require_pathname(0, base_pathname, false, false);

	jsal_push_string(pathname);
	return true;
}

static bool
js_FS_readFile(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	lstring_t*  content;
	void*       file_data;
	size_t      file_size;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);

	if (!(file_data = game_read_file(g_game, pathname, &file_size)))
		jsal_error(JS_ERROR, "couldn't read file '%s'", pathname);
	content = lstr_from_utf8(file_data, file_size, true);
	jsal_push_lstring_t(content);
	return true;
}

static bool
js_FS_relativePath(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* base_pathname;
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);
	base_pathname = jsal_require_pathname(1, NULL, false, false);

	path = game_relative_path(g_game, pathname, base_pathname);
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_FS_removeDirectory(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_rmdir(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't remove directory '%s'", pathname);
	return false;
}

static bool
js_FS_rename(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* new_pathname;
	const char* old_pathname;

	old_pathname = jsal_require_pathname(0, NULL, false, true);
	new_pathname = jsal_require_pathname(1, NULL, false, true);

	if (!game_rename(g_game, old_pathname, new_pathname))
		jsal_error(JS_ERROR, "couldn't rename file '%s'", old_pathname);
	return false;
}

static bool
js_FS_writeFile(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const void* file_data;
	size_t      file_size;
	const char* pathname;
	lstring_t*  text = NULL;

	pathname = jsal_require_pathname(0, NULL, false, true);
	text = jsal_require_lstring_t(1);

	file_data = lstr_cstr(text);
	file_size = lstr_len(text);
	if (!game_write_file(g_game, pathname, file_data, file_size))
		jsal_error(JS_ERROR, "couldn't write file '%s'", pathname);
	lstr_free(text);
	return false;
}

static bool
js_new_FileStream(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t*      file;
	enum file_op file_op;
	const char*  mode;
	const char*  pathname;

	jsal_require_string(0);
	file_op = jsal_require_int(1);
	if (file_op < 0 || file_op >= FILE_OP_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid file-op constant");

	pathname = jsal_require_pathname(0, NULL, false, file_op != FILE_OP_READ);
	if (file_op == FILE_OP_UPDATE && !game_file_exists(g_game, pathname))
		file_op = FILE_OP_WRITE;  // because 'r+b' requires the file to exist.
	mode = file_op == FILE_OP_READ ? "rb"
		: file_op == FILE_OP_WRITE ? "w+b"
		: file_op == FILE_OP_UPDATE ? "r+b"
		: NULL;
	if (!(file = file_open(g_game, pathname, mode)))
		jsal_error(JS_ERROR, "couldn't open file '%s'", pathname);
	if (file_op == FILE_OP_UPDATE)
		file_seek(file, 0, WHENCE_END);
	jsal_push_class_obj(CLASS_FILE_STREAM, file, true);
	return true;
}

static void
js_FileStream_finalize(void* host_ptr)
{
	file_close(host_ptr);
}

static bool
js_FileStream_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");

	jsal_push_string(file_pathname(file));
	return true;
}

static bool
js_FileStream_get_fileSize(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t* file;
	long    file_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");

	file_pos = file_position(file);
	file_seek(file, 0, WHENCE_END);
	jsal_push_number(file_position(file));
	file_seek(file, file_pos, WHENCE_SET);
	return true;
}

static bool
js_FileStream_get_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");

	jsal_push_number(file_position(file));
	return true;
}

static bool
js_FileStream_set_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t*   file;
	long long new_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");

	new_pos = jsal_require_number(0);
	file_seek(file, new_pos, WHENCE_SET);
	return false;
}

static bool
js_FileStream_dispose(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, CLASS_FILE_STREAM);

	jsal_set_class_ptr(-1, NULL);
	file_close(file);
	return false;
}

static bool
js_FileStream_read(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	void*   buffer;
	size_t  buffer_size;
	file_t* file;
	int     num_bytes;
	long    pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");
	if (num_args >= 1)
		num_bytes = jsal_require_int(0);

	if (num_bytes < 0)
		jsal_error(JS_RANGE_ERROR, "invalid read size '%d'", num_bytes);

	if (num_args < 1) {  // if no arguments, read entire file back to front
		pos = file_position(file);
		num_bytes = (file_seek(file, 0, WHENCE_END), file_position(file));
		file_seek(file, 0, WHENCE_SET);
	}

	jsal_push_new_buffer(JS_ARRAYBUFFER, num_bytes);
	buffer = jsal_get_buffer_ptr(-1, &buffer_size);
	num_bytes = (int)file_read(file, buffer, num_bytes, 1);
	if (num_args < 1)  // reset file position after whole-file read
		file_seek(file, pos, WHENCE_SET);
	return true;
}

static bool
js_FileStream_write(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const void* data;
	file_t*     file;
	size_t      num_bytes;

	data = jsal_require_buffer_ptr(0, &num_bytes);

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CLASS_FILE_STREAM)))
		jsal_error(JS_ERROR, "using a FileStream after dispose() is not allowed");

	if (file_write(file, data, num_bytes, 1) != num_bytes)
		jsal_error(JS_ERROR, "couldn't write to file");
	return false;
}

static bool
js_Font_get_Default(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(CLASS_FONT, g_system_font, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Font(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* filename;
	font_t*     font;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(font = font_load(filename)))
		jsal_error(JS_ERROR, "couldn't load font file '%s'", filename);
	jsal_push_this();
	jsal_push_class_obj(CLASS_FONT, font, true);
	return true;
}

static void
js_Font_finalize(void* host_ptr)
{
	font_unref(host_ptr);
}

static bool
js_Font_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, CLASS_FONT);

	jsal_push_string(font_path(font));
	return true;
}

static bool
js_Font_get_height(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, CLASS_FONT);

	jsal_push_int(font_height(font));
	return true;
}

static bool
js_Font_drawText(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t     color;
	font_t*     font;
	int         height;
	image_t*    surface;
	const char* text;
	int         width = 0;
	wraptext_t* wraptext;
	int         x;
	int         y;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, CLASS_FONT);
	surface = jsal_require_class_obj(0, CLASS_SURFACE);
	x = jsal_require_int(1);
	y = jsal_require_int(2);
	text = jsal_to_string(3);
	color = num_args >= 5 ? jsal_pegasus_require_color(4)
		: color_new(255, 255, 255, 255);
	if (num_args >= 6)
		width = jsal_require_int(5);

	if (surface == screen_backbuffer(g_screen) && screen_skip_frame(g_screen))
		return false;
	else {
		image_render_to(surface, NULL);
		shader_use(galileo_shader(), false);
		if (num_args < 6)
			font_draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
		else {
			wraptext = wraptext_new(text, font, width);
			height = font_height(font);
			for (i = 0; i < wraptext_len(wraptext); ++i)
				font_draw_text(font, color, x, y + i * height, TEXT_ALIGN_LEFT, wraptext_line(wraptext, i));
			wraptext_free(wraptext);
		}
	}
	return false;
}

static bool
js_Font_getTextSize(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	font_t*     font;
	int         num_lines;
	const char* text;
	int         width;
	wraptext_t* wraptext;

	jsal_push_this();
	font = jsal_require_class_obj(-1, CLASS_FONT);
	text = jsal_to_string(0);
	if (num_args >= 2)
		width = jsal_require_int(1);

	jsal_push_new_object();
	if (num_args >= 2) {
		wraptext = wraptext_new(text, font, width);
		num_lines = wraptext_len(wraptext);
		wraptext_free(wraptext);
		jsal_push_int(width);
		jsal_put_prop_string(-2, "width");
		jsal_push_int(font_height(font) * num_lines);
		jsal_put_prop_string(-2, "height");
	}
	else {
		jsal_push_int(font_get_width(font, text));
		jsal_put_prop_string(-2, "width");
		jsal_push_int(font_height(font));
		jsal_put_prop_string(-2, "height");
	}
	return true;
}

static bool
js_Font_wordWrap(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* text = jsal_to_string(0);
	int         width = jsal_require_int(1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, CLASS_FONT);
	jsal_pop(1);
	wraptext = wraptext_new(text, font, width);
	num_lines = wraptext_len(wraptext);
	jsal_push_new_array();
	for (i = 0; i < num_lines; ++i) {
		jsal_push_string(wraptext_line(wraptext, i));
		jsal_put_prop_index(-2, i);
	}
	wraptext_free(wraptext);
	return true;
}

static bool
js_new_IndexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	ibo_t*   ibo;
	uint16_t index;
	int      num_entries;

	int i;

	if (!jsal_is_array(0))
		jsal_error(JS_TYPE_ERROR, "expected an array");

	num_entries = (int)jsal_get_length(0);
	if (num_entries == 0)
		jsal_error(JS_RANGE_ERROR, "empty list not allowed");
	ibo = ibo_new();
	for (i = 0; i < num_entries; ++i) {
		jsal_get_prop_index(0, i);
		index = jsal_require_int(-1);
		if (index < 0 || index > UINT16_MAX) {
			ibo_unref(ibo);
			jsal_error(JS_RANGE_ERROR, "index value out of range");
		}
		ibo_add_index(ibo, index);
		jsal_pop(1);
	}
	if (!ibo_upload(ibo)) {
		ibo_unref(ibo);
		jsal_error(JS_ERROR, "upload to GPU failed");
	}
	jsal_push_class_obj(CLASS_INDEX_LIST, ibo, true);
	return true;
}

static void
js_IndexList_finalize(void* host_ptr)
{
	ibo_unref(host_ptr);
}

static void
js_JobToken_finalize(void* host_ptr)
{
	free(host_ptr);
}

static bool
js_Joystick_get_Null(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int* device;

	device = malloc(sizeof(int));
	*device = -1;
	jsal_push_class_obj(CLASS_JOYSTICK, device, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Null");
	jsal_pop(1);

	return true;
}

static bool
js_Joystick_getDevices(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int num_devices;

	int i;

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "joystickObjects");
	num_devices = (int)jsal_get_length(-1);
	jsal_push_new_array();
	for (i = 0; i < num_devices; ++i) {
		jsal_get_prop_index(-2, i);
		jsal_put_prop_index(-2, i);
	}
	return true;
}

static void
js_Joystick_finalize(void* host_ptr)
{
	free(host_ptr);
}

static bool
js_Joystick_get_name(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, CLASS_JOYSTICK);

	jsal_push_string(joy_name(*device));
	return true;
}

static bool
js_Joystick_get_numAxes(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, CLASS_JOYSTICK);

	if (*device != -1)
		jsal_push_int(joy_num_axes(*device));
	else
		jsal_push_number(INFINITY);
	return true;
}

static bool
js_Joystick_get_numButtons(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, CLASS_JOYSTICK);

	if (*device != -1)
		jsal_push_int(joy_num_buttons(*device));
	else
		jsal_push_number(INFINITY);
	return true;
}

static bool
js_Joystick_getPosition(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int  index;
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, CLASS_JOYSTICK);
	index = jsal_require_int(0);

	if (*device != -1 && (index < 0 || index >= joy_num_axes(*device)))
		jsal_error(JS_RANGE_ERROR, "joystick axis ID out of range");

	jsal_push_number(joy_position(*device, index));
	return true;
}

static bool
js_Joystick_isPressed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int  index;
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, CLASS_JOYSTICK);
	index = jsal_require_int(0);

	if (*device != -1 && (index < 0 || index >= joy_num_buttons(*device)))
		jsal_error(JS_RANGE_ERROR, "joystick button ID out of range");

	jsal_push_boolean(joy_is_button_down(*device, index));
	return true;
}

static bool
js_Keyboard_get_Default(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(CLASS_KEYBOARD, NULL, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_Keyboard_get_capsLock(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_CAPSLOCK));
	return true;
}

static bool
js_Keyboard_get_numLock(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_NUMLOCK));
	return true;
}

static bool
js_Keyboard_get_scrollLock(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_SCROLLLOCK));
	return true;
}

static bool
js_Keyboard_clearQueue(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);

	kb_clear_queue();
	return false;
}

static bool
js_Keyboard_getChar(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int  keycode;
	bool shifted;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);
	keycode = jsal_require_int(0);
	shifted = num_args >= 2 ? jsal_require_boolean(1)
		: false;

	switch (keycode) {
	case ALLEGRO_KEY_A: jsal_push_string(shifted ? "A" : "a"); break;
	case ALLEGRO_KEY_B: jsal_push_string(shifted ? "B" : "b"); break;
	case ALLEGRO_KEY_C: jsal_push_string(shifted ? "C" : "c"); break;
	case ALLEGRO_KEY_D: jsal_push_string(shifted ? "D" : "d"); break;
	case ALLEGRO_KEY_E: jsal_push_string(shifted ? "E" : "e"); break;
	case ALLEGRO_KEY_F: jsal_push_string(shifted ? "F" : "f"); break;
	case ALLEGRO_KEY_G: jsal_push_string(shifted ? "G" : "g"); break;
	case ALLEGRO_KEY_H: jsal_push_string(shifted ? "H" : "h"); break;
	case ALLEGRO_KEY_I: jsal_push_string(shifted ? "I" : "i"); break;
	case ALLEGRO_KEY_J: jsal_push_string(shifted ? "J" : "j"); break;
	case ALLEGRO_KEY_K: jsal_push_string(shifted ? "K" : "k"); break;
	case ALLEGRO_KEY_L: jsal_push_string(shifted ? "L" : "l"); break;
	case ALLEGRO_KEY_M: jsal_push_string(shifted ? "M" : "m"); break;
	case ALLEGRO_KEY_N: jsal_push_string(shifted ? "N" : "n"); break;
	case ALLEGRO_KEY_O: jsal_push_string(shifted ? "O" : "o"); break;
	case ALLEGRO_KEY_P: jsal_push_string(shifted ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: jsal_push_string(shifted ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: jsal_push_string(shifted ? "R" : "r"); break;
	case ALLEGRO_KEY_S: jsal_push_string(shifted ? "S" : "s"); break;
	case ALLEGRO_KEY_T: jsal_push_string(shifted ? "T" : "t"); break;
	case ALLEGRO_KEY_U: jsal_push_string(shifted ? "U" : "u"); break;
	case ALLEGRO_KEY_V: jsal_push_string(shifted ? "V" : "v"); break;
	case ALLEGRO_KEY_W: jsal_push_string(shifted ? "W" : "w"); break;
	case ALLEGRO_KEY_X: jsal_push_string(shifted ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: jsal_push_string(shifted ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: jsal_push_string(shifted ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: jsal_push_string(shifted ? "!" : "1"); break;
	case ALLEGRO_KEY_2: jsal_push_string(shifted ? "@" : "2"); break;
	case ALLEGRO_KEY_3: jsal_push_string(shifted ? "#" : "3"); break;
	case ALLEGRO_KEY_4: jsal_push_string(shifted ? "$" : "4"); break;
	case ALLEGRO_KEY_5: jsal_push_string(shifted ? "%" : "5"); break;
	case ALLEGRO_KEY_6: jsal_push_string(shifted ? "^" : "6"); break;
	case ALLEGRO_KEY_7: jsal_push_string(shifted ? "&" : "7"); break;
	case ALLEGRO_KEY_8: jsal_push_string(shifted ? "*" : "8"); break;
	case ALLEGRO_KEY_9: jsal_push_string(shifted ? "(" : "9"); break;
	case ALLEGRO_KEY_0: jsal_push_string(shifted ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: jsal_push_string(shifted ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: jsal_push_string(shifted ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: jsal_push_string(shifted ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: jsal_push_string(shifted ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: jsal_push_string(shifted ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: jsal_push_string(shifted ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: jsal_push_string(shifted ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: jsal_push_string(shifted ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: jsal_push_string(shifted ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: jsal_push_string(shifted ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: jsal_push_string(" "); break;
	case ALLEGRO_KEY_TAB: jsal_push_string("\t"); break;
	case ALLEGRO_KEY_TILDE: jsal_push_string(shifted ? "~" : "`"); break;
	default:
		jsal_push_string("");
	}
	return true;
}

static bool
js_Keyboard_getKey(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);

	if (kb_queue_len() > 0)
		jsal_push_int(kb_get_key());
	else
		jsal_push_null();
	return true;
}

static bool
js_Keyboard_isPressed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int keycode;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_KEYBOARD);
	keycode = jsal_require_int(0);

	jsal_push_boolean(kb_is_key_down(keycode));
	return true;
}

static bool
js_Mixer_get_Default(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(CLASS_MIXER, mixer_ref(s_def_mixer), false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Mixer(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int      bits;
	int      channels = 2;
	int      frequency;
	mixer_t* mixer;

	frequency = jsal_require_int(0);
	bits = jsal_require_int(1);
	if (num_args >= 3)
		channels = jsal_require_int(2);

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		jsal_error(JS_RANGE_ERROR, "invalid audio bit depth");
	if (channels < 1 || channels > 7)
		jsal_error(JS_RANGE_ERROR, "invalid channel count");
	
	if (!(mixer = mixer_new(frequency, bits, channels)))
		jsal_error(JS_ERROR, "couldn't create %d-bit %dch voice", bits, channels);
	jsal_push_class_obj(CLASS_MIXER, mixer, true);
	return true;
}

static void
js_Mixer_finalize(void* host_ptr)
{
	mixer_unref(host_ptr);
}

static bool
js_Mixer_get_volume(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mixer_t* mixer;

	jsal_push_this();
	mixer = jsal_require_class_obj(-1, CLASS_MIXER);

	jsal_push_number(mixer_get_gain(mixer));
	return true;
}

static bool
js_Mixer_set_volume(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float volume = jsal_require_number(0);

	mixer_t* mixer;

	jsal_push_this();
	mixer = jsal_require_class_obj(-1, CLASS_MIXER);

	mixer_set_gain(mixer, volume);
	return false;
}

static bool
js_new_Model(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	int       num_shapes;
	shader_t* shader;
	shape_t*  shape;

	int i;

	jsal_require_object_coercible(0);
	shader = num_args >= 2
		? jsal_require_class_obj(1, CLASS_SHADER)
		: galileo_shader();

	if (!jsal_is_array(0))
		jsal_error(JS_TYPE_ERROR, "array required");

	group = model_new(shader);
	num_shapes = jsal_get_length(0);
	for (i = 0; i < num_shapes; ++i) {
		jsal_get_prop_index(0, i);
		shape = jsal_require_class_obj(-1, CLASS_SHAPE);
		model_add_shape(group, shape);
	}
	jsal_push_class_obj(CLASS_MODEL, group, true);
	return true;
}

static void
js_Model_finalize(void* host_ptr)
{
	model_unref(host_ptr);
}

static bool
js_Model_get_shader(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	shader_t* shader;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);

	shader = model_get_shader(group);
	jsal_push_class_obj(CLASS_SHADER, shader_ref(shader), false);
	return true;
}

static bool
js_Model_get_transform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*     group;
	transform_t* transform;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);

	transform = model_get_transform(group);
	jsal_push_class_obj(CLASS_TRANSFORM, transform_ref(transform), false);
	return true;
}

static bool
js_Model_set_shader(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	shader_t* shader;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	shader = jsal_require_class_obj(0, CLASS_SHADER);

	model_set_shader(group, shader);
	return false;
}

static bool
js_Model_set_transform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*     group;
	transform_t* transform;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	transform = jsal_require_class_obj(0, CLASS_TRANSFORM);

	model_set_transform(group, transform);
	return false;
}

static bool
js_Model_draw(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t* group;
	image_t* surface;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	surface = num_args >= 1 ? jsal_require_class_obj(0, CLASS_SURFACE)
		: screen_backbuffer(g_screen);

	if (!screen_skip_frame(g_screen))
		model_draw(group, surface);
	return false;
}

static bool
js_Model_setBoolean(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	bool        value;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	value = jsal_require_boolean(1);

	model_put_bool(group, name, value);
	return false;
}

static bool
js_Model_setColorVector(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	color_t     color;
	model_t*    group;
	const char* name;
	float       values[4];

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	color = jsal_pegasus_require_color(1);

	values[0] = color.r / 255.0;
	values[1] = color.g / 255.0;
	values[2] = color.b / 255.0;
	values[3] = color.a / 255.0;
	model_put_float_vector(group, name, values, 4);
	return false;
}

static bool
js_Model_setFloat(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	float       value;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	value = jsal_require_number(1);

	model_put_float(group, name, value);
	return false;
}

static bool
js_Model_setFloatArray(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	int         size;
	float*      values;

	int i;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	if (!jsal_is_array(1))
		jsal_error(JS_TYPE_ERROR, "array was expected here");

	size = (int)jsal_get_length(1);

	values = malloc(size * sizeof(float));
	for (i = 0; i < size; ++i) {
		jsal_get_prop_index(1, (int)i);
		values[i] = jsal_require_number(-1);
		jsal_pop(1);
	}
	model_put_float_array(group, name, values, size);
	free(values);
	return false;
}

static bool
js_Model_setFloatVector(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	int         size;
	float       values[4];

	int i;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	jsal_require_array(1);

	size = (int)jsal_get_length(1);
	if (size < 2 || size > 4)
		jsal_error(JS_RANGE_ERROR, "invalid number of components");

	for (i = 0; i < size; ++i) {
		jsal_get_prop_index(1, (int)i);
		values[i] = jsal_require_number(-1);
		jsal_pop(1);
	}
	model_put_float_vector(group, name, values, size);
	return false;
}

static bool
js_Model_setInt(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	int         value;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	value = jsal_require_int(1);

	model_put_int(group, name, value);
	return false;
}

static bool
js_Model_setIntArray(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	int         size;
	int*        values;

	int i;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	if (!jsal_is_array(1))
		jsal_error(JS_TYPE_ERROR, "array was expected here");

	size = (int)jsal_get_length(1);
	values = malloc(size * sizeof(int));
	for (i = 0; i < size; ++i) {
		jsal_get_prop_index(1, (int)i);
		values[i] = jsal_require_int(-1);
		jsal_pop(1);
	}
	model_put_int_array(group, name, values, size);
	free(values);
	return false;
}

static bool
js_Model_setIntVector(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*    group;
	const char* name;
	int         size;
	int         values[4];

	int i;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	if (!jsal_is_array(1))
		jsal_error(JS_TYPE_ERROR, "array was expected here");

	size = (int)jsal_get_length(1);
	if (size < 2 || size > 4)
		jsal_error(JS_RANGE_ERROR, "invalid number of components");

	for (i = 0; i < size; ++i) {
		jsal_get_prop_index(1, (int)i);
		values[i] = jsal_require_int(-1);
		jsal_pop(1);
	}
	model_put_int_vector(group, name, values, size);
	return false;
}

static bool
js_Model_setMatrix(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	model_t*     group;
	const char*  name;
	transform_t* transform;

	jsal_push_this();
	group = jsal_require_class_obj(-1, CLASS_MODEL);
	name = jsal_require_string(0);
	transform = jsal_require_class_obj(1, CLASS_TRANSFORM);

	model_put_matrix(group, name, transform);
	return false;
}

static bool
js_Mouse_get_Default(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(CLASS_MOUSE, NULL, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_Mouse_get_x(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_MOUSE);

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(x);
	return true;
}

static bool
js_Mouse_get_y(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_MOUSE);

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(y);
	return true;
}

static bool
js_Mouse_clearQueue(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_MOUSE);

	mouse_clear_queue();
	return false;
}

static bool
js_Mouse_getEvent(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mouse_event_t event;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_MOUSE);

	if (mouse_queue_len() == 0)
		jsal_push_null();
	else {
		event = mouse_get_event();
		jsal_push_new_object();
		jsal_push_int(event.key);
		jsal_put_prop_string(-2, "key");
		jsal_push_int(event.x);
		jsal_put_prop_string(-2, "x");
		jsal_push_int(event.y);
		jsal_put_prop_string(-2, "y");
	}
	return true;
}

static bool
js_Mouse_isPressed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mouse_key_t key;

	jsal_push_this();
	jsal_require_class_obj(-1, CLASS_MOUSE);

	key = jsal_require_int(0);
	if (key < 0 || key >= MOUSE_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid MouseKey constant");

	jsal_push_boolean(mouse_is_key_down(key));
	return true;
}

static bool
js_RNG_fromSeed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	uint64_t seed;
	xoro_t*  xoro;

	seed = jsal_require_number(0);

	xoro = xoro_new(seed);
	jsal_push_class_obj(CLASS_RNG, xoro, false);
	return true;
}

static bool
js_RNG_fromState(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* state;
	xoro_t*     xoro;

	state = jsal_require_string(0);

	xoro = xoro_new(0);
	if (!xoro_set_state(xoro, state)) {
		xoro_unref(xoro);
		jsal_error(JS_TYPE_ERROR, "invalid RNG state string");
	}
	jsal_push_class_obj(CLASS_RNG, xoro, false);
	return true;
}

static bool
js_new_RNG(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	xoro_t* xoro;

	xoro = xoro_new((uint64_t)(al_get_time() * 1000000));
	jsal_push_class_obj(CLASS_RNG, xoro, true);
	return true;
}

static void
js_RNG_finalize(void* host_ptr)
{
	xoro_unref(host_ptr);
}

static bool
js_RNG_get_state(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	char    state[33];
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CLASS_RNG);

	xoro_get_state(xoro, state);
	jsal_push_string(state);
	return true;
}

static bool
js_RNG_set_state(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* state;
	xoro_t*     xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CLASS_RNG);
	state = jsal_require_string(0);

	if (!xoro_set_state(xoro, state))
		jsal_error(JS_TYPE_ERROR, "invalid RNG state string");
	return false;
}

static bool
js_RNG_next(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	xoro_t*     xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CLASS_RNG);

	jsal_push_number(xoro_gen_double(xoro));
	return true;
}

static bool
js_SSj_log(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	debugger_log(jsal_to_string(0), PRINT_NORMAL, true);
	return false;
}

static bool
js_SSj_trace(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	debugger_log(jsal_to_string(0), PRINT_TRACE, true);
	return false;
}

static bool
js_new_Sample(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* filename;
	sample_t*   sample;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(sample = sample_new(filename, true)))
		jsal_error(JS_ERROR, "couldn't load sample '%s'", filename);
	jsal_push_class_obj(CLASS_SAMPLE, sample, true);
	return true;
}

static void
js_Sample_finalize(void* host_ptr)
{
	sample_unref(host_ptr);
}

static bool
js_Sample_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, CLASS_SAMPLE);

	jsal_push_string(sample_path(sample));
	return true;
}

static bool
js_Sample_play(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mixer_t*  mixer;
	float     pan = 0.0;
	sample_t* sample;
	float     speed = 1.0;
	float     volume = 1.0;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, CLASS_SAMPLE);
	mixer = jsal_require_class_obj(0, CLASS_MIXER);
	if (num_args >= 2) {
		jsal_require_object_coercible(1);
		jsal_get_prop_string(1, "volume");
		if (!jsal_is_undefined(-1))
			volume = jsal_require_number(-3);
		jsal_get_prop_string(1, "pan");
		if (!jsal_is_undefined(-1))
			pan = jsal_require_number(-2);
		jsal_get_prop_string(1, "speed");
		if (!jsal_is_undefined(-1))
			speed = jsal_require_number(-1);
	}

	sample_set_gain(sample, volume);
	sample_set_pan(sample, pan);
	sample_set_speed(sample, speed);
	sample_play(sample, mixer);
	return false;
}

static bool
js_Sample_stopAll(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, CLASS_SAMPLE);

	sample_stop_all(sample);
	return false;
}

static bool
js_new_Server(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int       max_backlog;
	int       port;
	server_t* server;

	port = jsal_require_int(0);
	max_backlog = num_args >= 2 ? jsal_require_int(1) : 16;

	if (max_backlog <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid backlog size", max_backlog);

	if (!(server = server_new(NULL, port, 1024, max_backlog, false)))
		jsal_error(JS_ERROR, "couldn't create server");
	jsal_push_class_obj(CLASS_SERVER, server, true);
	return true;
}

static void
js_Server_finalize(void* host_ptr)
{
	server_unref(host_ptr);
}

static bool
js_Server_accept(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* new_socket;
	server_t* server;

	jsal_push_this();
	server = jsal_require_class_obj(-1, CLASS_SERVER);

	if (server == NULL)
		jsal_error(JS_ERROR, "server has shut down");
	new_socket = server_accept(server);
	if (new_socket != NULL)
		jsal_push_class_obj(CLASS_SOCKET, new_socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Server_close(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	server_t* server;

	jsal_push_this();
	server = jsal_require_class_obj(-1, CLASS_SERVER);

	jsal_set_class_ptr(-1, NULL);
	server_unref(server);
	return false;
}

static bool
js_Shader_get_Default(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shader_t* shader;

	if (!(shader = galileo_shader()))
		jsal_error(JS_ERROR, "couldn't compile default shaders");
	jsal_push_class_obj(CLASS_SHADER, shader_ref(shader), false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Shader(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* game_filename;
	const char* vs_filename;
	shader_t*   shader;

	if (!jsal_is_object(0))
		jsal_error(JS_TYPE_ERROR, "options must be an object");
	if (jsal_get_prop_string(0, "vertex"), !jsal_is_string(-1))
		jsal_error(JS_TYPE_ERROR, "'vertex' must be a string");
	if (jsal_get_prop_string(0, "fragment"), !jsal_is_string(-1))
		jsal_error(JS_TYPE_ERROR, "'fragment' must be a string");
	jsal_pop(2);

	jsal_get_prop_string(0, "vertex");
	jsal_get_prop_string(0, "fragment");
	vs_filename = jsal_require_pathname(-2, NULL, false, false);
	game_filename = jsal_require_pathname(-1, NULL, false, false);
	jsal_pop(2);
	if (!(shader = shader_new(vs_filename, game_filename)))
		jsal_error(JS_ERROR, "couldn't compile shader program");
	jsal_push_class_obj(CLASS_SHADER, shader, true);
	return true;
}

static void
js_Shader_finalize(void* host_ptr)
{
	shader_unref(host_ptr);
}

static bool
js_new_Shape(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	ibo_t*       ibo = NULL;
	shape_t*     shape;
	image_t*     texture = NULL;
	shape_type_t type;
	vbo_t*       vbo;

	type = jsal_require_int(0);
	if (jsal_is_class_obj(1, CLASS_TEXTURE) || jsal_is_null(1)) {
		texture = !jsal_is_null(1) ? jsal_require_class_obj(1, CLASS_TEXTURE) : NULL;
		vbo = jsal_require_class_obj(2, CLASS_VERTEX_LIST);
		if (num_args >= 4)
			ibo = jsal_require_class_obj(3, CLASS_INDEX_LIST);
	}
	else {
		vbo = jsal_require_class_obj(1, CLASS_VERTEX_LIST);
		if (num_args >= 3)
			ibo = jsal_require_class_obj(2, CLASS_INDEX_LIST);
	}

	if (type < 0 || type >= SHAPE_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid ShapeType constant");

	shape = shape_new(vbo, ibo, type, texture);
	jsal_push_class_obj(CLASS_SHAPE, shape, true);
	return true;
}

static void
js_Shape_finalize(void* host_ptr)
{
	shape_unref(host_ptr);
}

static bool
js_Shape_get_indexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	ibo_t*   ibo;
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);

	ibo = shape_get_ibo(shape);
	if (ibo != NULL)
		jsal_push_class_obj(CLASS_INDEX_LIST, ibo_ref(ibo), false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Shape_get_texture(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	image_t* texture;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);

	texture = shape_get_texture(shape);
	if (texture != NULL)
		jsal_push_class_obj(CLASS_TEXTURE, image_ref(texture), false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Shape_get_vertexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);

	jsal_push_class_obj(CLASS_VERTEX_LIST, vbo_ref(shape_get_vbo(shape)), false);
	return true;
}

static bool
js_Shape_set_indexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	ibo_t*   ibo = NULL;
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);
	if (!jsal_is_null(0))
		ibo = jsal_require_class_obj(0, CLASS_INDEX_LIST);

	shape_set_ibo(shape, ibo);
	return false;
}

static bool
js_Shape_set_texture(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	image_t* texture = NULL;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);
	if (!jsal_is_null(0))
		texture = jsal_require_class_obj(0, CLASS_TEXTURE);

	shape_set_texture(shape, texture);
	return false;
}

static bool
js_Shape_set_vertexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	vbo_t*   vbo;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);
	vbo = jsal_require_class_obj(0, CLASS_VERTEX_LIST);

	shape_set_vbo(shape, vbo);
	return false;
}

static bool
js_Shape_draw(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	shape_t*     shape;
	image_t*     surface = NULL;
	transform_t* transform = NULL;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, CLASS_SHAPE);
	surface = num_args >= 1 ? jsal_require_class_obj(0, CLASS_SURFACE)
		: screen_backbuffer(g_screen);
	if (num_args >= 2)
		transform = jsal_require_class_obj(1, CLASS_TRANSFORM);

	shape_draw(shape, surface, transform);
	return false;
}

static bool
js_new_Socket(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* hostname = NULL;
	int         port;
	socket_t*   socket;

	if (num_args >= 2) {
		hostname = jsal_require_string(0);
		port = jsal_require_int(1);
	}

	if (!(socket = socket_new(1024, false)))
		jsal_error(JS_ERROR, "couldn't create TCP socket");
	if (hostname != NULL && !socket_connect(socket, hostname, port))
		jsal_error(JS_ERROR, "couldn't connect to '%s'", hostname);
	jsal_push_class_obj(CLASS_SOCKET, socket, true);
	return true;
}

static void
js_Socket_finalize(void* host_ptr)
{
	socket_unref(host_ptr);
}

static bool
js_Socket_get_bytesPending(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is not connected");
	jsal_push_int((int)socket_peek(socket));
	return true;
}

static bool
js_Socket_get_connected(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	if (socket != NULL)
		jsal_push_boolean(socket_connected(socket));
	else
		jsal_push_boolean(false);
	return true;
}

static bool
js_Socket_get_remoteAddress(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	jsal_push_string(socket_hostname(socket));
	return true;
}

static bool
js_Socket_get_remotePort(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	jsal_push_int(socket_port(socket));
	return true;
}

static bool
js_Socket_close(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	jsal_set_class_ptr(-1, NULL);
	socket_unref(socket);
	return false;
}

static bool
js_Socket_connectTo(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* hostname;
	int         port;
	socket_t*   socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);
	hostname = jsal_require_string(0);
	port = jsal_require_int(1);

	if (!socket_connect(socket, hostname, port))
		jsal_error(JS_ERROR, "couldn't connect to %s", hostname);
	return false;
}

static bool
js_Socket_read(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	void*     buffer;
	size_t    bytes_read;
	int       num_bytes;
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);

	num_bytes = jsal_require_int(0);
	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	jsal_push_new_buffer(JS_ARRAYBUFFER, num_bytes);
	buffer = jsal_get_buffer_ptr(-1, NULL);
	bytes_read = socket_read(socket, buffer, num_bytes);
	return true;
}

static bool
js_Socket_write(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const uint8_t* payload;
	socket_t*      socket;
	size_t         write_size;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, CLASS_SOCKET);
	payload = jsal_require_buffer_ptr(0, &write_size);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	socket_write(socket, payload, write_size);
	return false;
}

static bool
js_new_Sound(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const char* filename;
	sound_t*    sound;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(sound = sound_new(filename)))
		jsal_error(JS_ERROR, "couldn't load sound '%s'", filename);
	jsal_push_class_obj(CLASS_SOUND, sound, true);
	return true;
}

static void
js_Sound_finalize(void* host_ptr)
{
	sound_unref(host_ptr);
}

static bool
js_Sound_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_string(sound_path(sound));
	return true;
}

static bool
js_Sound_get_length(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_number(sound_len(sound));
	return true;
}

static bool
js_Sound_get_pan(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_number(sound_pan(sound));
	return true;
}

static bool
js_Sound_set_pan(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float    new_pan;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);
	new_pan = jsal_require_number(0);

	sound_set_pan(sound, new_pan);
	return false;
}

static bool
js_Sound_get_speed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_number(sound_speed(sound));
	return true;
}

static bool
js_Sound_set_speed(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float new_speed = jsal_require_number(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	sound_set_speed(sound, new_speed);
	return false;
}

static bool
js_Sound_get_playing(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_boolean(sound_playing(sound));
	return true;
}

static bool
js_Sound_get_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_number(sound_tell(sound));
	return true;
}

static bool
js_Sound_set_position(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	double   new_pos;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);
	new_pos = jsal_require_number(0);

	sound_seek(sound, new_pos);
	return false;
}

static bool
js_Sound_get_repeat(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_boolean(sound_repeat(sound));
	return true;
}

static bool
js_Sound_set_repeat(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	bool is_looped = jsal_require_boolean(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	sound_set_repeat(sound, is_looped);
	return false;
}

static bool
js_Sound_get_volume(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	jsal_push_number(sound_gain(sound));
	return true;
}

static bool
js_Sound_set_volume(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float volume = jsal_require_number(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	sound_set_gain(sound, volume);
	return false;
}

static bool
js_Sound_pause(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	sound_pause(sound, true);
	return false;
}

static bool
js_Sound_play(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mixer_t* mixer;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	if (num_args < 1)
		sound_pause(sound, false);
	else {
		mixer = jsal_require_class_obj(0, CLASS_MIXER);
		sound_play(sound, mixer);
	}
	return false;
}

static bool
js_Sound_stop(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, CLASS_SOUND);

	sound_stop(sound);
	return false;
}

static bool
js_new_SoundStream(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	stream_t* stream;
	int       frequency;
	int       bits;
	int       channels;

	frequency = num_args >= 1 ? jsal_require_int(0) : 22050;
	bits = num_args >= 2 ? jsal_require_int(1) : 8;
	channels = num_args >= 3 ? jsal_require_int(2) : 1;

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		jsal_error(JS_RANGE_ERROR, "invalid audio bit depth");
	if (channels < 1 || channels > 7)
		jsal_error(JS_RANGE_ERROR, "invalid number of channels");

	if (!(stream = stream_new(frequency, bits, channels)))
		jsal_error(JS_ERROR, "couldn't create stream");
	jsal_push_class_obj(CLASS_SOUND_STREAM, stream, true);
	return true;
}

static void
js_SoundStream_finalize(void* host_ptr)
{
	stream_unref(host_ptr);
}

static bool
js_SoundStream_get_length(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	stream_t*    stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, CLASS_SOUND_STREAM);

	jsal_push_number(stream_length(stream));
	return true;
}

static bool
js_SoundStream_pause(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, CLASS_SOUND_STREAM);

	stream_pause(stream, true);
	return false;
}

static bool
js_SoundStream_play(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	mixer_t*  mixer;
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, CLASS_SOUND_STREAM);

	if (num_args < 1)
		stream_pause(stream, false);
	else {
		mixer = jsal_require_class_obj(0, CLASS_MIXER);
		stream_play(stream, mixer);
	}
	return false;
}

static bool
js_SoundStream_stop(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, CLASS_SOUND_STREAM);

	stream_stop(stream);
	return false;
}

static bool
js_SoundStream_write(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const void* data;
	size_t      size;
	stream_t*   stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, CLASS_SOUND_STREAM);

	data = jsal_require_buffer_ptr(0, &size);
	stream_buffer(stream, data, size);
	return false;
}

static bool
js_new_Surface(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int         n_args;
	const char* filename;
	color_t     fill_color;
	image_t*    image;
	image_t*    src_image;
	int         width, height;

	n_args = jsal_get_top();
	if (n_args >= 2) {
		width = jsal_require_int(0);
		height = jsal_require_int(1);
		fill_color = n_args >= 3 ? jsal_pegasus_require_color(2) : color_new(0, 0, 0, 0);
		if (!(image = image_new(width, height)))
			jsal_error(JS_ERROR, "couldn't create surface");
		image_fill(image, fill_color);
	}
	else if (jsal_is_class_obj(0, CLASS_TEXTURE)) {
		src_image = jsal_require_class_obj(0, CLASS_TEXTURE);
		if (!(image = image_clone(src_image)))
			jsal_error(JS_ERROR, "couldn't create surface");
	}
	else {
		filename = jsal_require_pathname(0, NULL, false, false);
		if (!(image = image_load(filename)))
			jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	}
	jsal_push_class_obj(CLASS_SURFACE, image, true);
	return true;
}

static void
js_Surface_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Surface_get_height(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Surface_get_transform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t*     image;
	transform_t* transform;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);

	transform = image_get_transform(image);
	jsal_push_class_obj(CLASS_TRANSFORM, transform_ref(transform), false);
	return true;
}

static bool
js_Surface_get_width(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_Surface_set_transform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t*     image;
	transform_t* transform;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);
	transform = jsal_require_class_obj(0, CLASS_TRANSFORM);

	image_set_transform(image, transform);
	return false;
}

static bool
js_Surface_clipTo(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);
	x = jsal_require_int(0);
	y = jsal_require_int(1);
	width = jsal_require_int(2);
	height = jsal_require_int(3);

	image_set_scissor(image, rect(x, y, x + width, y + height));
	return false;
}

static bool
js_Surface_toTexture(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_SURFACE);

	if ((new_image = image_clone(image)) == NULL)
		jsal_error(JS_ERROR, "image creation failed");
	jsal_push_class_obj(CLASS_TEXTURE, new_image, false);
	return true;
}

static bool
js_new_TextDecoder(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	decoder_t*  decoder;
	bool        fatal = false;
	bool        ignore_bom = false;
	const char* label = "utf-8";

	if (num_args >= 1)
		label = jsal_require_string(0);
	if (num_args >= 2) {
		jsal_require_object_coercible(1);
		if (jsal_get_prop_string(1, "fatal"))
			fatal = jsal_require_boolean(-1);
		if (jsal_get_prop_string(1, "ignoreBOM"))
			ignore_bom = jsal_require_boolean(-1);
	}

	// TextDecoder only supports UTF-8 for now.  in the future it'd be nice to support
	// at least UTF-16 and maybe CP-1252.
	if (strcasecmp(label, "unicode-1-1-utf-8") != 0
		&& strcasecmp(label, "utf-8") != 0
		&& strcasecmp(label, "utf8") != 0)
	{
		jsal_error(JS_TYPE_ERROR, "'%s' encoding is not supported", label);
	}

	decoder = decoder_new(fatal, ignore_bom);
	jsal_push_class_obj(CLASS_TEXT_DEC, decoder, true);
	return true;
}

static void
js_TextDecoder_finalize(void* host_ptr)
{
	decoder_free(host_ptr);
}

static bool
js_TextDecoder_get_encoding(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CLASS_TEXT_DEC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextDecoder_get_fatal(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CLASS_TEXT_DEC);

	jsal_push_boolean(decoder_fatal(decoder));
	return 1;
}

static bool
js_TextDecoder_get_ignoreBOM(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CLASS_TEXT_DEC);

	jsal_push_boolean(decoder_ignore_bom(decoder));
	return true;
}

static bool
js_TextDecoder_decode(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	decoder_t*  decoder;
	lstring_t*  head;
	const char* input = "";
	size_t      length = 0;
	lstring_t*  string;
	bool        streaming = false;
	lstring_t*  tail = NULL;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CLASS_TEXT_DEC);
	if (num_args >= 1)
		input = jsal_require_buffer_ptr(0, &length);
	if (num_args >= 2) {
		jsal_require_object_coercible(1);
		if (jsal_get_prop_string(1, "stream"))
			streaming = jsal_require_boolean(-1);
	}

	if (!(string = decoder_run(decoder, input, length)))
		jsal_error(JS_TYPE_ERROR, "data is not valid utf-8");
	if (!streaming) {
		if (!(tail = decoder_finish(decoder)))
			jsal_error(JS_TYPE_ERROR, "data is not valid utf-8");
		head = string;
		string = lstr_cat(head, tail);
		lstr_free(head);
		lstr_free(tail);
		jsal_push_lstring_t(string);
		lstr_free(string);
	}
	else {
		jsal_push_lstring_t(string);
		lstr_free(string);
	}
	return true;
}

static bool
js_new_TextEncoder(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	encoder_t* encoder;

	encoder = encoder_new();
	jsal_push_class_obj(CLASS_TEXT_ENC, encoder, true);
	return true;
}

static void
js_TextEncoder_finalize(void* host_ptr)
{
	encoder_free(host_ptr);
}

static bool
js_TextEncoder_get_encoding(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	encoder_t* encoder;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, CLASS_TEXT_ENC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextEncoder_encode(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	void*      buffer;
	lstring_t* input;
	uint8_t*   output;
	size_t     size;

	encoder_t* encoder;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, CLASS_TEXT_ENC);
	if (num_args >= 1)
		input = jsal_require_lstring_t(0);
	else
		input = lstr_new("");

	output = encoder_run(encoder, input, &size);
	jsal_push_new_buffer(JS_UINT8ARRAY, size);
	buffer = jsal_get_buffer_ptr(-1, NULL);
	memcpy(buffer, output, size);
	lstr_free(input);
	return true;
}

static bool
js_new_Texture(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	const color_t* buffer;
	size_t         buffer_size;
	const char*    filename;
	color_t        fill_color;
	int            height;
	image_t*       image;
	image_lock_t*  lock;
	color_t*       p_line;
	image_t*       src_image;
	int            width;

	int y;

	if (num_args >= 3 && jsal_is_class_obj(2, CLASS_COLOR)) {
		// create an Image filled with a single pixel value
		width = jsal_require_int(0);
		height = jsal_require_int(1);
		fill_color = jsal_pegasus_require_color(2);
		if (!(image = image_new(width, height)))
			jsal_error(JS_ERROR, "image creation failed");
		image_fill(image, fill_color);
	}
	else if (num_args >= 3 && (buffer = jsal_get_buffer_ptr(2, &buffer_size))) {
		// create an Image from an ArrayBuffer or similar object
		width = jsal_require_int(0);
		height = jsal_require_int(1);
		if (buffer_size < width * height * sizeof(color_t))
			jsal_error(JS_ERROR, "not enough data in buffer");
		if (!(image = image_new(width, height)))
			jsal_error(JS_ERROR, "image creation failed");
		if (!(lock = image_lock(image))) {
			image_unref(image);
			jsal_error(JS_ERROR, "image lock failed");
		}
		p_line = lock->pixels;
		for (y = 0; y < height; ++y) {
			memcpy(p_line, buffer + y * width, width * sizeof(color_t));
			p_line += lock->pitch;
		}
		image_unlock(image, lock);
	}
	else if (jsal_is_class_obj(0, CLASS_SURFACE)) {
		// create an Image from a Surface
		src_image = jsal_require_class_obj(0, CLASS_SURFACE);
		if (!(image = image_clone(src_image)))
			jsal_error(JS_ERROR, "image creation failed");
	}
	else {
		// create an Image by loading an image file
		filename = jsal_require_pathname(0, NULL, false, false);
		if (!(image = image_load(filename)))
			jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	}
	jsal_push_class_obj(CLASS_TEXTURE, image, true);
	return true;
}

static void
js_Texture_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Texture_get_fileName(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t*    image;
	const char* path;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_TEXTURE);

	if (path = image_path(image))
		jsal_push_string(path);
	else
		jsal_push_null();
	return true;
}

static bool
js_Texture_get_height(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_TEXTURE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Texture_get_width(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CLASS_TEXTURE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_new_Transform(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	transform_t* transform;

	transform = transform_new();
	jsal_push_class_obj(CLASS_TRANSFORM, transform, true);
	return true;
}

static void
js_Transform_finalize(void* host_ptr)
{
	transform_unref(host_ptr);
}

static bool
js_Transform_get_matrix(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	transform_t* transform;
	float*       values;

	int i, j;

	if (magic == 0) {
		// on first access: set up getters and setters
		jsal_push_this();
		transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
		values = transform_values(transform);
		jsal_push_new_object();
		for (i = 0; i < 4; ++i) {
			jsal_push_new_object();
			for (j = 0; j < 4; ++j) {
				jsal_push_function(js_Transform_get_matrix, "get", 0, 1 + (j * 4 + i));
				jsal_push_this();
				jsal_put_prop_string(-2, "\xFF" "transform");
				
				jsal_push_function(js_Transform_set_matrix, "set", 0, 1 + (j * 4 + i));
				jsal_push_this();
				jsal_put_prop_string(-2, "\xFF" "transform");
				
				jsal_push_eval("({ enumerable: true })");
				jsal_pull(-2);
				jsal_put_prop_string(-2, "set");
				jsal_pull(-2);
				jsal_put_prop_string(-2, "get");
				jsal_def_prop_index(-2, j);

			}
			jsal_push_eval("({ enumerable: true })");
			jsal_pull(-2);
			jsal_put_prop_string(-2, "value");
			jsal_def_prop_index(-2, i);
		}

		// store the generated object so we don't have to keep generating it on
		// every access
		jsal_push_this();
		jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
		jsal_dup(-3);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "matrix");
		jsal_pop(1);
		return true;
	}
	else {
		jsal_push_ref(me);
		jsal_get_prop_string(-1, "\xFF" "transform");
		transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
		values = transform_values(transform);
		jsal_push_number(values[magic - 1]);
		return true;
	}
}

static bool
js_Transform_set_matrix(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float        new_value;
	transform_t* transform;
	float*       values;

	new_value = jsal_require_number(0);

	jsal_push_ref(me);
	jsal_get_prop_string(-1, "\xFF" "transform");
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	values = transform_values(transform);
	values[magic - 1] = new_value;
	return false;
}

static bool
js_Transform_compose(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	transform_t* other;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	other = jsal_require_class_obj(0, CLASS_TRANSFORM);

	transform_compose(transform, other);
	return true;
}

static bool
js_Transform_identity(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);

	transform_identity(transform);
	return true;
}

static bool
js_Transform_project2D(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	transform_t* transform;
	float        x1, x2;
	float        y1, y2;
	float        z1 = -1.0f;
	float        z2 = 1.0f;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	x1 = jsal_require_number(0);
	y1 = jsal_require_number(1);
	x2 = jsal_require_number(2);
	y2 = jsal_require_number(3);
	if (num_args >= 6) {
		z1 = jsal_require_number(4);
		z2 = jsal_require_number(5);
	}

	transform_orthographic(transform, x1, y1, x2, y2, z1, z2);
	return true;
}

static bool
js_Transform_project3D(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float        aspect;
	float        fov;
	float        fw, fh;
	transform_t* transform;
	float        z1, z2;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	fov = jsal_require_number(0);
	aspect = jsal_require_number(1);
	z1 = jsal_require_number(2);
	z2 = jsal_require_number(3);

	if (fov >= 180.0 || fov <= 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid field of view angle '%g'", fov);
	if (aspect <= 0.0 || aspect == INFINITY)
		jsal_error(JS_RANGE_ERROR, "invalid aspect ratio '%g'", aspect);
	if (z1 <= 0.0 || z2 <= 0.0 || z2 < z1)
		jsal_error(JS_RANGE_ERROR, "invalid near/far range [%g,%g]", z1, z2);

	fh = tan(fov * M_PI / 360) * z1;
	fw = fh * aspect;
	transform_perspective(transform, -fw, -fh, fw, fh, z1, z2);
	return true;
}

static bool
js_Transform_rotate(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float        theta;
	transform_t* transform;
	float        vx = 0.0;
	float        vy = 0.0;
	float        vz = 1.0;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	theta = jsal_require_number(0);
	if (num_args >= 2) {
		vx = jsal_require_number(1);
		vy = jsal_require_number(2);
		vz = jsal_require_number(3);
	}

	theta *= M_PI / 180;  // convert to radians
	transform_rotate(transform, theta, vx, vy, vz);
	return true;
}

static bool
js_Transform_scale(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float        sx;
	float        sy;
	float        sz = 1.0;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	sx = jsal_require_number(0);
	sy = jsal_require_number(1);
	if (num_args >= 3)
		sz = jsal_require_number(2);

	transform_scale(transform, sx, sy, sz);
	return true;
}

static bool
js_Transform_translate(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	float        dx;
	float        dy;
	float        dz = 0.0;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, CLASS_TRANSFORM);
	dx = jsal_require_number(0);
	dy = jsal_require_number(1);
	if (num_args >= 3)
		dz = jsal_require_number(2);

	transform_translate(transform, dx, dy, dz);
	return true;
}

static bool
js_new_VertexList(js_ref_t* me, int num_args, bool is_ctor, int magic)
{
	int       num_entries;
	int       stack_idx;
	vbo_t*    vbo;
	vertex_t  vertex;

	int i;

	jsal_require_array(0);

	num_entries = (int)jsal_get_length(0);
	if (num_entries == 0)
		jsal_error(JS_RANGE_ERROR, "empty list not allowed");
	
	vbo = vbo_new();
	for (i = 0; i < num_entries; ++i) {
		jsal_get_prop_index(0, i);
		jsal_require_object(-1);
		stack_idx = jsal_normalize_index(-1);
		vertex.x = jsal_get_prop_key(stack_idx, s_key_x) ? jsal_require_number(-1) : 0.0;
		vertex.y = jsal_get_prop_key(stack_idx, s_key_y) ? jsal_require_number(-1) : 0.0;
		vertex.z = jsal_get_prop_key(stack_idx, s_key_z) ? jsal_require_number(-1) : 0.0;
		if (jsal_get_prop_key(stack_idx, s_key_u))
			vertex.u = jsal_require_number(-1);
		else
			vertex.u = 0;
		if (jsal_get_prop_key(stack_idx, s_key_v))
			vertex.v = jsal_require_number(-1);
		else
			vertex.v = 0;
		vertex.color = jsal_get_prop_key(stack_idx, s_key_color)
			? jsal_pegasus_require_color(-1)
			: color_new(255, 255, 255, 255);
		vbo_add_vertex(vbo, vertex);
		jsal_pop(7);
	}
	if (!vbo_upload(vbo)) {
		vbo_unref(vbo);
		jsal_error(JS_ERROR, "upload to GPU failed");
	}
	
	jsal_push_class_obj(CLASS_VERTEX_LIST, vbo, true);
	return true;
}

static void
js_VertexList_finalize(void* host_ptr)
{
	vbo_unref(host_ptr);
}
