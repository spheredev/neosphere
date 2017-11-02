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
#include "audio.h"
#include "color.h"
#include "console.h"
#include "debugger.h"
#include "dispatch.h"
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
#define API_LEVEL   1

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

static bool js_require                       (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_APILevel           (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Compiler           (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Engine             (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Game               (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_Version            (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_frameRate          (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_frameSkip          (int num_args, bool is_ctor, int magic);
static bool js_Sphere_get_fullScreen         (int num_args, bool is_ctor, int magic);
static bool js_Sphere_set_frameRate          (int num_args, bool is_ctor, int magic);
static bool js_Sphere_set_frameSkip          (int num_args, bool is_ctor, int magic);
static bool js_Sphere_set_fullScreen         (int num_args, bool is_ctor, int magic);
static bool js_Sphere_abort                  (int num_args, bool is_ctor, int magic);
static bool js_Sphere_now                    (int num_args, bool is_ctor, int magic);
static bool js_Sphere_restart                (int num_args, bool is_ctor, int magic);
static bool js_Sphere_setResolution          (int num_args, bool is_ctor, int magic);
static bool js_Sphere_shutDown               (int num_args, bool is_ctor, int magic);
static bool js_Sphere_sleep                  (int num_args, bool is_ctor, int magic);
static bool js_Color_get_Color               (int num_args, bool is_ctor, int magic);
static bool js_Color_is                      (int num_args, bool is_ctor, int magic);
static bool js_Color_mix                     (int num_args, bool is_ctor, int magic);
static bool js_Color_of                      (int num_args, bool is_ctor, int magic);
static bool js_new_Color                     (int num_args, bool is_ctor, int magic);
static bool js_Color_get_name                (int num_args, bool is_ctor, int magic);
static bool js_Color_get_r                   (int num_args, bool is_ctor, int magic);
static bool js_Color_get_g                   (int num_args, bool is_ctor, int magic);
static bool js_Color_get_b                   (int num_args, bool is_ctor, int magic);
static bool js_Color_get_a                   (int num_args, bool is_ctor, int magic);
static bool js_Color_set_r                   (int num_args, bool is_ctor, int magic);
static bool js_Color_set_g                   (int num_args, bool is_ctor, int magic);
static bool js_Color_set_b                   (int num_args, bool is_ctor, int magic);
static bool js_Color_set_a                   (int num_args, bool is_ctor, int magic);
static bool js_Color_clone                   (int num_args, bool is_ctor, int magic);
static bool js_Color_fadeTo                  (int num_args, bool is_ctor, int magic);
static bool js_new_DirectoryStream           (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_fileCount (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_fileName  (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_get_position  (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_set_position  (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_iterator      (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_dispose       (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_next          (int num_args, bool is_ctor, int magic);
static bool js_DirectoryStream_rewind        (int num_args, bool is_ctor, int magic);
static bool js_Dispatch_cancelAll            (int num_args, bool is_ctor, int magic);
static bool js_Dispatch_later                (int num_args, bool is_ctor, int magic);
static bool js_Dispatch_now                  (int num_args, bool is_ctor, int magic);
static bool js_Dispatch_onRender             (int num_args, bool is_ctor, int magic);
static bool js_Dispatch_onUpdate             (int num_args, bool is_ctor, int magic);
static bool js_FS_createDirectory            (int num_args, bool is_ctor, int magic);
static bool js_FS_deleteFile                 (int num_args, bool is_ctor, int magic);
static bool js_FS_directoryExists            (int num_args, bool is_ctor, int magic);
static bool js_FS_evaluateScript             (int num_args, bool is_ctor, int magic);
static bool js_FS_fileExists                 (int num_args, bool is_ctor, int magic);
static bool js_FS_fullPath                   (int num_args, bool is_ctor, int magic);
static bool js_FS_readFile                   (int num_args, bool is_ctor, int magic);
static bool js_FS_relativePath               (int num_args, bool is_ctor, int magic);
static bool js_FS_rename                     (int num_args, bool is_ctor, int magic);
static bool js_FS_removeDirectory            (int num_args, bool is_ctor, int magic);
static bool js_FS_writeFile                  (int num_args, bool is_ctor, int magic);
static bool js_new_FileStream                (int num_args, bool is_ctor, int magic);
static bool js_FileStream_dispose            (int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_fileName       (int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_fileSize       (int num_args, bool is_ctor, int magic);
static bool js_FileStream_get_position       (int num_args, bool is_ctor, int magic);
static bool js_FileStream_set_position       (int num_args, bool is_ctor, int magic);
static bool js_FileStream_read               (int num_args, bool is_ctor, int magic);
static bool js_FileStream_write              (int num_args, bool is_ctor, int magic);
static bool js_Font_get_Default              (int num_args, bool is_ctor, int magic);
static bool js_new_Font                      (int num_args, bool is_ctor, int magic);
static bool js_Font_get_fileName             (int num_args, bool is_ctor, int magic);
static bool js_Font_get_height               (int num_args, bool is_ctor, int magic);
static bool js_Font_drawText                 (int num_args, bool is_ctor, int magic);
static bool js_Font_getTextSize              (int num_args, bool is_ctor, int magic);
static bool js_Font_wordWrap                 (int num_args, bool is_ctor, int magic);
static bool js_new_IndexList                 (int num_args, bool is_ctor, int magic);
static bool js_JobToken_cancel               (int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_Null             (int num_args, bool is_ctor, int magic);
static bool js_Joystick_getDevices           (int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_name             (int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_numAxes          (int num_args, bool is_ctor, int magic);
static bool js_Joystick_get_numButtons       (int num_args, bool is_ctor, int magic);
static bool js_Joystick_getPosition          (int num_args, bool is_ctor, int magic);
static bool js_Joystick_isPressed            (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_Default          (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_capsLock         (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_numLock          (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_get_scrollLock       (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_charOf               (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_clearQueue           (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_getKey               (int num_args, bool is_ctor, int magic);
static bool js_Keyboard_isPressed            (int num_args, bool is_ctor, int magic);
static bool js_Mixer_get_Default             (int num_args, bool is_ctor, int magic);
static bool js_new_Mixer                     (int num_args, bool is_ctor, int magic);
static bool js_Mixer_get_volume              (int num_args, bool is_ctor, int magic);
static bool js_Mixer_set_volume              (int num_args, bool is_ctor, int magic);
static bool js_new_Model                     (int num_args, bool is_ctor, int magic);
static bool js_Model_get_shader              (int num_args, bool is_ctor, int magic);
static bool js_Model_get_transform           (int num_args, bool is_ctor, int magic);
static bool js_Model_set_shader              (int num_args, bool is_ctor, int magic);
static bool js_Model_set_transform           (int num_args, bool is_ctor, int magic);
static bool js_Model_draw                    (int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_Default             (int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_x                   (int num_args, bool is_ctor, int magic);
static bool js_Mouse_get_y                   (int num_args, bool is_ctor, int magic);
static bool js_Mouse_clearQueue              (int num_args, bool is_ctor, int magic);
static bool js_Mouse_getEvent                (int num_args, bool is_ctor, int magic);
static bool js_Mouse_isPressed               (int num_args, bool is_ctor, int magic);
static bool js_RNG_fromSeed                  (int num_args, bool is_ctor, int magic);
static bool js_RNG_fromState                 (int num_args, bool is_ctor, int magic);
static bool js_new_RNG                       (int num_args, bool is_ctor, int magic);
static bool js_RNG_get_state                 (int num_args, bool is_ctor, int magic);
static bool js_RNG_set_state                 (int num_args, bool is_ctor, int magic);
static bool js_RNG_iterator                  (int num_args, bool is_ctor, int magic);
static bool js_RNG_next                      (int num_args, bool is_ctor, int magic);
static bool js_SSj_flipScreen                (int num_args, bool is_ctor, int magic);
static bool js_SSj_log                       (int num_args, bool is_ctor, int magic);
static bool js_new_Sample                    (int num_args, bool is_ctor, int magic);
static bool js_Sample_get_fileName           (int num_args, bool is_ctor, int magic);
static bool js_Sample_play                   (int num_args, bool is_ctor, int magic);
static bool js_Sample_stopAll                (int num_args, bool is_ctor, int magic);
static bool js_new_Server                    (int num_args, bool is_ctor, int magic);
static bool js_Server_close                  (int num_args, bool is_ctor, int magic);
static bool js_Server_accept                 (int num_args, bool is_ctor, int magic);
static bool js_Shader_get_Default            (int num_args, bool is_ctor, int magic);
static bool js_new_Shader                    (int num_args, bool is_ctor, int magic);
static bool js_Shader_clone                  (int num_args, bool is_ctor, int magic);
static bool js_Shader_setBoolean             (int num_args, bool is_ctor, int magic);
static bool js_Shader_setColorVector         (int num_args, bool is_ctor, int magic);
static bool js_Shader_setFloat               (int num_args, bool is_ctor, int magic);
static bool js_Shader_setFloatArray          (int num_args, bool is_ctor, int magic);
static bool js_Shader_setFloatVector         (int num_args, bool is_ctor, int magic);
static bool js_Shader_setInt                 (int num_args, bool is_ctor, int magic);
static bool js_Shader_setIntArray            (int num_args, bool is_ctor, int magic);
static bool js_Shader_setIntVector           (int num_args, bool is_ctor, int magic);
static bool js_Shader_setMatrix              (int num_args, bool is_ctor, int magic);
static bool js_new_Shape                     (int num_args, bool is_ctor, int magic);
static bool js_Shape_get_indexList           (int num_args, bool is_ctor, int magic);
static bool js_Shape_get_texture             (int num_args, bool is_ctor, int magic);
static bool js_Shape_get_vertexList          (int num_args, bool is_ctor, int magic);
static bool js_Shape_set_indexList           (int num_args, bool is_ctor, int magic);
static bool js_Shape_set_texture             (int num_args, bool is_ctor, int magic);
static bool js_Shape_set_vertexList          (int num_args, bool is_ctor, int magic);
static bool js_Shape_draw                    (int num_args, bool is_ctor, int magic);
static bool js_new_Socket                    (int num_args, bool is_ctor, int magic);
static bool js_Socket_get_bytesPending       (int num_args, bool is_ctor, int magic);
static bool js_Socket_get_connected          (int num_args, bool is_ctor, int magic);
static bool js_Socket_get_remoteAddress      (int num_args, bool is_ctor, int magic);
static bool js_Socket_get_remotePort         (int num_args, bool is_ctor, int magic);
static bool js_Socket_close                  (int num_args, bool is_ctor, int magic);
static bool js_Socket_connectTo              (int num_args, bool is_ctor, int magic);
static bool js_Socket_read                   (int num_args, bool is_ctor, int magic);
static bool js_Socket_write                  (int num_args, bool is_ctor, int magic);
static bool js_new_Sound                     (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_fileName            (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_length              (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_pan                 (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_playing             (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_position            (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_repeat              (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_speed               (int num_args, bool is_ctor, int magic);
static bool js_Sound_get_volume              (int num_args, bool is_ctor, int magic);
static bool js_Sound_set_pan                 (int num_args, bool is_ctor, int magic);
static bool js_Sound_set_position            (int num_args, bool is_ctor, int magic);
static bool js_Sound_set_repeat              (int num_args, bool is_ctor, int magic);
static bool js_Sound_set_speed               (int num_args, bool is_ctor, int magic);
static bool js_Sound_set_volume              (int num_args, bool is_ctor, int magic);
static bool js_Sound_pause                   (int num_args, bool is_ctor, int magic);
static bool js_Sound_play                    (int num_args, bool is_ctor, int magic);
static bool js_Sound_stop                    (int num_args, bool is_ctor, int magic);
static bool js_new_SoundStream               (int num_args, bool is_ctor, int magic);
static bool js_SoundStream_get_length        (int num_args, bool is_ctor, int magic);
static bool js_SoundStream_play              (int num_args, bool is_ctor, int magic);
static bool js_SoundStream_pause             (int num_args, bool is_ctor, int magic);
static bool js_SoundStream_stop              (int num_args, bool is_ctor, int magic);
static bool js_SoundStream_write             (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_Screen            (int num_args, bool is_ctor, int magic);
static bool js_new_Surface                   (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_height            (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_transform         (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_width             (int num_args, bool is_ctor, int magic);
static bool js_Surface_set_transform         (int num_args, bool is_ctor, int magic);
static bool js_Surface_clipTo                (int num_args, bool is_ctor, int magic);
static bool js_Surface_toTexture             (int num_args, bool is_ctor, int magic);
static bool js_new_TextDecoder               (int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_encoding      (int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_fatal         (int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_get_ignoreBOM     (int num_args, bool is_ctor, int magic);
static bool js_TextDecoder_decode            (int num_args, bool is_ctor, int magic);
static bool js_new_TextEncoder               (int num_args, bool is_ctor, int magic);
static bool js_TextEncoder_get_encoding      (int num_args, bool is_ctor, int magic);
static bool js_TextEncoder_encode            (int num_args, bool is_ctor, int magic);
static bool js_new_Texture                   (int num_args, bool is_ctor, int magic);
static bool js_Texture_get_fileName          (int num_args, bool is_ctor, int magic);
static bool js_Texture_get_height            (int num_args, bool is_ctor, int magic);
static bool js_Texture_get_width             (int num_args, bool is_ctor, int magic);
static bool js_new_Transform                 (int num_args, bool is_ctor, int magic);
static bool js_Transform_get_matrix          (int num_args, bool is_ctor, int magic);
static bool js_Transform_set_matrix          (int num_args, bool is_ctor, int magic);
static bool js_Transform_compose             (int num_args, bool is_ctor, int magic);
static bool js_Transform_identity            (int num_args, bool is_ctor, int magic);
static bool js_Transform_project2D           (int num_args, bool is_ctor, int magic);
static bool js_Transform_project3D           (int num_args, bool is_ctor, int magic);
static bool js_Transform_rotate              (int num_args, bool is_ctor, int magic);
static bool js_Transform_scale               (int num_args, bool is_ctor, int magic);
static bool js_Transform_translate           (int num_args, bool is_ctor, int magic);
static bool js_new_VertexList                (int num_args, bool is_ctor, int magic);

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

static void      create_joystick_objects     (void);
static path_t*   find_module_file            (const char* id, const char* origin, const char* sys_origin, bool es6_mode);
static void      jsal_pegasus_push_color     (color_t color, bool in_ctor);
static void      jsal_pegasus_push_job_token (int64_t token);
static void      jsal_pegasus_push_require   (const char* module_id);
static color_t   jsal_pegasus_require_color  (int index);
static script_t* jsal_pegasus_require_script (int index);
static void      handle_module_import        (void);
static path_t*   load_package_json           (const char* filename);
static bool      run_sphere_v2_event_loop    (int num_args, bool is_ctor, int magic);

static mixer_t* s_def_mixer;
static int      s_frame_rate = 60;
static int      s_next_module_id = 1;

static js_ref_t* s_key_color;
static js_ref_t* s_key_done;
static js_ref_t* s_key_inBackground;
static js_ref_t* s_key_priority;
static js_ref_t* s_key_stack;
static js_ref_t* s_key_u;
static js_ref_t* s_key_v;
static js_ref_t* s_key_value;
static js_ref_t* s_key_x;
static js_ref_t* s_key_y;
static js_ref_t* s_key_z;

void
pegasus_init(void)
{
	const struct x11_color* p;

	console_log(1, "initializing Sphere v%d L%d API", API_VERSION, API_LEVEL);

	s_def_mixer = mixer_new(44100, 16, 2);
	jsal_on_import_module(handle_module_import);

	s_key_color = jsal_new_key("color");
	s_key_done = jsal_new_key("done");
	s_key_inBackground = jsal_new_key("inBackground");
	s_key_priority = jsal_new_key("priority");
	s_key_stack = jsal_new_key("stack");
	s_key_u = jsal_new_key("u");
	s_key_v = jsal_new_key("v");
	s_key_value = jsal_new_key("value");
	s_key_x = jsal_new_key("x");
	s_key_y = jsal_new_key("y");
	s_key_z = jsal_new_key("z");

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
	api_define_static_prop("Sphere", "Compiler", js_Sphere_get_Compiler, NULL);
	api_define_static_prop("Sphere", "Engine", js_Sphere_get_Engine, NULL);
	api_define_static_prop("Sphere", "Game", js_Sphere_get_Game, NULL);
	api_define_static_prop("Sphere", "Version", js_Sphere_get_Version, NULL);
	api_define_static_prop("Sphere", "frameRate", js_Sphere_get_frameRate, js_Sphere_set_frameRate);
	api_define_static_prop("Sphere", "frameSkip", js_Sphere_get_frameSkip, js_Sphere_set_frameSkip);
	api_define_static_prop("Sphere", "fullScreen", js_Sphere_get_fullScreen, js_Sphere_set_fullScreen);
	api_define_function("Sphere", "abort", js_Sphere_abort, 0);
	api_define_function("Sphere", "now", js_Sphere_now, 0);
	api_define_function("Sphere", "restart", js_Sphere_restart, 0);
	api_define_function("Sphere", "setResolution", js_Sphere_setResolution, 0);
	api_define_function("Sphere", "shutDown", js_Sphere_shutDown, 0);
	api_define_function("Sphere", "sleep", js_Sphere_sleep, 0);
	api_define_class("Color", PEGASUS_COLOR, js_new_Color, js_Color_finalize);
	api_define_function("Color", "is", js_Color_is, 0);
	api_define_function("Color", "mix", js_Color_mix, 0);
	api_define_function("Color", "of", js_Color_of, 0);
	api_define_property("Color", "name", false, js_Color_get_name, NULL);
	api_define_property("Color", "r", true, js_Color_get_r, js_Color_set_r);
	api_define_property("Color", "g", true, js_Color_get_g, js_Color_set_g);
	api_define_property("Color", "b", true, js_Color_get_b, js_Color_set_b);
	api_define_property("Color", "a", true, js_Color_get_a, js_Color_set_a);
	api_define_method("Color", "clone", js_Color_clone, 0);
	api_define_method("Color", "fadeTo", js_Color_fadeTo, 0);
	api_define_class("DirectoryStream", PEGASUS_DIR_STREAM, js_new_DirectoryStream, js_DirectoryStream_finalize);
	api_define_property("DirectoryStream", "fileCount", false, js_DirectoryStream_get_fileCount, NULL);
	api_define_property("DirectoryStream", "fileName", false, js_DirectoryStream_get_fileName, NULL);
	api_define_property("DirectoryStream", "position", false, js_DirectoryStream_get_position, js_DirectoryStream_set_position);
	api_define_method("DirectoryStream", "@@iterator", js_DirectoryStream_iterator, 0);
	api_define_method("DirectoryStream", "dispose", js_DirectoryStream_dispose, 0);
	api_define_method("DirectoryStream", "next", js_DirectoryStream_next, 0);
	api_define_method("DirectoryStream", "rewind", js_DirectoryStream_rewind, 0);
	api_define_function("Dispatch", "cancelAll", js_Dispatch_cancelAll, 0);
	api_define_function("Dispatch", "later", js_Dispatch_later, 0);
	api_define_function("Dispatch", "now", js_Dispatch_now, 0);
	api_define_function("Dispatch", "onRender", js_Dispatch_onRender, 0);
	api_define_function("Dispatch", "onUpdate", js_Dispatch_onUpdate, 0);
	api_define_class("FileStream", PEGASUS_FILE_STREAM, js_new_FileStream, js_FileStream_finalize);
	api_define_method("FileStream", "dispose", js_FileStream_dispose, 0);
	api_define_property("FileStream", "fileName", false, js_FileStream_get_fileName, NULL);
	api_define_property("FileStream", "fileSize", false, js_FileStream_get_fileSize, NULL);
	api_define_property("FileStream", "position", false, js_FileStream_get_position, js_FileStream_set_position);
	api_define_method("FileStream", "read", js_FileStream_read, 0);
	api_define_method("FileStream", "write", js_FileStream_write, 0);
	api_define_class("Font", PEGASUS_FONT, js_new_Font, js_Font_finalize);
	api_define_static_prop("Font", "Default", js_Font_get_Default, NULL);
	api_define_property("Font", "fileName", false, js_Font_get_fileName, NULL);
	api_define_property("Font", "height", false, js_Font_get_height, NULL);
	api_define_method("Font", "drawText", js_Font_drawText, 0);
	api_define_method("Font", "getTextSize", js_Font_getTextSize, 0);
	api_define_method("Font", "wordWrap", js_Font_wordWrap, 0);
	api_define_function("FS", "createDirectory", js_FS_createDirectory, 0);
	api_define_function("FS", "deleteFile", js_FS_deleteFile, 0);
	api_define_function("FS", "directoryExists", js_FS_directoryExists, 0);
	api_define_function("FS", "evaluateScript", js_FS_evaluateScript, 0);
	api_define_function("FS", "fileExists", js_FS_fileExists, 0);
	api_define_function("FS", "fullPath", js_FS_fullPath, 0);
	api_define_function("FS", "readFile", js_FS_readFile, 0);
	api_define_function("FS", "relativePath", js_FS_relativePath, 0);
	api_define_function("FS", "removeDirectory", js_FS_removeDirectory, 0);
	api_define_function("FS", "rename", js_FS_rename, 0);
	api_define_function("FS", "writeFile", js_FS_writeFile, 0);
	api_define_class("IndexList", PEGASUS_INDEX_LIST, js_new_IndexList, js_IndexList_finalize);
	api_define_class("JobToken", PEGASUS_JOB_TOKEN, NULL, js_JobToken_finalize);
	api_define_method("JobToken", "cancel", js_JobToken_cancel, 0);
	api_define_class("Joystick", PEGASUS_JOYSTICK, NULL, js_Joystick_finalize);
	api_define_static_prop("Joystick", "Null", js_Joystick_get_Null, NULL);
	api_define_function("Joystick", "getDevices", js_Joystick_getDevices, 0);
	api_define_property("Joystick", "name", false, js_Joystick_get_name, NULL);
	api_define_property("Joystick", "numAxes", false, js_Joystick_get_numAxes, NULL);
	api_define_property("Joystick", "numButtons", false, js_Joystick_get_numButtons, NULL);
	api_define_method("Joystick", "getPosition", js_Joystick_getPosition, 0);
	api_define_method("Joystick", "isPressed", js_Joystick_isPressed, 0);
	api_define_class("Keyboard", PEGASUS_KEYBOARD, NULL, NULL);
	api_define_static_prop("Keyboard", "Default", js_Keyboard_get_Default, NULL);
	api_define_property("Keyboard", "capsLock", false, js_Keyboard_get_capsLock, NULL);
	api_define_property("Keyboard", "numLock", false, js_Keyboard_get_numLock, NULL);
	api_define_property("Keyboard", "scrollLock", false, js_Keyboard_get_scrollLock, NULL);
	api_define_method("Keyboard", "charOf", js_Keyboard_charOf, 0);
	api_define_method("Keyboard", "clearQueue", js_Keyboard_clearQueue, 0);
	api_define_method("Keyboard", "getKey", js_Keyboard_getKey, 0);
	api_define_method("Keyboard", "isPressed", js_Keyboard_isPressed, 0);
	api_define_class("Mixer", PEGASUS_MIXER, js_new_Mixer, js_Mixer_finalize);
	api_define_static_prop("Mixer", "Default", js_Mixer_get_Default, NULL);
	api_define_property("Mixer", "volume", false, js_Mixer_get_volume, js_Mixer_set_volume);
	api_define_class("Model", PEGASUS_MODEL, js_new_Model, js_Model_finalize);
	api_define_property("Model", "shader", false, js_Model_get_shader, js_Model_set_shader);
	api_define_property("Model", "transform", false, js_Model_get_transform, js_Model_set_transform);
	api_define_method("Model", "draw", js_Model_draw, 0);
	api_define_class("Mouse", PEGASUS_MOUSE, NULL, NULL);
	api_define_static_prop("Mouse", "Default", js_Mouse_get_Default, NULL);
	api_define_property("Mouse", "x", false, js_Mouse_get_x, NULL);
	api_define_property("Mouse", "y", false, js_Mouse_get_y, NULL);
	api_define_method("Mouse", "clearQueue", js_Mouse_clearQueue, 0);
	api_define_method("Mouse", "getEvent", js_Mouse_getEvent, 0);
	api_define_method("Mouse", "isPressed", js_Mouse_isPressed, 0);
	api_define_class("RNG", PEGASUS_RNG, js_new_RNG, js_RNG_finalize);
	api_define_function("RNG", "fromSeed", js_RNG_fromSeed, 0);
	api_define_function("RNG", "fromState", js_RNG_fromState, 0);
	api_define_property("RNG", "state", false, js_RNG_get_state, js_RNG_set_state);
	api_define_method("RNG", "@@iterator", js_RNG_iterator, 0);
	api_define_method("RNG", "next", js_RNG_next, 0);
	api_define_function("SSj", "flipScreen", js_SSj_flipScreen, 0);
	api_define_function("SSj", "log", js_SSj_log, KI_LOG_NORMAL);
	api_define_function("SSj", "trace", js_SSj_log, KI_LOG_TRACE);
	api_define_class("Sample", PEGASUS_SAMPLE, js_new_Sample, js_Sample_finalize);
	api_define_property("Sample", "fileName", false, js_Sample_get_fileName, NULL);
	api_define_method("Sample", "play", js_Sample_play, 0);
	api_define_method("Sample", "stopAll", js_Sample_stopAll, 0);
	api_define_class("Server", PEGASUS_SERVER, js_new_Server, js_Server_finalize);
	api_define_method("Server", "close", js_Server_close, 0);
	api_define_method("Server", "accept", js_Server_accept, 0);
	api_define_class("Shader", PEGASUS_SHADER, js_new_Shader, js_Shader_finalize);
	api_define_static_prop("Shader", "Default", js_Shader_get_Default, NULL);
	api_define_method("Shader", "clone", js_Shader_clone, 0);
	api_define_method("Shader", "setBoolean", js_Shader_setBoolean, 0);
	api_define_method("Shader", "setColorVector", js_Shader_setColorVector, 0);
	api_define_method("Shader", "setFloat", js_Shader_setFloat, 0);
	api_define_method("Shader", "setFloatArray", js_Shader_setFloatArray, 0);
	api_define_method("Shader", "setFloatVector", js_Shader_setFloatVector, 0);
	api_define_method("Shader", "setInt", js_Shader_setInt, 0);
	api_define_method("Shader", "setIntArray", js_Shader_setIntArray, 0);
	api_define_method("Shader", "setIntVector", js_Shader_setIntVector, 0);
	api_define_method("Shader", "setMatrix", js_Shader_setMatrix, 0);
	api_define_class("Shape", PEGASUS_SHAPE, js_new_Shape, js_Shape_finalize);
	api_define_property("Shape", "indexList", false, js_Shape_get_indexList, js_Shape_set_indexList);
	api_define_property("Shape", "texture", false, js_Shape_get_texture, js_Shape_set_texture);
	api_define_property("Shape", "vertexList", false, js_Shape_get_vertexList, js_Shape_set_vertexList);
	api_define_method("Shape", "draw", js_Shape_draw, 0);
	api_define_class("Socket", PEGASUS_SOCKET, js_new_Socket, js_Socket_finalize);
	api_define_property("Socket", "bytesPending", false, js_Socket_get_bytesPending, NULL);
	api_define_property("Socket", "connected", false, js_Socket_get_connected, NULL);
	api_define_property("Socket", "remoteAddress", false, js_Socket_get_remoteAddress, NULL);
	api_define_property("Socket", "remotePort", false, js_Socket_get_remotePort, NULL);
	api_define_method("Socket", "close", js_Socket_close, 0);
	api_define_method("Socket", "connectTo", js_Socket_connectTo, 0);
	api_define_method("Socket", "read", js_Socket_read, 0);
	api_define_method("Socket", "write", js_Socket_write, 0);
	api_define_class("Sound", PEGASUS_SOUND, js_new_Sound, js_Sound_finalize);
	api_define_property("Sound", "fileName", false, js_Sound_get_fileName, NULL);
	api_define_property("Sound", "length", false, js_Sound_get_length, NULL);
	api_define_property("Sound", "pan", false, js_Sound_get_pan, js_Sound_set_pan);
	api_define_property("Sound", "playing", false, js_Sound_get_playing, NULL);
	api_define_property("Sound", "position", false, js_Sound_get_position, js_Sound_set_position);
	api_define_property("Sound", "repeat", false, js_Sound_get_repeat, js_Sound_set_repeat);
	api_define_property("Sound", "speed", false, js_Sound_get_speed, js_Sound_set_speed);
	api_define_property("Sound", "volume", false, js_Sound_get_volume, js_Sound_set_volume);
	api_define_method("Sound", "pause", js_Sound_pause, 0);
	api_define_method("Sound", "play", js_Sound_play, 0);
	api_define_method("Sound", "stop", js_Sound_stop, 0);
	api_define_class("SoundStream", PEGASUS_SOUND_STREAM, js_new_SoundStream, js_SoundStream_finalize);
	api_define_property("SoundStream", "length", false, js_SoundStream_get_length, NULL);
	api_define_method("SoundStream", "pause", js_SoundStream_pause, 0);
	api_define_method("SoundStream", "play", js_SoundStream_play, 0);
	api_define_method("SoundStream", "stop", js_SoundStream_stop, 0);
	api_define_method("SoundStream", "write", js_SoundStream_write, 0);
	api_define_class("Surface", PEGASUS_SURFACE, js_new_Surface, js_Surface_finalize);
	api_define_static_prop("Surface", "Screen", js_Surface_get_Screen, NULL);
	api_define_property("Surface", "height", false, js_Surface_get_height, NULL);
	api_define_property("Surface", "transform", false, js_Surface_get_transform, js_Surface_set_transform);
	api_define_property("Surface", "width", false, js_Surface_get_width, NULL);
	api_define_method("Surface", "clipTo", js_Surface_clipTo, 0);
	api_define_method("Surface", "toTexture", js_Surface_toTexture, 0);
	api_define_class("TextDecoder", PEGASUS_TEXT_DEC, js_new_TextDecoder, js_TextDecoder_finalize);
	api_define_property("TextDecoder", "encoding", false, js_TextDecoder_get_encoding, NULL);
	api_define_property("TextDecoder", "fatal", false, js_TextDecoder_get_fatal, NULL);
	api_define_property("TextDecoder", "ignoreBOM", false, js_TextDecoder_get_ignoreBOM, NULL);
	api_define_method("TextDecoder", "decode", js_TextDecoder_decode, 0);
	api_define_class("TextEncoder", PEGASUS_TEXT_ENC, js_new_TextEncoder, js_TextEncoder_finalize);
	api_define_property("TextEncoder", "encoding", false, js_TextEncoder_get_encoding, NULL);
	api_define_method("TextEncoder", "encode", js_TextEncoder_encode, 0);
	api_define_class("Texture", PEGASUS_TEXTURE, js_new_Texture, js_Texture_finalize);
	api_define_property("Texture", "fileName", false, js_Texture_get_fileName, NULL);
	api_define_property("Texture", "height", false, js_Texture_get_height, NULL);
	api_define_property("Texture", "width", false, js_Texture_get_width, NULL);
	api_define_class("Transform", PEGASUS_TRANSFORM, js_new_Transform, js_Transform_finalize);
	api_define_property("Transform", "matrix", false, js_Transform_get_matrix, NULL);
	api_define_method("Transform", "compose", js_Transform_compose, 0);
	api_define_method("Transform", "identity", js_Transform_identity, 0);
	api_define_method("Transform", "project2D", js_Transform_project2D, 0);
	api_define_method("Transform", "project3D", js_Transform_project3D, 0);
	api_define_method("Transform", "rotate", js_Transform_rotate, 0);
	api_define_method("Transform", "scale", js_Transform_scale, 0);
	api_define_method("Transform", "translate", js_Transform_translate, 0);
	api_define_class("VertexList", PEGASUS_VERTEX_LIST, js_new_VertexList, js_VertexList_finalize);

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
	create_joystick_objects();

	// register predefined X11 colors
	jsal_get_global_string("Color");
	p = COLORS;
	while (p->name != NULL) {
		jsal_push_eval("({ enumerable: false, configurable: true })");
		jsal_push_new_function(js_Color_get_Color, "get", 0, (int)(p - COLORS));
		jsal_put_prop_string(-2, "get");
		jsal_def_prop_string(-2, p->name);
		++p;
	}
	jsal_pop(1);
}

void
pegasus_uninit(void)
{
	jsal_unref(s_key_color);
	jsal_unref(s_key_done);
	jsal_unref(s_key_inBackground);
	jsal_unref(s_key_priority);
	jsal_unref(s_key_stack);
	jsal_unref(s_key_u);
	jsal_unref(s_key_v);
	jsal_unref(s_key_value);
	jsal_unref(s_key_x);
	jsal_unref(s_key_y);
	jsal_unref(s_key_z);

	mixer_unref(s_def_mixer);
}

bool
pegasus_eval_module(const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  JSAL's stack-based API is powerful, but gets very
	// messy very quickly when dealing with object properties.  I tried to add comments
	// to illuminate what's going on, but it's still likely to be confusing for someone
	// not familiar with Lua or similar APIs.  proceed with caution.

	// notes:
	//     - the final value of 'module.exports' is left on top of the JSAL value stack.
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

	console_log(1, "evaluating JS module '%s'", filename);

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

	// evaluate .mjs scripts as ES modules ("mJS")
	if (path_has_extension(file_path, ".mjs")) {
		lstr_free(code_string);
		code_string = lstr_newf(
			"/* synthesized by the mJS module loader */\n"
			"import * as exports from \"%s\";\n"
			"global.$EXPORTS = exports;", filename);
		jsal_push_lstring_t(code_string);
		module_name = strnewf("%%/moduleShim-%d.mjs", s_next_module_id++);
		debugger_add_source(module_name, code_string);
		is_module_loaded = jsal_try_eval_module(module_name);
		lstr_free(code_string);
		free(module_name);
		if (!is_module_loaded)
			goto on_error;
		jsal_pop(2);
		jsal_get_global_string("$EXPORTS");
		jsal_del_global_string("$EXPORTS");
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

bool
pegasus_start_event_loop(void)
{
	if (jsal_try(run_sphere_v2_event_loop, 0)) {
		jsal_pop(1);  // don't need return value
		return true;
	}
	else {
		// leave the error for the caller, don't pop it off
		return false;
	}
}

static void
jsal_pegasus_push_color(color_t color, bool in_ctor)
{
	color_t* color_ptr;
	
	color_ptr = malloc(sizeof(color_t));
	*color_ptr = color;
	jsal_push_class_obj(PEGASUS_COLOR, color_ptr, in_ctor);
}

static void
jsal_pegasus_push_job_token(int64_t token)
{
	int64_t* ptr;

	ptr = malloc(sizeof(int64_t));
	*ptr = token;
	jsal_push_class_obj(PEGASUS_JOB_TOKEN, ptr, false);
}

static void
jsal_pegasus_push_require(const char* module_id)
{
	jsal_push_new_function(js_require, "require", 1, 0);
	
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

	color_ptr = jsal_require_class_obj(index, PEGASUS_COLOR);
	return *color_ptr;
}

static script_t*
jsal_pegasus_require_script(int index)
{
	return script_new_function(index);
}

static void
create_joystick_objects(void)
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
		jsal_push_class_obj(PEGASUS_JOYSTICK, device, false);
		jsal_put_prop_index(-2, i);
	}
	jsal_put_prop_string(-2, "joystickObjects");
	jsal_pop(1);
}

static path_t*
find_module_file(const char* id, const char* origin, const char* sys_origin, bool es6_mode)
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
handle_module_import(void)
{
	const char* const PATHS[] =
	{
		"@/lib",
		"#/game_modules",
		"#/runtime",
	};

	char*       caller_id = NULL;
	const char* origin = NULL;
	path_t*     path;
	char*       shim_name;
	lstring_t*  shim_source;
	char*       source;
	size_t      source_len;
	char*       specifier;

	int i;

	// strdup() here because the JSAL-managed strings may get overwritten
	// in the course of a filename lookup.
	specifier = strdup(jsal_require_string(0));
	if (!jsal_is_null(1))
		caller_id = strdup(jsal_require_string(1));

	if (caller_id == NULL && (strncmp(specifier, "./", 2) == 0 || strncmp(specifier, "../", 3) == 0))
		jsal_error(JS_URI_ERROR, "relative import() outside of an mJS module");

	// HACK: the way JSAL is currently designed, the same filename is used for
	//       module resolution as for display; this works around the limitation
	//       until more comprehensive refactoring can be done.
	if (caller_id != NULL)
		origin = debugger_compiled_name(caller_id);

	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if (path = find_module_file(specifier, origin, PATHS[i], true))
			break;  // short-circuit
	}
	free(caller_id);
	if (path == NULL) {
		jsal_push_new_error(JS_URI_ERROR, "couldn't find module '%s'", specifier);
		free(specifier);
		jsal_throw();
	}
	free(specifier);

	if (path_has_extension(path, ".mjs")) {
		source = game_read_file(g_game, path_cstr(path), &source_len);
		jsal_push_string(debugger_source_name(path_cstr(path)));
		jsal_push_lstring(source, source_len);
		free(source);
	}
	else {
		// ES module shim, allows 'import' to work with CommonJS modules
		shim_name = strnewf("%%/moduleShim-%d.mjs", s_next_module_id++);
		shim_source = lstr_newf(
			"/* synthesized by the mJS module loader */\n"
			"export default require(\"%s\");", path_cstr(path));
		debugger_add_source(shim_name, shim_source);
		jsal_push_string(shim_name);
		jsal_push_lstring_t(shim_source);
		free(shim_name);
		lstr_free(shim_source);

	}
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
run_sphere_v2_event_loop(int num_args, bool is_ctor, int magic)
{
	while (dispatch_busy() || jsal_busy()) {
		if (!screen_skip_frame(g_screen))
			dispatch_run(JOB_RENDER);
		screen_flip(g_screen, s_frame_rate, true);
		image_set_scissor(screen_backbuffer(g_screen), screen_bounds(g_screen));
		dispatch_run(JOB_UPDATE);
		dispatch_run(JOB_TICK);
		++g_tick_count;
	}
	return false;
}

static bool
js_require(int num_args, bool is_ctor, int magic)
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

	jsal_push_callee();
	if (jsal_get_prop_string(-1, "id"))
		parent_id = jsal_get_string(-1);

	if (parent_id == NULL && (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0))
		jsal_error(JS_URI_ERROR, "relative require() outside of a CommonJS module");
	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if (path = find_module_file(id, parent_id, PATHS[i], false))
			break;  // short-circuit
	}
	if (path == NULL)
		jsal_error(JS_URI_ERROR, "couldn't find module '%s'", id);
	if (!pegasus_eval_module(path_cstr(path)))
		jsal_throw();
	return true;
}

static bool
js_Sphere_get_APILevel(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(API_LEVEL);
	return true;
}

static bool
js_Sphere_get_Compiler(int num_args, bool is_ctor, int magic)
{
	const char* compiler;

	compiler = game_compiler(g_game);
	jsal_push_undefined();
	if (compiler != NULL)
		jsal_push_string(compiler);
	return true;
}

static bool
js_Sphere_get_Engine(int num_args, bool is_ctor, int magic)
{
	jsal_push_sprintf("%s %s", SPHERE_ENGINE_NAME, SPHERE_VERSION);
	return true;
}

static bool
js_Sphere_get_Game(int num_args, bool is_ctor, int magic)
{
	jsal_push_lstring_t(game_manifest(g_game));
	jsal_parse(-1);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Game");
	jsal_pop(1);

	return true;
}

static bool
js_Sphere_get_Version(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(API_VERSION);
	return true;
}

static bool
js_Sphere_get_frameRate(int num_args, bool is_ctor, int magic)
{
	// as far as Sphere v2 code is concerned, infinity, not 0, means "unthrottled".
	// that's stored as a zero internally though, so we need to translate.
	jsal_push_number(s_frame_rate > 0 ? s_frame_rate : INFINITY);
	return true;
}

static bool
js_Sphere_get_frameSkip(int num_args, bool is_ctor, int magic)
{
	jsal_push_number(screen_get_frameskip(g_screen));
	return true;
}

static bool
js_Sphere_get_fullScreen(int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(screen_get_fullscreen(g_screen));
	return true;
}

static bool
js_Sphere_set_frameRate(int num_args, bool is_ctor, int magic)
{
	double framerate;

	framerate = jsal_require_number(0);

	if (framerate < 1.0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");
	if (framerate != INFINITY)
		s_frame_rate = framerate;
	else
		s_frame_rate = 0;  // unthrottled
	return false;
}

static bool
js_Sphere_set_frameSkip(int num_args, bool is_ctor, int magic)
{
	double max_skips;

	max_skips = jsal_require_number(0);

	if (max_skips < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid frameskip");
	screen_set_frameskip(g_screen, max_skips);
	return false;
}

static bool
js_Sphere_set_fullScreen(int num_args, bool is_ctor, int magic)
{
	bool fullscreen;

	fullscreen = jsal_require_boolean(0);

	screen_set_fullscreen(g_screen, fullscreen);
	return false;
}

static bool
js_Sphere_abort(int num_args, bool is_ctor, int magic)
{
	const char* message;
	char*       text;

	message = num_args >= 1
		? jsal_to_string(0)
		: "some type of eaty pig just ate your game\n\n\n...and you*munch*";

	text = strnewf("Abort Requested\n\n%s\n", message);
	sphere_abort(text);
}

static bool
js_Sphere_now(int num_args, bool is_ctor, int magic)
{
	jsal_push_number(g_tick_count);
	return true;
}

static bool
js_Sphere_restart(int num_args, bool is_ctor, int magic)
{
	g_restarting = true;
	dispatch_cancel_all(true);
	return false;
}

static bool
js_Sphere_setResolution(int num_args, bool is_ctor, int magic)
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
js_Sphere_shutDown(int num_args, bool is_ctor, int magic)
{
	g_restarting = false;
	dispatch_cancel_all(true);
	return false;
}

static bool
js_Sphere_sleep(int num_args, bool is_ctor, int magic)
{
	int       num_frames;
	js_ref_t* resolver;
	script_t* script;

	num_frames = jsal_require_int(0);

	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid sleep timeout '%d'", num_frames);

	jsal_push_new_promise(&resolver, NULL);
	jsal_push_ref(resolver);
	script = script_new_function(-1);
	jsal_pop(1);
	jsal_unref(resolver);
	dispatch_defer(script, num_frames, JOB_UPDATE, true);
	return true;
}

static bool
js_Color_get_Color(int num_args, bool is_ctor, int magic)
{
	const struct x11_color* data;

	data = &COLORS[magic];
	jsal_pegasus_push_color(color_new(data->r, data->g, data->b, data->a), false);
	return true;
}

static bool
js_Color_is(int num_args, bool is_ctor, int magic)
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
js_Color_mix(int num_args, bool is_ctor, int magic)
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
js_Color_of(int num_args, bool is_ctor, int magic)
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
js_new_Color(int num_args, bool is_ctor, int magic)
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
js_Color_get_name(int num_args, bool is_ctor, int magic)
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
js_Color_get_r(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);

	jsal_push_number(color->r / 255.0);
	return true;
}

static bool
js_Color_get_g(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);

	jsal_push_number(color->g / 255.0);
	return true;
}

static bool
js_Color_get_b(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);

	jsal_push_number(color->b / 255.0);
	return true;
}

static bool
js_Color_get_a(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);

	jsal_push_number(color->a / 255.0);
	return true;
}

static bool
js_Color_set_r(int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);
	value = jsal_require_number(0);

	color->r = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_g(int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);
	value = jsal_require_number(0);

	color->g = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_b(int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);
	value = jsal_require_number(0);

	color->b = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_set_a(int num_args, bool is_ctor, int magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, PEGASUS_COLOR);
	value = jsal_require_number(0);

	color->a = fmin(fmax(value, 0.0), 1.0) * 255;
	return false;
}

static bool
js_Color_clone(int num_args, bool is_ctor, int magic)
{
	color_t color;

	jsal_push_this();
	color = jsal_pegasus_require_color(-1);

	jsal_pegasus_push_color(color, false);
	return true;
}

static bool
js_Color_fadeTo(int num_args, bool is_ctor, int magic)
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
js_new_DirectoryStream(int num_args, bool is_ctor, int magic)
{
	const char*  pathname;
	directory_t* stream;

	pathname = jsal_require_pathname(0, NULL, false, false);

	if (!(stream = directory_open(g_game, pathname)))
		jsal_error(JS_ERROR, "couldn't open directory '%s'", pathname);
	jsal_push_class_obj(PEGASUS_DIR_STREAM, stream, true);
	return true;
}

static void
js_DirectoryStream_finalize(void* host_ptr)
{
	directory_close(host_ptr);
}

static bool
js_DirectoryStream_get_fileCount(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	jsal_push_int(directory_num_files(directory));
	return true;
}

static bool
js_DirectoryStream_get_fileName(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	jsal_push_string(directory_pathname(directory));
	return true;
}

static bool
js_DirectoryStream_get_position(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	jsal_push_int(directory_position(directory));
	return true;
}

static bool
js_DirectoryStream_set_position(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;
	int          position;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");
	position = jsal_require_int(0);

	if (!directory_seek(directory, position))
		jsal_error(JS_ERROR, "couldn't set stream position");
	return false;
}

static bool
js_DirectoryStream_iterator(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	return true;
}

static bool
js_DirectoryStream_dispose(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM);

	jsal_set_class_ptr(-1, NULL);
	directory_close(directory);
	return false;
}

static bool
js_DirectoryStream_next(int num_args, bool is_ctor, int magic)
{
	directory_t*  directory;
	const path_t* entry_path;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	entry_path = directory_next(directory);
	jsal_push_new_object();
	if (entry_path != NULL) {
		jsal_push_boolean(false);
		jsal_put_prop_key(-2, s_key_done);
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
		jsal_put_prop_key(-2, s_key_value);
	}
	else {
		jsal_push_boolean(true);
		jsal_put_prop_key(-2, s_key_done);
	}
	return true;
}

static bool
js_DirectoryStream_rewind(int num_args, bool is_ctor, int magic)
{
	directory_t* directory;

	jsal_push_this();
	if (!(directory = jsal_require_class_obj(-1, PEGASUS_DIR_STREAM)))
		jsal_error(JS_ERROR, "the DirectoryStream has already been disposed");

	directory_rewind(directory);
	return false;
}

static bool
js_JobToken_cancel(int num_args, bool is_ctor, int magic)
{
	int64_t* token;

	jsal_push_this();
	token = jsal_require_class_obj(-1, PEGASUS_JOB_TOKEN);

	dispatch_cancel(*token);
	return false;
}

static bool
js_Dispatch_cancelAll(int num_args, bool is_ctor, int magic)
{
	dispatch_cancel_all(false);
	return false;
}

static bool
js_Dispatch_later(int num_args, bool is_ctor, int magic)
{
	script_t* script;
	int       timeout;
	int64_t   token;

	timeout = jsal_require_int(0);
	script = jsal_pegasus_require_script(1);

	if (!(token = dispatch_defer(script, timeout, JOB_UPDATE, false)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_now(int num_args, bool is_ctor, int magic)
{
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);

	if (!(token = dispatch_defer(script, 0, JOB_TICK, false)))
		jsal_error(JS_ERROR, "dispatch failed");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_onRender(int num_args, bool is_ctor, int magic)
{
	bool      background = false;
	double    priority = 0.0;
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);
	if (num_args >= 2) {
		jsal_require_object(1);
		if (jsal_get_prop_key(1, s_key_inBackground))
			background = jsal_require_boolean(-1);
		if (jsal_get_prop_key(1, s_key_priority))
			priority = jsal_require_number(-1);
		jsal_pop(2);
	}

	if (!(token = dispatch_recur(script, priority, background, JOB_RENDER)))
		jsal_error(JS_ERROR, "couldn't set up Dispatch job");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_Dispatch_onUpdate(int num_args, bool is_ctor, int magic)
{
	bool      background = false;
	double    priority = 0.0;
	script_t* script;
	int64_t   token;

	script = jsal_pegasus_require_script(0);
	if (num_args >= 2) {
		jsal_require_object(1);
		if (jsal_get_prop_key(1, s_key_inBackground))
			background = jsal_require_boolean(-1);
		if (jsal_get_prop_key(1, s_key_priority))
			priority = jsal_require_number(-1);
		jsal_pop(2);
	}

	if (!(token = dispatch_recur(script, priority, background, JOB_UPDATE)))
		jsal_error(JS_ERROR, "couldn't set up Dispatch job");
	jsal_pegasus_push_job_token(token);
	return true;
}

static bool
js_FS_createDirectory(int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_mkdir(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't create directory '%s'", pathname);
	return false;
}

static bool
js_FS_deleteFile(int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_unlink(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't delete file '%s'", pathname);
	return false;
}

static bool
js_FS_directoryExists(int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);

	jsal_push_boolean(game_dir_exists(g_game, pathname));
	return true;
}

static bool
js_FS_evaluateScript(int num_args, bool is_ctor, int magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!game_file_exists(g_game, filename))
		jsal_error(JS_ERROR, "script file not found '%s'", filename);
	if (!script_eval(filename))
		jsal_throw();
	return true;
}

static bool
js_FS_fileExists(int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, false);

	jsal_push_boolean(game_file_exists(g_game, pathname));
	return true;
}

static bool
js_FS_fullPath(int num_args, bool is_ctor, int magic)
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
js_FS_readFile(int num_args, bool is_ctor, int magic)
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
js_FS_relativePath(int num_args, bool is_ctor, int magic)
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
js_FS_removeDirectory(int num_args, bool is_ctor, int magic)
{
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false, true);

	if (!game_rmdir(g_game, pathname))
		jsal_error(JS_ERROR, "couldn't remove directory '%s'", pathname);
	return false;
}

static bool
js_FS_rename(int num_args, bool is_ctor, int magic)
{
	const char* new_pathname;
	const char* old_pathname;

	old_pathname = jsal_require_pathname(0, NULL, false, true);
	new_pathname = jsal_require_pathname(1, NULL, false, true);

	if (!game_rename(g_game, old_pathname, new_pathname))
		jsal_error(JS_ERROR, "couldn't rename '%s' to '%s'", old_pathname, new_pathname);
	return false;
}

static bool
js_FS_writeFile(int num_args, bool is_ctor, int magic)
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
js_new_FileStream(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_FILE_STREAM, file, true);
	return true;
}

static void
js_FileStream_finalize(void* host_ptr)
{
	file_close(host_ptr);
}

static bool
js_FileStream_get_fileName(int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");

	jsal_push_string(file_pathname(file));
	return true;
}

static bool
js_FileStream_get_fileSize(int num_args, bool is_ctor, int magic)
{
	file_t* file;
	long    file_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");

	file_pos = file_position(file);
	file_seek(file, 0, WHENCE_END);
	jsal_push_number(file_position(file));
	file_seek(file, file_pos, WHENCE_SET);
	return true;
}

static bool
js_FileStream_get_position(int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");

	jsal_push_number(file_position(file));
	return true;
}

static bool
js_FileStream_set_position(int num_args, bool is_ctor, int magic)
{
	file_t*   file;
	long long new_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");

	new_pos = jsal_require_number(0);
	file_seek(file, new_pos, WHENCE_SET);
	return false;
}

static bool
js_FileStream_dispose(int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM);

	jsal_set_class_ptr(-1, NULL);
	file_close(file);
	return false;
}

static bool
js_FileStream_read(int num_args, bool is_ctor, int magic)
{
	void*     buffer;
	size_t    buffer_size;
	file_t*   file;
	int       num_bytes;
	long      pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");
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
js_FileStream_write(int num_args, bool is_ctor, int magic)
{
	const void* data;
	file_t*     file;
	size_t      num_bytes;

	data = jsal_require_buffer_ptr(0, &num_bytes);

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, PEGASUS_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream cannot be used after disposal");

	if (file_write(file, data, num_bytes, 1) != num_bytes)
		jsal_error(JS_ERROR, "couldn't write to file");
	return false;
}

static bool
js_Font_get_Default(int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(PEGASUS_FONT, g_system_font, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Font(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	font_t*     font;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(font = font_load(filename)))
		jsal_error(JS_ERROR, "couldn't load font file '%s'", filename);
	jsal_push_this();
	jsal_push_class_obj(PEGASUS_FONT, font, true);
	return true;
}

static void
js_Font_finalize(void* host_ptr)
{
	font_unref(host_ptr);
}

static bool
js_Font_get_fileName(int num_args, bool is_ctor, int magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, PEGASUS_FONT);

	jsal_push_string(font_path(font));
	return true;
}

static bool
js_Font_get_height(int num_args, bool is_ctor, int magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, PEGASUS_FONT);

	jsal_push_int(font_height(font));
	return true;
}

static bool
js_Font_drawText(int num_args, bool is_ctor, int magic)
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
	font = jsal_require_class_obj(-1, PEGASUS_FONT);
	surface = jsal_require_class_obj(0, PEGASUS_SURFACE);
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
js_Font_getTextSize(int num_args, bool is_ctor, int magic)
{
	font_t*     font;
	int         num_lines;
	const char* text;
	int         width;
	wraptext_t* wraptext;

	jsal_push_this();
	font = jsal_require_class_obj(-1, PEGASUS_FONT);
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
js_Font_wordWrap(int num_args, bool is_ctor, int magic)
{
	const char* text = jsal_to_string(0);
	int         width = jsal_require_int(1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, PEGASUS_FONT);
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
js_new_IndexList(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_INDEX_LIST, ibo, true);
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
js_Joystick_get_Null(int num_args, bool is_ctor, int magic)
{
	int* device;

	device = malloc(sizeof(int));
	*device = -1;
	jsal_push_class_obj(PEGASUS_JOYSTICK, device, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Null");
	jsal_pop(1);

	return true;
}

static bool
js_Joystick_getDevices(int num_args, bool is_ctor, int magic)
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
js_Joystick_get_name(int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, PEGASUS_JOYSTICK);

	jsal_push_string(joy_name(*device));
	return true;
}

static bool
js_Joystick_get_numAxes(int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, PEGASUS_JOYSTICK);

	if (*device != -1)
		jsal_push_int(joy_num_axes(*device));
	else
		jsal_push_number(INFINITY);
	return true;
}

static bool
js_Joystick_get_numButtons(int num_args, bool is_ctor, int magic)
{
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, PEGASUS_JOYSTICK);

	if (*device != -1)
		jsal_push_int(joy_num_buttons(*device));
	else
		jsal_push_number(INFINITY);
	return true;
}

static bool
js_Joystick_getPosition(int num_args, bool is_ctor, int magic)
{
	int  index;
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, PEGASUS_JOYSTICK);
	index = jsal_require_int(0);

	if (*device != -1 && (index < 0 || index >= joy_num_axes(*device)))
		jsal_error(JS_RANGE_ERROR, "joystick axis ID out of range");

	jsal_push_number(joy_position(*device, index));
	return true;
}

static bool
js_Joystick_isPressed(int num_args, bool is_ctor, int magic)
{
	int  index;
	int* device;

	jsal_push_this();
	device = jsal_require_class_obj(-1, PEGASUS_JOYSTICK);
	index = jsal_require_int(0);

	if (*device != -1 && (index < 0 || index >= joy_num_buttons(*device)))
		jsal_error(JS_RANGE_ERROR, "joystick button ID out of range");

	jsal_push_boolean(joy_is_button_down(*device, index));
	return true;
}

static bool
js_Keyboard_get_Default(int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(PEGASUS_KEYBOARD, NULL, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_Keyboard_get_capsLock(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_CAPSLOCK));
	return true;
}

static bool
js_Keyboard_get_numLock(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_NUMLOCK));
	return true;
}

static bool
js_Keyboard_get_scrollLock(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);

	jsal_push_boolean(kb_is_toggled(ALLEGRO_KEY_SCROLLLOCK));
	return true;
}

static bool
js_Keyboard_charOf(int num_args, bool is_ctor, int magic)
{
	int  keycode;
	bool shifted = false;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);
	keycode = jsal_require_int(0);
	if (num_args >= 2)
		shifted = jsal_require_boolean(1);

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
js_Keyboard_clearQueue(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);

	kb_clear_queue();
	return false;
}

static bool
js_Keyboard_getKey(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);

	if (kb_queue_len() > 0)
		jsal_push_int(kb_get_key());
	else
		jsal_push_null();
	return true;
}

static bool
js_Keyboard_isPressed(int num_args, bool is_ctor, int magic)
{
	int keycode;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_KEYBOARD);
	keycode = jsal_require_int(0);

	jsal_push_boolean(kb_is_key_down(keycode));
	return true;
}

static bool
js_Mixer_get_Default(int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(PEGASUS_MIXER, mixer_ref(s_def_mixer), false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Mixer(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_MIXER, mixer, true);
	return true;
}

static void
js_Mixer_finalize(void* host_ptr)
{
	mixer_unref(host_ptr);
}

static bool
js_Mixer_get_volume(int num_args, bool is_ctor, int magic)
{
	mixer_t* mixer;

	jsal_push_this();
	mixer = jsal_require_class_obj(-1, PEGASUS_MIXER);

	jsal_push_number(mixer_get_gain(mixer));
	return true;
}

static bool
js_Mixer_set_volume(int num_args, bool is_ctor, int magic)
{
	float volume = jsal_require_number(0);

	mixer_t* mixer;

	jsal_push_this();
	mixer = jsal_require_class_obj(-1, PEGASUS_MIXER);

	mixer_set_gain(mixer, volume);
	return false;
}

static bool
js_new_Model(int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	int       num_shapes;
	shader_t* shader;
	shape_t*  shape;

	int i;

	jsal_require_object_coercible(0);
	shader = num_args >= 2
		? jsal_require_class_obj(1, PEGASUS_SHADER)
		: galileo_shader();

	if (!jsal_is_array(0))
		jsal_error(JS_TYPE_ERROR, "array required");

	group = model_new(shader);
	num_shapes = jsal_get_length(0);
	for (i = 0; i < num_shapes; ++i) {
		jsal_get_prop_index(0, i);
		shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);
		model_add_shape(group, shape);
	}
	jsal_push_class_obj(PEGASUS_MODEL, group, true);
	return true;
}

static void
js_Model_finalize(void* host_ptr)
{
	model_unref(host_ptr);
}

static bool
js_Model_get_shader(int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	shader_t* shader;

	jsal_push_this();
	group = jsal_require_class_obj(-1, PEGASUS_MODEL);

	shader = model_get_shader(group);
	jsal_push_class_obj(PEGASUS_SHADER, shader_ref(shader), false);
	return true;
}

static bool
js_Model_get_transform(int num_args, bool is_ctor, int magic)
{
	model_t*     group;
	transform_t* transform;

	jsal_push_this();
	group = jsal_require_class_obj(-1, PEGASUS_MODEL);

	transform = model_get_transform(group);
	jsal_push_class_obj(PEGASUS_TRANSFORM, transform_ref(transform), false);
	return true;
}

static bool
js_Model_set_shader(int num_args, bool is_ctor, int magic)
{
	model_t*  group;
	shader_t* shader;

	jsal_push_this();
	group = jsal_require_class_obj(-1, PEGASUS_MODEL);
	shader = jsal_require_class_obj(0, PEGASUS_SHADER);

	model_set_shader(group, shader);
	return false;
}

static bool
js_Model_set_transform(int num_args, bool is_ctor, int magic)
{
	model_t*     group;
	transform_t* transform;

	jsal_push_this();
	group = jsal_require_class_obj(-1, PEGASUS_MODEL);
	transform = jsal_require_class_obj(0, PEGASUS_TRANSFORM);

	model_set_transform(group, transform);
	return false;
}

static bool
js_Model_draw(int num_args, bool is_ctor, int magic)
{
	model_t* group;
	image_t* surface;

	jsal_push_this();
	group = jsal_require_class_obj(-1, PEGASUS_MODEL);
	surface = num_args >= 1 ? jsal_require_class_obj(0, PEGASUS_SURFACE)
		: screen_backbuffer(g_screen);

	if (!screen_skip_frame(g_screen))
		model_draw(group, surface);
	return false;
}

static bool
js_Shader_clone(int num_args, bool is_ctor, int magic)
{
	shader_t* dolly;
	shader_t* shader;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);

	dolly = shader_dup(shader);
	jsal_push_class_obj(PEGASUS_SHADER, dolly, false);
	return true;
}

static bool
js_Shader_setBoolean(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	bool        value;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
	name = jsal_require_string(0);
	value = jsal_require_boolean(1);

	shader_put_bool(shader, name, value);
	return false;
}

static bool
js_Shader_setColorVector(int num_args, bool is_ctor, int magic)
{
	color_t     color;
	const char* name;
	shader_t*   shader;
	float       values[4];

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
	name = jsal_require_string(0);
	color = jsal_pegasus_require_color(1);

	values[0] = color.r / 255.0;
	values[1] = color.g / 255.0;
	values[2] = color.b / 255.0;
	values[3] = color.a / 255.0;
	shader_put_float_vector(shader, name, values, 4);
	return false;
}

static bool
js_Shader_setFloat(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	float       value;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
	name = jsal_require_string(0);
	value = jsal_require_number(1);

	shader_put_float(shader, name, value);
	return false;
}

static bool
js_Shader_setFloatArray(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	int         size;
	float*      values;

	int i;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
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
	shader_put_float_array(shader, name, values, size);
	free(values);
	return false;
}

static bool
js_Shader_setFloatVector(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	int         size;
	float       values[4];

	int i;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
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
	shader_put_float_vector(shader, name, values, size);
	return false;
}

static bool
js_Shader_setInt(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	int         value;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
	name = jsal_require_string(0);
	value = jsal_require_int(1);

	shader_put_int(shader, name, value);
	return false;
}

static bool
js_Shader_setIntArray(int num_args, bool is_ctor, int magic)
{
	const char* name;
	shader_t*   shader;
	int         size;
	int*        values;

	int i;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
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
	shader_put_int_array(shader, name, values, size);
	free(values);
	return false;
}

static bool
js_Shader_setIntVector(int num_args, bool is_ctor, int magic)
{
	shader_t*   shader;
	const char* name;
	int         size;
	int         values[4];

	int i;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
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
	shader_put_int_vector(shader, name, values, size);
	return false;
}

static bool
js_Shader_setMatrix(int num_args, bool is_ctor, int magic)
{
	const char*  name;
	shader_t*    shader;
	transform_t* transform;

	jsal_push_this();
	shader = jsal_require_class_obj(-1, PEGASUS_SHADER);
	name = jsal_require_string(0);
	transform = jsal_require_class_obj(1, PEGASUS_TRANSFORM);

	shader_put_matrix(shader, name, transform);
	return false;
}

static bool
js_Mouse_get_Default(int num_args, bool is_ctor, int magic)
{
	jsal_push_class_obj(PEGASUS_MOUSE, NULL, false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_Mouse_get_x(int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_MOUSE);

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(x);
	return true;
}

static bool
js_Mouse_get_y(int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_MOUSE);

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(y);
	return true;
}

static bool
js_Mouse_clearQueue(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_MOUSE);

	mouse_clear_queue();
	return false;
}

static bool
js_Mouse_getEvent(int num_args, bool is_ctor, int magic)
{
	mouse_event_t event;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_MOUSE);

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
js_Mouse_isPressed(int num_args, bool is_ctor, int magic)
{
	mouse_key_t key;

	jsal_push_this();
	jsal_require_class_obj(-1, PEGASUS_MOUSE);

	key = jsal_require_int(0);
	if (key < 0 || key >= MOUSE_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid MouseKey constant");

	jsal_push_boolean(mouse_is_key_down(key));
	return true;
}

static bool
js_RNG_fromSeed(int num_args, bool is_ctor, int magic)
{
	uint64_t seed;
	xoro_t*  xoro;

	seed = jsal_require_number(0);

	xoro = xoro_new(seed);
	jsal_push_class_obj(PEGASUS_RNG, xoro, false);
	return true;
}

static bool
js_RNG_fromState(int num_args, bool is_ctor, int magic)
{
	const char* state;
	xoro_t*     xoro;

	state = jsal_require_string(0);

	xoro = xoro_new(0);
	if (!xoro_set_state(xoro, state)) {
		xoro_unref(xoro);
		jsal_error(JS_TYPE_ERROR, "invalid RNG state string");
	}
	jsal_push_class_obj(PEGASUS_RNG, xoro, false);
	return true;
}

static bool
js_new_RNG(int num_args, bool is_ctor, int magic)
{
	xoro_t* xoro;

	xoro = xoro_new((uint64_t)(al_get_time() * 1000000));
	jsal_push_class_obj(PEGASUS_RNG, xoro, true);
	return true;
}

static void
js_RNG_finalize(void* host_ptr)
{
	xoro_unref(host_ptr);
}

static bool
js_RNG_get_state(int num_args, bool is_ctor, int magic)
{
	char    state[33];
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, PEGASUS_RNG);

	xoro_get_state(xoro, state);
	jsal_push_string(state);
	return true;
}

static bool
js_RNG_set_state(int num_args, bool is_ctor, int magic)
{
	const char* state;
	xoro_t*     xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, PEGASUS_RNG);
	state = jsal_require_string(0);

	if (!xoro_set_state(xoro, state))
		jsal_error(JS_TYPE_ERROR, "invalid RNG state string");
	return false;
}

static bool
js_RNG_iterator(int num_args, bool is_ctor, int magic)
{
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, PEGASUS_RNG);

	return true;
}

static bool
js_RNG_next(int num_args, bool is_ctor, int magic)
{
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, PEGASUS_RNG);

	jsal_push_new_object();
	jsal_push_boolean(false);
	jsal_put_prop_key(-2, s_key_done);
	jsal_push_number(xoro_gen_double(xoro));
	jsal_put_prop_key(-2, s_key_value);
	return true;
}

static bool
js_SSj_flipScreen(int num_args, bool is_ctor, int magic)
{
#if defined(MINISPHERE_SPHERUN)
	screen_flip(g_screen, 0, false);
#endif
	return false;
}

static bool
js_SSj_log(int num_args, bool is_ctor, int magic)
{
#if defined(MINISPHERE_SPHERUN)
	const char* text;

	if (jsal_is_error(0)) {
		jsal_get_prop_key(0, s_key_stack);
		jsal_replace(0);
	} 
	else if (jsal_is_object(0) && !jsal_is_function(0)) {
		jsal_stringify(0);
	}
	
	text = jsal_to_string(0);
	debugger_log(text, (ki_log_op_t)magic, true);
#endif
	return false;
}

static bool
js_new_Sample(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	sample_t*   sample;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(sample = sample_new(filename, true)))
		jsal_error(JS_ERROR, "couldn't load sample '%s'", filename);
	jsal_push_class_obj(PEGASUS_SAMPLE, sample, true);
	return true;
}

static void
js_Sample_finalize(void* host_ptr)
{
	sample_unref(host_ptr);
}

static bool
js_Sample_get_fileName(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, PEGASUS_SAMPLE);

	jsal_push_string(sample_path(sample));
	return true;
}

static bool
js_Sample_play(int num_args, bool is_ctor, int magic)
{
	mixer_t*  mixer;
	float     pan = 0.0;
	sample_t* sample;
	float     speed = 1.0;
	float     volume = 1.0;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, PEGASUS_SAMPLE);
	mixer = jsal_require_class_obj(0, PEGASUS_MIXER);
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
js_Sample_stopAll(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, PEGASUS_SAMPLE);

	sample_stop_all(sample);
	return false;
}

static bool
js_new_Server(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_SERVER, server, true);
	return true;
}

static void
js_Server_finalize(void* host_ptr)
{
	server_unref(host_ptr);
}

static bool
js_Server_accept(int num_args, bool is_ctor, int magic)
{
	socket_t* new_socket;
	server_t* server;

	jsal_push_this();
	server = jsal_require_class_obj(-1, PEGASUS_SERVER);

	if (server == NULL)
		jsal_error(JS_ERROR, "server has shut down");
	new_socket = server_accept(server);
	if (new_socket != NULL)
		jsal_push_class_obj(PEGASUS_SOCKET, new_socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Server_close(int num_args, bool is_ctor, int magic)
{
	server_t* server;

	jsal_push_this();
	server = jsal_require_class_obj(-1, PEGASUS_SERVER);

	jsal_set_class_ptr(-1, NULL);
	server_unref(server);
	return false;
}

static bool
js_Shader_get_Default(int num_args, bool is_ctor, int magic)
{
	shader_t* shader;

	if (!(shader = galileo_shader()))
		jsal_error(JS_ERROR, "couldn't compile default shaders");
	jsal_push_class_obj(PEGASUS_SHADER, shader_ref(shader), false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Default");
	jsal_pop(1);

	return true;
}

static bool
js_new_Shader(int num_args, bool is_ctor, int magic)
{
	const char* fragment_pathname;
	shader_t*   shader;
	const char* vertex_pathname;

	jsal_require_object(0);
	jsal_get_prop_string(0, "fragmentFile");
	jsal_require_string(-1);
	jsal_get_prop_string(0, "vertexFile");
	jsal_require_string(-1);

	fragment_pathname = jsal_require_pathname(-2, NULL, false, false);
	vertex_pathname = jsal_require_pathname(-1, NULL, false, false);
	if (!(shader = shader_new(vertex_pathname, fragment_pathname)))
		jsal_error(JS_ERROR, "couldn't compile shader program");
	jsal_push_class_obj(PEGASUS_SHADER, shader, true);
	return true;
}

static void
js_Shader_finalize(void* host_ptr)
{
	shader_unref(host_ptr);
}

static bool
js_new_Shape(int num_args, bool is_ctor, int magic)
{
	ibo_t*       ibo = NULL;
	shape_t*     shape;
	image_t*     texture = NULL;
	shape_type_t type;
	vbo_t*       vbo;

	type = jsal_require_int(0);
	if (jsal_is_class_obj(1, PEGASUS_TEXTURE) || jsal_is_null(1)) {
		texture = !jsal_is_null(1) ? jsal_require_class_obj(1, PEGASUS_TEXTURE) : NULL;
		vbo = jsal_require_class_obj(2, PEGASUS_VERTEX_LIST);
		if (num_args >= 4)
			ibo = jsal_require_class_obj(3, PEGASUS_INDEX_LIST);
	}
	else {
		vbo = jsal_require_class_obj(1, PEGASUS_VERTEX_LIST);
		if (num_args >= 3)
			ibo = jsal_require_class_obj(2, PEGASUS_INDEX_LIST);
	}

	if (type < 0 || type >= SHAPE_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid ShapeType constant");

	shape = shape_new(vbo, ibo, type, texture);
	jsal_push_class_obj(PEGASUS_SHAPE, shape, true);
	return true;
}

static void
js_Shape_finalize(void* host_ptr)
{
	shape_unref(host_ptr);
}

static bool
js_Shape_get_indexList(int num_args, bool is_ctor, int magic)
{
	ibo_t*   ibo;
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);

	ibo = shape_get_ibo(shape);
	if (ibo != NULL)
		jsal_push_class_obj(PEGASUS_INDEX_LIST, ibo_ref(ibo), false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Shape_get_texture(int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	image_t* texture;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);

	texture = shape_get_texture(shape);
	if (texture != NULL)
		jsal_push_class_obj(PEGASUS_TEXTURE, image_ref(texture), false);
	else
		jsal_push_null();
	return true;
}

static bool
js_Shape_get_vertexList(int num_args, bool is_ctor, int magic)
{
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);

	jsal_push_class_obj(PEGASUS_VERTEX_LIST, vbo_ref(shape_get_vbo(shape)), false);
	return true;
}

static bool
js_Shape_set_indexList(int num_args, bool is_ctor, int magic)
{
	ibo_t*   ibo = NULL;
	shape_t* shape;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);
	if (!jsal_is_null(0))
		ibo = jsal_require_class_obj(0, PEGASUS_INDEX_LIST);

	shape_set_ibo(shape, ibo);
	return false;
}

static bool
js_Shape_set_texture(int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	image_t* texture = NULL;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);
	if (!jsal_is_null(0))
		texture = jsal_require_class_obj(0, PEGASUS_TEXTURE);

	shape_set_texture(shape, texture);
	return false;
}

static bool
js_Shape_set_vertexList(int num_args, bool is_ctor, int magic)
{
	shape_t* shape;
	vbo_t*   vbo;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);
	vbo = jsal_require_class_obj(0, PEGASUS_VERTEX_LIST);

	shape_set_vbo(shape, vbo);
	return false;
}

static bool
js_Shape_draw(int num_args, bool is_ctor, int magic)
{
	shape_t*     shape;
	image_t*     surface = NULL;
	transform_t* transform = NULL;

	jsal_push_this();
	shape = jsal_require_class_obj(-1, PEGASUS_SHAPE);
	surface = num_args >= 1 ? jsal_require_class_obj(0, PEGASUS_SURFACE)
		: screen_backbuffer(g_screen);
	if (num_args >= 2)
		transform = jsal_require_class_obj(1, PEGASUS_TRANSFORM);

	shape_draw(shape, surface, transform);
	return false;
}

static bool
js_new_Socket(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_SOCKET, socket, true);
	return true;
}

static void
js_Socket_finalize(void* host_ptr)
{
	socket_unref(host_ptr);
}

static bool
js_Socket_get_bytesPending(int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is not connected");
	jsal_push_int((int)socket_peek(socket));
	return true;
}

static bool
js_Socket_get_connected(int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

	if (socket != NULL)
		jsal_push_boolean(socket_connected(socket));
	else
		jsal_push_boolean(false);
	return true;
}

static bool
js_Socket_get_remoteAddress(int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	jsal_push_string(socket_hostname(socket));
	return true;
}

static bool
js_Socket_get_remotePort(int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	jsal_push_int(socket_port(socket));
	return true;
}

static bool
js_Socket_close(int num_args, bool is_ctor, int magic)
{
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

	jsal_set_class_ptr(-1, NULL);
	socket_unref(socket);
	return false;
}

static bool
js_Socket_connectTo(int num_args, bool is_ctor, int magic)
{
	const char* hostname;
	int         port;
	socket_t*   socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);
	hostname = jsal_require_string(0);
	port = jsal_require_int(1);

	if (!socket_connect(socket, hostname, port))
		jsal_error(JS_ERROR, "couldn't connect to %s", hostname);
	return false;
}

static bool
js_Socket_read(int num_args, bool is_ctor, int magic)
{
	void*     buffer;
	size_t    bytes_read;
	int       num_bytes;
	socket_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);

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
js_Socket_write(int num_args, bool is_ctor, int magic)
{
	const uint8_t* payload;
	socket_t*      socket;
	size_t         write_size;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, PEGASUS_SOCKET);
	payload = jsal_require_buffer_ptr(0, &write_size);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket is closed");
	if (!socket_connected(socket))
		jsal_error(JS_ERROR, "socket disconnected");
	socket_write(socket, payload, write_size);
	return false;
}

static bool
js_new_Sound(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	sound_t*    sound;

	filename = jsal_require_pathname(0, NULL, false, false);

	if (!(sound = sound_new(filename)))
		jsal_error(JS_ERROR, "couldn't load sound '%s'", filename);
	jsal_push_class_obj(PEGASUS_SOUND, sound, true);
	return true;
}

static void
js_Sound_finalize(void* host_ptr)
{
	sound_unref(host_ptr);
}

static bool
js_Sound_get_fileName(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_string(sound_path(sound));
	return true;
}

static bool
js_Sound_get_length(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_number(sound_len(sound));
	return true;
}

static bool
js_Sound_get_pan(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_number(sound_pan(sound));
	return true;
}

static bool
js_Sound_set_pan(int num_args, bool is_ctor, int magic)
{
	float    new_pan;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);
	new_pan = jsal_require_number(0);

	sound_set_pan(sound, new_pan);
	return false;
}

static bool
js_Sound_get_speed(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_number(sound_speed(sound));
	return true;
}

static bool
js_Sound_set_speed(int num_args, bool is_ctor, int magic)
{
	float new_speed = jsal_require_number(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	sound_set_speed(sound, new_speed);
	return false;
}

static bool
js_Sound_get_playing(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_boolean(sound_playing(sound));
	return true;
}

static bool
js_Sound_get_position(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_number(sound_tell(sound));
	return true;
}

static bool
js_Sound_set_position(int num_args, bool is_ctor, int magic)
{
	double   new_pos;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);
	new_pos = jsal_require_number(0);

	sound_seek(sound, new_pos);
	return false;
}

static bool
js_Sound_get_repeat(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_boolean(sound_repeat(sound));
	return true;
}

static bool
js_Sound_set_repeat(int num_args, bool is_ctor, int magic)
{
	bool is_looped = jsal_require_boolean(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	sound_set_repeat(sound, is_looped);
	return false;
}

static bool
js_Sound_get_volume(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	jsal_push_number(sound_gain(sound));
	return true;
}

static bool
js_Sound_set_volume(int num_args, bool is_ctor, int magic)
{
	float volume = jsal_require_number(0);

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	sound_set_gain(sound, volume);
	return false;
}

static bool
js_Sound_pause(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	sound_pause(sound, true);
	return false;
}

static bool
js_Sound_play(int num_args, bool is_ctor, int magic)
{
	mixer_t* mixer;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	if (num_args < 1)
		sound_pause(sound, false);
	else {
		mixer = jsal_require_class_obj(0, PEGASUS_MIXER);
		sound_play(sound, mixer);
	}
	return false;
}

static bool
js_Sound_stop(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, PEGASUS_SOUND);

	sound_stop(sound);
	return false;
}

static bool
js_new_SoundStream(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_SOUND_STREAM, stream, true);
	return true;
}

static void
js_SoundStream_finalize(void* host_ptr)
{
	stream_unref(host_ptr);
}

static bool
js_SoundStream_get_length(int num_args, bool is_ctor, int magic)
{
	stream_t*    stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, PEGASUS_SOUND_STREAM);

	jsal_push_number(stream_length(stream));
	return true;
}

static bool
js_SoundStream_pause(int num_args, bool is_ctor, int magic)
{
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, PEGASUS_SOUND_STREAM);

	stream_pause(stream, true);
	return false;
}

static bool
js_SoundStream_play(int num_args, bool is_ctor, int magic)
{
	mixer_t*  mixer;
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, PEGASUS_SOUND_STREAM);

	if (num_args < 1)
		stream_pause(stream, false);
	else {
		mixer = jsal_require_class_obj(0, PEGASUS_MIXER);
		stream_play(stream, mixer);
	}
	return false;
}

static bool
js_SoundStream_stop(int num_args, bool is_ctor, int magic)
{
	stream_t* stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, PEGASUS_SOUND_STREAM);

	stream_stop(stream);
	return false;
}

static bool
js_SoundStream_write(int num_args, bool is_ctor, int magic)
{
	const void* data;
	size_t      size;
	stream_t*   stream;

	jsal_push_this();
	stream = jsal_require_class_obj(-1, PEGASUS_SOUND_STREAM);

	data = jsal_require_buffer_ptr(0, &size);
	stream_buffer(stream, data, size);
	return false;
}

static bool
js_Surface_get_Screen(int num_args, bool is_ctor, int magic)
{
	image_t* backbuffer;

	backbuffer = screen_backbuffer(g_screen);
	jsal_push_class_obj(PEGASUS_SURFACE, image_ref(backbuffer), false);

	jsal_push_this();
	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_dup(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "Screen");
	jsal_pop(1);

	return true;
}

static bool
js_new_Surface(int num_args, bool is_ctor, int magic)
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
	else if (jsal_is_class_obj(0, PEGASUS_TEXTURE)) {
		src_image = jsal_require_class_obj(0, PEGASUS_TEXTURE);
		if (!(image = image_clone(src_image)))
			jsal_error(JS_ERROR, "couldn't create surface");
	}
	else {
		filename = jsal_require_pathname(0, NULL, false, false);
		if (!(image = image_load(filename)))
			jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	}
	jsal_push_class_obj(PEGASUS_SURFACE, image, true);
	return true;
}

static void
js_Surface_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Surface_get_height(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Surface_get_transform(int num_args, bool is_ctor, int magic)
{
	image_t*     image;
	transform_t* transform;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);

	transform = image_get_transform(image);
	jsal_push_class_obj(PEGASUS_TRANSFORM, transform_ref(transform), false);
	return true;
}

static bool
js_Surface_get_width(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_Surface_set_transform(int num_args, bool is_ctor, int magic)
{
	image_t*     image;
	transform_t* transform;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);
	transform = jsal_require_class_obj(0, PEGASUS_TRANSFORM);

	image_set_transform(image, transform);
	return false;
}

static bool
js_Surface_clipTo(int num_args, bool is_ctor, int magic)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);
	x = jsal_require_int(0);
	y = jsal_require_int(1);
	width = jsal_require_int(2);
	height = jsal_require_int(3);

	image_set_scissor(image, rect(x, y, x + width, y + height));
	return false;
}

static bool
js_Surface_toTexture(int num_args, bool is_ctor, int magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_SURFACE);

	if ((new_image = image_clone(image)) == NULL)
		jsal_error(JS_ERROR, "image creation failed");
	jsal_push_class_obj(PEGASUS_TEXTURE, new_image, false);
	return true;
}

static bool
js_new_TextDecoder(int num_args, bool is_ctor, int magic)
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
	jsal_push_class_obj(PEGASUS_TEXT_DEC, decoder, true);
	return true;
}

static void
js_TextDecoder_finalize(void* host_ptr)
{
	decoder_free(host_ptr);
}

static bool
js_TextDecoder_get_encoding(int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, PEGASUS_TEXT_DEC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextDecoder_get_fatal(int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, PEGASUS_TEXT_DEC);

	jsal_push_boolean(decoder_fatal(decoder));
	return true;
}

static bool
js_TextDecoder_get_ignoreBOM(int num_args, bool is_ctor, int magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, PEGASUS_TEXT_DEC);

	jsal_push_boolean(decoder_ignore_bom(decoder));
	return true;
}

static bool
js_TextDecoder_decode(int num_args, bool is_ctor, int magic)
{
	decoder_t*     decoder;
	lstring_t*     head;
	const uint8_t* input = (uint8_t*)"";
	size_t         length = 0;
	lstring_t*     string;
	bool           streaming = false;
	lstring_t*     tail = NULL;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, PEGASUS_TEXT_DEC);
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
js_new_TextEncoder(int num_args, bool is_ctor, int magic)
{
	encoder_t* encoder;

	encoder = encoder_new();
	jsal_push_class_obj(PEGASUS_TEXT_ENC, encoder, true);
	return true;
}

static void
js_TextEncoder_finalize(void* host_ptr)
{
	encoder_free(host_ptr);
}

static bool
js_TextEncoder_get_encoding(int num_args, bool is_ctor, int magic)
{
	encoder_t* encoder;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, PEGASUS_TEXT_ENC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextEncoder_encode(int num_args, bool is_ctor, int magic)
{
	void*      buffer;
	lstring_t* input;
	uint8_t*   output;
	size_t     size;

	encoder_t* encoder;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, PEGASUS_TEXT_ENC);
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
js_new_Texture(int num_args, bool is_ctor, int magic)
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

	if (num_args >= 3 && jsal_is_class_obj(2, PEGASUS_COLOR)) {
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
	else if (jsal_is_class_obj(0, PEGASUS_SURFACE)) {
		// create an Image from a Surface
		src_image = jsal_require_class_obj(0, PEGASUS_SURFACE);
		if (!(image = image_clone(src_image)))
			jsal_error(JS_ERROR, "image creation failed");
	}
	else {
		// create an Image by loading an image file
		filename = jsal_require_pathname(0, NULL, false, false);
		if (!(image = image_load(filename)))
			jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	}
	jsal_push_class_obj(PEGASUS_TEXTURE, image, true);
	return true;
}

static void
js_Texture_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Texture_get_fileName(int num_args, bool is_ctor, int magic)
{
	image_t*    image;
	const char* path;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_TEXTURE);

	if (path = image_path(image))
		jsal_push_string(path);
	else
		jsal_push_null();
	return true;
}

static bool
js_Texture_get_height(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_TEXTURE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Texture_get_width(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, PEGASUS_TEXTURE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_new_Transform(int num_args, bool is_ctor, int magic)
{
	transform_t* transform;

	transform = transform_new();
	jsal_push_class_obj(PEGASUS_TRANSFORM, transform, true);
	return true;
}

static void
js_Transform_finalize(void* host_ptr)
{
	transform_unref(host_ptr);
}

static bool
js_Transform_get_matrix(int num_args, bool is_ctor, int magic)
{
	transform_t* transform;
	float*       values;

	int i, j;

	if (magic == 0) {
		// on first access: set up getters and setters
		jsal_push_this();
		transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
		values = transform_values(transform);
		jsal_push_new_object();
		for (i = 0; i < 4; ++i) {
			jsal_push_new_object();
			for (j = 0; j < 4; ++j) {
				jsal_push_new_function(js_Transform_get_matrix, "get", 0, 1 + (j * 4 + i));
				jsal_push_this();
				jsal_put_prop_string(-2, "\xFF" "transform");
				
				jsal_push_new_function(js_Transform_set_matrix, "set", 0, 1 + (j * 4 + i));
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
		jsal_push_callee();
		jsal_get_prop_string(-1, "\xFF" "transform");
		transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
		values = transform_values(transform);
		jsal_push_number(values[magic - 1]);
		return true;
	}
}

static bool
js_Transform_set_matrix(int num_args, bool is_ctor, int magic)
{
	float        new_value;
	transform_t* transform;
	float*       values;

	new_value = jsal_require_number(0);

	jsal_push_callee();
	jsal_get_prop_string(-1, "\xFF" "transform");
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
	values = transform_values(transform);
	values[magic - 1] = new_value;
	return false;
}

static bool
js_Transform_compose(int num_args, bool is_ctor, int magic)
{
	transform_t* other;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
	other = jsal_require_class_obj(0, PEGASUS_TRANSFORM);

	transform_compose(transform, other);
	return true;
}

static bool
js_Transform_identity(int num_args, bool is_ctor, int magic)
{
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);

	transform_identity(transform);
	return true;
}

static bool
js_Transform_project2D(int num_args, bool is_ctor, int magic)
{
	transform_t* transform;
	float        x1, x2;
	float        y1, y2;
	float        z1 = -1.0f;
	float        z2 = 1.0f;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
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
js_Transform_project3D(int num_args, bool is_ctor, int magic)
{
	float        aspect;
	float        fov;
	float        fw, fh;
	transform_t* transform;
	float        z1, z2;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
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
js_Transform_rotate(int num_args, bool is_ctor, int magic)
{
	float        theta;
	transform_t* transform;
	float        vx = 0.0;
	float        vy = 0.0;
	float        vz = 1.0;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
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
js_Transform_scale(int num_args, bool is_ctor, int magic)
{
	float        sx;
	float        sy;
	float        sz = 1.0;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
	sx = jsal_require_number(0);
	sy = jsal_require_number(1);
	if (num_args >= 3)
		sz = jsal_require_number(2);

	transform_scale(transform, sx, sy, sz);
	return true;
}

static bool
js_Transform_translate(int num_args, bool is_ctor, int magic)
{
	float        dx;
	float        dy;
	float        dz = 0.0;
	transform_t* transform;

	jsal_push_this();
	transform = jsal_require_class_obj(-1, PEGASUS_TRANSFORM);
	dx = jsal_require_number(0);
	dy = jsal_require_number(1);
	if (num_args >= 3)
		dz = jsal_require_number(2);

	transform_translate(transform, dx, dy, dz);
	return true;
}

static bool
js_new_VertexList(int num_args, bool is_ctor, int magic)
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
	
	jsal_push_class_obj(PEGASUS_VERTEX_LIST, vbo, true);
	return true;
}

static void
js_VertexList_finalize(void* host_ptr)
{
	vbo_unref(host_ptr);
}
