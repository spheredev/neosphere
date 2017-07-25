#include "minisphere.h"
#include "pegasus.h"

#include "api.h"
#include "async.h"
#include "audio.h"
#include "color.h"
#include "console.h"
#include "debugger.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
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

static duk_ret_t js_require                    (duk_context* ctx);
static duk_ret_t js_screen_get_frameRate       (duk_context* ctx);
static duk_ret_t js_screen_get_frameSkip       (duk_context* ctx);
static duk_ret_t js_screen_get_fullScreen      (duk_context* ctx);
static duk_ret_t js_screen_set_frameRate       (duk_context* ctx);
static duk_ret_t js_screen_set_frameSkip       (duk_context* ctx);
static duk_ret_t js_screen_set_fullScreen      (duk_context* ctx);
static duk_ret_t js_screen_flip                (duk_context* ctx);
static duk_ret_t js_screen_now                 (duk_context* ctx);
static duk_ret_t js_screen_resize              (duk_context* ctx);
static duk_ret_t js_Sphere_get_APILevel        (duk_context* ctx);
static duk_ret_t js_Sphere_get_Game            (duk_context* ctx);
static duk_ret_t js_Sphere_get_Platform        (duk_context* ctx);
static duk_ret_t js_Sphere_get_Version         (duk_context* ctx);
static duk_ret_t js_Sphere_abort               (duk_context* ctx);
static duk_ret_t js_Sphere_exit                (duk_context* ctx);
static duk_ret_t js_Sphere_restart             (duk_context* ctx);
static duk_ret_t js_Sphere_run                 (duk_context* ctx);
static duk_ret_t js_Sphere_sleep               (duk_context* ctx);
static duk_ret_t js_Color_get_Color            (duk_context* ctx);
static duk_ret_t js_Color_is                   (duk_context* ctx);
static duk_ret_t js_Color_mix                  (duk_context* ctx);
static duk_ret_t js_Color_of                   (duk_context* ctx);
static duk_ret_t js_new_Color                  (duk_context* ctx);
static duk_ret_t js_Color_get_name             (duk_context* ctx);
static duk_ret_t js_Color_clone                (duk_context* ctx);
static duk_ret_t js_Color_fade                 (duk_context* ctx);
static duk_ret_t js_Dispatch_cancel            (duk_context* ctx);
static duk_ret_t js_Dispatch_cancelAll         (duk_context* ctx);
static duk_ret_t js_Dispatch_later             (duk_context* ctx);
static duk_ret_t js_Dispatch_now               (duk_context* ctx);
static duk_ret_t js_Dispatch_onRender          (duk_context* ctx);
static duk_ret_t js_Dispatch_onUpdate          (duk_context* ctx);
static duk_ret_t js_FS_createDirectory         (duk_context* ctx);
static duk_ret_t js_FS_deleteFile              (duk_context* ctx);
static duk_ret_t js_FS_exists                  (duk_context* ctx);
static duk_ret_t js_FS_readFile                (duk_context* ctx);
static duk_ret_t js_FS_rename                  (duk_context* ctx);
static duk_ret_t js_FS_resolve                 (duk_context* ctx);
static duk_ret_t js_FS_removeDirectory         (duk_context* ctx);
static duk_ret_t js_FS_writeFile               (duk_context* ctx);
static duk_ret_t js_new_FileStream             (duk_context* ctx);
static duk_ret_t js_FileStream_finalize        (duk_context* ctx);
static duk_ret_t js_FileStream_dispose         (duk_context* ctx);
static duk_ret_t js_FileStream_get_fileName    (duk_context* ctx);
static duk_ret_t js_FileStream_get_fileSize    (duk_context* ctx);
static duk_ret_t js_FileStream_get_position    (duk_context* ctx);
static duk_ret_t js_FileStream_set_position    (duk_context* ctx);
static duk_ret_t js_FileStream_read            (duk_context* ctx);
static duk_ret_t js_FileStream_write           (duk_context* ctx);
static duk_ret_t js_Font_get_Default           (duk_context* ctx);
static duk_ret_t js_new_Font                   (duk_context* ctx);
static duk_ret_t js_Font_finalize              (duk_context* ctx);
static duk_ret_t js_Font_get_fileName          (duk_context* ctx);
static duk_ret_t js_Font_get_height            (duk_context* ctx);
static duk_ret_t js_Font_drawText              (duk_context* ctx);
static duk_ret_t js_Font_getTextSize           (duk_context* ctx);
static duk_ret_t js_Font_wordWrap              (duk_context* ctx);
static duk_ret_t js_new_IndexList              (duk_context* ctx);
static duk_ret_t js_IndexList_finalize         (duk_context* ctx);
static duk_ret_t js_JobToken_finalize          (duk_context* ctx);
static duk_ret_t js_Joystick_get_Null          (duk_context* ctx);
static duk_ret_t js_Joystick_getDevices        (duk_context* ctx);
static duk_ret_t js_Joystick_finalize          (duk_context* ctx);
static duk_ret_t js_Joystick_get_name          (duk_context* ctx);
static duk_ret_t js_Joystick_get_numAxes       (duk_context* ctx);
static duk_ret_t js_Joystick_get_numButtons    (duk_context* ctx);
static duk_ret_t js_Joystick_getPosition       (duk_context* ctx);
static duk_ret_t js_Joystick_isPressed         (duk_context* ctx);
static duk_ret_t js_Keyboard_get_Default       (duk_context* ctx);
static duk_ret_t js_Keyboard_get_capsLock      (duk_context* ctx);
static duk_ret_t js_Keyboard_get_numLock       (duk_context* ctx);
static duk_ret_t js_Keyboard_get_scrollLock    (duk_context* ctx);
static duk_ret_t js_Keyboard_clearQueue        (duk_context* ctx);
static duk_ret_t js_Keyboard_getChar           (duk_context* ctx);
static duk_ret_t js_Keyboard_getKey            (duk_context* ctx);
static duk_ret_t js_Keyboard_isPressed         (duk_context* ctx);
static duk_ret_t js_Mixer_get_Default          (duk_context* ctx);
static duk_ret_t js_new_Mixer                  (duk_context* ctx);
static duk_ret_t js_Mixer_finalize             (duk_context* ctx);
static duk_ret_t js_Mixer_get_volume           (duk_context* ctx);
static duk_ret_t js_Mixer_set_volume           (duk_context* ctx);
static duk_ret_t js_new_Model                  (duk_context* ctx);
static duk_ret_t js_Model_finalize             (duk_context* ctx);
static duk_ret_t js_Model_get_shader           (duk_context* ctx);
static duk_ret_t js_Model_get_transform        (duk_context* ctx);
static duk_ret_t js_Model_set_shader           (duk_context* ctx);
static duk_ret_t js_Model_set_transform        (duk_context* ctx);
static duk_ret_t js_Model_draw                 (duk_context* ctx);
static duk_ret_t js_Model_setBoolean           (duk_context* ctx);
static duk_ret_t js_Model_setColorVector       (duk_context* ctx);
static duk_ret_t js_Model_setFloat             (duk_context* ctx);
static duk_ret_t js_Model_setFloatArray        (duk_context* ctx);
static duk_ret_t js_Model_setFloatVector       (duk_context* ctx);
static duk_ret_t js_Model_setInt               (duk_context* ctx);
static duk_ret_t js_Model_setIntArray          (duk_context* ctx);
static duk_ret_t js_Model_setIntVector         (duk_context* ctx);
static duk_ret_t js_Model_setMatrix            (duk_context* ctx);
static duk_ret_t js_Mouse_get_Default          (duk_context* ctx);
static duk_ret_t js_Mouse_get_x                (duk_context* ctx);
static duk_ret_t js_Mouse_get_y                (duk_context* ctx);
static duk_ret_t js_Mouse_clearQueue           (duk_context* ctx);
static duk_ret_t js_Mouse_getEvent             (duk_context* ctx);
static duk_ret_t js_Mouse_isPressed            (duk_context* ctx);
static duk_ret_t js_RNG_fromSeed               (duk_context* ctx);
static duk_ret_t js_RNG_fromState              (duk_context* ctx);
static duk_ret_t js_new_RNG                    (duk_context* ctx);
static duk_ret_t js_RNG_finalize               (duk_context* ctx);
static duk_ret_t js_RNG_get_state              (duk_context* ctx);
static duk_ret_t js_RNG_set_state              (duk_context* ctx);
static duk_ret_t js_RNG_next                   (duk_context* ctx);
static duk_ret_t js_SSj_log                    (duk_context* ctx);
static duk_ret_t js_SSj_trace                  (duk_context* ctx);
static duk_ret_t js_new_Sample                 (duk_context* ctx);
static duk_ret_t js_Sample_finalize            (duk_context* ctx);
static duk_ret_t js_Sample_get_fileName        (duk_context* ctx);
static duk_ret_t js_Sample_play                (duk_context* ctx);
static duk_ret_t js_Sample_stopAll             (duk_context* ctx);
static duk_ret_t js_new_Server                 (duk_context* ctx);
static duk_ret_t js_Server_finalize            (duk_context* ctx);
static duk_ret_t js_Server_close               (duk_context* ctx);
static duk_ret_t js_Server_accept              (duk_context* ctx);
static duk_ret_t js_Shader_get_Default         (duk_context* ctx);
static duk_ret_t js_new_Shader                 (duk_context* ctx);
static duk_ret_t js_Shader_finalize            (duk_context* ctx);
static duk_ret_t js_new_Shape                  (duk_context* ctx);
static duk_ret_t js_Shape_finalize             (duk_context* ctx);
static duk_ret_t js_Shape_get_indexList        (duk_context* ctx);
static duk_ret_t js_Shape_get_texture          (duk_context* ctx);
static duk_ret_t js_Shape_get_vertexList       (duk_context* ctx);
static duk_ret_t js_Shape_set_indexList        (duk_context* ctx);
static duk_ret_t js_Shape_set_texture          (duk_context* ctx);
static duk_ret_t js_Shape_set_vertexList       (duk_context* ctx);
static duk_ret_t js_Shape_draw                 (duk_context* ctx);
static duk_ret_t js_new_Socket                 (duk_context* ctx);
static duk_ret_t js_Socket_finalize            (duk_context* ctx);
static duk_ret_t js_Socket_get_bytesPending    (duk_context* ctx);
static duk_ret_t js_Socket_get_connected       (duk_context* ctx);
static duk_ret_t js_Socket_get_remoteAddress   (duk_context* ctx);
static duk_ret_t js_Socket_get_remotePort      (duk_context* ctx);
static duk_ret_t js_Socket_close               (duk_context* ctx);
static duk_ret_t js_Socket_connectTo           (duk_context* ctx);
static duk_ret_t js_Socket_read                (duk_context* ctx);
static duk_ret_t js_Socket_write               (duk_context* ctx);
static duk_ret_t js_new_Sound                  (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_get_fileName         (duk_context* ctx);
static duk_ret_t js_Sound_get_length           (duk_context* ctx);
static duk_ret_t js_Sound_get_pan              (duk_context* ctx);
static duk_ret_t js_Sound_get_playing          (duk_context* ctx);
static duk_ret_t js_Sound_get_position         (duk_context* ctx);
static duk_ret_t js_Sound_get_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_get_speed            (duk_context* ctx);
static duk_ret_t js_Sound_get_volume           (duk_context* ctx);
static duk_ret_t js_Sound_set_pan              (duk_context* ctx);
static duk_ret_t js_Sound_set_position         (duk_context* ctx);
static duk_ret_t js_Sound_set_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_set_speed            (duk_context* ctx);
static duk_ret_t js_Sound_set_volume           (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_new_SoundStream            (duk_context* ctx);
static duk_ret_t js_SoundStream_finalize       (duk_context* ctx);
static duk_ret_t js_SoundStream_get_bufferSize (duk_context* ctx);
static duk_ret_t js_SoundStream_buffer         (duk_context* ctx);
static duk_ret_t js_SoundStream_play           (duk_context* ctx);
static duk_ret_t js_SoundStream_pause          (duk_context* ctx);
static duk_ret_t js_SoundStream_stop           (duk_context* ctx);
static duk_ret_t js_new_Surface                (duk_context* ctx);
static duk_ret_t js_Surface_finalize           (duk_context* ctx);
static duk_ret_t js_Surface_get_height         (duk_context* ctx);
static duk_ret_t js_Surface_get_transform      (duk_context* ctx);
static duk_ret_t js_Surface_get_width          (duk_context* ctx);
static duk_ret_t js_Surface_set_transform      (duk_context* ctx);
static duk_ret_t js_Surface_clipTo             (duk_context* ctx);
static duk_ret_t js_Surface_toTexture          (duk_context* ctx);
static duk_ret_t js_new_Texture                (duk_context* ctx);
static duk_ret_t js_Texture_finalize           (duk_context* ctx);
static duk_ret_t js_Texture_get_fileName       (duk_context* ctx);
static duk_ret_t js_Texture_get_height         (duk_context* ctx);
static duk_ret_t js_Texture_get_width          (duk_context* ctx);
static duk_ret_t js_new_Transform              (duk_context* ctx);
static duk_ret_t js_Transform_finalize         (duk_context* ctx);
static duk_ret_t js_Transform_get_matrix       (duk_context* ctx);
static duk_ret_t js_Transform_set_matrix       (duk_context* ctx);
static duk_ret_t js_Transform_compose          (duk_context* ctx);
static duk_ret_t js_Transform_identity         (duk_context* ctx);
static duk_ret_t js_Transform_project2D        (duk_context* ctx);
static duk_ret_t js_Transform_project3D        (duk_context* ctx);
static duk_ret_t js_Transform_rotate           (duk_context* ctx);
static duk_ret_t js_Transform_scale            (duk_context* ctx);
static duk_ret_t js_Transform_translate        (duk_context* ctx);
static duk_ret_t js_new_VertexList             (duk_context* ctx);
static duk_ret_t js_VertexList_finalize        (duk_context* ctx);

static void      duk_pegasus_push_color     (duk_context* ctx, color_t color);
static void      duk_pegasus_push_job_token (duk_context* ctx, int64_t token);
static void      duk_pegasus_push_require   (duk_context* ctx, const char* module_id);
static color_t   duk_pegasus_require_color  (duk_context* ctx, duk_idx_t index);
static script_t* duk_pegasus_require_script (duk_context* ctx, duk_idx_t index);
static duk_ret_t duk_safe_event_loop        (duk_context* ctx);
static path_t*   find_module                (const char* id, const char* origin, const char* sys_origin);
static void      load_joysticks             (duk_context* ctx);
static path_t*   load_package_json          (const char* filename);

static mixer_t* s_def_mixer;
static int      s_framerate = 60;

void
initialize_pegasus_api(duk_context* ctx)
{
	const struct x11_color* p;

	console_log(1, "initializing Sphere v%d L%d API", API_VERSION, API_LEVEL);

	s_def_mixer = mixer_new(44100, 16, 2);

	// initialize CommonJS cache and global require()
	duk_push_global_stash(ctx);
	dukrub_push_bare_object(ctx);
	duk_put_prop_string(ctx, -2, "moduleCache");
	duk_pop(ctx);

	duk_push_global_object(ctx);
	duk_push_string(ctx, "require");
	duk_pegasus_push_require(ctx, NULL);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);

	// initialize the Sphere v2 API
	api_define_static_prop(ctx, "Sphere", "APILevel", js_Sphere_get_APILevel, NULL);
	api_define_static_prop(ctx, "Sphere", "Game", js_Sphere_get_Game, NULL);
	api_define_static_prop(ctx, "Sphere", "Platform", js_Sphere_get_Platform, NULL);
	api_define_static_prop(ctx, "Sphere", "Version", js_Sphere_get_Version, NULL);
	api_define_function(ctx, "Sphere", "abort", js_Sphere_abort);
	api_define_function(ctx, "Sphere", "exit", js_Sphere_exit);
	api_define_function(ctx, "Sphere", "restart", js_Sphere_restart);
	api_define_function(ctx, "Sphere", "run", js_Sphere_run);
	api_define_function(ctx, "Sphere", "sleep", js_Sphere_sleep);
	api_define_class(ctx, "SoundStream", js_new_SoundStream, js_SoundStream_finalize);
	api_define_property(ctx, "SoundStream", "bufferSize", js_SoundStream_get_bufferSize, NULL);
	api_define_method(ctx, "SoundStream", "buffer", js_SoundStream_buffer);
	api_define_method(ctx, "SoundStream", "pause", js_SoundStream_pause);
	api_define_method(ctx, "SoundStream", "play", js_SoundStream_play);
	api_define_method(ctx, "SoundStream", "stop", js_SoundStream_stop);
	api_define_class(ctx, "Color", js_new_Color, NULL);
	api_define_function(ctx, "Color", "is", js_Color_is);
	api_define_function(ctx, "Color", "mix", js_Color_mix);
	api_define_function(ctx, "Color", "of", js_Color_of);
	api_define_property(ctx, "Color", "name", js_Color_get_name, NULL);
	api_define_method(ctx, "Color", "clone", js_Color_clone);
	api_define_method(ctx, "Color", "fade", js_Color_fade);
	api_define_function(ctx, "Dispatch", "cancel", js_Dispatch_cancel);
	api_define_function(ctx, "Dispatch", "cancelAll", js_Dispatch_cancelAll);
	api_define_function(ctx, "Dispatch", "later", js_Dispatch_later);
	api_define_function(ctx, "Dispatch", "now", js_Dispatch_now);
	api_define_function(ctx, "Dispatch", "onRender", js_Dispatch_onRender);
	api_define_function(ctx, "Dispatch", "onUpdate", js_Dispatch_onUpdate);
	api_define_class(ctx, "FileStream", js_new_FileStream, js_FileStream_finalize);
	api_define_method(ctx, "FileStream", "dispose", js_FileStream_dispose);
	api_define_property(ctx, "FileStream", "fileName", js_FileStream_get_fileName, NULL);
	api_define_property(ctx, "FileStream", "fileSize", js_FileStream_get_fileSize, NULL);
	api_define_property(ctx, "FileStream", "position", js_FileStream_get_position, js_FileStream_set_position);
	api_define_method(ctx, "FileStream", "read", js_FileStream_read);
	api_define_method(ctx, "FileStream", "write", js_FileStream_write);
	api_define_class(ctx, "Font", js_new_Font, js_Font_finalize);
	api_define_static_prop(ctx, "Font", "Default", js_Font_get_Default, NULL);
	api_define_property(ctx, "Font", "fileName", js_Font_get_fileName, NULL);
	api_define_property(ctx, "Font", "height", js_Font_get_height, NULL);
	api_define_method(ctx, "Font", "drawText", js_Font_drawText);
	api_define_method(ctx, "Font", "getTextSize", js_Font_getTextSize);
	api_define_method(ctx, "Font", "wordWrap", js_Font_wordWrap);
	api_define_function(ctx, "FS", "createDirectory", js_FS_createDirectory);
	api_define_function(ctx, "FS", "deleteFile", js_FS_deleteFile);
	api_define_function(ctx, "FS", "exists", js_FS_exists);
	api_define_function(ctx, "FS", "readFile", js_FS_readFile);
	api_define_function(ctx, "FS", "removeDirectory", js_FS_removeDirectory);
	api_define_function(ctx, "FS", "rename", js_FS_rename);
	api_define_function(ctx, "FS", "resolve", js_FS_resolve);
	api_define_function(ctx, "FS", "writeFile", js_FS_writeFile);
	api_define_class(ctx, "IndexList", js_new_IndexList, js_IndexList_finalize);
	api_define_class(ctx, "JobToken", NULL, js_JobToken_finalize);
	api_define_class(ctx, "Joystick", NULL, js_Joystick_finalize);
	api_define_static_prop(ctx, "Joystick", "Null", js_Joystick_get_Null, NULL);
	api_define_function(ctx, "Joystick", "getDevices", js_Joystick_getDevices);
	api_define_property(ctx, "Joystick", "name", js_Joystick_get_name, NULL);
	api_define_property(ctx, "Joystick", "numAxes", js_Joystick_get_numAxes, NULL);
	api_define_property(ctx, "Joystick", "numButtons", js_Joystick_get_numButtons, NULL);
	api_define_method(ctx, "Joystick", "getPosition", js_Joystick_getPosition);
	api_define_method(ctx, "Joystick", "isPressed", js_Joystick_isPressed);
	api_define_class(ctx, "Keyboard", NULL, NULL);
	api_define_static_prop(ctx, "Keyboard", "Default", js_Keyboard_get_Default, NULL);
	api_define_property(ctx, "Keyboard", "capsLock", js_Keyboard_get_capsLock, NULL);
	api_define_property(ctx, "Keyboard", "numLock", js_Keyboard_get_numLock, NULL);
	api_define_property(ctx, "Keyboard", "scrollLock", js_Keyboard_get_scrollLock, NULL);
	api_define_method(ctx, "Keyboard", "clearQueue", js_Keyboard_clearQueue);
	api_define_method(ctx, "Keyboard", "getChar", js_Keyboard_getChar);
	api_define_method(ctx, "Keyboard", "getKey", js_Keyboard_getKey);
	api_define_method(ctx, "Keyboard", "isPressed", js_Keyboard_isPressed);
	api_define_class(ctx, "Mixer", js_new_Mixer, js_Mixer_finalize);
	api_define_static_prop(ctx, "Mixer", "Default", js_Mixer_get_Default, NULL);
	api_define_property(ctx, "Mixer", "volume", js_Mixer_get_volume, js_Mixer_set_volume);
	api_define_class(ctx, "Model", js_new_Model, js_Model_finalize);
	api_define_property(ctx, "Model", "shader", js_Model_get_shader, js_Model_set_shader);
	api_define_property(ctx, "Model", "transform", js_Model_get_transform, js_Model_set_transform);
	api_define_method(ctx, "Model", "draw", js_Model_draw);
	api_define_method(ctx, "Model", "setBoolean", js_Model_setBoolean);
	api_define_method(ctx, "Model", "setColorVector", js_Model_setColorVector);
	api_define_method(ctx, "Model", "setFloat", js_Model_setFloat);
	api_define_method(ctx, "Model", "setFloatArray", js_Model_setFloatArray);
	api_define_method(ctx, "Model", "setFloatVector", js_Model_setFloatVector);
	api_define_method(ctx, "Model", "setInt", js_Model_setInt);
	api_define_method(ctx, "Model", "setIntArray", js_Model_setIntArray);
	api_define_method(ctx, "Model", "setIntVector", js_Model_setIntVector);
	api_define_method(ctx, "Model", "setMatrix", js_Model_setMatrix);
	api_define_class(ctx, "Mouse", NULL, NULL);
	api_define_static_prop(ctx, "Mouse", "Default", js_Mouse_get_Default, NULL);
	api_define_property(ctx, "Mouse", "x", js_Mouse_get_x, NULL);
	api_define_property(ctx, "Mouse", "y", js_Mouse_get_y, NULL);
	api_define_method(ctx, "Mouse", "clearQueue", js_Mouse_clearQueue);
	api_define_method(ctx, "Mouse", "getEvent", js_Mouse_getEvent);
	api_define_method(ctx, "Mouse", "isPressed", js_Mouse_isPressed);
	api_define_class(ctx, "RNG", js_new_RNG, js_RNG_finalize);
	api_define_function(ctx, "RNG", "fromSeed", js_RNG_fromSeed);
	api_define_function(ctx, "RNG", "fromState", js_RNG_fromState);
	api_define_property(ctx, "RNG", "state", js_RNG_get_state, js_RNG_set_state);
	api_define_method(ctx, "RNG", "next", js_RNG_next);
	api_define_function(ctx, "SSj", "log", js_SSj_log);
	api_define_function(ctx, "SSj", "trace", js_SSj_trace);
	api_define_class(ctx, "Sample", js_new_Sample, js_Sample_finalize);
	api_define_property(ctx, "Sample", "fileName", js_Sample_get_fileName, NULL);
	api_define_method(ctx, "Sample", "play", js_Sample_play);
	api_define_method(ctx, "Sample", "stopAll", js_Sample_stopAll);
	api_define_class(ctx, "Server", js_new_Server, js_Server_finalize);
	api_define_method(ctx, "Server", "close", js_Server_close);
	api_define_method(ctx, "Server", "accept", js_Server_accept);
	api_define_class(ctx, "Shader", js_new_Shader, js_Shader_finalize);
	api_define_static_prop(ctx, "Shader", "Default", js_Shader_get_Default, NULL);
	api_define_class(ctx, "Shape", js_new_Shape, js_Shape_finalize);
	api_define_property(ctx, "Shape", "indexList", js_Shape_get_indexList, js_Shape_set_indexList);
	api_define_property(ctx, "Shape", "texture", js_Shape_get_texture, js_Shape_set_texture);
	api_define_property(ctx, "Shape", "vertexList", js_Shape_get_vertexList, js_Shape_set_vertexList);
	api_define_method(ctx, "Shape", "draw", js_Shape_draw);
	api_define_class(ctx, "Socket", js_new_Socket, js_Socket_finalize);
	api_define_property(ctx, "Socket", "bytesPending", js_Socket_get_bytesPending, NULL);
	api_define_property(ctx, "Socket", "connected", js_Socket_get_connected, NULL);
	api_define_property(ctx, "Socket", "remoteAddress", js_Socket_get_remoteAddress, NULL);
	api_define_property(ctx, "Socket", "remotePort", js_Socket_get_remotePort, NULL);
	api_define_method(ctx, "Socket", "close", js_Socket_close);
	api_define_method(ctx, "Socket", "connectTo", js_Socket_connectTo);
	api_define_method(ctx, "Socket", "read", js_Socket_read);
	api_define_method(ctx, "Socket", "write", js_Socket_write);
	api_define_class(ctx, "Sound", js_new_Sound, js_Sound_finalize);
	api_define_property(ctx, "Sound", "fileName", js_Sound_get_fileName, NULL);
	api_define_property(ctx, "Sound", "length", js_Sound_get_length, NULL);
	api_define_property(ctx, "Sound", "pan", js_Sound_get_pan, js_Sound_set_pan);
	api_define_property(ctx, "Sound", "playing", js_Sound_get_playing, NULL);
	api_define_property(ctx, "Sound", "position", js_Sound_get_position, js_Sound_set_position);
	api_define_property(ctx, "Sound", "repeat", js_Sound_get_repeat, js_Sound_set_repeat);
	api_define_property(ctx, "Sound", "speed", js_Sound_get_speed, js_Sound_set_speed);
	api_define_property(ctx, "Sound", "volume", js_Sound_get_volume, js_Sound_set_volume);
	api_define_method(ctx, "Sound", "pause", js_Sound_pause);
	api_define_method(ctx, "Sound", "play", js_Sound_play);
	api_define_method(ctx, "Sound", "stop", js_Sound_stop);
	api_define_class(ctx, "Surface", js_new_Surface, js_Surface_finalize);
	api_define_property(ctx, "Surface", "height", js_Surface_get_height, NULL);
	api_define_property(ctx, "Surface", "transform", js_Surface_get_transform, js_Surface_set_transform);
	api_define_property(ctx, "Surface", "width", js_Surface_get_width, NULL);
	api_define_method(ctx, "Surface", "clipTo", js_Surface_clipTo);
	api_define_method(ctx, "Surface", "toTexture", js_Surface_toTexture);
	api_define_class(ctx, "Texture", js_new_Texture, js_Texture_finalize);
	api_define_property(ctx, "Texture", "fileName", js_Texture_get_fileName, NULL);
	api_define_property(ctx, "Texture", "height", js_Texture_get_height, NULL);
	api_define_property(ctx, "Texture", "width", js_Texture_get_width, NULL);
	api_define_class(ctx, "Transform", js_new_Transform, js_Transform_finalize);
	api_define_property(ctx, "Transform", "matrix", js_Transform_get_matrix, NULL);
	api_define_method(ctx, "Transform", "compose", js_Transform_compose);
	api_define_method(ctx, "Transform", "identity", js_Transform_identity);
	api_define_method(ctx, "Transform", "project2D", js_Transform_project2D);
	api_define_method(ctx, "Transform", "project3D", js_Transform_project3D);
	api_define_method(ctx, "Transform", "rotate", js_Transform_rotate);
	api_define_method(ctx, "Transform", "scale", js_Transform_scale);
	api_define_method(ctx, "Transform", "translate", js_Transform_translate);
	api_define_class(ctx, "VertexList", js_new_VertexList, js_VertexList_finalize);

	api_define_object(ctx, NULL, "screen", "Surface", screen_backbuffer(g_screen));
	api_define_static_prop(ctx, "screen", "frameRate", js_screen_get_frameRate, js_screen_set_frameRate);
	api_define_static_prop(ctx, "screen", "frameSkip", js_screen_get_frameSkip, js_screen_set_frameSkip);
	api_define_static_prop(ctx, "screen", "fullScreen", js_screen_get_fullScreen, js_screen_set_fullScreen);
	api_define_function(ctx, "screen", "flip", js_screen_flip);
	api_define_function(ctx, "screen", "now", js_screen_now);
	api_define_function(ctx, "screen", "resize", js_screen_resize);

	api_define_const(ctx, "FileOp", "Read", FILE_OP_READ);
	api_define_const(ctx, "FileOp", "Write", FILE_OP_WRITE);
	api_define_const(ctx, "FileOp", "Update", FILE_OP_UPDATE);
	api_define_const(ctx, "Key", "Alt", ALLEGRO_KEY_ALT);
	api_define_const(ctx, "Key", "AltGr", ALLEGRO_KEY_ALTGR);
	api_define_const(ctx, "Key", "Apostrophe", ALLEGRO_KEY_QUOTE);
	api_define_const(ctx, "Key", "Backslash", ALLEGRO_KEY_BACKSLASH);
	api_define_const(ctx, "Key", "Backspace", ALLEGRO_KEY_BACKSPACE);
	api_define_const(ctx, "Key", "CapsLock", ALLEGRO_KEY_CAPSLOCK);
	api_define_const(ctx, "Key", "CloseBrace", ALLEGRO_KEY_CLOSEBRACE);
	api_define_const(ctx, "Key", "Comma", ALLEGRO_KEY_COMMA);
	api_define_const(ctx, "Key", "Delete", ALLEGRO_KEY_DELETE);
	api_define_const(ctx, "Key", "Down", ALLEGRO_KEY_DOWN);
	api_define_const(ctx, "Key", "End", ALLEGRO_KEY_END);
	api_define_const(ctx, "Key", "Enter", ALLEGRO_KEY_ENTER);
	api_define_const(ctx, "Key", "Equals", ALLEGRO_KEY_EQUALS);
	api_define_const(ctx, "Key", "Escape", ALLEGRO_KEY_ESCAPE);
	api_define_const(ctx, "Key", "F1", ALLEGRO_KEY_F1);
	api_define_const(ctx, "Key", "F2", ALLEGRO_KEY_F2);
	api_define_const(ctx, "Key", "F3", ALLEGRO_KEY_F3);
	api_define_const(ctx, "Key", "F4", ALLEGRO_KEY_F4);
	api_define_const(ctx, "Key", "F5", ALLEGRO_KEY_F5);
	api_define_const(ctx, "Key", "F6", ALLEGRO_KEY_F6);
	api_define_const(ctx, "Key", "F7", ALLEGRO_KEY_F7);
	api_define_const(ctx, "Key", "F8", ALLEGRO_KEY_F8);
	api_define_const(ctx, "Key", "F9", ALLEGRO_KEY_F9);
	api_define_const(ctx, "Key", "F10", ALLEGRO_KEY_F10);
	api_define_const(ctx, "Key", "F11", ALLEGRO_KEY_F11);
	api_define_const(ctx, "Key", "F12", ALLEGRO_KEY_F12);
	api_define_const(ctx, "Key", "Home", ALLEGRO_KEY_HOME);
	api_define_const(ctx, "Key", "Hyphen", ALLEGRO_KEY_MINUS);
	api_define_const(ctx, "Key", "Insert", ALLEGRO_KEY_INSERT);
	api_define_const(ctx, "Key", "LCtrl", ALLEGRO_KEY_LCTRL);
	api_define_const(ctx, "Key", "LShift", ALLEGRO_KEY_LSHIFT);
	api_define_const(ctx, "Key", "Left", ALLEGRO_KEY_LEFT);
	api_define_const(ctx, "Key", "NumLock", ALLEGRO_KEY_NUMLOCK);
	api_define_const(ctx, "Key", "OpenBrace", ALLEGRO_KEY_OPENBRACE);
	api_define_const(ctx, "Key", "PageDown", ALLEGRO_KEY_PGDN);
	api_define_const(ctx, "Key", "PageUp", ALLEGRO_KEY_PGUP);
	api_define_const(ctx, "Key", "Period", ALLEGRO_KEY_FULLSTOP);
	api_define_const(ctx, "Key", "RCtrl", ALLEGRO_KEY_RCTRL);
	api_define_const(ctx, "Key", "RShift", ALLEGRO_KEY_RSHIFT);
	api_define_const(ctx, "Key", "Right", ALLEGRO_KEY_RIGHT);
	api_define_const(ctx, "Key", "ScrollLock", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const(ctx, "Key", "Semicolon", ALLEGRO_KEY_SEMICOLON);
	api_define_const(ctx, "Key", "Slash", ALLEGRO_KEY_SLASH);
	api_define_const(ctx, "Key", "Space", ALLEGRO_KEY_SPACE);
	api_define_const(ctx, "Key", "Tab", ALLEGRO_KEY_TAB);
	api_define_const(ctx, "Key", "Tilde", ALLEGRO_KEY_TILDE);
	api_define_const(ctx, "Key", "Up", ALLEGRO_KEY_UP);
	api_define_const(ctx, "Key", "A", ALLEGRO_KEY_A);
	api_define_const(ctx, "Key", "B", ALLEGRO_KEY_B);
	api_define_const(ctx, "Key", "C", ALLEGRO_KEY_C);
	api_define_const(ctx, "Key", "D", ALLEGRO_KEY_D);
	api_define_const(ctx, "Key", "E", ALLEGRO_KEY_E);
	api_define_const(ctx, "Key", "F", ALLEGRO_KEY_F);
	api_define_const(ctx, "Key", "G", ALLEGRO_KEY_G);
	api_define_const(ctx, "Key", "H", ALLEGRO_KEY_H);
	api_define_const(ctx, "Key", "I", ALLEGRO_KEY_I);
	api_define_const(ctx, "Key", "J", ALLEGRO_KEY_J);
	api_define_const(ctx, "Key", "K", ALLEGRO_KEY_K);
	api_define_const(ctx, "Key", "L", ALLEGRO_KEY_L);
	api_define_const(ctx, "Key", "M", ALLEGRO_KEY_M);
	api_define_const(ctx, "Key", "N", ALLEGRO_KEY_N);
	api_define_const(ctx, "Key", "O", ALLEGRO_KEY_O);
	api_define_const(ctx, "Key", "P", ALLEGRO_KEY_P);
	api_define_const(ctx, "Key", "Q", ALLEGRO_KEY_Q);
	api_define_const(ctx, "Key", "R", ALLEGRO_KEY_R);
	api_define_const(ctx, "Key", "S", ALLEGRO_KEY_S);
	api_define_const(ctx, "Key", "T", ALLEGRO_KEY_T);
	api_define_const(ctx, "Key", "U", ALLEGRO_KEY_U);
	api_define_const(ctx, "Key", "V", ALLEGRO_KEY_V);
	api_define_const(ctx, "Key", "W", ALLEGRO_KEY_W);
	api_define_const(ctx, "Key", "X", ALLEGRO_KEY_X);
	api_define_const(ctx, "Key", "Y", ALLEGRO_KEY_Y);
	api_define_const(ctx, "Key", "Z", ALLEGRO_KEY_Z);
	api_define_const(ctx, "Key", "D1", ALLEGRO_KEY_1);
	api_define_const(ctx, "Key", "D2", ALLEGRO_KEY_2);
	api_define_const(ctx, "Key", "D3", ALLEGRO_KEY_3);
	api_define_const(ctx, "Key", "D4", ALLEGRO_KEY_4);
	api_define_const(ctx, "Key", "D5", ALLEGRO_KEY_5);
	api_define_const(ctx, "Key", "D6", ALLEGRO_KEY_6);
	api_define_const(ctx, "Key", "D7", ALLEGRO_KEY_7);
	api_define_const(ctx, "Key", "D8", ALLEGRO_KEY_8);
	api_define_const(ctx, "Key", "D9", ALLEGRO_KEY_9);
	api_define_const(ctx, "Key", "D0", ALLEGRO_KEY_0);
	api_define_const(ctx, "Key", "NumPad1", ALLEGRO_KEY_PAD_1);
	api_define_const(ctx, "Key", "NumPad2", ALLEGRO_KEY_PAD_2);
	api_define_const(ctx, "Key", "NumPad3", ALLEGRO_KEY_PAD_3);
	api_define_const(ctx, "Key", "NumPad4", ALLEGRO_KEY_PAD_4);
	api_define_const(ctx, "Key", "NumPad5", ALLEGRO_KEY_PAD_5);
	api_define_const(ctx, "Key", "NumPad6", ALLEGRO_KEY_PAD_6);
	api_define_const(ctx, "Key", "NumPad7", ALLEGRO_KEY_PAD_7);
	api_define_const(ctx, "Key", "NumPad8", ALLEGRO_KEY_PAD_8);
	api_define_const(ctx, "Key", "NumPad9", ALLEGRO_KEY_PAD_9);
	api_define_const(ctx, "Key", "NumPad0", ALLEGRO_KEY_PAD_0);
	api_define_const(ctx, "Key", "NumPadEnter", ALLEGRO_KEY_PAD_ENTER);
	api_define_const(ctx, "Key", "Add", ALLEGRO_KEY_PAD_PLUS);
	api_define_const(ctx, "Key", "Decimal", ALLEGRO_KEY_PAD_DELETE);
	api_define_const(ctx, "Key", "Divide", ALLEGRO_KEY_PAD_SLASH);
	api_define_const(ctx, "Key", "Multiply", ALLEGRO_KEY_PAD_ASTERISK);
	api_define_const(ctx, "Key", "Subtract", ALLEGRO_KEY_PAD_MINUS);
	api_define_const(ctx, "MouseKey", "Left", MOUSE_KEY_LEFT);
	api_define_const(ctx, "MouseKey", "Right", MOUSE_KEY_RIGHT);
	api_define_const(ctx, "MouseKey", "Middle", MOUSE_KEY_MIDDLE);
	api_define_const(ctx, "MouseKey", "WheelUp", MOUSE_KEY_WHEEL_UP);
	api_define_const(ctx, "MouseKey", "WheelDown", MOUSE_KEY_WHEEL_DOWN);
	api_define_const(ctx, "ShapeType", "Fan", SHAPE_TRI_FAN);
	api_define_const(ctx, "ShapeType", "Lines", SHAPE_LINES);
	api_define_const(ctx, "ShapeType", "LineLoop", SHAPE_LINE_LOOP);
	api_define_const(ctx, "ShapeType", "LineStrip", SHAPE_LINE_STRIP);
	api_define_const(ctx, "ShapeType", "Points", SHAPE_POINTS);
	api_define_const(ctx, "ShapeType", "Triangles", SHAPE_TRIANGLES);
	api_define_const(ctx, "ShapeType", "TriStrip", SHAPE_TRI_STRIP);

	// pre-create joystick objects for Joystick.getDevices().  this ensures that
	// multiple requests for the device list return the same Joystick object(s).
	load_joysticks(ctx);

	// register predefined X11 colors
	duk_get_global_string(ctx, "Color");
	p = COLORS;
	while (p->name != NULL) {
		duk_push_string(ctx, p->name);
		duk_push_c_function(ctx, js_Color_get_Color, DUK_VARARGS);
		duk_push_int(ctx, (int)(p - COLORS));
		duk_put_prop_string(ctx, -2, "\xFF" "index");
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER
			| DUK_DEFPROP_CLEAR_ENUMERABLE
			| DUK_DEFPROP_SET_CONFIGURABLE);
		++p;
	}
	duk_pop(ctx);
}

bool
pegasus_run(void)
{
	if (dukrub_safe_call(g_duk, duk_safe_event_loop, 0, 1) == 0) {
		duk_pop(g_duk);  // don't need return value
		return true;
	}
	else {
		// leave the error for the caller, don't pop it off
		return false;
	}
}

duk_bool_t
duk_pegasus_eval_module(duk_context* ctx, const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  Duktape's stack-based API is powerful, but gets
	// very messy very quickly when dealing with object properties.  I tried to add
	// comments to illuminate what's going on, but it's still likely to be confusing for
	// someone not familiar with Duktape code.  proceed with caution.

	// notes:
	//     - the final value of `module.exports` is left on top of the Duktape value stack.
	//     - `module.id` is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be in canonical form.
	//     - this is a protected call.  if the module being loaded throws, the error will be
	//       caught and left on top of the stack for the caller to deal with.

	lstring_t* code_string;
	path_t*    dir_path;
	path_t*    file_path;
	size_t     source_size;
	char*      source;

	file_path = path_new(filename);
	dir_path = path_strip(path_dup(file_path));

	// is the requested module already in the cache?
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	if (duk_get_prop_string(ctx, -1, filename)) {
		duk_remove(ctx, -2);
		duk_remove(ctx, -2);
		goto have_module;
	}
	else {
		duk_pop_3(ctx);
	}

	console_log(1, "initializing JS module `%s`", filename);

	source = sfs_fslurp(g_fs, filename, NULL, &source_size);
	code_string = lstr_from_cp1252(source, source_size);
	free(source);

	// construct a module object for the new module
	duk_push_object(ctx);  // module object
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "exports");  // module.exports = {}
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "filename");  // module.filename
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "id");  // module.id
	duk_push_false(ctx);
	duk_put_prop_string(ctx, -2, "loaded");  // module.loaded = false
	duk_pegasus_push_require(ctx, filename);
	duk_put_prop_string(ctx, -2, "require");  // module.require

	// cache the module object in advance
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_dup(ctx, -3);
	duk_put_prop_string(ctx, -2, filename);
	duk_pop_2(ctx);

	if (strcmp(path_extension(file_path), ".json") == 0) {
		// JSON file, decode to JavaScript object
		duk_push_lstring_t(ctx, code_string);
		lstr_free(code_string);
		if (duk_json_pdecode(ctx) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_put_prop_string(ctx, -2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the simplest way to
		// implement CommonJS semantics and matches the behavior of Node.js.
		duk_push_string(ctx, "(function(exports, require, module, __filename, __dirname) {");
		duk_push_string(ctx, strncmp(lstr_cstr(code_string), "#!", 2) == 0 ? "//" : "");  // shebang
		duk_push_lstring_t(ctx, code_string);
		duk_push_string(ctx, "\n})");
		duk_concat(ctx, 4);
		duk_push_string(ctx, debugger_source_name(filename));
		if (duk_pcompile(ctx, DUK_COMPILE_EVAL) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_call(ctx, 0);
		duk_push_string(ctx, "name");
		duk_push_string(ctx, "main");
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_FORCE);
		lstr_free(code_string);

		// go, go, go!
		duk_dup(ctx, -2);                           // this = module
		duk_get_prop_string(ctx, -3, "exports");    // exports
		duk_get_prop_string(ctx, -4, "require");    // require
		duk_dup(ctx, -5);                           // module
		duk_push_string(ctx, filename);             // __filename
		duk_push_string(ctx, path_cstr(dir_path));  // __dirname
		if (duk_pcall_method(ctx, 5) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_pop(ctx);
	}

	// module executed successfully, set `module.loaded` to true
	duk_push_true(ctx);
	duk_put_prop_string(ctx, -2, "loaded");

have_module:
	// `module` is on the stack, we need `module.exports`
	duk_get_prop_string(ctx, -1, "exports");
	duk_remove(ctx, -2);
	return 1;

on_error:
	// note: it's assumed that at this point, the only things left in our portion of the
	//       Duktape stack are the module object and the thrown error.
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_del_prop_string(ctx, -1, filename);
	duk_pop_2(ctx);
	duk_remove(ctx, -2);  // leave the error on the stack
	return 0;
}

static void
duk_pegasus_push_color(duk_context* ctx, color_t color)
{
	duk_get_global_string(ctx, "Color");
	duk_push_number(ctx, color.r / 255.0);
	duk_push_number(ctx, color.g / 255.0);
	duk_push_number(ctx, color.b / 255.0);
	duk_push_number(ctx, color.a / 255.0);
	duk_new(ctx, 4);
}

static void
duk_pegasus_push_job_token(duk_context* ctx, int64_t token)
{
	int64_t* ptr;

	ptr = malloc(sizeof(int64_t));
	*ptr = token;
	duk_push_class_obj(ctx, "JobToken", ptr);
}

static void
duk_pegasus_push_require(duk_context* ctx, const char* module_id)
{
	duk_push_c_function(ctx, js_require, 1);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, "require");
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.name
	duk_push_string(ctx, "cache");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_remove(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.cache
	if (module_id != NULL) {
		duk_push_string(ctx, "id");
		duk_push_string(ctx, module_id);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.id
	}
}

static color_t
duk_pegasus_require_color(duk_context* ctx, duk_idx_t index)
{
	float a;
	float b;
	float g;
	float r;

	index = duk_require_normalize_index(ctx, index);
	duk_require_class_obj(ctx, index, "Color");
	duk_get_prop_string(ctx, index, "r");
	duk_get_prop_string(ctx, index, "g");
	duk_get_prop_string(ctx, index, "b");
	duk_get_prop_string(ctx, index, "a");
	a = fmin(fmax(duk_get_number(ctx, -1) * 255, 0), 255);
	b = fmin(fmax(duk_get_number(ctx, -2) * 255, 0), 255);
	g = fmin(fmax(duk_get_number(ctx, -3) * 255, 0), 255);
	r = fmin(fmax(duk_get_number(ctx, -4) * 255, 0), 255);
	duk_pop_n(ctx, 4);
	return color_new(r, g, b, a);
}

static script_t*
duk_pegasus_require_script(duk_context* ctx, duk_idx_t index)
{
	return script_new_func(ctx, index);
}

static duk_ret_t
duk_safe_event_loop(duk_context* ctx)
{
	while (async_busy()) {
		screen_flip(g_screen, s_framerate);
		image_set_scissor(screen_backbuffer(g_screen), new_rect(0, 0, g_res_x, g_res_y));
	}
	return 0;
}

static path_t*
find_module(const char* id, const char* origin, const char* sys_origin)
{
	const char* const filenames[] =
	{
		"%s",
		"%s.js",
		"%s.json",
		"%s/package.json",
		"%s/index.js",
		"%s/index.json",
	};

	path_t*   origin_path;
	char*     filename;
	path_t*   main_path;
	path_t*   path;

	int i;

	if (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0)
		// resolve module relative to calling module
		origin_path = path_new(origin != NULL ? origin : "./");
	else
		// resolve module from designated module repository
		origin_path = path_new(sys_origin);

	for (i = 0; i < (int)(sizeof(filenames) / sizeof(filenames[0])); ++i) {
		filename = strnewf(filenames[i], id);
		if (strncmp(id, "@/", 2) == 0 || strncmp(id, "~/", 2) == 0 || strncmp(id, "#/", 2) == 0)
			path = path_new("./");
		else
			path = path_dup(origin_path);
		path_strip(path);
		path_append(path, filename);
		path_collapse(path, true);
		free(filename);
		if (sfs_fexist(g_fs, path_cstr(path), NULL)) {
			if (strcmp(path_filename(path), "package.json") != 0)
				return path;
			else {
				if (!(main_path = load_package_json(path_cstr(path))))
					goto next_filename;
				if (sfs_fexist(g_fs, path_cstr(main_path), NULL)) {
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
load_joysticks(duk_context* ctx)
{
	int* device;
	int  num_devices;

	int i;

	duk_push_global_stash(ctx);
	duk_push_array(ctx);
	num_devices = joy_num_devices();
	for (i = 0; i < num_devices; ++i) {
		device = malloc(sizeof(int));
		*device = i;
		duk_push_class_obj(ctx, "Joystick", device);
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "joystickObjects");
	duk_pop(ctx);
}

static path_t*
load_package_json(const char* filename)
{
	duk_idx_t duk_top;
	char*     json;
	size_t    json_size;
	path_t*   path;

	duk_top = duk_get_top(g_duk);
	if (!(json = sfs_fslurp(g_fs, filename, NULL, &json_size)))
		goto on_error;
	duk_push_lstring(g_duk, json, json_size);
	free(json);
	if (duk_json_pdecode(g_duk) != DUK_EXEC_SUCCESS)
		goto on_error;
	if (!duk_is_object_coercible(g_duk, -1))
		goto on_error;
	duk_get_prop_string(g_duk, -1, "main");
	if (!duk_is_string(g_duk, -1))
		goto on_error;
	path = path_strip(path_new(filename));
	path_append(path, duk_get_string(g_duk, -1));
	path_collapse(path, true);
	if (!sfs_fexist(g_fs, path_cstr(path), NULL))
		goto on_error;
	return path;

on_error:
	duk_set_top(g_duk, duk_top);
	return NULL;
}

static duk_ret_t
js_require(duk_context* ctx)
{
	const char* id;
	const char* parent_id = NULL;
	path_t*     path;

	duk_push_current_function(ctx);
	if (duk_get_prop_string(ctx, -1, "id"))
		parent_id = duk_get_string(ctx, -1);
	id = duk_require_string(ctx, 0);

	if (parent_id == NULL && (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "relative require not allowed in global code");
	if (!(path = find_module(id, parent_id, "lib/")) && !(path = find_module(id, parent_id, "#/modules/")))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "module not found `%s`", id);
	if (!duk_pegasus_eval_module(ctx, path_cstr(path)))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_screen_get_frameRate(duk_context* ctx)
{
	// as far as Sphere v2 code is concerned, infinity, not 0, means "unthrottled".
	// that's stored as a zero internally though, so we need to translate.
	duk_push_number(ctx, s_framerate > 0 ? s_framerate : INFINITY);
	return 1;
}

static duk_ret_t
js_screen_set_frameRate(duk_context* ctx)
{
	double framerate;

	framerate = duk_require_number(ctx, 0);

	if (framerate < 1.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame rate");
	if (framerate != INFINITY)
		s_framerate = framerate;
	else
		s_framerate = 0;  // unthrottled
	return 0;
}

static duk_ret_t
js_screen_get_frameSkip(duk_context* ctx)
{
	duk_push_number(ctx, screen_get_frameskip(g_screen));
	return 1;
}

static duk_ret_t
js_screen_set_frameSkip(duk_context* ctx)
{
	double max_skips;

	max_skips = duk_require_number(ctx, 0);

	if (max_skips < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frameskip");
	screen_set_frameskip(g_screen, max_skips);
	return 0;
}

static duk_ret_t
js_screen_get_fullScreen(duk_context* ctx)
{
	duk_push_boolean(ctx, screen_fullscreen(g_screen));
	return 1;
}

static duk_ret_t
js_screen_set_fullScreen(duk_context* ctx)
{
	bool fullscreen;

	fullscreen = duk_require_boolean(ctx, 0);
	
	screen_set_fullscreen(g_screen, fullscreen);
	return 0;
}

static duk_ret_t
js_screen_flip(duk_context* ctx)
{
	screen_flip(g_screen, s_framerate);
	image_set_scissor(screen_backbuffer(g_screen), new_rect(0, 0, g_res_x, g_res_y));
	return 0;
}

static duk_ret_t
js_screen_now(duk_context* ctx)
{
	duk_push_number(ctx, screen_now(g_screen));
	return 1;
}

static duk_ret_t
js_screen_resize(duk_context* ctx)
{
	int  width;
	int  height;

	width = duk_require_int(ctx, 0);
	height = duk_require_int(ctx, 1);

	if (width < 0 || height < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid screen resolution");
	screen_resize(g_screen, width, height);
	return 0;
}

static duk_ret_t
js_Sphere_get_APILevel(duk_context* ctx)
{
	duk_push_int(ctx, API_LEVEL);
	return 1;
}

static duk_ret_t
js_Sphere_get_Game(duk_context* ctx)
{
	duk_push_lstring_t(ctx, fs_manifest(g_fs));
	duk_json_decode(ctx, -1);

	duk_push_this(ctx);
	duk_push_string(ctx, "Game");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_Sphere_get_Platform(duk_context* ctx)
{
	duk_push_sprintf(ctx, "%s %s", ENGINE_NAME, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_Sphere_get_Version(duk_context* ctx)
{
	duk_push_int(ctx, API_VERSION);
	return 1;
}

static duk_ret_t
js_Sphere_abort(duk_context* ctx)
{
	const char* filename;
	int         line_number;
	const char* message;
	int         num_args;
	char*       text;

	num_args = duk_get_top(ctx);
	message = num_args >= 1
		? duk_to_string(ctx, 0)
		: "some type of weird pig just ate your game\n\n\n...and you*munch*";

	dukrub_inspect_callstack_entry(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber");
	duk_get_prop_string(ctx, -2, "function");
	duk_get_prop_string(ctx, -1, "fileName");
	filename = duk_get_string(ctx, -1);
	line_number = duk_get_int(ctx, -2);
	text = strnewf("%s:%d\nmanual abort\n\n%s", filename, line_number, message);
	abort_game(text);
}

static duk_ret_t
js_Sphere_exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_Sphere_restart(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_Sphere_run(duk_context* ctx)
{
	do_events(true);
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_Sphere_sleep(duk_context* ctx)
{
	double timeout;

	timeout = duk_require_number(ctx, 0);

	if (timeout < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid sleep timeout");
	delay(timeout);
	return 0;
}

static duk_ret_t
js_Color_get_Color(duk_context* ctx)
{
	const struct x11_color* data;
	int                     index;

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "index");
	index = duk_get_int(ctx, -1);
	
	data = &COLORS[index];
	duk_pegasus_push_color(ctx, color_new(data->r, data->g, data->b, data->a));
	return 1;
}

static duk_ret_t
js_Color_is(duk_context* ctx)
{
	color_t color1;
	color_t color2;

	color1 = duk_pegasus_require_color(ctx, 0);
	color2 = duk_pegasus_require_color(ctx, 1);

	duk_push_boolean(ctx, color1.r == color2.r
		&& color1.g == color2.g
		&& color1.b == color2.b);
	return 1;
}

static duk_ret_t
js_Color_mix(duk_context* ctx)
{
	color_t color1;
	color_t color2;
	int     num_args;
	float   w1 = 1.0;
	float   w2 = 1.0;

	num_args = duk_get_top(ctx);
	color1 = duk_pegasus_require_color(ctx, 0);
	color2 = duk_pegasus_require_color(ctx, 1);
	if (num_args > 2) {
		w1 = duk_require_number(ctx, 2);
		w2 = duk_require_number(ctx, 3);
	}

	if (w1 < 0.0 || w2 < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid weight(s)", w1, w2);

	duk_pegasus_push_color(ctx, color_mix(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_Color_of(duk_context* ctx)
{
	color_t     color;
	size_t      hex_length;
	const char* name;
	uint32_t    value;

	const struct x11_color* p;

	duk_push_this(ctx);
	name = duk_require_string(ctx, 0);

	// check if caller gave an X11 color name
	p = &COLORS[0];
	while (p->name != NULL) {
		if (strcasecmp(name, p->name) == 0) {
			duk_pegasus_push_color(ctx, color_new(p->r, p->g, p->b, p->a));
			return 1;
		}
		++p;
	}

	// is `name` an RGB or ARGB signature?
	if (name[0] != '#')
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "unrecognized color name");
	hex_length = strspn(&name[1], "0123456789ABCDEFabcdef");
	if (hex_length != strlen(name) - 1 || (hex_length != 6 && hex_length != 8))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "invalid RGB signature");
	value = strtoul(&name[1], NULL, 16);
	color.a = hex_length == 8 ? (value >> 24) & 0xFF : 255;
	color.r = (value >> 16) & 0xFF;
	color.g = (value >> 8) & 0xFF;
	color.b = value & 0xFF;
	duk_pegasus_push_color(ctx, color);
	return 1;
}

static duk_ret_t
js_new_Color(duk_context* ctx)
{
	float a;
	float b;
	float g;
	int   num_args;
	int   obj_index;
	float r;
	
	num_args = duk_get_top(ctx);
	r = duk_require_number(ctx, 0);
	g = duk_require_number(ctx, 1);
	b = duk_require_number(ctx, 2);
	a = num_args >= 4 ? duk_require_number(ctx, 3) : 1.0;

	// construct a Color object
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Color", NULL);
	obj_index = duk_normalize_index(ctx, -1);
	duk_push_number(ctx, r);
	duk_push_number(ctx, g);
	duk_push_number(ctx, b);
	duk_push_number(ctx, a);
	duk_put_prop_string(ctx, obj_index, "a");
	duk_put_prop_string(ctx, obj_index, "b");
	duk_put_prop_string(ctx, obj_index, "g");
	duk_put_prop_string(ctx, obj_index, "r");
	return 0;
}

static duk_ret_t
js_Color_get_name(duk_context* ctx)
{
	color_t color;

	const struct x11_color* p;

	duk_push_this(ctx);
	color = duk_pegasus_require_color(ctx, -1);

	p = &COLORS[0];
	while (p->name != NULL) {
		if (color.r == p->r && color.g == p->g && color.b == p->b && color.a == p->a) {
			duk_push_string(ctx, p->name);
			return 1;
		}
		++p;
	}

	duk_push_sprintf(ctx, "#%.2x%.2x%.2x%.2x", color.a, color.r, color.g, color.b);
	return 1;
}

static duk_ret_t
js_Color_clone(duk_context* ctx)
{
	color_t color;

	duk_push_this(ctx);
	color = duk_pegasus_require_color(ctx, -1);

	duk_pegasus_push_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Color_fade(duk_context* ctx)
{
	float   a;
	color_t color;

	duk_push_this(ctx);
	color = duk_pegasus_require_color(ctx, -1);
	a = duk_require_number(ctx, 0);

	color.a = fmin(fmax(color.a * a, 0), 255);
	duk_pegasus_push_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Dispatch_cancel(duk_context* ctx)
{
	int64_t* token;

	token = duk_require_class_obj(ctx, 0, "JobToken");

	async_cancel(*token);
	return 0;
}

static duk_ret_t
js_Dispatch_cancelAll(duk_context* ctx)
{
	async_cancel_all(false);
	return 0;
}

static duk_ret_t
js_Dispatch_later(duk_context* ctx)
{
	script_t* script;
	uint32_t  timeout;
	int64_t   token;

	timeout = duk_require_uint(ctx, 0);
	script = duk_pegasus_require_script(ctx, 1);

	if (!(token = async_defer(script, timeout, ASYNC_UPDATE)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "dispatch failed");
	duk_pegasus_push_job_token(ctx, token);
	return 1;
}

static duk_ret_t
js_Dispatch_now(duk_context* ctx)
{
	script_t* script;
	int64_t   token;

	script = duk_pegasus_require_script(ctx, 0);

	if (!(token = async_defer(script, 0, ASYNC_TICK)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "dispatch failed");
	duk_pegasus_push_job_token(ctx, token);
	return 1;
}

static duk_ret_t
js_Dispatch_onRender(duk_context* ctx)
{
	int       num_args;
	double    priority;
	script_t* script;
	int64_t   token;

	num_args = duk_get_top(ctx);
	script = duk_pegasus_require_script(ctx, 0);
	priority = num_args >= 2 ? duk_require_number(ctx, 1)
		: 0.0;

	if (!(token = async_recur(script, priority, ASYNC_RENDER)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "dispatch failed");
	duk_pegasus_push_job_token(ctx, token);
	return 1;
}

static duk_ret_t
js_Dispatch_onUpdate(duk_context* ctx)
{
	int       num_args;
	double    priority;
	script_t* script;
	int64_t   token;

	num_args = duk_get_top(ctx);
	script = duk_pegasus_require_script(ctx, 0);
	priority = num_args >= 2 ? duk_require_number(ctx, 1)
		: 0.0;

	if (!(token = async_recur(script, priority, ASYNC_UPDATE)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "dispatch failed");
	duk_pegasus_push_job_token(ctx, token);
	return 1;
}

static duk_ret_t
js_FS_createDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, NULL, false, true);

	if (!sfs_mkdir(g_fs, name, NULL))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "directory creation failed");
	return 0;
}

static duk_ret_t
js_FS_deleteFile(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false, true);

	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unlink failed", filename);
	return 0;
}

static duk_ret_t
js_FS_exists(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false, false);

	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
	return 1;
}

static duk_ret_t
js_FS_readFile(duk_context* ctx)
{
	lstring_t*  content;
	void*       file_data;
	size_t      file_size;
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false, false);

	if (!(file_data = sfs_fslurp(g_fs, filename, NULL, &file_size)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't read file '%s'", filename);
	content = lstr_from_cp1252(file_data, file_size);
	duk_push_lstring_t(ctx, content);
	return 1;
}

static duk_ret_t
js_FS_removeDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, NULL, false, true);

	if (!sfs_rmdir(g_fs, name, NULL))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "directory removal failed", name);
	return 0;
}

static duk_ret_t
js_FS_rename(duk_context* ctx)
{
	const char* name1;
	const char* name2;

	name1 = duk_require_path(ctx, 0, NULL, false, true);
	name2 = duk_require_path(ctx, 1, NULL, false, true);

	if (!sfs_rename(g_fs, name1, name2, NULL))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "rename failed", name1, name2);
	return 0;
}

static duk_ret_t
js_FS_resolve(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false, false);

	duk_push_string(ctx, filename);
	return 1;
}

static duk_ret_t
js_FS_writeFile(duk_context* ctx)
{
	const void* file_data;
	size_t      file_size;
	const char* filename;
	lstring_t*  text = NULL;

	filename = duk_require_path(ctx, 0, NULL, false, true);
	text = duk_require_lstring_t(ctx, 1);
	
	file_data = lstr_cstr(text);
	file_size = lstr_len(text);
	if (!sfs_fspew(g_fs, filename, NULL, file_data, file_size))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't write file '%s'", filename);
	lstr_free(text);
	return 0;
}

static duk_ret_t
js_new_FileStream(duk_context* ctx)
{
	sfs_file_t*  file;
	enum file_op file_op;
	const char*  filename;
	const char*  mode;

	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");
	
	duk_require_string(ctx, 0);
	file_op = duk_require_int(ctx, 1);
	if (file_op < 0 || file_op >= FILE_OP_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid file-op constant");

	filename = duk_require_path(ctx, 0, NULL, false, file_op != FILE_OP_READ);
	if (file_op == FILE_OP_UPDATE && !sfs_fexist(g_fs, filename, NULL))
		file_op = FILE_OP_WRITE;  // because 'r+b' requires the file to exist.
	mode = file_op == FILE_OP_READ ? "rb"
		: file_op == FILE_OP_WRITE ? "w+b"
		: file_op == FILE_OP_UPDATE ? "r+b"
		: NULL;
	if (!(file = sfs_fopen(g_fs, filename, NULL, mode)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "failure to open file");
	if (file_op == FILE_OP_UPDATE)
		sfs_fseek(file, 0, SFS_SEEK_END);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "FileStream", file);
	return 0;
}

static duk_ret_t
js_FileStream_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_class_obj(ctx, 0, "FileStream");

	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_get_fileName(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");

	duk_push_string(ctx, sfs_fpath(file));
	return 1;
}

static duk_ret_t
js_FileStream_get_fileSize(duk_context* ctx)
{
	sfs_file_t* file;
	long        file_pos;

	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");

	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_number(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_FileStream_get_position(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");

	duk_push_number(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_FileStream_set_position(duk_context* ctx)
{
	sfs_file_t* file;
	long long   new_pos;

	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");

	new_pos = duk_require_number(ctx, 0);
	sfs_fseek(file, new_pos, SFS_SEEK_SET);
	return 0;
}

static duk_ret_t
js_FileStream_dispose(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");

	duk_set_class_ptr(ctx, -1, NULL);
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_read(duk_context* ctx)
{
	// FileStream:read([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;
	if (num_bytes < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid read size");

	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FileStream_write(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_require_buffer_data(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	if (!(file = duk_require_class_obj(ctx, -1, "FileStream")))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "use of disposed object");

	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "failure to write to file");
	return 0;
}

static duk_ret_t
js_Font_get_Default(duk_context* ctx)
{
	duk_push_class_obj(ctx, "Font", g_sys_font);

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_Font(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, NULL, false, false);

	if (!(font = font_load(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load font `%s`", filename);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Font", font);
	return 0;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_class_obj(ctx, 0, "Font");
	font_free(font);
	return 0;
}

static duk_ret_t
js_Font_get_fileName(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "Font");

	duk_push_string(ctx, font_path(font));
	return 1;
}

static duk_ret_t
js_Font_get_height(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "Font");

	duk_push_int(ctx, font_height(font));
	return 1;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	color_t     color;
	font_t*     font;
	int         height;
	int         num_args;
	image_t*    surface;
	const char* text;
	int         width;
	wraptext_t* wraptext;
	int         x;
	int         y;

	int i;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "Font");
	surface = duk_require_class_obj(ctx, 0, "Surface");
	x = duk_require_int(ctx, 1);
	y = duk_require_int(ctx, 2);
	text = duk_to_string(ctx, 3);
	color = num_args >= 5 ? duk_pegasus_require_color(ctx, 4)
		: color_new(255, 255, 255, 255);
	width = num_args >= 6 ? duk_require_int(ctx, 5) : 0;

	if (surface == screen_backbuffer(g_screen) && screen_is_skipframe(g_screen))
		return 0;
	else {
		image_render_to(surface, NULL);
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
	return 0;
}

static duk_ret_t
js_Font_getTextSize(duk_context* ctx)
{
	font_t*     font;
	int         num_args;
	int         num_lines;
	const char* text;
	int         width;
	wraptext_t* wraptext;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "Font");
	text = duk_to_string(ctx, 0);
	if (num_args >= 2)
		width = duk_require_int(ctx, 1);

	duk_push_object(ctx);
	if (num_args >= 2) {
		wraptext = wraptext_new(text, font, width);
		num_lines = wraptext_len(wraptext);
		wraptext_free(wraptext);
		duk_push_int(ctx, width);
		duk_put_prop_string(ctx, -2, "width");
		duk_push_int(ctx, font_height(font) * num_lines);
		duk_put_prop_string(ctx, -2, "height");
	}
	else {
		duk_push_int(ctx, font_get_width(font, text));
		duk_put_prop_string(ctx, -2, "width");
		duk_push_int(ctx, font_height(font));
		duk_put_prop_string(ctx, -2, "height");
	}
	return 1;
}

static duk_ret_t
js_Font_wordWrap(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int         width = duk_require_int(ctx, 1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "Font");
	duk_pop(ctx);
	wraptext = wraptext_new(text, font, width);
	num_lines = wraptext_len(wraptext);
	duk_push_array(ctx);
	for (i = 0; i < num_lines; ++i) {
		duk_push_string(ctx, wraptext_line(wraptext, i));
		duk_put_prop_index(ctx, -2, i);
	}
	wraptext_free(wraptext);
	return 1;
}

static duk_ret_t
js_new_IndexList(duk_context* ctx)
{
	ibo_t*   ibo;
	uint16_t index;
	int      num_entries;

	int i;
	
	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "expected an array");

	num_entries = (int)duk_get_length(ctx, 0);
	if (num_entries == 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "empty list not allowed");
	ibo = ibo_new();
	for (i = 0; i < num_entries; ++i) {
		duk_get_prop_index(ctx, 0, i);
		index = duk_require_int(ctx, -1);
		if (index < 0 || index > UINT16_MAX) {
			ibo_free(ibo);
			duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "index value out of range");
		}
		ibo_add_index(ibo, index);
		duk_pop(ctx);
	}
	if (!ibo_upload(ibo)) {
		ibo_free(ibo);
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "upload to GPU failed");
	}
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "IndexList", ibo);
	return 0;
}

static duk_ret_t
js_IndexList_finalize(duk_context* ctx)
{
	ibo_t* ibo;

	ibo = duk_require_class_obj(ctx, 0, "IndexList");
	ibo_free(ibo);
	return 0;
}

static duk_ret_t
js_JobToken_finalize(duk_context* ctx)
{
	int64_t* token;

	token = duk_require_class_obj(ctx, 0, "JobToken");

	free(token);
	return 0;
}

static duk_ret_t
js_Joystick_get_Null(duk_context* ctx)
{
	int* device;
	
	device = malloc(sizeof(int));
	*device = -1;
	duk_push_class_obj(ctx, "Joystick", device);

	duk_push_this(ctx);
	duk_push_string(ctx, "Null");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_Joystick_getDevices(duk_context* ctx)
{
	int num_devices;

	int i;

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "joystickObjects");
	num_devices = (int)duk_get_length(ctx, -1);
	duk_push_array(ctx);
	for (i = 0; i < num_devices; ++i) {
		duk_get_prop_index(ctx, -2, i);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
js_Joystick_finalize(duk_context* ctx)
{
	int* device;

	device = duk_require_class_obj(ctx, 0, "Joystick");
	free(device);
	return 0;
}

static duk_ret_t
js_Joystick_get_name(duk_context* ctx)
{
	int* device;

	duk_push_this(ctx);
	device = duk_require_class_obj(ctx, -1, "Joystick");

	duk_push_string(ctx, joy_name(*device));
	return 1;
}

static duk_ret_t
js_Joystick_get_numAxes(duk_context* ctx)
{
	int* device;

	duk_push_this(ctx);
	device = duk_require_class_obj(ctx, -1, "Joystick");
	
	if (*device != -1)
		duk_push_int(ctx, joy_num_axes(*device));
	else
		duk_push_number(ctx, INFINITY);
	return 1;
}

static duk_ret_t
js_Joystick_get_numButtons(duk_context* ctx)
{
	int* device;

	duk_push_this(ctx);
	device = duk_require_class_obj(ctx, -1, "Joystick");
	
	if (*device != -1)
		duk_push_int(ctx, joy_num_buttons(*device));
	else
		duk_push_number(ctx, INFINITY);
	return 1;
}

static duk_ret_t
js_Joystick_getPosition(duk_context* ctx)
{
	int  index;
	int* device;

	duk_push_this(ctx);
	device = duk_require_class_obj(ctx, -1, "Joystick");
	index = duk_require_int(ctx, 0);
	
	if (*device != -1 && (index < 0 || index >= joy_num_axes(*device)))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "joystick axis ID out of range");

	duk_push_number(ctx, joy_position(*device, index));
	return 1;
}

static duk_ret_t
js_Joystick_isPressed(duk_context* ctx)
{
	int  index;
	int* device;

	duk_push_this(ctx);
	device = duk_require_class_obj(ctx, -1, "Joystick");
	index = duk_require_int(ctx, 0);

	if (*device != -1 && (index < 0 || index >= joy_num_buttons(*device)))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "joystick button ID out of range");

	duk_push_boolean(ctx, joy_is_button_down(*device, index));
	return 1;
}

static duk_ret_t
js_Keyboard_get_Default(duk_context* ctx)
{
	duk_push_class_obj(ctx, "Keyboard", NULL);

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_Keyboard_get_capsLock(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");
	
	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_CAPSLOCK));
	return 1;
}

static duk_ret_t
js_Keyboard_get_numLock(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");

	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_NUMLOCK));
	return 1;
}

static duk_ret_t
js_Keyboard_get_scrollLock(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");

	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_SCROLLLOCK));
	return 1;
}

static duk_ret_t
js_Keyboard_clearQueue(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");

	kb_clear_queue();
	return 0;
}

static duk_ret_t
js_Keyboard_getChar(duk_context* ctx)
{
	int  keycode;
	int  num_args;
	bool shifted;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");
	keycode = duk_require_int(ctx, 0);
	shifted = num_args >= 2 ? duk_require_boolean(ctx, 1)
		: false;

	switch (keycode) {
	case ALLEGRO_KEY_A: duk_push_string(ctx, shifted ? "A" : "a"); break;
	case ALLEGRO_KEY_B: duk_push_string(ctx, shifted ? "B" : "b"); break;
	case ALLEGRO_KEY_C: duk_push_string(ctx, shifted ? "C" : "c"); break;
	case ALLEGRO_KEY_D: duk_push_string(ctx, shifted ? "D" : "d"); break;
	case ALLEGRO_KEY_E: duk_push_string(ctx, shifted ? "E" : "e"); break;
	case ALLEGRO_KEY_F: duk_push_string(ctx, shifted ? "F" : "f"); break;
	case ALLEGRO_KEY_G: duk_push_string(ctx, shifted ? "G" : "g"); break;
	case ALLEGRO_KEY_H: duk_push_string(ctx, shifted ? "H" : "h"); break;
	case ALLEGRO_KEY_I: duk_push_string(ctx, shifted ? "I" : "i"); break;
	case ALLEGRO_KEY_J: duk_push_string(ctx, shifted ? "J" : "j"); break;
	case ALLEGRO_KEY_K: duk_push_string(ctx, shifted ? "K" : "k"); break;
	case ALLEGRO_KEY_L: duk_push_string(ctx, shifted ? "L" : "l"); break;
	case ALLEGRO_KEY_M: duk_push_string(ctx, shifted ? "M" : "m"); break;
	case ALLEGRO_KEY_N: duk_push_string(ctx, shifted ? "N" : "n"); break;
	case ALLEGRO_KEY_O: duk_push_string(ctx, shifted ? "O" : "o"); break;
	case ALLEGRO_KEY_P: duk_push_string(ctx, shifted ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: duk_push_string(ctx, shifted ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: duk_push_string(ctx, shifted ? "R" : "r"); break;
	case ALLEGRO_KEY_S: duk_push_string(ctx, shifted ? "S" : "s"); break;
	case ALLEGRO_KEY_T: duk_push_string(ctx, shifted ? "T" : "t"); break;
	case ALLEGRO_KEY_U: duk_push_string(ctx, shifted ? "U" : "u"); break;
	case ALLEGRO_KEY_V: duk_push_string(ctx, shifted ? "V" : "v"); break;
	case ALLEGRO_KEY_W: duk_push_string(ctx, shifted ? "W" : "w"); break;
	case ALLEGRO_KEY_X: duk_push_string(ctx, shifted ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: duk_push_string(ctx, shifted ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: duk_push_string(ctx, shifted ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: duk_push_string(ctx, shifted ? "!" : "1"); break;
	case ALLEGRO_KEY_2: duk_push_string(ctx, shifted ? "@" : "2"); break;
	case ALLEGRO_KEY_3: duk_push_string(ctx, shifted ? "#" : "3"); break;
	case ALLEGRO_KEY_4: duk_push_string(ctx, shifted ? "$" : "4"); break;
	case ALLEGRO_KEY_5: duk_push_string(ctx, shifted ? "%" : "5"); break;
	case ALLEGRO_KEY_6: duk_push_string(ctx, shifted ? "^" : "6"); break;
	case ALLEGRO_KEY_7: duk_push_string(ctx, shifted ? "&" : "7"); break;
	case ALLEGRO_KEY_8: duk_push_string(ctx, shifted ? "*" : "8"); break;
	case ALLEGRO_KEY_9: duk_push_string(ctx, shifted ? "(" : "9"); break;
	case ALLEGRO_KEY_0: duk_push_string(ctx, shifted ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: duk_push_string(ctx, shifted ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: duk_push_string(ctx, shifted ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: duk_push_string(ctx, shifted ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: duk_push_string(ctx, shifted ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: duk_push_string(ctx, shifted ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: duk_push_string(ctx, shifted ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: duk_push_string(ctx, shifted ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: duk_push_string(ctx, shifted ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: duk_push_string(ctx, shifted ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: duk_push_string(ctx, shifted ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: duk_push_string(ctx, " "); break;
	case ALLEGRO_KEY_TAB: duk_push_string(ctx, "\t"); break;
	case ALLEGRO_KEY_TILDE: duk_push_string(ctx, shifted ? "~" : "`"); break;
	default:
		duk_push_string(ctx, "");
	}
	return 1;
}

static duk_ret_t
js_Keyboard_getKey(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");

	if (kb_queue_len() > 0)
		duk_push_int(ctx, kb_get_key());
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Keyboard_isPressed(duk_context* ctx)
{
	int keycode;

	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Keyboard");
	keycode = duk_require_int(ctx, 0);

	duk_push_boolean(ctx, kb_is_key_down(keycode));
	return 1;
}

static duk_ret_t
js_Mixer_get_Default(duk_context* ctx)
{
	duk_push_class_obj(ctx, "Mixer", mixer_ref(s_def_mixer));

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_Mixer(duk_context* ctx)
{
	int      bits;
	int      channels = 2;
	int      frequency;
	mixer_t* mixer;
	int      num_args;

	num_args = duk_get_top(ctx);
	frequency = duk_require_int(ctx, 0);
	bits = duk_require_int(ctx, 1);
	if (num_args >= 3)
		channels = duk_require_int(ctx, 2);

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid audio bit depth");
	if (channels < 1 || channels > 7)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid channel count");
	if (!(mixer = mixer_new(frequency, bits, channels)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create %d-bit %dch voice", bits, channels);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Mixer", mixer);
	return 0;
}

static duk_ret_t
js_Mixer_finalize(duk_context* ctx)
{
	mixer_t* mixer;

	mixer = duk_require_class_obj(ctx, 0, "Mixer");
	mixer_free(mixer);
	return 0;
}

static duk_ret_t
js_Mixer_get_volume(duk_context* ctx)
{
	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_class_obj(ctx, -1, "Mixer");

	duk_push_number(ctx, mixer_get_gain(mixer));
	return 1;
}

static duk_ret_t
js_Mixer_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_class_obj(ctx, -1, "Mixer");

	mixer_set_gain(mixer, volume);
	return 0;
}

static duk_ret_t
js_new_Model(duk_context* ctx)
{
	model_t*  group;
	int       num_args;
	size_t    num_shapes;
	shader_t* shader;
	shape_t*  shape;

	duk_uarridx_t i;

	num_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	shader = num_args >= 2
		? duk_require_class_obj(ctx, 1, "Shader")
		: galileo_shader();

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array required");

	group = model_new(shader);
	num_shapes = duk_get_length(ctx, 0);
	for (i = 0; i < num_shapes; ++i) {
		duk_get_prop_index(ctx, 0, i);
		shape = duk_require_class_obj(ctx, -1, "Shape");
		model_add_shape(group, shape);
	}
	duk_push_class_obj(ctx, "Model", group);
	return 1;
}

static duk_ret_t
js_Model_finalize(duk_context* ctx)
{
	model_t* group;

	group = duk_require_class_obj(ctx, 0, "Model");
	model_free(group);
	return 0;
}

static duk_ret_t
js_Model_get_shader(duk_context* ctx)
{
	model_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");

	shader = model_get_shader(group);
	duk_push_class_obj(ctx, "Shader", shader_ref(shader));
	return 1;
}

static duk_ret_t
js_Model_get_transform(duk_context* ctx)
{
	model_t*     group;
	transform_t* transform;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");

	transform = model_get_transform(group);
	duk_push_class_obj(ctx, "Transform", transform_ref(transform));
	return 1;
}

static duk_ret_t
js_Model_set_shader(duk_context* ctx)
{
	model_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	shader = duk_require_class_obj(ctx, 0, "Shader");

	model_set_shader(group, shader);
	return 0;
}

static duk_ret_t
js_Model_set_transform(duk_context* ctx)
{
	model_t*     group;
	transform_t* transform;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	transform = duk_require_class_obj(ctx, 0, "Transform");

	model_set_transform(group, transform);
	return 0;
}

static duk_ret_t
js_Model_draw(duk_context* ctx)
{
	model_t* group;
	int      num_args;
	image_t* surface;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	surface = num_args >= 1 ? duk_require_class_obj(ctx, 0, "Surface")
		: screen_backbuffer(g_screen);

	if (!screen_is_skipframe(g_screen))
		model_draw(group, surface);
	return 0;
}

static duk_ret_t
js_Model_setBoolean(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	bool        value;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	value = duk_require_boolean(ctx, 1);

	model_put_bool(group, name, value);
	return 0;
}

static duk_ret_t
js_Model_setColorVector(duk_context* ctx)
{
	color_t     color;
	model_t*    group;
	const char* name;
	float       values[4];

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	color = duk_pegasus_require_color(ctx, 1);

	values[0] = color.r / 255.0;
	values[1] = color.g / 255.0;
	values[2] = color.b / 255.0;
	values[3] = color.a / 255.0;
	model_put_float_vector(group, name, values, 4);
	return 0;
}

static duk_ret_t
js_Model_setFloat(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	float       value;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	value = duk_require_number(ctx, 1);

	model_put_float(group, name, value);
	return 0;
}

static duk_ret_t
js_Model_setFloatArray(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	int         size;
	float*      values;

	int i;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array was expected here");

	size = (int)duk_get_length(ctx, 1);

	values = malloc(size * sizeof(float));
	for (i = 0; i < size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		values[i] = duk_require_number(ctx, -1);
		duk_pop(ctx);
	}
	model_put_float_array(group, name, values, size);
	free(values);
	return 0;
}

static duk_ret_t
js_Model_setFloatVector(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	int         size;
	float       values[4];

	int i;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array was expected here");
	
	size = (int)duk_get_length(ctx, 1);
	if (size < 2 || size > 4)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid number of components");

	for (i = 0; i < size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		values[i] = duk_require_number(ctx, -1);
		duk_pop(ctx);
	}
	model_put_float_vector(group, name, values, size);
	return 0;
}

static duk_ret_t
js_Model_setInt(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	int         value;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	value = duk_require_int(ctx, 1);

	model_put_int(group, name, value);
	return 0;
}

static duk_ret_t
js_Model_setIntArray(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	int         size;
	int*        values;

	int i;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array was expected here");

	size = (int)duk_get_length(ctx, 1);
	values = malloc(size * sizeof(int));
	for (i = 0; i < size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		values[i] = duk_require_int(ctx, -1);
		duk_pop(ctx);
	}
	model_put_int_array(group, name, values, size);
	free(values);
	return 0;
}

static duk_ret_t
js_Model_setIntVector(duk_context* ctx)
{
	model_t*    group;
	const char* name;
	int         size;
	int         values[4];

	int i;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array was expected here");

	size = (int)duk_get_length(ctx, 1);
	if (size < 2 || size > 4)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid number of components");

	for (i = 0; i < size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		values[i] = duk_require_int(ctx, -1);
		duk_pop(ctx);
	}
	model_put_int_vector(group, name, values, size);
	return 0;
}

static duk_ret_t
js_Model_setMatrix(duk_context* ctx)
{
	model_t*     group;
	const char*  name;
	transform_t* transform;

	duk_push_this(ctx);
	group = duk_require_class_obj(ctx, -1, "Model");
	name = duk_require_string(ctx, 0);
	transform = duk_require_class_obj(ctx, 1, "Transform");

	model_put_matrix(group, name, transform);
	return 0;
}

static duk_ret_t
js_Mouse_get_Default(duk_context* ctx)
{
	duk_push_class_obj(ctx, "Mouse", NULL);

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_Mouse_get_x(duk_context* ctx)
{
	int x;
	int y;

	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Mouse");
	
	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_Mouse_get_y(duk_context* ctx)
{
	int x;
	int y;

	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Mouse");

	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_Mouse_clearQueue(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Mouse");

	mouse_clear_queue();
	return 0;
}

static duk_ret_t
js_Mouse_getEvent(duk_context* ctx)
{
	mouse_event_t event;

	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Mouse");

	if (mouse_queue_len() == 0)
		duk_push_null(ctx);
	else {
		event = mouse_get_event();
		duk_push_object(ctx);
		duk_push_int(ctx, event.key);
		duk_put_prop_string(ctx, -2, "key");
		duk_push_int(ctx, event.x);
		duk_put_prop_string(ctx, -2, "x");
		duk_push_int(ctx, event.y);
		duk_put_prop_string(ctx, -2, "y");
	}
	return 1;
}

static duk_ret_t
js_Mouse_isPressed(duk_context* ctx)
{
	mouse_key_t key;

	duk_push_this(ctx);
	duk_require_class_obj(ctx, -1, "Mouse");

	key = duk_require_int(ctx, 0);
	if (key < 0 || key >= MOUSE_KEY_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid MouseKey constant");

	duk_push_boolean(ctx, mouse_is_key_down(key));
	return 1;
}

static duk_ret_t
js_RNG_fromSeed(duk_context* ctx)
{
	uint64_t seed;
	xoro_t*  xoro;

	seed = duk_require_number(ctx, 0);
	
	xoro = xoro_new(seed);
	duk_push_class_obj(ctx, "RNG", xoro);
	return 1;
}

static duk_ret_t
js_RNG_fromState(duk_context* ctx)
{
	const char* state;
	xoro_t*     xoro;

	state = duk_require_string(ctx, 0);

	xoro = xoro_new(0);
	if (!xoro_set_state(xoro, state)) {
		xoro_free(xoro);
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "invalid RNG state string");
	}
	duk_push_class_obj(ctx, "RNG", xoro);
	return 1;
}

static duk_ret_t
js_new_RNG(duk_context* ctx)
{
	xoro_t* xoro;
	
	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");
	
	xoro = xoro_new((uint64_t)(al_get_time() * 1000000));
	duk_push_class_obj(ctx, "RNG", xoro);
	return 1;
}

static duk_ret_t
js_RNG_finalize(duk_context* ctx)
{
	xoro_t* xoro;

	xoro = duk_require_class_obj(ctx, 0, "RNG");

	xoro_free(xoro);
	return 0;
}

static duk_ret_t
js_RNG_get_state(duk_context* ctx)
{
	char    state[33];
	xoro_t* xoro;

	duk_push_this(ctx);
	xoro = duk_require_class_obj(ctx, -1, "RNG");
	
	xoro_get_state(xoro, state);
	duk_push_string(ctx, state);
	return 1;
}

static duk_ret_t
js_RNG_set_state(duk_context* ctx)
{
	const char* state;
	xoro_t*     xoro;

	duk_push_this(ctx);
	xoro = duk_require_class_obj(ctx, -1, "RNG");
	state = duk_require_string(ctx, 0);

	if (!xoro_set_state(xoro, state))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "invalid RNG state string");
	return 0;
}

static duk_ret_t
js_RNG_next(duk_context* ctx)
{
	xoro_t*     xoro;

	duk_push_this(ctx);
	xoro = duk_require_class_obj(ctx, -1, "RNG");

	duk_push_number(ctx, xoro_gen_double(xoro));
	return 1;
}

static duk_ret_t
js_SSj_log(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debugger_log(duk_get_string(ctx, -1), PRINT_NORMAL, true);
	return 0;
}

static duk_ret_t
js_SSj_trace(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debugger_log(duk_get_string(ctx, -1), PRINT_TRACE, true);
	return 0;
}

static duk_ret_t
js_new_Sample(duk_context* ctx)
{
	const char* filename;
	sample_t*   sample;

	filename = duk_require_path(ctx, 0, NULL, false, false);

	if (!(sample = sample_new(filename, true)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load sample `%s`", filename);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Sample", sample);
	return 0;
}

static duk_ret_t
js_Sample_finalize(duk_context* ctx)
{
	sample_t* sample;

	sample = duk_require_class_obj(ctx, 0, "Sample");
	sample_free(sample);
	return 0;
}

static duk_ret_t
js_Sample_get_fileName(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "Sample");

	duk_push_string(ctx, sample_path(sample));
	return 1;
}

static duk_ret_t
js_Sample_play(duk_context* ctx)
{
	mixer_t*  mixer;
	int       num_args;
	float     pan = 0.0;
	sample_t* sample;
	float     speed = 1.0;
	float     volume = 1.0;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "Sample");
	mixer = duk_require_class_obj(ctx, 0, "Mixer");
	if (num_args >= 2) {
		duk_require_object_coercible(ctx, 1);
		duk_get_prop_string(ctx, 1, "volume");
		if (!duk_is_undefined(ctx, -1))
			volume = duk_require_number(ctx, -3);
		duk_get_prop_string(ctx, 1, "pan");
		if (!duk_is_undefined(ctx, -1))
			pan = duk_require_number(ctx, -2);
		duk_get_prop_string(ctx, 1, "speed");
		if (!duk_is_undefined(ctx, -1))
			speed = duk_require_number(ctx, -1);
	}

	sample_set_gain(sample, volume);
	sample_set_pan(sample, pan);
	sample_set_speed(sample, speed);
	sample_play(sample, mixer);
	return 0;
}

static duk_ret_t
js_Sample_stopAll(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "Sample");

	sample_stop_all(sample);
	return 0;
}

static duk_ret_t
js_new_Server(duk_context* ctx)
{
	int       max_backlog;
	int       num_args;
	int       port;
	server_t* server;

	num_args = duk_get_top(ctx);
	port = duk_require_int(ctx, 0);
	max_backlog = num_args >= 2 ? duk_require_int(ctx, 1) : 16;
	if (max_backlog <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid backlog size", max_backlog);

	if (server = server_new(NULL, port, 1024, max_backlog))
		duk_push_class_obj(ctx, "Server", server);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_finalize(duk_context* ctx)
{
	server_t* server;

	server = duk_require_class_obj(ctx, 0, "Server");
	server_free(server);
	return 0;
}

static duk_ret_t
js_Server_accept(duk_context* ctx)
{
	socket_t* new_socket;
	server_t* server;

	duk_push_this(ctx);
	server = duk_require_class_obj(ctx, -1, "Server");

	if (server == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "server has shut down");
	new_socket = server_accept(server);
	if (new_socket != NULL)
		duk_push_class_obj(ctx, "Socket", new_socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_close(duk_context* ctx)
{
	server_t* server;

	duk_push_this(ctx);
	server = duk_require_class_obj(ctx, -1, "Server");

	duk_set_class_ptr(ctx, -1, NULL);
	server_free(server);
	return 0;
}

static duk_ret_t
js_Shader_get_Default(duk_context* ctx)
{
	shader_t* shader;

	if (!(shader = galileo_shader()))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't compile default shaders");
	duk_push_class_obj(ctx, "Shader", shader_ref(shader));

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_Shader(duk_context* ctx)
{
	const char* fs_filename;
	const char* vs_filename;
	shader_t*   shader;

	if (!duk_is_object(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "options must be an object");
	if (duk_get_prop_string(ctx, 0, "vertex"), !duk_is_string(ctx, -1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "'vertex' must be a string");
	if (duk_get_prop_string(ctx, 0, "fragment"), !duk_is_string(ctx, -1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "'fragment' must be a string");
	duk_pop_2(ctx);

	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_path(ctx, -2, NULL, false, false);
	fs_filename = duk_require_path(ctx, -1, NULL, false, false);
	duk_pop_2(ctx);
	if (!(shader = shader_new(vs_filename, fs_filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't compile shader program");
	duk_push_class_obj(ctx, "Shader", shader);
	return 1;
}

static duk_ret_t
js_Shader_finalize(duk_context* ctx)
{
	shader_t* shader = duk_require_class_obj(ctx, 0, "Shader");

	shader_free(shader);
	return 0;
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	ibo_t*       ibo = NULL;
	int          num_args;
	shape_t*     shape;
	image_t*     texture = NULL;
	shape_type_t type;
	vbo_t*       vbo;

	num_args = duk_get_top(ctx);
	type = duk_require_int(ctx, 0);
	if (duk_is_class_obj(ctx, 1, "Texture") || duk_is_null(ctx, 1)) {
		texture = !duk_is_null(ctx, 1) ? duk_require_class_obj(ctx, 1, "Texture") : NULL;
		vbo = duk_require_class_obj(ctx, 2, "VertexList");
		if (num_args >= 4)
			ibo = duk_require_class_obj(ctx, 3, "IndexList");
	}
	else {
		vbo = duk_require_class_obj(ctx, 1, "VertexList");
		if (num_args >= 3)
			ibo = duk_require_class_obj(ctx, 2, "IndexList");
	}

	if (type < 0 || type >= SHAPE_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid ShapeType constant");

	shape = shape_new(vbo, ibo, type, texture);
	duk_push_class_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_class_obj(ctx, 0, "Shape");
	shape_free(shape);
	return 0;
}

static duk_ret_t
js_Shape_get_indexList(duk_context* ctx)
{
	ibo_t*   ibo;
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");

	ibo = shape_get_ibo(shape);
	if (ibo != NULL)
		duk_push_class_obj(ctx, "IndexList", ibo_ref(ibo));
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Shape_get_texture(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");

	texture = shape_get_texture(shape);
	if (texture != NULL)
		duk_push_class_obj(ctx, "Texture", image_ref(texture));
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Shape_get_vertexList(duk_context* ctx)
{
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");

	duk_push_class_obj(ctx, "VertexList", vbo_ref(shape_get_vbo(shape)));
	return 1;
}

static duk_ret_t
js_Shape_set_indexList(duk_context* ctx)
{
	ibo_t*   ibo = NULL;
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");
	if (!duk_is_null(ctx, 0))
		ibo = duk_require_class_obj(ctx, 0, "IndexList");

	shape_set_ibo(shape, ibo);
	return 0;
}

static duk_ret_t
js_Shape_set_texture(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture = NULL;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");
	if (!duk_is_null(ctx, 0))
		texture = duk_require_class_obj(ctx, 0, "Texture");

	shape_set_texture(shape, texture);
	return 0;
}

static duk_ret_t
js_Shape_set_vertexList(duk_context* ctx)
{
	shape_t* shape;
	vbo_t*   vbo;

	duk_push_this(ctx);
	shape = duk_require_class_obj(ctx, -1, "Shape");
	vbo = duk_require_class_obj(ctx, 0, "VertexList");

	shape_set_vbo(shape, vbo);
	return 0;
}

static duk_ret_t
js_Shape_draw(duk_context* ctx)
{
	int          num_args;
	shape_t*     shape;
	image_t*     surface = NULL;
	transform_t* transform = NULL;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	shape = duk_require_class_obj(ctx, -1, "Shape");
	if (num_args >= 1)
		surface = duk_require_class_obj(ctx, 0, "Surface");
	if (num_args >= 2)
		transform = duk_require_class_obj(ctx, 1, "Transform");

	if (ibo_len(shape_get_ibo(shape)) > vbo_len(shape_get_vbo(shape)))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "vertex list too small");

	shape_draw(shape, surface, transform);
	return 0;
}

static duk_ret_t
js_new_Socket(duk_context* ctx)
{
	const char* hostname = NULL;
	int         num_args;
	int         port;
	socket_t*   socket;

	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");

	num_args = duk_get_top(ctx);
	if (num_args >= 2) {
		hostname = duk_require_string(ctx, 0);
		port = duk_require_int(ctx, 1);
	}

	if (!(socket = socket_new(1024)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create TCP socket");
	if (hostname != NULL && !socket_connect(socket, hostname, port))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't connect to %s", hostname);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Socket", socket);
	return 0;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_class_obj(ctx, 0, "Socket");

	socket_free(socket);
	return 0;
}

static duk_ret_t
js_Socket_get_bytesPending(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");

	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	duk_push_uint(ctx, (duk_uint_t)socket_peek(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_connected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");

	if (socket != NULL)
		duk_push_boolean(ctx, socket_connected(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_Socket_get_remoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");

	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!socket_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket disconnected");
	duk_push_string(ctx, socket_hostname(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_remotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");

	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!socket_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket disconnected");
	duk_push_int(ctx, socket_port(socket));
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");
	
	duk_set_class_ptr(ctx, -1, NULL);
	socket_free(socket);
	return 0;
}

static duk_ret_t
js_Socket_connectTo(duk_context* ctx)
{
	const char* hostname;
	int         port;
	socket_t*   socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");
	hostname = duk_require_string(ctx, 0);
	port = duk_require_int(ctx, 1);

	if (!socket_connect(socket, hostname, port))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't connect to %s", hostname);
	return 0;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	void*      buffer;
	size_t     bytes_read;
	duk_size_t num_bytes;
	socket_t*  socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");

	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!socket_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket disconnected");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	bytes_read = socket_read(socket, buffer, num_bytes);
	duk_push_buffer_object(ctx, -1, 0, bytes_read, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	const uint8_t* payload;
	socket_t*      socket;
	duk_size_t     write_size;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "Socket");
	payload = duk_require_buffer_data(ctx, 0, &write_size);

	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!socket_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket disconnected");
	socket_write(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_new_Sound(duk_context* ctx)
{
	const char* filename;
	duk_int_t   num_args;
	sound_t*    sound;

	num_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, NULL, false, false);

	if (!(sound = sound_new(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load sound `%s`", filename);
	duk_push_class_obj(ctx, "Sound", sound);
	return 1;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	sound = duk_require_class_obj(ctx, 0, "Sound");
	sound_free(sound);
	return 0;
}

static duk_ret_t
js_Sound_get_fileName(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_string(ctx, sound_path(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_length(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_len(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_pan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_pan(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_pan(duk_context* ctx)
{
	float    new_pan;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");
	new_pan = duk_require_number(ctx, 0);

	sound_set_pan(sound, new_pan);
	return 0;
}

static duk_ret_t
js_Sound_get_speed(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_speed(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_speed(duk_context* ctx)
{
	float new_speed = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	sound_set_speed(sound, new_speed);
	return 0;
}

static duk_ret_t
js_Sound_get_playing(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_boolean(ctx, sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_position(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_tell(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_position(duk_context* ctx)
{
	double   new_pos;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");
	new_pos = duk_require_number(ctx, 0);

	sound_seek(sound, new_pos);
	return 0;
}

static duk_ret_t
js_Sound_get_repeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_boolean(ctx, sound_repeat(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_repeat(duk_context* ctx)
{
	bool is_looped = duk_require_boolean(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	sound_set_repeat(sound, is_looped);
	return 0;
}

static duk_ret_t
js_Sound_get_volume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_gain(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	sound_set_gain(sound, volume);
	return 0;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	sound_pause(sound, true);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	mixer_t* mixer;
	int      num_args;
	sound_t* sound;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	if (num_args < 1)
		sound_pause(sound, false);
	else {
		mixer = duk_require_class_obj(ctx, 0, "Mixer");
		sound_play(sound, mixer);
	}
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "Sound");

	sound_stop(sound);
	return 0;
}

static duk_ret_t
js_new_SoundStream(duk_context* ctx)
{
	stream_t* stream;
	int       argc;
	int       frequency;
	int       bits;
	int       channels;

	argc = duk_get_top(ctx);
	frequency = argc >= 1 ? duk_require_int(ctx, 0) : 22050;
	bits = argc >= 2 ? duk_require_int(ctx, 1) : 8;
	channels = argc >= 3 ? duk_require_int(ctx, 1) : 1;
	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid audio bit depth", bits);
	if (!(stream = stream_new(frequency, bits, channels)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "stream creation failed");
	duk_push_class_obj(ctx, "SoundStream", stream);
	return 1;
}

static duk_ret_t
js_SoundStream_finalize(duk_context* ctx)
{
	stream_t* stream;

	stream = duk_require_class_obj(ctx, 0, "SoundStream");
	stream_free(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_get_bufferSize(duk_context* ctx)
{
	stream_t*    stream;

	duk_push_this(ctx);
	stream = duk_require_class_obj(ctx, -1, "SoundStream");

	duk_push_number(ctx, stream_bytes_left(stream));
	return 1;
}

static duk_ret_t
js_SoundStream_buffer(duk_context* ctx)
{
	// SoundStream:buffer(data);
	// Arguments:
	//     data: An ArrayBuffer or TypedArray containing the audio data
	//           to feed into the stream buffer.

	const void* data;
	duk_size_t  size;
	stream_t*   stream;

	duk_push_this(ctx);
	stream = duk_require_class_obj(ctx, -1, "SoundStream");

	data = duk_require_buffer_data(ctx, 0, &size);
	stream_buffer(stream, data, size);
	return 0;
}

static duk_ret_t
js_SoundStream_pause(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_class_obj(ctx, -1, "SoundStream");

	stream_pause(stream, true);
	return 0;
}

static duk_ret_t
js_SoundStream_play(duk_context* ctx)
{
	mixer_t*  mixer;
	int       num_args;
	stream_t* stream;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	stream = duk_require_class_obj(ctx, -1, "SoundStream");

	if (num_args < 1)
		stream_pause(stream, false);
	else {
		mixer = duk_require_class_obj(ctx, 0, "Mixer");
		stream_play(stream, mixer);
	}
	return 0;
}

static duk_ret_t
js_SoundStream_stop(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_class_obj(ctx, -1, "SoundStream");

	stream_stop(stream);
	return 0;
}

static duk_ret_t
js_new_Surface(duk_context* ctx)
{
	int         n_args;
	const char* filename;
	color_t     fill_color;
	image_t*    image;
	image_t*    src_image;
	int         width, height;

	n_args = duk_get_top(ctx);
	if (n_args >= 2) {
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = n_args >= 3 ? duk_pegasus_require_color(ctx, 2) : color_new(0, 0, 0, 0);
		if (!(image = image_new(width, height)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create surface");
		image_fill(image, fill_color);
	}
	else if (duk_is_class_obj(ctx, 0, "Texture")) {
		src_image = duk_require_class_obj(ctx, 0, "Texture");
		if (!(image = image_clone(src_image)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create surface");
	}
	else {
		filename = duk_require_path(ctx, 0, NULL, false, false);
		if (!(image = image_load(filename)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load image `%s`", filename);
	}
	duk_push_class_obj(ctx, "Surface", image);
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_class_obj(ctx, 0, "Surface");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");

	duk_push_int(ctx, image_height(image));
	return 1;
}

static duk_ret_t
js_Surface_get_transform(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");

	duk_push_class_obj(ctx, "Transform", image_get_transform(image));
	return 1;
}

static duk_ret_t
js_Surface_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");

	duk_push_int(ctx, image_width(image));
	return 1;
}

static duk_ret_t
js_Surface_set_transform(duk_context* ctx)
{
	image_t*     image;
	transform_t* transform;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");
	transform = duk_require_class_obj(ctx, 0, "Transform");

	image_set_transform(image, transform);
	return 0;
}

static duk_ret_t
js_Surface_clipTo(duk_context* ctx)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	width = duk_require_int(ctx, 2);
	height = duk_require_int(ctx, 3);

	image_set_scissor(image, new_rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_Surface_toTexture(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Surface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "image creation failed");
	duk_push_class_obj(ctx, "Texture", new_image);
	return 1;
}

static duk_ret_t
js_new_Texture(duk_context* ctx)
{
	const color_t* buffer;
	size_t         buffer_size;
	const char*    filename;
	color_t        fill_color;
	int            height;
	image_t*       image;
	image_lock_t*  lock;
	int            num_args;
	color_t*       p_line;
	image_t*       src_image;
	int            width;

	int y;

	num_args = duk_get_top(ctx);
	if (num_args >= 3 && duk_is_class_obj(ctx, 2, "Color")) {
		// create an Image filled with a single pixel value
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = duk_pegasus_require_color(ctx, 2);
		if (!(image = image_new(width, height)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "image creation failed");
		image_fill(image, fill_color);
	}
	else if (num_args >= 3 && (buffer = duk_get_buffer_data(ctx, 2, &buffer_size))) {
		// create an Image from an ArrayBuffer or similar object
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		if (buffer_size < width * height * sizeof(color_t))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not enough data in buffer");
		if (!(image = image_new(width, height)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "image creation failed");
		if (!(lock = image_lock(image))) {
			image_free(image);
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "image lock failed");
		}
		p_line = lock->pixels;
		for (y = 0; y < height; ++y) {
			memcpy(p_line, buffer + y * width, width * sizeof(color_t));
			p_line += lock->pitch;
		}
		image_unlock(image, lock);
	}
	else if (duk_is_class_obj(ctx, 0, "Surface")) {
		// create an Image from a Surface
		src_image = duk_require_class_obj(ctx, 0, "Surface");
		if (!(image = image_clone(src_image)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "image creation failed");
	}
	else {
		// create an Image by loading an image file
		filename = duk_require_path(ctx, 0, NULL, false, false);
		if (!(image = image_load(filename)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load image `%s`", filename);
	}
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Texture", image);
	return 0;
}

static duk_ret_t
js_Texture_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_class_obj(ctx, 0, "Texture");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Texture_get_fileName(duk_context* ctx)
{
	image_t*    image;
	const char* path;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Texture");

	if (path = image_path(image))
		duk_push_string(ctx, path);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Texture_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Texture");

	duk_push_int(ctx, image_height(image));
	return 1;
}

static duk_ret_t
js_Texture_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "Texture");

	duk_push_int(ctx, image_width(image));
	return 1;
}

static duk_ret_t
js_new_Transform(duk_context* ctx)
{
	transform_t* transform;

	transform = transform_new();
	duk_push_class_obj(ctx, "Transform", transform);
	return 1;
}

static duk_ret_t
js_Transform_finalize(duk_context* ctx)
{
	transform_t* transform;

	transform = duk_require_class_obj(ctx, 0, "Transform");

	transform_free(transform);
	return 0;
}

static duk_ret_t
js_Transform_get_matrix(duk_context* ctx)
{
	int          magic;
	transform_t* transform;
	float*       values;

	int i, j;

	magic = duk_get_current_magic(ctx);

	if (magic == 0) {
		// on first access: set up getters and setters
		duk_push_this(ctx);
		transform = duk_require_class_obj(ctx, -1, "Transform");
		values = transform_values(transform);
		duk_push_object(ctx);
		for (i = 0; i < 4; ++i) {
			duk_push_int(ctx, i);
			duk_push_object(ctx);
			for (j = 0; j < 4; ++j) {
				duk_push_int(ctx, j);
				duk_push_c_function(ctx, js_Transform_get_matrix, DUK_VARARGS);
				duk_set_magic(ctx, -1, 1 + (j * 4 + i));
				duk_push_this(ctx);
				duk_put_prop_string(ctx, -2, "\xFF" "transform");
				duk_push_c_function(ctx, js_Transform_set_matrix, DUK_VARARGS);
				duk_set_magic(ctx, -1, 1 + (j * 4 + i));
				duk_push_this(ctx);
				duk_put_prop_string(ctx, -2, "\xFF" "transform");
				duk_def_prop(ctx, -4, DUK_DEFPROP_SET_ENUMERABLE
					| DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);
			}
			duk_def_prop(ctx, -3, DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);
		}
		
		// store the generated object so we don't have to keep generating it on
		// every access
		duk_push_this(ctx);
		duk_push_string(ctx, "matrix");
		duk_dup(ctx, -3);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
			| DUK_DEFPROP_CLEAR_ENUMERABLE
			| DUK_DEFPROP_CLEAR_WRITABLE
			| DUK_DEFPROP_SET_CONFIGURABLE);
		duk_pop(ctx);
		return 1;
	}
	else {
		duk_push_current_function(ctx);
		duk_get_prop_string(ctx, -1, "\xFF" "transform");
		transform = duk_require_class_obj(ctx, -1, "Transform");
		values = transform_values(transform);
		duk_push_number(ctx, values[magic - 1]);
		return 1;
	}
}

static duk_ret_t
js_Transform_set_matrix(duk_context* ctx)
{
	int          magic;
	float        new_value;
	transform_t* transform;
	float*       values;

	magic = duk_get_current_magic(ctx);
	new_value = duk_require_number(ctx, 0);

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "transform");
	transform = duk_require_class_obj(ctx, -1, "Transform");
	values = transform_values(transform);
	values[magic - 1] = new_value;
	return 0;
}

static duk_ret_t
js_Transform_compose(duk_context* ctx)
{
	transform_t* other;
	transform_t* transform;

	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	other = duk_require_class_obj(ctx, 0, "Transform");

	transform_compose(transform, other);
	return 1;
}

static duk_ret_t
js_Transform_identity(duk_context* ctx)
{
	transform_t* transform;

	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");

	transform_identity(transform);
	return 1;
}

static duk_ret_t
js_Transform_project2D(duk_context* ctx)
{
	int          num_args;
	transform_t* transform;
	float        x1, x2;
	float        y1, y2;
	float        z1 = -1.0f;
	float        z2 = 1.0f;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	x1 = duk_require_number(ctx, 0);
	y1 = duk_require_number(ctx, 1);
	x2 = duk_require_number(ctx, 2);
	y2 = duk_require_number(ctx, 3);
	if (num_args >= 6) {
		z1 = duk_require_number(ctx, 4);
		z2 = duk_require_number(ctx, 5);
	}

	transform_orthographic(transform, x1, y1, x2, y2, z1, z2);
	return 1;
}

static duk_ret_t
js_Transform_project3D(duk_context* ctx)
{
	float        aspect;
	float        fov;
	int          num_args;
	float        fw, fh;
	transform_t* transform;
	float        z1, z2;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	fov = duk_require_number(ctx, 0);
	aspect = duk_require_number(ctx, 1);
	z1 = duk_require_number(ctx, 2);
	z2 = duk_require_number(ctx, 3);

	fh = tan(fov / 360 * M_PI) * z1;
	fw = fh * aspect;
	transform_perspective(transform, -fw, -fh, fw, fh, z1, z2);
	return 1;
}

static duk_ret_t
js_Transform_rotate(duk_context* ctx)
{
	int          num_args;
	float        theta;
	transform_t* transform;
	float        vx = 0.0;
	float        vy = 0.0;
	float        vz = 1.0;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	theta = duk_require_number(ctx, 0);
	if (num_args >= 2) {
		vx = duk_require_number(ctx, 1);
		vy = duk_require_number(ctx, 2);
		vz = duk_require_number(ctx, 3);
	}

	transform_rotate(transform, theta, vx, vy, vz);
	return 1;
}

static duk_ret_t
js_Transform_scale(duk_context* ctx)
{
	int          num_args;
	float        sx;
	float        sy;
	float        sz = 1.0;
	transform_t* transform;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	sx = duk_require_number(ctx, 0);
	sy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		sz = duk_require_number(ctx, 2);

	transform_scale(transform, sx, sy, sz);
	return 1;
}

static duk_ret_t
js_Transform_translate(duk_context* ctx)
{
	float        dx;
	float        dy;
	float        dz = 0.0;
	int          num_args;
	transform_t* transform;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	transform = duk_require_class_obj(ctx, -1, "Transform");
	dx = duk_require_number(ctx, 0);
	dy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		dz = duk_require_number(ctx, 2);

	transform_translate(transform, dx, dy, dz);
	return 1;
}

static duk_ret_t
js_new_VertexList(duk_context* ctx)
{
	int       num_entries;
	duk_idx_t stack_idx;
	vbo_t*    vbo;
	vertex_t  vertex;

	int i;

	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "expected an array");

	num_entries = (int)duk_get_length(ctx, 0);
	if (num_entries == 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "empty list not allowed");
	vbo = vbo_new();
	for (i = 0; i < num_entries; ++i) {
		duk_get_prop_index(ctx, 0, i);
		duk_require_object_coercible(ctx, -1);
		stack_idx = duk_normalize_index(ctx, -1);
		vertex.x = duk_get_prop_string(ctx, stack_idx, "x") ? duk_require_number(ctx, -1) : 0.0;
		vertex.y = duk_get_prop_string(ctx, stack_idx, "y") ? duk_require_number(ctx, -1) : 0.0;
		vertex.z = duk_get_prop_string(ctx, stack_idx, "z") ? duk_require_number(ctx, -1) : 0.0;
		if (duk_get_prop_string(ctx, stack_idx, "u"))
			vertex.u = duk_require_number(ctx, -1);
		else
			vertex.u = 0;
		if (duk_get_prop_string(ctx, stack_idx, "v"))
			vertex.v = duk_require_number(ctx, -1);
		else
			vertex.v = 0;
		vertex.color = duk_get_prop_string(ctx, stack_idx, "color")
			? duk_pegasus_require_color(ctx, -1)
			: color_new(255, 255, 255, 255);
		vbo_add_vertex(vbo, vertex);
		duk_pop_n(ctx, 7);
	}
	if (!vbo_upload(vbo)) {
		vbo_free(vbo);
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "upload to GPU failed");
	}
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "VertexList", vbo);
	return 1;
}

static duk_ret_t
js_VertexList_finalize(duk_context* ctx)
{
	vbo_t* vbo;

	vbo = duk_require_class_obj(ctx, 0, "VertexList");
	vbo_free(vbo);
	return 0;
}
