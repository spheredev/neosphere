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

// WARNING: the Sphere v1 API contains a lot of redundancy and as a result its implementation here
//          duplicates code often.  it would be nice to prune some of this mess back, but Sphere v1
//          is deprecated so I have more important things to do.  maybe someday...

#include "minisphere.h"
#include "vanilla.h"

#include "animation.h"
#include "api.h"
#include "async.h"
#include "audio.h"
#include "byte_array.h"
#include "debugger.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "legacy.h"
#include "logger.h"
#include "map_engine.h"
#include "spriteset.h"
#include "windowstyle.h"

#define API_VERSION        2.0
#define API_VERSION_STRING "v2.0"

static duk_ret_t js_Abort                            (duk_context* ctx);
static duk_ret_t js_AddTrigger                       (duk_context* ctx);
static duk_ret_t js_AddZone                          (duk_context* ctx);
static duk_ret_t js_ApplyColorMask                   (duk_context* ctx);
static duk_ret_t js_AreKeysLeft                      (duk_context* ctx);
static duk_ret_t js_AreZonesAt                       (duk_context* ctx);
static duk_ret_t js_AttachCamera                     (duk_context* ctx);
static duk_ret_t js_AttachInput                      (duk_context* ctx);
static duk_ret_t js_AttachPlayerInput                (duk_context* ctx);
static duk_ret_t js_BezierCurve                      (duk_context* ctx);
static duk_ret_t js_BindJoystickButton               (duk_context* ctx);
static duk_ret_t js_BindKey                          (duk_context* ctx);
static duk_ret_t js_BlendColors                      (duk_context* ctx);
static duk_ret_t js_CallDefaultMapScript             (duk_context* ctx);
static duk_ret_t js_CallDefaultPersonScript          (duk_context* ctx);
static duk_ret_t js_CallMapScript                    (duk_context* ctx);
static duk_ret_t js_CallPersonScript                 (duk_context* ctx);
static duk_ret_t js_ChangeMap                        (duk_context* ctx);
static duk_ret_t js_ClearPersonCommands              (duk_context* ctx);
static duk_ret_t js_CreatePerson                     (duk_context* ctx);
static duk_ret_t js_CreateByteArray                  (duk_context* ctx);
static duk_ret_t js_CreateByteArrayFromString        (duk_context* ctx);
static duk_ret_t js_CreateColor                      (duk_context* ctx);
static duk_ret_t js_CreateColorMatrix                (duk_context* ctx);
static duk_ret_t js_CreateDirectory                  (duk_context* ctx);
static duk_ret_t js_CreateSpriteset                  (duk_context* ctx);
static duk_ret_t js_CreateStringFromByteArray        (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode             (duk_context* ctx);
static duk_ret_t js_CreateSurface                    (duk_context* ctx);
static duk_ret_t js_DeflateByteArray                 (duk_context* ctx);
static duk_ret_t js_DestroyPerson                    (duk_context* ctx);
static duk_ret_t js_DetachCamera                     (duk_context* ctx);
static duk_ret_t js_DetachInput                      (duk_context* ctx);
static duk_ret_t js_DetachPlayerInput                (duk_context* ctx);
static duk_ret_t js_DoEvents                         (duk_context* ctx);
static duk_ret_t js_DoesFileExist                    (duk_context* ctx);
static duk_ret_t js_DoesPersonExist                  (duk_context* ctx);
static duk_ret_t js_Delay                            (duk_context* ctx);
static duk_ret_t js_EvaluateScript                   (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript             (duk_context* ctx);
static duk_ret_t js_ExecuteGame                      (duk_context* ctx);
static duk_ret_t js_ExecuteTrigger                   (duk_context* ctx);
static duk_ret_t js_ExecuteZoneScript                (duk_context* ctx);
static duk_ret_t js_ExecuteZones                     (duk_context* ctx);
static duk_ret_t js_Exit                             (duk_context* ctx);
static duk_ret_t js_ExitMapEngine                    (duk_context* ctx);
static duk_ret_t js_FilledCircle                     (duk_context* ctx);
static duk_ret_t js_FilledComplex                    (duk_context* ctx);
static duk_ret_t js_FilledEllipse                    (duk_context* ctx);
static duk_ret_t js_FlipScreen                       (duk_context* ctx);
static duk_ret_t js_FollowPerson                     (duk_context* ctx);
static duk_ret_t js_GarbageCollect                   (duk_context* ctx);
static duk_ret_t js_GetActingPerson                  (duk_context* ctx);
static duk_ret_t js_GetCameraPerson                  (duk_context* ctx);
static duk_ret_t js_GetCameraX                       (duk_context* ctx);
static duk_ret_t js_GetCameraY                       (duk_context* ctx);
static duk_ret_t js_GetClippingRectangle             (duk_context* ctx);
static duk_ret_t js_GetCurrentMap                    (duk_context* ctx);
static duk_ret_t js_GetCurrentPerson                 (duk_context* ctx);
static duk_ret_t js_GetCurrentTrigger                (duk_context* ctx);
static duk_ret_t js_GetCurrentZone                   (duk_context* ctx);
static duk_ret_t js_GetDirectoryList                 (duk_context* ctx);
static duk_ret_t js_GetFileList                      (duk_context* ctx);
static duk_ret_t js_GetFrameRate                     (duk_context* ctx);
static duk_ret_t js_GetGameList                      (duk_context* ctx);
static duk_ret_t js_GetInputPerson                   (duk_context* ctx);
static duk_ret_t js_GetJoystickAxis                  (duk_context* ctx);
static duk_ret_t js_GetKey                           (duk_context* ctx);
static duk_ret_t js_GetKeyString                     (duk_context* ctx);
static duk_ret_t js_GetLayerAngle                    (duk_context* ctx);
static duk_ret_t js_GetLayerHeight                   (duk_context* ctx);
static duk_ret_t js_GetLayerMask                     (duk_context* ctx);
static duk_ret_t js_GetLayerName                     (duk_context* ctx);
static duk_ret_t js_GetLayerWidth                    (duk_context* ctx);
static duk_ret_t js_GetLocalAddress                  (duk_context* ctx);
static duk_ret_t js_GetLocalName                     (duk_context* ctx);
static duk_ret_t js_GetMapEngine                     (duk_context* ctx);
static duk_ret_t js_GetMapEngineFrameRate            (duk_context* ctx);
static duk_ret_t js_GetMouseWheelEvent               (duk_context* ctx);
static duk_ret_t js_GetMouseX                        (duk_context* ctx);
static duk_ret_t js_GetMouseY                        (duk_context* ctx);
static duk_ret_t js_GetNextAnimatedTile              (duk_context* ctx);
static duk_ret_t js_GetNumJoysticks                  (duk_context* ctx);
static duk_ret_t js_GetNumJoystickAxes               (duk_context* ctx);
static duk_ret_t js_GetNumJoystickButtons            (duk_context* ctx);
static duk_ret_t js_GetNumLayers                     (duk_context* ctx);
static duk_ret_t js_GetNumMouseWheelEvents           (duk_context* ctx);
static duk_ret_t js_GetNumTiles                      (duk_context* ctx);
static duk_ret_t js_GetNumTriggers                   (duk_context* ctx);
static duk_ret_t js_GetNumZones                      (duk_context* ctx);
static duk_ret_t js_GetObstructingPerson             (duk_context* ctx);
static duk_ret_t js_GetObstructingTile               (duk_context* ctx);
static duk_ret_t js_GetPersonAngle                   (duk_context* ctx);
static duk_ret_t js_GetPersonBase                    (duk_context* ctx);
static duk_ret_t js_GetPersonData                    (duk_context* ctx);
static duk_ret_t js_GetPersonDirection               (duk_context* ctx);
static duk_ret_t js_GetPersonFollowDistance          (duk_context* ctx);
static duk_ret_t js_GetPersonFollowers               (duk_context* ctx);
static duk_ret_t js_GetPersonFrame                   (duk_context* ctx);
static duk_ret_t js_GetPersonFrameNext               (duk_context* ctx);
static duk_ret_t js_GetPersonFrameRevert             (duk_context* ctx);
static duk_ret_t js_GetPersonIgnoreList              (duk_context* ctx);
static duk_ret_t js_GetPersonLayer                   (duk_context* ctx);
static duk_ret_t js_GetPersonLeader                  (duk_context* ctx);
static duk_ret_t js_GetPersonList                    (duk_context* ctx);
static duk_ret_t js_GetPersonMask                    (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetX                 (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetY                 (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedX                  (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedY                  (duk_context* ctx);
static duk_ret_t js_GetPersonSpriteset               (duk_context* ctx);
static duk_ret_t js_GetPersonValue                   (duk_context* ctx);
static duk_ret_t js_GetPersonX                       (duk_context* ctx);
static duk_ret_t js_GetPersonY                       (duk_context* ctx);
static duk_ret_t js_GetPersonXFloat                  (duk_context* ctx);
static duk_ret_t js_GetPersonYFloat                  (duk_context* ctx);
static duk_ret_t js_GetPlayerKey                     (duk_context* ctx);
static duk_ret_t js_GetScreenHeight                  (duk_context* ctx);
static duk_ret_t js_GetScreenWidth                   (duk_context* ctx);
static duk_ret_t js_GetSystemArrow                   (duk_context* ctx);
static duk_ret_t js_GetSystemDownArrow               (duk_context* ctx);
static duk_ret_t js_GetSystemFont                    (duk_context* ctx);
static duk_ret_t js_GetSystemUpArrow                 (duk_context* ctx);
static duk_ret_t js_GetSystemWindowStyle             (duk_context* ctx);
static duk_ret_t js_GetTalkActivationButton          (duk_context* ctx);
static duk_ret_t js_GetTalkActivationKey             (duk_context* ctx);
static duk_ret_t js_GetTalkDistance                  (duk_context* ctx);
static duk_ret_t js_GetTile                          (duk_context* ctx);
static duk_ret_t js_GetTileDelay                     (duk_context* ctx);
static duk_ret_t js_GetTileHeight                    (duk_context* ctx);
static duk_ret_t js_GetTileImage                     (duk_context* ctx);
static duk_ret_t js_GetTileName                      (duk_context* ctx);
static duk_ret_t js_GetTileSurface                   (duk_context* ctx);
static duk_ret_t js_GetTileWidth                     (duk_context* ctx);
static duk_ret_t js_GetTime                          (duk_context* ctx);
static duk_ret_t js_GetToggleState                   (duk_context* ctx);
static duk_ret_t js_GetTriggerLayer                  (duk_context* ctx);
static duk_ret_t js_GetTriggerX                      (duk_context* ctx);
static duk_ret_t js_GetTriggerY                      (duk_context* ctx);
static duk_ret_t js_GetVersion                       (duk_context* ctx);
static duk_ret_t js_GetVersionString                 (duk_context* ctx);
static duk_ret_t js_GetZoneHeight                    (duk_context* ctx);
static duk_ret_t js_GetZoneLayer                     (duk_context* ctx);
static duk_ret_t js_GetZoneSteps                     (duk_context* ctx);
static duk_ret_t js_GetZoneWidth                     (duk_context* ctx);
static duk_ret_t js_GetZoneX                         (duk_context* ctx);
static duk_ret_t js_GetZoneY                         (duk_context* ctx);
static duk_ret_t js_GrabImage                        (duk_context* ctx);
static duk_ret_t js_GrabSurface                      (duk_context* ctx);
static duk_ret_t js_GradientCircle                   (duk_context* ctx);
static duk_ret_t js_GradientComplex                  (duk_context* ctx);
static duk_ret_t js_GradientEllipse                  (duk_context* ctx);
static duk_ret_t js_GradientLine                     (duk_context* ctx);
static duk_ret_t js_GradientRectangle                (duk_context* ctx);
static duk_ret_t js_GradientTriangle                 (duk_context* ctx);
static duk_ret_t js_HashByteArray                    (duk_context* ctx);
static duk_ret_t js_HashFromFile                     (duk_context* ctx);
static duk_ret_t js_IgnorePersonObstructions         (duk_context* ctx);
static duk_ret_t js_IgnoreTileObstructions           (duk_context* ctx);
static duk_ret_t js_InflateByteArray                 (duk_context* ctx);
static duk_ret_t js_IsAnyKeyPressed                  (duk_context* ctx);
static duk_ret_t js_IsCameraAttached                 (duk_context* ctx);
static duk_ret_t js_IsCommandQueueEmpty              (duk_context* ctx);
static duk_ret_t js_IsIgnoringPersonObstructions     (duk_context* ctx);
static duk_ret_t js_IsIgnoringTileObstructions       (duk_context* ctx);
static duk_ret_t js_IsInputAttached                  (duk_context* ctx);
static duk_ret_t js_IsJoystickButtonPressed          (duk_context* ctx);
static duk_ret_t js_IsKeyPressed                     (duk_context* ctx);
static duk_ret_t js_IsLayerReflective                (duk_context* ctx);
static duk_ret_t js_IsLayerVisible                   (duk_context* ctx);
static duk_ret_t js_IsMapEngineRunning               (duk_context* ctx);
static duk_ret_t js_IsMouseButtonPressed             (duk_context* ctx);
static duk_ret_t js_IsPersonObstructed               (duk_context* ctx);
static duk_ret_t js_IsPersonVisible                  (duk_context* ctx);
static duk_ret_t js_IsTriggerAt                      (duk_context* ctx);
static duk_ret_t js_Line                             (duk_context* ctx);
static duk_ret_t js_LineSeries                       (duk_context* ctx);
static duk_ret_t js_ListenOnPort                     (duk_context* ctx);
static duk_ret_t js_LoadAnimation                    (duk_context* ctx);
static duk_ret_t js_LoadFont                         (duk_context* ctx);
static duk_ret_t js_LoadImage                        (duk_context* ctx);
static duk_ret_t js_LoadSound                        (duk_context* ctx);
static duk_ret_t js_LoadSoundEffect                  (duk_context* ctx);
static duk_ret_t js_LoadSpriteset                    (duk_context* ctx);
static duk_ret_t js_LoadSurface                      (duk_context* ctx);
static duk_ret_t js_LoadWindowStyle                  (duk_context* ctx);
static duk_ret_t js_MapEngine                        (duk_context* ctx);
static duk_ret_t js_MapToScreenX                     (duk_context* ctx);
static duk_ret_t js_MapToScreenY                     (duk_context* ctx);
static duk_ret_t js_OpenAddress                      (duk_context* ctx);
static duk_ret_t js_OpenFile                         (duk_context* ctx);
static duk_ret_t js_OpenLog                          (duk_context* ctx);
static duk_ret_t js_OpenRawFile                      (duk_context* ctx);
static duk_ret_t js_OutlinedCircle                   (duk_context* ctx);
static duk_ret_t js_OutlinedComplex                  (duk_context* ctx);
static duk_ret_t js_OutlinedEllipse                  (duk_context* ctx);
static duk_ret_t js_OutlinedRectangle                (duk_context* ctx);
static duk_ret_t js_OutlinedRoundRectangle           (duk_context* ctx);
static duk_ret_t js_Point                            (duk_context* ctx);
static duk_ret_t js_PointSeries                      (duk_context* ctx);
static duk_ret_t js_Polygon                          (duk_context* ctx);
static duk_ret_t js_Print                            (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand               (duk_context* ctx);
static duk_ret_t js_QueuePersonScript                (duk_context* ctx);
static duk_ret_t js_Rectangle                        (duk_context* ctx);
static duk_ret_t js_RemoveDirectory                  (duk_context* ctx);
static duk_ret_t js_RemoveFile                       (duk_context* ctx);
static duk_ret_t js_RemoveTrigger                    (duk_context* ctx);
static duk_ret_t js_RemoveZone                       (duk_context* ctx);
static duk_ret_t js_Rename                           (duk_context* ctx);
static duk_ret_t js_RenderMap                        (duk_context* ctx);
static duk_ret_t js_ReplaceTilesOnLayer              (duk_context* ctx);
static duk_ret_t js_RequireScript                    (duk_context* ctx);
static duk_ret_t js_RequireSystemScript              (duk_context* ctx);
static duk_ret_t js_RestartGame                      (duk_context* ctx);
static duk_ret_t js_RoundRectangle                   (duk_context* ctx);
static duk_ret_t js_ScreenToMapX                     (duk_context* ctx);
static duk_ret_t js_ScreenToMapY                     (duk_context* ctx);
static duk_ret_t js_SetCameraX                       (duk_context* ctx);
static duk_ret_t js_SetCameraY                       (duk_context* ctx);
static duk_ret_t js_SetClippingRectangle             (duk_context* ctx);
static duk_ret_t js_SetColorMask                     (duk_context* ctx);
static duk_ret_t js_SetDefaultMapScript              (duk_context* ctx);
static duk_ret_t js_SetDefaultPersonScript           (duk_context* ctx);
static duk_ret_t js_SetDelayScript                   (duk_context* ctx);
static duk_ret_t js_SetFrameRate                     (duk_context* ctx);
static duk_ret_t js_SetLayerAngle                    (duk_context* ctx);
static duk_ret_t js_SetLayerHeight                   (duk_context* ctx);
static duk_ret_t js_SetLayerMask                     (duk_context* ctx);
static duk_ret_t js_SetLayerReflective               (duk_context* ctx);
static duk_ret_t js_SetLayerRenderer                 (duk_context* ctx);
static duk_ret_t js_SetLayerScaleFactorX             (duk_context* ctx);
static duk_ret_t js_SetLayerScaleFactorY             (duk_context* ctx);
static duk_ret_t js_SetLayerSize                     (duk_context* ctx);
static duk_ret_t js_SetLayerVisible                  (duk_context* ctx);
static duk_ret_t js_SetLayerWidth                    (duk_context* ctx);
static duk_ret_t js_SetMapEngineFrameRate            (duk_context* ctx);
static duk_ret_t js_SetMousePosition                 (duk_context* ctx);
static duk_ret_t js_SetNextAnimatedTile              (duk_context* ctx);
static duk_ret_t js_SetPersonAngle                   (duk_context* ctx);
static duk_ret_t js_SetPersonData                    (duk_context* ctx);
static duk_ret_t js_SetPersonDirection               (duk_context* ctx);
static duk_ret_t js_SetPersonFollowDistance          (duk_context* ctx);
static duk_ret_t js_SetPersonFrame                   (duk_context* ctx);
static duk_ret_t js_SetPersonFrameNext               (duk_context* ctx);
static duk_ret_t js_SetPersonFrameRevert             (duk_context* ctx);
static duk_ret_t js_SetPersonIgnoreList              (duk_context* ctx);
static duk_ret_t js_SetPersonLayer                   (duk_context* ctx);
static duk_ret_t js_SetPersonMask                    (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetX                 (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetY                 (duk_context* ctx);
static duk_ret_t js_SetPersonScaleAbsolute           (duk_context* ctx);
static duk_ret_t js_SetPersonScaleFactor             (duk_context* ctx);
static duk_ret_t js_SetPersonScript                  (duk_context* ctx);
static duk_ret_t js_SetPersonSpeed                   (duk_context* ctx);
static duk_ret_t js_SetPersonSpeedXY                 (duk_context* ctx);
static duk_ret_t js_SetPersonSpriteset               (duk_context* ctx);
static duk_ret_t js_SetPersonValue                   (duk_context* ctx);
static duk_ret_t js_SetPersonVisible                 (duk_context* ctx);
static duk_ret_t js_SetPersonX                       (duk_context* ctx);
static duk_ret_t js_SetPersonXYFloat                 (duk_context* ctx);
static duk_ret_t js_SetPersonY                       (duk_context* ctx);
static duk_ret_t js_SetRenderScript                  (duk_context* ctx);
static duk_ret_t js_SetTalkActivationButton          (duk_context* ctx);
static duk_ret_t js_SetTalkActivationKey             (duk_context* ctx);
static duk_ret_t js_SetTalkDistance                  (duk_context* ctx);
static duk_ret_t js_SetTile                          (duk_context* ctx);
static duk_ret_t js_SetTileDelay                     (duk_context* ctx);
static duk_ret_t js_SetTileImage                     (duk_context* ctx);
static duk_ret_t js_SetTileName                      (duk_context* ctx);
static duk_ret_t js_SetTileSurface                   (duk_context* ctx);
static duk_ret_t js_SetTriggerLayer                  (duk_context* ctx);
static duk_ret_t js_SetTriggerScript                 (duk_context* ctx);
static duk_ret_t js_SetTriggerXY                     (duk_context* ctx);
static duk_ret_t js_SetUpdateScript                  (duk_context* ctx);
static duk_ret_t js_SetZoneDimensions                (duk_context* ctx);
static duk_ret_t js_SetZoneLayer                     (duk_context* ctx);
static duk_ret_t js_SetZoneScript                    (duk_context* ctx);
static duk_ret_t js_SetZoneSteps                     (duk_context* ctx);
static duk_ret_t js_Triangle                         (duk_context* ctx);
static duk_ret_t js_UnbindJoystickButton             (duk_context* ctx);
static duk_ret_t js_UnbindKey                        (duk_context* ctx);
static duk_ret_t js_UpdateMapEngine                  (duk_context* ctx);
static duk_ret_t js_Animation_finalize               (duk_context* ctx);
static duk_ret_t js_Animation_get_height             (duk_context* ctx);
static duk_ret_t js_Animation_get_width              (duk_context* ctx);
static duk_ret_t js_Animation_drawFrame              (duk_context* ctx);
static duk_ret_t js_Animation_drawZoomedFrame        (duk_context* ctx);
static duk_ret_t js_Animation_getDelay               (duk_context* ctx);
static duk_ret_t js_Animation_getNumFrames           (duk_context* ctx);
static duk_ret_t js_Animation_readNextFrame          (duk_context* ctx);
static duk_ret_t js_ByteArray_finalize               (duk_context* ctx);
static duk_ret_t js_ByteArray_proxy_get              (duk_context* ctx);
static duk_ret_t js_ByteArray_proxy_set              (duk_context* ctx);
static duk_ret_t js_ByteArray_get_length             (duk_context* ctx);
static duk_ret_t js_ByteArray_concat                 (duk_context* ctx);
static duk_ret_t js_ByteArray_slice                  (duk_context* ctx);
static duk_ret_t js_ByteArray_toString               (duk_context* ctx);
static duk_ret_t js_Color_toString                   (duk_context* ctx);
static duk_ret_t js_ColorMatrix_toString             (duk_context* ctx);
static duk_ret_t js_File_finalize                    (duk_context* ctx);
static duk_ret_t js_File_close                       (duk_context* ctx);
static duk_ret_t js_File_flush                       (duk_context* ctx);
static duk_ret_t js_File_getKey                      (duk_context* ctx);
static duk_ret_t js_File_getNumKeys                  (duk_context* ctx);
static duk_ret_t js_File_read                        (duk_context* ctx);
static duk_ret_t js_File_toString                    (duk_context* ctx);
static duk_ret_t js_File_write                       (duk_context* ctx);
static duk_ret_t js_Font_finalize                    (duk_context* ctx);
static duk_ret_t js_Font_clone                       (duk_context* ctx);
static duk_ret_t js_Font_drawText                    (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox                 (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText              (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage           (duk_context* ctx);
static duk_ret_t js_Font_getColorMask                (duk_context* ctx);
static duk_ret_t js_Font_getHeight                   (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight             (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth              (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage           (duk_context* ctx);
static duk_ret_t js_Font_setColorMask                (duk_context* ctx);
static duk_ret_t js_Font_toString                    (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString              (duk_context* ctx);
static duk_ret_t js_Image_finalize                   (duk_context* ctx);
static duk_ret_t js_Image_get_height                 (duk_context* ctx);
static duk_ret_t js_Image_get_width                  (duk_context* ctx);
static duk_ret_t js_Image_blit                       (duk_context* ctx);
static duk_ret_t js_Image_blitMask                   (duk_context* ctx);
static duk_ret_t js_Image_createSurface              (duk_context* ctx);
static duk_ret_t js_Image_rotateBlit                 (duk_context* ctx);
static duk_ret_t js_Image_rotateBlitMask             (duk_context* ctx);
static duk_ret_t js_Image_toString                   (duk_context* ctx);
static duk_ret_t js_Image_transformBlit              (duk_context* ctx);
static duk_ret_t js_Image_transformBlitMask          (duk_context* ctx);
static duk_ret_t js_Image_zoomBlit                   (duk_context* ctx);
static duk_ret_t js_Image_zoomBlitMask               (duk_context* ctx);
static duk_ret_t js_Logger_finalize                  (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock                (duk_context* ctx);
static duk_ret_t js_Logger_endBlock                  (duk_context* ctx);
static duk_ret_t js_Logger_toString                  (duk_context* ctx);
static duk_ret_t js_Logger_write                     (duk_context* ctx);
static duk_ret_t js_RawFile_finalize                 (duk_context* ctx);
static duk_ret_t js_RawFile_close                    (duk_context* ctx);
static duk_ret_t js_RawFile_getPosition              (duk_context* ctx);
static duk_ret_t js_RawFile_getSize                  (duk_context* ctx);
static duk_ret_t js_RawFile_read                     (duk_context* ctx);
static duk_ret_t js_RawFile_setPosition              (duk_context* ctx);
static duk_ret_t js_RawFile_toString                 (duk_context* ctx);
static duk_ret_t js_RawFile_write                    (duk_context* ctx);
static duk_ret_t js_Socket_finalize                  (duk_context* ctx);
static duk_ret_t js_Socket_close                     (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize        (duk_context* ctx);
static duk_ret_t js_Socket_isConnected               (duk_context* ctx);
static duk_ret_t js_Socket_read                      (duk_context* ctx);
static duk_ret_t js_Socket_toString                  (duk_context* ctx);
static duk_ret_t js_Socket_write                     (duk_context* ctx);
static duk_ret_t js_Sound_finalize                   (duk_context* ctx);
static duk_ret_t js_Sound_getLength                  (duk_context* ctx);
static duk_ret_t js_Sound_getPan                     (duk_context* ctx);
static duk_ret_t js_Sound_getPitch                   (duk_context* ctx);
static duk_ret_t js_Sound_getPosition                (duk_context* ctx);
static duk_ret_t js_Sound_getRepeat                  (duk_context* ctx);
static duk_ret_t js_Sound_getVolume                  (duk_context* ctx);
static duk_ret_t js_Sound_isPlaying                  (duk_context* ctx);
static duk_ret_t js_Sound_isSeekable                 (duk_context* ctx);
static duk_ret_t js_Sound_pause                      (duk_context* ctx);
static duk_ret_t js_Sound_play                       (duk_context* ctx);
static duk_ret_t js_Sound_reset                      (duk_context* ctx);
static duk_ret_t js_Sound_setPan                     (duk_context* ctx);
static duk_ret_t js_Sound_setPitch                   (duk_context* ctx);
static duk_ret_t js_Sound_setPosition                (duk_context* ctx);
static duk_ret_t js_Sound_setRepeat                  (duk_context* ctx);
static duk_ret_t js_Sound_setVolume                  (duk_context* ctx);
static duk_ret_t js_Sound_stop                       (duk_context* ctx);
static duk_ret_t js_Sound_toString                   (duk_context* ctx);
static duk_ret_t js_SoundEffect_finalize             (duk_context* ctx);
static duk_ret_t js_SoundEffect_getPan               (duk_context* ctx);
static duk_ret_t js_SoundEffect_getPitch             (duk_context* ctx);
static duk_ret_t js_SoundEffect_getVolume            (duk_context* ctx);
static duk_ret_t js_SoundEffect_setPan               (duk_context* ctx);
static duk_ret_t js_SoundEffect_setPitch             (duk_context* ctx);
static duk_ret_t js_SoundEffect_setVolume            (duk_context* ctx);
static duk_ret_t js_SoundEffect_play                 (duk_context* ctx);
static duk_ret_t js_SoundEffect_stop                 (duk_context* ctx);
static duk_ret_t js_SoundEffect_toString             (duk_context* ctx);
static duk_ret_t js_Spriteset_finalize               (duk_context* ctx);
static duk_ret_t js_Spriteset_get_filename           (duk_context* ctx);
static duk_ret_t js_Spriteset_clone                  (duk_context* ctx);
static duk_ret_t js_Spriteset_save                   (duk_context* ctx);
static duk_ret_t js_Spriteset_toString               (duk_context* ctx);
static duk_ret_t js_Surface_finalize                 (duk_context* ctx);
static duk_ret_t js_Surface_get_height               (duk_context* ctx);
static duk_ret_t js_Surface_get_width                (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX             (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX4            (duk_context* ctx);
static duk_ret_t js_Surface_applyLookup              (duk_context* ctx);
static duk_ret_t js_Surface_bezierCurve              (duk_context* ctx);
static duk_ret_t js_Surface_blit                     (duk_context* ctx);
static duk_ret_t js_Surface_blitMaskSurface          (duk_context* ctx);
static duk_ret_t js_Surface_blitSurface              (duk_context* ctx);
static duk_ret_t js_Surface_clone                    (duk_context* ctx);
static duk_ret_t js_Surface_cloneSection             (duk_context* ctx);
static duk_ret_t js_Surface_createImage              (duk_context* ctx);
static duk_ret_t js_Surface_drawText                 (duk_context* ctx);
static duk_ret_t js_Surface_filledCircle             (duk_context* ctx);
static duk_ret_t js_Surface_filledEllipse            (duk_context* ctx);
static duk_ret_t js_Surface_flipHorizontally         (duk_context* ctx);
static duk_ret_t js_Surface_flipVertically           (duk_context* ctx);
static duk_ret_t js_Surface_getPixel                 (duk_context* ctx);
static duk_ret_t js_Surface_gradientCircle           (duk_context* ctx);
static duk_ret_t js_Surface_gradientEllipse          (duk_context* ctx);
static duk_ret_t js_Surface_gradientLine             (duk_context* ctx);
static duk_ret_t js_Surface_gradientRectangle        (duk_context* ctx);
static duk_ret_t js_Surface_line                     (duk_context* ctx);
static duk_ret_t js_Surface_lineSeries               (duk_context* ctx);
static duk_ret_t js_Surface_outlinedCircle           (duk_context* ctx);
static duk_ret_t js_Surface_outlinedEllipse          (duk_context* ctx);
static duk_ret_t js_Surface_outlinedRectangle        (duk_context* ctx);
static duk_ret_t js_Surface_pointSeries              (duk_context* ctx);
static duk_ret_t js_Surface_rotate                   (duk_context* ctx);
static duk_ret_t js_Surface_rotateBlitMaskSurface    (duk_context* ctx);
static duk_ret_t js_Surface_rotateBlitSurface        (duk_context* ctx);
static duk_ret_t js_Surface_rectangle                (duk_context* ctx);
static duk_ret_t js_Surface_replaceColor             (duk_context* ctx);
static duk_ret_t js_Surface_rescale                  (duk_context* ctx);
static duk_ret_t js_Surface_save                     (duk_context* ctx);
static duk_ret_t js_Surface_setAlpha                 (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode             (duk_context* ctx);
static duk_ret_t js_Surface_setPixel                 (duk_context* ctx);
static duk_ret_t js_Surface_toString                 (duk_context* ctx);
static duk_ret_t js_Surface_transformBlitMaskSurface (duk_context* ctx);
static duk_ret_t js_Surface_transformBlitSurface     (duk_context* ctx);
static duk_ret_t js_Surface_zoomBlitMaskSurface      (duk_context* ctx);
static duk_ret_t js_Surface_zoomBlitSurface          (duk_context* ctx);
static duk_ret_t js_WindowStyle_finalize             (duk_context* ctx);
static duk_ret_t js_WindowStyle_drawWindow           (duk_context* ctx);
static duk_ret_t js_WindowStyle_getColorMask         (duk_context* ctx);
static duk_ret_t js_WindowStyle_setColorMask         (duk_context* ctx);
static duk_ret_t js_WindowStyle_toString             (duk_context* ctx);

enum blend_mode
{
	BLEND_BLEND,
	BLEND_REPLACE,
	BLEND_RGB_ONLY,
	BLEND_ALPHA_ONLY,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_MULTIPLY,
	BLEND_AVERAGE,
	BLEND_INVERT,
	BLEND_MAX
};

enum line_series_type
{
	LINE_MULTIPLE,
	LINE_STRIP,
	LINE_LOOP,
};

enum sound_effect_mode
{
	SE_SINGLE,
	SE_MULTIPLE,
};

static int      s_frame_rate = 0;
static mixer_t* s_sound_mixer;

void
initialize_vanilla_api(duk_context* ctx)
{
	console_log(1, "initializing Sphere v1 API (%s)", API_VERSION_STRING);

	s_sound_mixer = mixer_new(44100, 16, 2);

	// set up a dictionary to track RequireScript() calls
	duk_push_global_stash(ctx);
	duk_push_bare_object(ctx);
	duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);

	// stashed person data (used by Get/SetPersonData() APIs)
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "personData");
	duk_pop(ctx);

	// register the absolutely ginormous (and deprecated!) Sphere v1 API.
	api_define_function(ctx, NULL, "Abort", js_Abort);
	api_define_function(ctx, NULL, "AddTrigger", js_AddTrigger);
	api_define_function(ctx, NULL, "AddZone", js_AddZone);
	api_define_function(ctx, NULL, "ApplyColorMask", js_ApplyColorMask);
	api_define_function(ctx, NULL, "AreKeysLeft", js_AreKeysLeft);
	api_define_function(ctx, NULL, "AreZonesAt", js_AreZonesAt);
	api_define_function(ctx, NULL, "AttachCamera", js_AttachCamera);
	api_define_function(ctx, NULL, "AttachInput", js_AttachInput);
	api_define_function(ctx, NULL, "AttachPlayerInput", js_AttachPlayerInput);
	api_define_function(ctx, NULL, "BezierCurve", js_BezierCurve);
	api_define_function(ctx, NULL, "BindJoystickButton", js_BindJoystickButton);
	api_define_function(ctx, NULL, "BindKey", js_BindKey);
	api_define_function(ctx, NULL, "BlendColors", js_BlendColors);
	api_define_function(ctx, NULL, "BlendColorsWeighted", js_BlendColors);
	api_define_function(ctx, NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	api_define_function(ctx, NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript);
	api_define_function(ctx, NULL, "CallMapScript", js_CallMapScript);
	api_define_function(ctx, NULL, "CallPersonScript", js_CallPersonScript);
	api_define_function(ctx, NULL, "ChangeMap", js_ChangeMap);
	api_define_function(ctx, NULL, "ClearPersonCommands", js_ClearPersonCommands);
	api_define_function(ctx, NULL, "CreateByteArray", js_CreateByteArray);
	api_define_function(ctx, NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString);
	api_define_function(ctx, NULL, "CreateColor", js_CreateColor);
	api_define_function(ctx, NULL, "CreateColorMatrix", js_CreateColorMatrix);
	api_define_function(ctx, NULL, "CreateDirectory", js_CreateDirectory);
	api_define_function(ctx, NULL, "CreatePerson", js_CreatePerson);
	api_define_function(ctx, NULL, "CreateSpriteset", js_CreateSpriteset);
	api_define_function(ctx, NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray);
	api_define_function(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_define_function(ctx, NULL, "CreateSurface", js_CreateSurface);
	api_define_function(ctx, NULL, "DeflateByteArray", js_DeflateByteArray);
	api_define_function(ctx, NULL, "Delay", js_Delay);
	api_define_function(ctx, NULL, "DestroyPerson", js_DestroyPerson);
	api_define_function(ctx, NULL, "DetachCamera", js_DetachCamera);
	api_define_function(ctx, NULL, "DetachInput", js_DetachInput);
	api_define_function(ctx, NULL, "DetachPlayerInput", js_DetachPlayerInput);
	api_define_function(ctx, NULL, "DoEvents", js_DoEvents);
	api_define_function(ctx, NULL, "DoesFileExist", js_DoesFileExist);
	api_define_function(ctx, NULL, "DoesPersonExist", js_DoesPersonExist);
	api_define_function(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	api_define_function(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_define_function(ctx, NULL, "Exit", js_Exit);
	api_define_function(ctx, NULL, "ExitMapEngine", js_ExitMapEngine);
	api_define_function(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	api_define_function(ctx, NULL, "ExecuteTrigger", js_ExecuteTrigger);
	api_define_function(ctx, NULL, "ExecuteZoneScript", js_ExecuteZoneScript);
	api_define_function(ctx, NULL, "ExecuteZones", js_ExecuteZones);
	api_define_function(ctx, NULL, "FilledCircle", js_FilledCircle);
	api_define_function(ctx, NULL, "FilledComplex", js_FilledComplex);
	api_define_function(ctx, NULL, "FilledEllipse", js_FilledEllipse);
	api_define_function(ctx, NULL, "FlipScreen", js_FlipScreen);
	api_define_function(ctx, NULL, "FollowPerson", js_FollowPerson);
	api_define_function(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	api_define_function(ctx, NULL, "GetActingPerson", js_GetActingPerson);
	api_define_function(ctx, NULL, "GetCameraPerson", js_GetCameraPerson);
	api_define_function(ctx, NULL, "GetCameraX", js_GetCameraX);
	api_define_function(ctx, NULL, "GetCameraY", js_GetCameraY);
	api_define_function(ctx, NULL, "GetClippingRectangle", js_GetClippingRectangle);
	api_define_function(ctx, NULL, "GetCurrentMap", js_GetCurrentMap);
	api_define_function(ctx, NULL, "GetCurrentPerson", js_GetCurrentPerson);
	api_define_function(ctx, NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	api_define_function(ctx, NULL, "GetCurrentZone", js_GetCurrentZone);
	api_define_function(ctx, NULL, "GetDirectoryList", js_GetDirectoryList);
	api_define_function(ctx, NULL, "GetFileList", js_GetFileList);
	api_define_function(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	api_define_function(ctx, NULL, "GetGameList", js_GetGameList);
	api_define_function(ctx, NULL, "GetInputPerson", js_GetInputPerson);
	api_define_function(ctx, NULL, "GetJoystickAxis", js_GetJoystickAxis);
	api_define_function(ctx, NULL, "GetLocalAddress", js_GetLocalAddress);
	api_define_function(ctx, NULL, "GetLocalName", js_GetLocalName);
	api_define_function(ctx, NULL, "GetKey", js_GetKey);
	api_define_function(ctx, NULL, "GetKeyString", js_GetKeyString);
	api_define_function(ctx, NULL, "GetLayerAngle", js_GetLayerAngle);
	api_define_function(ctx, NULL, "GetLayerHeight", js_GetLayerHeight);
	api_define_function(ctx, NULL, "GetLayerMask", js_GetLayerMask);
	api_define_function(ctx, NULL, "GetLayerName", js_GetLayerName);
	api_define_function(ctx, NULL, "GetLayerWidth", js_GetLayerWidth);
	api_define_function(ctx, NULL, "GetMapEngine", js_GetMapEngine);
	api_define_function(ctx, NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	api_define_function(ctx, NULL, "GetMouseWheelEvent", js_GetMouseWheelEvent);
	api_define_function(ctx, NULL, "GetMouseX", js_GetMouseX);
	api_define_function(ctx, NULL, "GetMouseY", js_GetMouseY);
	api_define_function(ctx, NULL, "GetNumJoysticks", js_GetNumJoysticks);
	api_define_function(ctx, NULL, "GetNumJoystickAxes", js_GetNumJoystickAxes);
	api_define_function(ctx, NULL, "GetNumJoystickButtons", js_GetNumJoystickButtons);
	api_define_function(ctx, NULL, "GetNumMouseWheelEvents", js_GetNumMouseWheelEvents);
	api_define_function(ctx, NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	api_define_function(ctx, NULL, "GetNumLayers", js_GetNumLayers);
	api_define_function(ctx, NULL, "GetNumTiles", js_GetNumTiles);
	api_define_function(ctx, NULL, "GetNumTriggers", js_GetNumTriggers);
	api_define_function(ctx, NULL, "GetNumZones", js_GetNumZones);
	api_define_function(ctx, NULL, "GetObstructingPerson", js_GetObstructingPerson);
	api_define_function(ctx, NULL, "GetObstructingTile", js_GetObstructingTile);
	api_define_function(ctx, NULL, "GetPersonAngle", js_GetPersonAngle);
	api_define_function(ctx, NULL, "GetPersonBase", js_GetPersonBase);
	api_define_function(ctx, NULL, "GetPersonData", js_GetPersonData);
	api_define_function(ctx, NULL, "GetPersonDirection", js_GetPersonDirection);
	api_define_function(ctx, NULL, "GetPersonFollowDistance", js_GetPersonFollowDistance);
	api_define_function(ctx, NULL, "GetPersonFollowers", js_GetPersonFollowers);
	api_define_function(ctx, NULL, "GetPersonFrame", js_GetPersonFrame);
	api_define_function(ctx, NULL, "GetPersonFrameNext", js_GetPersonFrameNext);
	api_define_function(ctx, NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert);
	api_define_function(ctx, NULL, "GetPersonIgnoreList", js_GetPersonIgnoreList);
	api_define_function(ctx, NULL, "GetPersonLayer", js_GetPersonLayer);
	api_define_function(ctx, NULL, "GetPersonLeader", js_GetPersonLeader);
	api_define_function(ctx, NULL, "GetPersonList", js_GetPersonList);
	api_define_function(ctx, NULL, "GetPersonMask", js_GetPersonMask);
	api_define_function(ctx, NULL, "GetPersonOffsetX", js_GetPersonOffsetX);
	api_define_function(ctx, NULL, "GetPersonOffsetY", js_GetPersonOffsetY);
	api_define_function(ctx, NULL, "GetPersonSpeedX", js_GetPersonSpeedX);
	api_define_function(ctx, NULL, "GetPersonSpeedY", js_GetPersonSpeedY);
	api_define_function(ctx, NULL, "GetPersonSpriteset", js_GetPersonSpriteset);
	api_define_function(ctx, NULL, "GetPersonValue", js_GetPersonValue);
	api_define_function(ctx, NULL, "GetPersonX", js_GetPersonX);
	api_define_function(ctx, NULL, "GetPersonXFloat", js_GetPersonXFloat);
	api_define_function(ctx, NULL, "GetPersonY", js_GetPersonY);
	api_define_function(ctx, NULL, "GetPersonYFloat", js_GetPersonYFloat);
	api_define_function(ctx, NULL, "GetPlayerKey", js_GetPlayerKey);
	api_define_function(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	api_define_function(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	api_define_function(ctx, NULL, "GetSystemArrow", js_GetSystemArrow);
	api_define_function(ctx, NULL, "GetSystemDownArrow", js_GetSystemDownArrow);
	api_define_function(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	api_define_function(ctx, NULL, "GetSystemUpArrow", js_GetSystemUpArrow);
	api_define_function(ctx, NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	api_define_function(ctx, NULL, "GetTalkActivationButton", js_GetTalkActivationButton);
	api_define_function(ctx, NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	api_define_function(ctx, NULL, "GetTalkDistance", js_GetTalkDistance);
	api_define_function(ctx, NULL, "GetTile", js_GetTile);
	api_define_function(ctx, NULL, "GetTileDelay", js_GetTileDelay);
	api_define_function(ctx, NULL, "GetTileImage", js_GetTileImage);
	api_define_function(ctx, NULL, "GetTileHeight", js_GetTileHeight);
	api_define_function(ctx, NULL, "GetTileName", js_GetTileName);
	api_define_function(ctx, NULL, "GetTileSurface", js_GetTileSurface);
	api_define_function(ctx, NULL, "GetTileWidth", js_GetTileWidth);
	api_define_function(ctx, NULL, "GetTime", js_GetTime);
	api_define_function(ctx, NULL, "GetToggleState", js_GetToggleState);
	api_define_function(ctx, NULL, "GetTriggerLayer", js_GetTriggerLayer);
	api_define_function(ctx, NULL, "GetTriggerX", js_GetTriggerX);
	api_define_function(ctx, NULL, "GetTriggerY", js_GetTriggerY);
	api_define_function(ctx, NULL, "GetVersion", js_GetVersion);
	api_define_function(ctx, NULL, "GetVersionString", js_GetVersionString);
	api_define_function(ctx, NULL, "GetZoneHeight", js_GetZoneHeight);
	api_define_function(ctx, NULL, "GetZoneLayer", js_GetZoneLayer);
	api_define_function(ctx, NULL, "GetZoneSteps", js_GetZoneSteps);
	api_define_function(ctx, NULL, "GetZoneWidth", js_GetZoneWidth);
	api_define_function(ctx, NULL, "GetZoneX", js_GetZoneX);
	api_define_function(ctx, NULL, "GetZoneY", js_GetZoneY);
	api_define_function(ctx, NULL, "GrabImage", js_GrabImage);
	api_define_function(ctx, NULL, "GrabSurface", js_GrabSurface);
	api_define_function(ctx, NULL, "GradientCircle", js_GradientCircle);
	api_define_function(ctx, NULL, "GradientComplex", js_GradientComplex);
	api_define_function(ctx, NULL, "GradientEllipse", js_GradientEllipse);
	api_define_function(ctx, NULL, "GradientLine", js_GradientLine);
	api_define_function(ctx, NULL, "GradientRectangle", js_GradientRectangle);
	api_define_function(ctx, NULL, "GradientTriangle", js_GradientTriangle);
	api_define_function(ctx, NULL, "HashByteArray", js_HashByteArray);
	api_define_function(ctx, NULL, "HashFromFile", js_HashFromFile);
	api_define_function(ctx, NULL, "IgnorePersonObstructions", js_IgnorePersonObstructions);
	api_define_function(ctx, NULL, "IgnoreTileObstructions", js_IgnoreTileObstructions);
	api_define_function(ctx, NULL, "InflateByteArray", js_InflateByteArray);
	api_define_function(ctx, NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed);
	api_define_function(ctx, NULL, "IsCameraAttached", js_IsCameraAttached);
	api_define_function(ctx, NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty);
	api_define_function(ctx, NULL, "IsIgnoringPersonObstructions", js_IsIgnoringPersonObstructions);
	api_define_function(ctx, NULL, "IsIgnoringTileObstructions", js_IsIgnoringTileObstructions);
	api_define_function(ctx, NULL, "IsInputAttached", js_IsInputAttached);
	api_define_function(ctx, NULL, "IsJoystickButtonPressed", js_IsJoystickButtonPressed);
	api_define_function(ctx, NULL, "IsKeyPressed", js_IsKeyPressed);
	api_define_function(ctx, NULL, "IsLayerReflective", js_IsLayerReflective);
	api_define_function(ctx, NULL, "IsLayerVisible", js_IsLayerVisible);
	api_define_function(ctx, NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	api_define_function(ctx, NULL, "IsMouseButtonPressed", js_IsMouseButtonPressed);
	api_define_function(ctx, NULL, "IsPersonObstructed", js_IsPersonObstructed);
	api_define_function(ctx, NULL, "IsPersonVisible", js_IsPersonVisible);
	api_define_function(ctx, NULL, "IsTriggerAt", js_IsTriggerAt);
	api_define_function(ctx, NULL, "Line", js_Line);
	api_define_function(ctx, NULL, "LineSeries", js_LineSeries);
	api_define_function(ctx, NULL, "ListenOnPort", js_ListenOnPort);
	api_define_function(ctx, NULL, "LoadAnimation", js_LoadAnimation);
	api_define_function(ctx, NULL, "LoadFont", js_LoadFont);
	api_define_function(ctx, NULL, "LoadImage", js_LoadImage);
	api_define_function(ctx, NULL, "LoadSound", js_LoadSound);
	api_define_function(ctx, NULL, "LoadSoundEffect", js_LoadSoundEffect);
	api_define_function(ctx, NULL, "LoadSpriteset", js_LoadSpriteset);
	api_define_function(ctx, NULL, "LoadSurface", js_LoadSurface);
	api_define_function(ctx, NULL, "LoadWindowStyle", js_LoadWindowStyle);
	api_define_function(ctx, NULL, "MapEngine", js_MapEngine);
	api_define_function(ctx, NULL, "MapToScreenX", js_MapToScreenX);
	api_define_function(ctx, NULL, "MapToScreenY", js_MapToScreenY);
	api_define_function(ctx, NULL, "OpenAddress", js_OpenAddress);
	api_define_function(ctx, NULL, "OpenFile", js_OpenFile);
	api_define_function(ctx, NULL, "OpenLog", js_OpenLog);
	api_define_function(ctx, NULL, "OpenRawFile", js_OpenRawFile);
	api_define_function(ctx, NULL, "OutlinedCircle", js_OutlinedCircle);
	api_define_function(ctx, NULL, "OutlinedComplex", js_OutlinedComplex);
	api_define_function(ctx, NULL, "OutlinedEllipse", js_OutlinedEllipse);
	api_define_function(ctx, NULL, "OutlinedRectangle", js_OutlinedRectangle);
	api_define_function(ctx, NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle);
	api_define_function(ctx, NULL, "Point", js_Point);
	api_define_function(ctx, NULL, "PointSeries", js_PointSeries);
	api_define_function(ctx, NULL, "Polygon", js_Polygon);
	api_define_function(ctx, NULL, "Print", js_Print);
	api_define_function(ctx, NULL, "QueuePersonCommand", js_QueuePersonCommand);
	api_define_function(ctx, NULL, "QueuePersonScript", js_QueuePersonScript);
	api_define_function(ctx, NULL, "Rectangle", js_Rectangle);
	api_define_function(ctx, NULL, "RemoveDirectory", js_RemoveDirectory);
	api_define_function(ctx, NULL, "RemoveFile", js_RemoveFile);
	api_define_function(ctx, NULL, "RemoveTrigger", js_RemoveTrigger);
	api_define_function(ctx, NULL, "RemoveZone", js_RemoveZone);
	api_define_function(ctx, NULL, "RenderMap", js_RenderMap);
	api_define_function(ctx, NULL, "Rename", js_Rename);
	api_define_function(ctx, NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer);
	api_define_function(ctx, NULL, "RequireScript", js_RequireScript);
	api_define_function(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	api_define_function(ctx, NULL, "RestartGame", js_RestartGame);
	api_define_function(ctx, NULL, "RoundRectangle", js_RoundRectangle);
	api_define_function(ctx, NULL, "ScreenToMapX", js_ScreenToMapX);
	api_define_function(ctx, NULL, "ScreenToMapY", js_ScreenToMapY);
	api_define_function(ctx, NULL, "SetCameraX", js_SetCameraX);
	api_define_function(ctx, NULL, "SetCameraY", js_SetCameraY);
	api_define_function(ctx, NULL, "SetClippingRectangle", js_SetClippingRectangle);
	api_define_function(ctx, NULL, "SetColorMask", js_SetColorMask);
	api_define_function(ctx, NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	api_define_function(ctx, NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript);
	api_define_function(ctx, NULL, "SetDelayScript", js_SetDelayScript);
	api_define_function(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	api_define_function(ctx, NULL, "SetLayerAngle", js_SetLayerAngle);
	api_define_function(ctx, NULL, "SetLayerHeight", js_SetLayerHeight);
	api_define_function(ctx, NULL, "SetLayerMask", js_SetLayerMask);
	api_define_function(ctx, NULL, "SetLayerReflective", js_SetLayerReflective);
	api_define_function(ctx, NULL, "SetLayerRenderer", js_SetLayerRenderer);
	api_define_function(ctx, NULL, "SetLayerScaleFactorX", js_SetLayerScaleFactorX);
	api_define_function(ctx, NULL, "SetLayerScaleFactorY", js_SetLayerScaleFactorY);
	api_define_function(ctx, NULL, "SetLayerSize", js_SetLayerSize);
	api_define_function(ctx, NULL, "SetLayerVisible", js_SetLayerVisible);
	api_define_function(ctx, NULL, "SetLayerWidth", js_SetLayerWidth);
	api_define_function(ctx, NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	api_define_function(ctx, NULL, "SetMousePosition", js_SetMousePosition);
	api_define_function(ctx, NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	api_define_function(ctx, NULL, "SetPersonAngle", js_SetPersonAngle);
	api_define_function(ctx, NULL, "SetPersonData", js_SetPersonData);
	api_define_function(ctx, NULL, "SetPersonDirection", js_SetPersonDirection);
	api_define_function(ctx, NULL, "SetPersonFollowDistance", js_SetPersonFollowDistance);
	api_define_function(ctx, NULL, "SetPersonFrame", js_SetPersonFrame);
	api_define_function(ctx, NULL, "SetPersonFrameNext", js_SetPersonFrameNext);
	api_define_function(ctx, NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert);
	api_define_function(ctx, NULL, "SetPersonIgnoreList", js_SetPersonIgnoreList);
	api_define_function(ctx, NULL, "SetPersonLayer", js_SetPersonLayer);
	api_define_function(ctx, NULL, "SetPersonMask", js_SetPersonMask);
	api_define_function(ctx, NULL, "SetPersonOffsetX", js_SetPersonOffsetX);
	api_define_function(ctx, NULL, "SetPersonOffsetY", js_SetPersonOffsetY);
	api_define_function(ctx, NULL, "SetPersonScaleAbsolute", js_SetPersonScaleAbsolute);
	api_define_function(ctx, NULL, "SetPersonScaleFactor", js_SetPersonScaleFactor);
	api_define_function(ctx, NULL, "SetPersonScript", js_SetPersonScript);
	api_define_function(ctx, NULL, "SetPersonSpeed", js_SetPersonSpeed);
	api_define_function(ctx, NULL, "SetPersonSpeedXY", js_SetPersonSpeedXY);
	api_define_function(ctx, NULL, "SetPersonSpriteset", js_SetPersonSpriteset);
	api_define_function(ctx, NULL, "SetPersonValue", js_SetPersonValue);
	api_define_function(ctx, NULL, "SetPersonVisible", js_SetPersonVisible);
	api_define_function(ctx, NULL, "SetPersonX", js_SetPersonX);
	api_define_function(ctx, NULL, "SetPersonXYFloat", js_SetPersonXYFloat);
	api_define_function(ctx, NULL, "SetPersonY", js_SetPersonY);
	api_define_function(ctx, NULL, "SetRenderScript", js_SetRenderScript);
	api_define_function(ctx, NULL, "SetTalkActivationButton", js_SetTalkActivationButton);
	api_define_function(ctx, NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	api_define_function(ctx, NULL, "SetTalkDistance", js_SetTalkDistance);
	api_define_function(ctx, NULL, "SetTile", js_SetTile);
	api_define_function(ctx, NULL, "SetTileDelay", js_SetTileDelay);
	api_define_function(ctx, NULL, "SetTileImage", js_SetTileImage);
	api_define_function(ctx, NULL, "SetTileName", js_SetTileName);
	api_define_function(ctx, NULL, "SetTileSurface", js_SetTileSurface);
	api_define_function(ctx, NULL, "SetTriggerLayer", js_SetTriggerLayer);
	api_define_function(ctx, NULL, "SetTriggerScript", js_SetTriggerScript);
	api_define_function(ctx, NULL, "SetTriggerXY", js_SetTriggerXY);
	api_define_function(ctx, NULL, "SetUpdateScript", js_SetUpdateScript);
	api_define_function(ctx, NULL, "SetZoneDimensions", js_SetZoneDimensions);
	api_define_function(ctx, NULL, "SetZoneLayer", js_SetZoneLayer);
	api_define_function(ctx, NULL, "SetZoneScript", js_SetZoneScript);
	api_define_function(ctx, NULL, "SetZoneSteps", js_SetZoneSteps);
	api_define_function(ctx, NULL, "Triangle", js_Triangle);
	api_define_function(ctx, NULL, "UnbindJoystickButton", js_UnbindJoystickButton);
	api_define_function(ctx, NULL, "UnbindKey", js_UnbindKey);
	api_define_function(ctx, NULL, "UpdateMapEngine", js_UpdateMapEngine);

	api_define_class(ctx, "ssAnimation", NULL, js_Animation_finalize);
	api_define_property(ctx, "ssAnimation", "width", js_Animation_get_width, NULL);
	api_define_property(ctx, "ssAnimation", "height", js_Animation_get_height, NULL);
	api_define_method(ctx, "ssAnimation", "getDelay", js_Animation_getDelay);
	api_define_method(ctx, "ssAnimation", "getNumFrames", js_Animation_getNumFrames);
	api_define_method(ctx, "ssAnimation", "drawFrame", js_Animation_drawFrame);
	api_define_method(ctx, "ssAnimation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	api_define_method(ctx, "ssAnimation", "readNextFrame", js_Animation_readNextFrame);

	api_define_class(ctx, "ssByteArray", NULL, js_ByteArray_finalize);
	api_define_property(ctx, "ssByteArray", "length", js_ByteArray_get_length, NULL);
	api_define_method(ctx, "ssByteArray", "concat", js_ByteArray_concat);
	api_define_method(ctx, "ssByteArray", "slice", js_ByteArray_slice);
	api_define_method(ctx, "ssByteArray", "toString", js_ByteArray_toString);

	api_define_class(ctx, "ssColor", NULL, NULL);
	api_define_method(ctx, "ssColor", "toString", js_Color_toString);

	api_define_class(ctx, "ssColorMatrix", NULL, NULL);
	api_define_method(ctx, "ssColorMatrix", "toString", js_ColorMatrix_toString);

	api_define_class(ctx, "ssFile", NULL, js_File_finalize);
	api_define_method(ctx, "ssFile", "close", js_File_close);
	api_define_method(ctx, "ssFile", "flush", js_File_flush);
	api_define_method(ctx, "ssFile", "getKey", js_File_getKey);
	api_define_method(ctx, "ssFile", "getNumKeys", js_File_getNumKeys);
	api_define_method(ctx, "ssFile", "read", js_File_read);
	api_define_method(ctx, "ssFile", "toString", js_File_toString);
	api_define_method(ctx, "ssFile", "write", js_File_write);

	api_define_class(ctx, "ssFont", NULL, js_Font_finalize);
	api_define_method(ctx, "ssFont", "clone", js_Font_clone);
	api_define_method(ctx, "ssFont", "drawText", js_Font_drawText);
	api_define_method(ctx, "ssFont", "drawTextBox", js_Font_drawTextBox);
	api_define_method(ctx, "ssFont", "drawZoomedText", js_Font_drawZoomedText);
	api_define_method(ctx, "ssFont", "getCharacterImage", js_Font_getCharacterImage);
	api_define_method(ctx, "ssFont", "getColorMask", js_Font_getColorMask);
	api_define_method(ctx, "ssFont", "getHeight", js_Font_getHeight);
	api_define_method(ctx, "ssFont", "getStringHeight", js_Font_getStringHeight);
	api_define_method(ctx, "ssFont", "getStringWidth", js_Font_getStringWidth);
	api_define_method(ctx, "ssFont", "setCharacterImage", js_Font_setCharacterImage);
	api_define_method(ctx, "ssFont", "setColorMask", js_Font_setColorMask);
	api_define_method(ctx, "ssFont", "toString", js_Font_toString);
	api_define_method(ctx, "ssFont", "wordWrapString", js_Font_wordWrapString);

	api_define_class(ctx, "ssImage", NULL, js_Image_finalize);
	api_define_property(ctx, "ssImage", "height", js_Image_get_height, NULL);
	api_define_property(ctx, "ssImage", "width", js_Image_get_width, NULL);
	api_define_method(ctx, "ssImage", "blit", js_Image_blit);
	api_define_method(ctx, "ssImage", "blitMask", js_Image_blitMask);
	api_define_method(ctx, "ssImage", "createSurface", js_Image_createSurface);
	api_define_method(ctx, "ssImage", "rotateBlit", js_Image_rotateBlit);
	api_define_method(ctx, "ssImage", "rotateBlitMask", js_Image_rotateBlitMask);
	api_define_method(ctx, "ssImage", "toString", js_Image_toString);
	api_define_method(ctx, "ssImage", "transformBlit", js_Image_transformBlit);
	api_define_method(ctx, "ssImage", "transformBlitMask", js_Image_transformBlitMask);
	api_define_method(ctx, "ssImage", "zoomBlit", js_Image_zoomBlit);
	api_define_method(ctx, "ssImage", "zoomBlitMask", js_Image_zoomBlitMask);

	api_define_class(ctx, "ssLogger", NULL, js_Logger_finalize);
	api_define_method(ctx, "ssLogger", "toString", js_Logger_toString);
	api_define_method(ctx, "ssLogger", "beginBlock", js_Logger_beginBlock);
	api_define_method(ctx, "ssLogger", "endBlock", js_Logger_endBlock);
	api_define_method(ctx, "ssLogger", "write", js_Logger_write);

	api_define_class(ctx, "ssRawFile", NULL, js_RawFile_finalize);
	api_define_method(ctx, "ssRawFile", "close", js_RawFile_close);
	api_define_method(ctx, "ssRawFile", "getPosition", js_RawFile_getPosition);
	api_define_method(ctx, "ssRawFile", "getSize", js_RawFile_getSize);
	api_define_method(ctx, "ssRawFile", "read", js_RawFile_read);
	api_define_method(ctx, "ssRawFile", "setPosition", js_RawFile_setPosition);
	api_define_method(ctx, "ssRawFile", "toString", js_RawFile_toString);
	api_define_method(ctx, "ssRawFile", "write", js_RawFile_write);

	api_define_class(ctx, "ssSocket", NULL, js_Socket_finalize);
	api_define_method(ctx, "ssSocket", "close", js_Socket_close);
	api_define_method(ctx, "ssSocket", "getPendingReadSize", js_Socket_getPendingReadSize);
	api_define_method(ctx, "ssSocket", "isConnected", js_Socket_isConnected);
	api_define_method(ctx, "ssSocket", "read", js_Socket_read);
	api_define_method(ctx, "ssSocket", "toString", js_Socket_toString);
	api_define_method(ctx, "ssSocket", "write", js_Socket_write);

	api_define_class(ctx, "ssSound", NULL, js_Sound_finalize);
	api_define_method(ctx, "ssSound", "getLength", js_Sound_getLength);
	api_define_method(ctx, "ssSound", "getPan", js_Sound_getPan);
	api_define_method(ctx, "ssSound", "getPitch", js_Sound_getPitch);
	api_define_method(ctx, "ssSound", "getPosition", js_Sound_getPosition);
	api_define_method(ctx, "ssSound", "getRepeat", js_Sound_getRepeat);
	api_define_method(ctx, "ssSound", "getVolume", js_Sound_getVolume);
	api_define_method(ctx, "ssSound", "isPlaying", js_Sound_isPlaying);
	api_define_method(ctx, "ssSound", "isSeekable", js_Sound_isSeekable);
	api_define_method(ctx, "ssSound", "pause", js_Sound_pause);
	api_define_method(ctx, "ssSound", "play", js_Sound_play);
	api_define_method(ctx, "ssSound", "reset", js_Sound_reset);
	api_define_method(ctx, "ssSound", "setPan", js_Sound_setPan);
	api_define_method(ctx, "ssSound", "setPitch", js_Sound_setPitch);
	api_define_method(ctx, "ssSound", "setPosition", js_Sound_setPosition);
	api_define_method(ctx, "ssSound", "setRepeat", js_Sound_setRepeat);
	api_define_method(ctx, "ssSound", "setVolume", js_Sound_setVolume);
	api_define_method(ctx, "ssSound", "stop", js_Sound_stop);
	api_define_method(ctx, "ssSound", "toString", js_Sound_toString);

	api_define_class(ctx, "ssSoundEffect", NULL, js_SoundEffect_finalize);
	api_define_method(ctx, "ssSoundEffect", "getPan", js_SoundEffect_getPan);
	api_define_method(ctx, "ssSoundEffect", "getPitch", js_SoundEffect_getPitch);
	api_define_method(ctx, "ssSoundEffect", "getVolume", js_SoundEffect_getVolume);
	api_define_method(ctx, "ssSoundEffect", "setPan", js_SoundEffect_setPan);
	api_define_method(ctx, "ssSoundEffect", "setPitch", js_SoundEffect_setPitch);
	api_define_method(ctx, "ssSoundEffect", "setVolume", js_SoundEffect_setVolume);
	api_define_method(ctx, "ssSoundEffect", "play", js_SoundEffect_play);
	api_define_method(ctx, "ssSoundEffect", "stop", js_SoundEffect_stop);
	api_define_method(ctx, "ssSoundEffect", "toString", js_SoundEffect_toString);

	api_define_class(ctx, "ssSpriteset", NULL, js_Spriteset_finalize);
	api_define_property(ctx, "ssSpriteset", "filename", js_Spriteset_get_filename, NULL);
	api_define_method(ctx, "ssSpriteset", "clone", js_Spriteset_clone);
	api_define_method(ctx, "ssSpriteset", "save", js_Spriteset_save);
	api_define_method(ctx, "ssSpriteset", "toString", js_Spriteset_toString);

	api_define_class(ctx, "ssSurface", NULL, js_Surface_finalize);
	api_define_property(ctx, "ssSurface", "height", js_Surface_get_height, NULL);
	api_define_property(ctx, "ssSurface", "width", js_Surface_get_width, NULL);
	api_define_method(ctx, "ssSurface", "applyColorFX", js_Surface_applyColorFX);
	api_define_method(ctx, "ssSurface", "applyColorFX4", js_Surface_applyColorFX4);
	api_define_method(ctx, "ssSurface", "applyLookup", js_Surface_applyLookup);
	api_define_method(ctx, "ssSurface", "bezierCurve", js_Surface_bezierCurve);
	api_define_method(ctx, "ssSurface", "blit", js_Surface_blit);
	api_define_method(ctx, "ssSurface", "blitMaskSurface", js_Surface_blitMaskSurface);
	api_define_method(ctx, "ssSurface", "blitSurface", js_Surface_blitSurface);
	api_define_method(ctx, "ssSurface", "clone", js_Surface_clone);
	api_define_method(ctx, "ssSurface", "cloneSection", js_Surface_cloneSection);
	api_define_method(ctx, "ssSurface", "createImage", js_Surface_createImage);
	api_define_method(ctx, "ssSurface", "drawText", js_Surface_drawText);
	api_define_method(ctx, "ssSurface", "filledCircle", js_Surface_filledCircle);
	api_define_method(ctx, "ssSurface", "filledEllipse", js_Surface_filledEllipse);
	api_define_method(ctx, "ssSurface", "flipHorizontally", js_Surface_flipHorizontally);
	api_define_method(ctx, "ssSurface", "flipVertically", js_Surface_flipVertically);
	api_define_method(ctx, "ssSurface", "getPixel", js_Surface_getPixel);
	api_define_method(ctx, "ssSurface", "gradientCircle", js_Surface_gradientCircle);
	api_define_method(ctx, "ssSurface", "gradientEllipse", js_Surface_gradientEllipse);
	api_define_method(ctx, "ssSurface", "gradientLine", js_Surface_gradientLine);
	api_define_method(ctx, "ssSurface", "gradientRectangle", js_Surface_gradientRectangle);
	api_define_method(ctx, "ssSurface", "line", js_Surface_line);
	api_define_method(ctx, "ssSurface", "lineSeries", js_Surface_lineSeries);
	api_define_method(ctx, "ssSurface", "outlinedCircle", js_Surface_outlinedCircle);
	api_define_method(ctx, "ssSurface", "outlinedEllipse", js_Surface_outlinedEllipse);
	api_define_method(ctx, "ssSurface", "outlinedRectangle", js_Surface_outlinedRectangle);
	api_define_method(ctx, "ssSurface", "pointSeries", js_Surface_pointSeries);
	api_define_method(ctx, "ssSurface", "rotate", js_Surface_rotate);
	api_define_method(ctx, "ssSurface", "rotateBlitMaskSurface", js_Surface_rotateBlitMaskSurface);
	api_define_method(ctx, "ssSurface", "rotateBlitSurface", js_Surface_rotateBlitSurface);
	api_define_method(ctx, "ssSurface", "rectangle", js_Surface_rectangle);
	api_define_method(ctx, "ssSurface", "replaceColor", js_Surface_replaceColor);
	api_define_method(ctx, "ssSurface", "rescale", js_Surface_rescale);
	api_define_method(ctx, "ssSurface", "save", js_Surface_save);
	api_define_method(ctx, "ssSurface", "setAlpha", js_Surface_setAlpha);
	api_define_method(ctx, "ssSurface", "setBlendMode", js_Surface_setBlendMode);
	api_define_method(ctx, "ssSurface", "setPixel", js_Surface_setPixel);
	api_define_method(ctx, "ssSurface", "toString", js_Surface_toString);
	api_define_method(ctx, "ssSurface", "transformBlitMaskSurface", js_Surface_transformBlitMaskSurface);
	api_define_method(ctx, "ssSurface", "transformBlitSurface", js_Surface_transformBlitSurface);
	api_define_method(ctx, "ssSurface", "zoomBlitMaskSurface", js_Surface_zoomBlitMaskSurface);
	api_define_method(ctx, "ssSurface", "zoomBlitSurface", js_Surface_zoomBlitSurface);

	api_define_class(ctx, "ssWindowStyle", NULL, js_WindowStyle_finalize);
	api_define_method(ctx, "ssWindowStyle", "drawWindow", js_WindowStyle_drawWindow);
	api_define_method(ctx, "ssWindowStyle", "getColorMask", js_WindowStyle_getColorMask);
	api_define_method(ctx, "ssWindowStyle", "setColorMask", js_WindowStyle_setColorMask);
	api_define_method(ctx, "ssWindowStyle", "toString", js_WindowStyle_toString);

	// blend modes for Surfaces
	api_define_const(ctx, NULL, "BLEND", BLEND_BLEND);
	api_define_const(ctx, NULL, "REPLACE", BLEND_REPLACE);
	api_define_const(ctx, NULL, "RGB_ONLY", BLEND_RGB_ONLY);
	api_define_const(ctx, NULL, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
	api_define_const(ctx, NULL, "ADD", BLEND_ADD);
	api_define_const(ctx, NULL, "SUBTRACT", BLEND_SUBTRACT);
	api_define_const(ctx, NULL, "MULTIPLY", BLEND_MULTIPLY);
	api_define_const(ctx, NULL, "AVERAGE", BLEND_AVERAGE);
	api_define_const(ctx, NULL, "INVERT", BLEND_INVERT);

	// person movement commands
	api_define_const(ctx, NULL, "COMMAND_WAIT", COMMAND_WAIT);
	api_define_const(ctx, NULL, "COMMAND_ANIMATE", COMMAND_ANIMATE);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTH", COMMAND_FACE_NORTH);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTHEAST", COMMAND_FACE_NORTHEAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_EAST", COMMAND_FACE_EAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTHEAST", COMMAND_FACE_SOUTHEAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTH", COMMAND_FACE_SOUTH);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTHWEST", COMMAND_FACE_SOUTHWEST);
	api_define_const(ctx, NULL, "COMMAND_FACE_WEST", COMMAND_FACE_WEST);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTHWEST", COMMAND_FACE_NORTHWEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTH", COMMAND_MOVE_NORTH);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTHEAST", COMMAND_MOVE_NORTHEAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_EAST", COMMAND_MOVE_EAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTHEAST", COMMAND_MOVE_SOUTHEAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTH", COMMAND_MOVE_SOUTH);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTHWEST", COMMAND_MOVE_SOUTHWEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_WEST", COMMAND_MOVE_WEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTHWEST", COMMAND_MOVE_NORTHWEST);

	// joystick axes
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_X", 0);
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_Y", 1);
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_Z", 2);
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_R", 3);
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_U", 4);
	api_define_const(ctx, NULL, "JOYSTICK_AXIS_V", 5);

	// LineSeries() modes
	api_define_const(ctx, NULL, "LINE_MULTIPLE", LINE_MULTIPLE);
	api_define_const(ctx, NULL, "LINE_STRIP", LINE_STRIP);
	api_define_const(ctx, NULL, "LINE_LOOP", LINE_LOOP);

	// keyboard keys
	api_define_const(ctx, NULL, "KEY_NONE", 0);
	api_define_const(ctx, NULL, "KEY_SHIFT", ALLEGRO_KEY_LSHIFT);
	api_define_const(ctx, NULL, "KEY_CTRL", ALLEGRO_KEY_LCTRL);
	api_define_const(ctx, NULL, "KEY_ALT", ALLEGRO_KEY_ALT);
	api_define_const(ctx, NULL, "KEY_UP", ALLEGRO_KEY_UP);
	api_define_const(ctx, NULL, "KEY_DOWN", ALLEGRO_KEY_DOWN);
	api_define_const(ctx, NULL, "KEY_LEFT", ALLEGRO_KEY_LEFT);
	api_define_const(ctx, NULL, "KEY_RIGHT", ALLEGRO_KEY_RIGHT);
	api_define_const(ctx, NULL, "KEY_APOSTROPHE", ALLEGRO_KEY_QUOTE);
	api_define_const(ctx, NULL, "KEY_BACKSLASH", ALLEGRO_KEY_BACKSLASH);
	api_define_const(ctx, NULL, "KEY_BACKSPACE", ALLEGRO_KEY_BACKSPACE);
	api_define_const(ctx, NULL, "KEY_CLOSEBRACE", ALLEGRO_KEY_CLOSEBRACE);
	api_define_const(ctx, NULL, "KEY_CAPSLOCK", ALLEGRO_KEY_CAPSLOCK);
	api_define_const(ctx, NULL, "KEY_COMMA", ALLEGRO_KEY_COMMA);
	api_define_const(ctx, NULL, "KEY_DELETE", ALLEGRO_KEY_DELETE);
	api_define_const(ctx, NULL, "KEY_END", ALLEGRO_KEY_END);
	api_define_const(ctx, NULL, "KEY_ENTER", ALLEGRO_KEY_ENTER);
	api_define_const(ctx, NULL, "KEY_EQUALS", ALLEGRO_KEY_EQUALS);
	api_define_const(ctx, NULL, "KEY_ESCAPE", ALLEGRO_KEY_ESCAPE);
	api_define_const(ctx, NULL, "KEY_HOME", ALLEGRO_KEY_HOME);
	api_define_const(ctx, NULL, "KEY_INSERT", ALLEGRO_KEY_INSERT);
	api_define_const(ctx, NULL, "KEY_MINUS", ALLEGRO_KEY_MINUS);
	api_define_const(ctx, NULL, "KEY_NUMLOCK", ALLEGRO_KEY_NUMLOCK);
	api_define_const(ctx, NULL, "KEY_OPENBRACE", ALLEGRO_KEY_OPENBRACE);
	api_define_const(ctx, NULL, "KEY_PAGEDOWN", ALLEGRO_KEY_PGDN);
	api_define_const(ctx, NULL, "KEY_PAGEUP", ALLEGRO_KEY_PGUP);
	api_define_const(ctx, NULL, "KEY_PERIOD", ALLEGRO_KEY_FULLSTOP);
	api_define_const(ctx, NULL, "KEY_SCROLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const(ctx, NULL, "KEY_SCROLLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const(ctx, NULL, "KEY_SEMICOLON", ALLEGRO_KEY_SEMICOLON);
	api_define_const(ctx, NULL, "KEY_SPACE", ALLEGRO_KEY_SPACE);
	api_define_const(ctx, NULL, "KEY_SLASH", ALLEGRO_KEY_SLASH);
	api_define_const(ctx, NULL, "KEY_TAB", ALLEGRO_KEY_TAB);
	api_define_const(ctx, NULL, "KEY_TILDE", ALLEGRO_KEY_TILDE);
	api_define_const(ctx, NULL, "KEY_F1", ALLEGRO_KEY_F1);
	api_define_const(ctx, NULL, "KEY_F2", ALLEGRO_KEY_F2);
	api_define_const(ctx, NULL, "KEY_F3", ALLEGRO_KEY_F3);
	api_define_const(ctx, NULL, "KEY_F4", ALLEGRO_KEY_F4);
	api_define_const(ctx, NULL, "KEY_F5", ALLEGRO_KEY_F5);
	api_define_const(ctx, NULL, "KEY_F6", ALLEGRO_KEY_F6);
	api_define_const(ctx, NULL, "KEY_F7", ALLEGRO_KEY_F7);
	api_define_const(ctx, NULL, "KEY_F8", ALLEGRO_KEY_F8);
	api_define_const(ctx, NULL, "KEY_F9", ALLEGRO_KEY_F9);
	api_define_const(ctx, NULL, "KEY_F10", ALLEGRO_KEY_F10);
	api_define_const(ctx, NULL, "KEY_F11", ALLEGRO_KEY_F11);
	api_define_const(ctx, NULL, "KEY_F12", ALLEGRO_KEY_F12);
	api_define_const(ctx, NULL, "KEY_A", ALLEGRO_KEY_A);
	api_define_const(ctx, NULL, "KEY_B", ALLEGRO_KEY_B);
	api_define_const(ctx, NULL, "KEY_C", ALLEGRO_KEY_C);
	api_define_const(ctx, NULL, "KEY_D", ALLEGRO_KEY_D);
	api_define_const(ctx, NULL, "KEY_E", ALLEGRO_KEY_E);
	api_define_const(ctx, NULL, "KEY_F", ALLEGRO_KEY_F);
	api_define_const(ctx, NULL, "KEY_G", ALLEGRO_KEY_G);
	api_define_const(ctx, NULL, "KEY_H", ALLEGRO_KEY_H);
	api_define_const(ctx, NULL, "KEY_I", ALLEGRO_KEY_I);
	api_define_const(ctx, NULL, "KEY_J", ALLEGRO_KEY_J);
	api_define_const(ctx, NULL, "KEY_K", ALLEGRO_KEY_K);
	api_define_const(ctx, NULL, "KEY_L", ALLEGRO_KEY_L);
	api_define_const(ctx, NULL, "KEY_M", ALLEGRO_KEY_M);
	api_define_const(ctx, NULL, "KEY_N", ALLEGRO_KEY_N);
	api_define_const(ctx, NULL, "KEY_O", ALLEGRO_KEY_O);
	api_define_const(ctx, NULL, "KEY_P", ALLEGRO_KEY_P);
	api_define_const(ctx, NULL, "KEY_Q", ALLEGRO_KEY_Q);
	api_define_const(ctx, NULL, "KEY_R", ALLEGRO_KEY_R);
	api_define_const(ctx, NULL, "KEY_S", ALLEGRO_KEY_S);
	api_define_const(ctx, NULL, "KEY_T", ALLEGRO_KEY_T);
	api_define_const(ctx, NULL, "KEY_U", ALLEGRO_KEY_U);
	api_define_const(ctx, NULL, "KEY_V", ALLEGRO_KEY_V);
	api_define_const(ctx, NULL, "KEY_W", ALLEGRO_KEY_W);
	api_define_const(ctx, NULL, "KEY_X", ALLEGRO_KEY_X);
	api_define_const(ctx, NULL, "KEY_Y", ALLEGRO_KEY_Y);
	api_define_const(ctx, NULL, "KEY_Z", ALLEGRO_KEY_Z);
	api_define_const(ctx, NULL, "KEY_1", ALLEGRO_KEY_1);
	api_define_const(ctx, NULL, "KEY_2", ALLEGRO_KEY_2);
	api_define_const(ctx, NULL, "KEY_3", ALLEGRO_KEY_3);
	api_define_const(ctx, NULL, "KEY_4", ALLEGRO_KEY_4);
	api_define_const(ctx, NULL, "KEY_5", ALLEGRO_KEY_5);
	api_define_const(ctx, NULL, "KEY_6", ALLEGRO_KEY_6);
	api_define_const(ctx, NULL, "KEY_7", ALLEGRO_KEY_7);
	api_define_const(ctx, NULL, "KEY_8", ALLEGRO_KEY_8);
	api_define_const(ctx, NULL, "KEY_9", ALLEGRO_KEY_9);
	api_define_const(ctx, NULL, "KEY_0", ALLEGRO_KEY_0);
	api_define_const(ctx, NULL, "KEY_NUM_1", ALLEGRO_KEY_PAD_1);
	api_define_const(ctx, NULL, "KEY_NUM_2", ALLEGRO_KEY_PAD_2);
	api_define_const(ctx, NULL, "KEY_NUM_3", ALLEGRO_KEY_PAD_3);
	api_define_const(ctx, NULL, "KEY_NUM_4", ALLEGRO_KEY_PAD_4);
	api_define_const(ctx, NULL, "KEY_NUM_5", ALLEGRO_KEY_PAD_5);
	api_define_const(ctx, NULL, "KEY_NUM_6", ALLEGRO_KEY_PAD_6);
	api_define_const(ctx, NULL, "KEY_NUM_7", ALLEGRO_KEY_PAD_7);
	api_define_const(ctx, NULL, "KEY_NUM_8", ALLEGRO_KEY_PAD_8);
	api_define_const(ctx, NULL, "KEY_NUM_9", ALLEGRO_KEY_PAD_9);
	api_define_const(ctx, NULL, "KEY_NUM_0", ALLEGRO_KEY_PAD_0);

	api_define_const(ctx, NULL, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
	api_define_const(ctx, NULL, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);
	api_define_const(ctx, NULL, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
	api_define_const(ctx, NULL, "MOUSE_WHEEL_UP", MOUSE_KEY_WHEEL_UP);
	api_define_const(ctx, NULL, "MOUSE_WHEEL_DOWN", MOUSE_KEY_WHEEL_DOWN);

	api_define_const(ctx, NULL, "PLAYER_1", PLAYER_1);
	api_define_const(ctx, NULL, "PLAYER_2", PLAYER_2);
	api_define_const(ctx, NULL, "PLAYER_3", PLAYER_3);
	api_define_const(ctx, NULL, "PLAYER_4", PLAYER_4);

	api_define_const(ctx, NULL, "PLAYER_KEY_MENU", PLAYER_KEY_MENU);
	api_define_const(ctx, NULL, "PLAYER_KEY_UP", PLAYER_KEY_UP);
	api_define_const(ctx, NULL, "PLAYER_KEY_DOWN", PLAYER_KEY_DOWN);
	api_define_const(ctx, NULL, "PLAYER_KEY_LEFT", PLAYER_KEY_LEFT);
	api_define_const(ctx, NULL, "PLAYER_KEY_RIGHT", PLAYER_KEY_RIGHT);
	api_define_const(ctx, NULL, "PLAYER_KEY_A", PLAYER_KEY_A);
	api_define_const(ctx, NULL, "PLAYER_KEY_B", PLAYER_KEY_B);
	api_define_const(ctx, NULL, "PLAYER_KEY_X", PLAYER_KEY_X);
	api_define_const(ctx, NULL, "PLAYER_KEY_Y", PLAYER_KEY_Y);

	// map script types
	api_define_const(ctx, NULL, "SCRIPT_ON_ENTER_MAP", MAP_SCRIPT_ON_ENTER);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP", MAP_SCRIPT_ON_LEAVE);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_NORTH", MAP_SCRIPT_ON_LEAVE_NORTH);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_EAST", MAP_SCRIPT_ON_LEAVE_EAST);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_SOUTH", MAP_SCRIPT_ON_LEAVE_SOUTH);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_WEST", MAP_SCRIPT_ON_LEAVE_WEST);

	// person script types
	api_define_const(ctx, NULL, "SCRIPT_ON_CREATE", PERSON_SCRIPT_ON_CREATE);
	api_define_const(ctx, NULL, "SCRIPT_ON_DESTROY", PERSON_SCRIPT_ON_DESTROY);
	api_define_const(ctx, NULL, "SCRIPT_ON_ACTIVATE_TOUCH", PERSON_SCRIPT_ON_TOUCH);
	api_define_const(ctx, NULL, "SCRIPT_ON_ACTIVATE_TALK", PERSON_SCRIPT_ON_TALK);
	api_define_const(ctx, NULL, "SCRIPT_COMMAND_GENERATOR", PERSON_SCRIPT_GENERATOR);

	api_define_const(ctx, NULL, "SE_SINGLE", 0);
	api_define_const(ctx, NULL, "SE_MULTIPLE", 1);
}

void
duk_push_sphere_bytearray(duk_context* ctx, bytearray_t* array)
{
	duk_idx_t obj_index;

	duk_push_class_obj(ctx, "ssByteArray", bytearray_ref(array));
	obj_index = duk_normalize_index(ctx, -1);

	// return proxy object so we can catch array accesses
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Proxy");
	duk_dup(ctx, obj_index);
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_ByteArray_proxy_get, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "get");
	duk_push_c_function(ctx, js_ByteArray_proxy_set, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "set");
	duk_new(ctx, 2);
	duk_get_prototype(ctx, obj_index);
	duk_set_prototype(ctx, -2);
	duk_remove(ctx, -2);
	duk_remove(ctx, -2);
}

void
duk_push_sphere_color(duk_context* ctx, color_t color)
{
	duk_get_global_string(ctx, "CreateColor");
	duk_push_number(ctx, color.r);
	duk_push_number(ctx, color.g);
	duk_push_number(ctx, color.b);
	duk_push_number(ctx, color.a);
	duk_call(ctx, 4);
}

void
duk_push_sphere_font(duk_context* ctx, font_t* font)
{
	duk_push_class_obj(ctx, "ssFont", font_ref(font));
	duk_push_sphere_color(ctx, color_new(255, 255, 255, 255));
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

void
duk_push_sphere_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	rect_t      base;
	image_t*    image;
	const char* pose_name;

	int i, j;

	duk_push_class_obj(ctx, "ssSpriteset", spriteset_ref(spriteset));

	// Spriteset:base
	base = spriteset_get_base(spriteset);
	duk_push_object(ctx);
	duk_push_int(ctx, base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, base.y2); duk_put_prop_string(ctx, -2, "y2");
	duk_put_prop_string(ctx, -2, "base");

	// Spriteset:images
	duk_push_array(ctx);
	for (i = 0; i < spriteset_num_images(spriteset); ++i) {
		image = spriteset_image(spriteset, i);
		duk_push_class_obj(ctx, "ssImage", image_ref(image));
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "images");

	// Spriteset:directions
	duk_push_array(ctx);
	for (i = 0; i < spriteset_num_poses(spriteset); ++i) {
		pose_name = spriteset_pose_name(spriteset, i);
		duk_push_object(ctx);
		duk_push_string(ctx, pose_name);
		duk_put_prop_string(ctx, -2, "name");
		duk_push_array(ctx);
		for (j = 0; j < spriteset_num_frames(spriteset, pose_name); ++j) {
			duk_push_object(ctx);
			duk_push_int(ctx, spriteset_frame_image_index(spriteset, pose_name, j));
			duk_put_prop_string(ctx, -2, "index");
			duk_push_int(ctx, spriteset_frame_delay(spriteset, pose_name, j));
			duk_put_prop_string(ctx, -2, "delay");
			duk_put_prop_index(ctx, -2, j);
		}
		duk_put_prop_string(ctx, -2, "frames");
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "directions");
}

static int
duk_require_map_layer(duk_context* ctx, duk_idx_t index)
{
	long        strtol_out;
	int         layer_index;
	const char* name;
	char*       p_end;

	duk_require_type_mask(ctx, index, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);

	if (duk_is_number(ctx, index)) {
		layer_index = duk_get_int(ctx, index);
		goto have_index;
	}
	else {
		// don't anyone ever say I'm not dedicated to compatibility!  there are a few
		// poorly written Sphere 1.5 games that pass layer IDs as strings.  usually this
		// would fail because miniSphere supports named layers, but here I go out of my
		// way to support it anyway.
		name = duk_get_string(ctx, index);
		strtol_out = strtol(name, &p_end, 0);
		if ((layer_index = map_layer_by_name(name)) >= 0)
			goto have_index;
		else if (name[0] != '\0' && *p_end == '\0') {
			layer_index = (int)strtol_out;
			goto have_index;
		}
		else
			duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "layer name does not exist '%s'", name);
	}

have_index:
	if (layer_index < 0 || layer_index >= map_num_layers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "layer index out of range");
	return layer_index;
}

color_t
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	int r, g, b;
	int a;

	duk_require_class_obj(ctx, index, "ssColor");
	duk_get_prop_string(ctx, index, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	a = a < 0 ? 0 : a > 255 ? 255 : a;
	return color_new(r, g, b, a);
}

colormatrix_t
duk_require_sphere_colormatrix(duk_context* ctx, duk_idx_t index)
{
	colormatrix_t matrix;

	duk_require_class_obj(ctx, index, "ssColorMatrix");
	duk_get_prop_string(ctx, index, "rn"); matrix.rn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rr"); matrix.rr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rg"); matrix.rg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rb"); matrix.rb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gn"); matrix.gn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gr"); matrix.gr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gg"); matrix.gg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gb"); matrix.gb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bn"); matrix.bn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "br"); matrix.br = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bg"); matrix.bg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bb"); matrix.bb = duk_get_int(ctx, -1); duk_pop(ctx);
	return matrix;
}

script_t*
duk_require_sphere_script(duk_context* ctx, duk_idx_t index, const char* name)
{
	lstring_t* codestring;
	script_t*  script;

	index = duk_require_normalize_index(ctx, index);

	if (duk_is_callable(ctx, index)) {
		// caller passed function directly
		script = script_new_func(ctx, index);
	}
	else if (duk_is_string(ctx, index)) {
		// caller passed code string, compile it
		codestring = duk_require_lstring_t(ctx, index);
		script = script_new(codestring, "%s", name);
		lstr_free(codestring);
	}
	else if (duk_is_null_or_undefined(ctx, index))
		return NULL;
	else
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "expected string, function, or null/undefined");
	return script;
}

spriteset_t*
duk_require_sphere_spriteset(duk_context* ctx, duk_idx_t index)
{
	// note: this creates a new spriteset from the JavaScript object passed in, which
	//       is rather inefficient, but consistent with Sphere 1.x behavior.  I assume
	//       this was the easiest way to allow JS code to modify spritesets, as it
	//       avoids the need to trap property accesses.

	rect_t       base;
	int          delay;
	image_t*     image;
	int          image_index;
	int          num_frames;
	int          num_images;
	int          num_poses;
	const char*  pose_name;
	spriteset_t* spriteset;

	int i, j;

	index = duk_require_normalize_index(ctx, index);
	duk_require_class_obj(ctx, index, "ssSpriteset");

	spriteset = spriteset_new();

	duk_get_prop_string(ctx, index, "base");
	duk_get_prop_string(ctx, -1, "x1");
	base.x1 = duk_require_int(ctx, -1);
	duk_get_prop_string(ctx, -2, "y1");
	base.y1 = duk_require_int(ctx, -1);
	duk_get_prop_string(ctx, -3, "x2");
	base.x2 = duk_require_int(ctx, -1);
	duk_get_prop_string(ctx, -4, "y2");
	base.y2 = duk_require_int(ctx, -1);
	spriteset_set_base(spriteset, base);
	duk_pop_n(ctx, 5);

	duk_get_prop_string(ctx, index, "images");
	num_images = (int)duk_get_length(ctx, -1);
	for (i = 0; i < num_images; ++i) {
		duk_get_prop_index(ctx, -1, i);
		image = duk_require_class_obj(ctx, -1, "ssImage");
		spriteset_add_image(spriteset, image);
		duk_pop(ctx);
	}
	duk_pop(ctx);

	duk_get_prop_string(ctx, index, "directions");
	num_poses = (int)duk_get_length(ctx, -1);
	for (i = 0; i < num_poses; ++i) {
		duk_get_prop_index(ctx, -1, i);
		duk_get_prop_string(ctx, -1, "name");
		pose_name = duk_require_string(ctx, -1);
		spriteset_add_pose(spriteset, pose_name);
		duk_get_prop_string(ctx, -2, "frames");
		num_frames = (int)duk_get_length(ctx, -1);
		for (j = 0; j < num_frames; ++j) {
			duk_get_prop_index(ctx, -1, j);
			duk_get_prop_string(ctx, -1, "index");
			image_index = duk_require_int(ctx, -1);
			duk_get_prop_string(ctx, -2, "delay");
			delay = duk_require_int(ctx, -1);
			spriteset_add_frame(spriteset, pose_name, image_index, delay);
			duk_pop_3(ctx);
		}
		duk_pop_3(ctx);
	}
	duk_pop(ctx);

	return spriteset;
}

static void
duk_push_sphere_windowstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_class_obj(ctx, "ssWindowStyle", winstyle_ref(winstyle));
	duk_push_sphere_color(ctx, color_new(255, 255, 255, 255));
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

static uint8_t*
duk_require_rgba_lut(duk_context* ctx, duk_idx_t index)
{
	uint8_t* lut;
	int      length;

	int i;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	lut = malloc(sizeof(uint8_t) * 256);
	length = fmin(duk_get_length(ctx, index), 256);
	for (i = length; i < 256; ++i) lut[i] = i;
	for (i = 0; i < length; ++i) {
		duk_get_prop_index(ctx, index, i);
		lut[i] = fmin(fmax(duk_to_int(ctx, -1), 0), 255);
		duk_pop(ctx);
	}
	return lut;
}

static void
use_sphere_blend_mode(int blend_mode)
{
	switch (blend_mode) {
	case BLEND_BLEND:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
			ALLEGRO_ADD, ALLEGRO_INVERSE_DEST_COLOR, ALLEGRO_ONE);
		break;
	case BLEND_REPLACE:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
		break;
	case BLEND_ADD:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	case BLEND_SUBTRACT:
		al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	case BLEND_MULTIPLY:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_DEST_COLOR, ALLEGRO_ZERO,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	case BLEND_INVERT:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_INVERSE_SRC_COLOR,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	}
}

static void
reset_blend_modes(void)
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	const char* filename;
	int         line_number;
	const char* message;
	int         num_args;
	char*       text;

	num_args = duk_get_top(ctx);
	message = num_args >= 1
		? duk_to_string(ctx, 0)
		: "some type of eaty pig just ate your game\n\n\n...and you*munch*";

	duk_inspect_callstack_entry(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber");
	duk_get_prop_string(ctx, -2, "function");
	duk_get_prop_string(ctx, -1, "fileName");
	filename = duk_get_string(ctx, -1);
	line_number = duk_get_int(ctx, -2);
	text = strnewf("%s:%d\nmanual abort\n\n%s", filename, line_number, message);
	sphere_abort(text);
}

static duk_ret_t
js_AddTrigger(duk_context* ctx)
{
	rect_t    bounds;
	int       layer;
	script_t* script;
	int       x;
	int       y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);
	script = duk_require_sphere_script(ctx, 3, "%/triggerScript.js");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	bounds = map_bounds();
	if (x < bounds.x1 || y < bounds.y1 || x >= bounds.x2 || y >= bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "X/Y out of bounds");

	if (!map_add_trigger(x, y, layer, script))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create trigger");
	duk_push_number(ctx, map_num_triggers() - 1);
	return 1;
}

static duk_ret_t
js_AddZone(duk_context* ctx)
{
	rect_t    bounds;
	int       height;
	int       layer;
	script_t* script;
	int       width;
	int       x;
	int       y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);
	layer = duk_require_map_layer(ctx, 4);
	script = duk_require_sphere_script(ctx, 5, "%/zoneScript.js");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone width/height");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "zone out of bounds");

	if (!map_add_zone(rect(x, y, width, height), layer, script, 8))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create zone");
	duk_push_number(ctx, map_num_zones() - 1);
	script_unref(script);
	return 1;
}

static duk_ret_t
js_ApplyColorMask(duk_context* ctx)
{
	color_t color;
	size2_t resolution;

	color = duk_require_sphere_color(ctx, 0);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	resolution = screen_size(g_screen);
	al_draw_filled_rectangle(0, 0, resolution.width, resolution.height,
		nativecolor(color));
	return 0;
}

static duk_ret_t
js_AreKeysLeft(duk_context* ctx)
{
	update_input();
	duk_push_boolean(ctx, kb_queue_len() > 0);
	return 1;
}

static duk_ret_t
js_AreZonesAt(duk_context* ctx)
{
	int layer;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	duk_push_boolean(ctx, map_zone_at(x, y, layer, 0) >= 0);
	return 1;
}

static duk_ret_t
js_AttachCamera(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	map_engine_set_subject(person);
	return 0;
}

static duk_ret_t
js_AttachInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	map_engine_set_player(PLAYER_1, person);
	return 0;
}

static duk_ret_t
js_AttachPlayerInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	player_id_t player;

	name = duk_require_string(ctx, 0);
	player = duk_to_int(ctx, 1);

	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	map_engine_set_player(player, person);
	return 0;
}

static duk_ret_t
js_BezierCurve(duk_context* ctx)
{
	color_t         color;
	float           cp[8];
	bool            is_quadratic = true;
	int             num_args;
	int             num_points;
	ALLEGRO_VERTEX* points;
	double          step_size;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	num_args = duk_get_top(ctx);
	color = duk_require_sphere_color(ctx, 0);
	step_size = duk_to_number(ctx, 1);
	x1 = duk_to_number(ctx, 2);
	y1 = duk_to_number(ctx, 3);
	x2 = duk_to_number(ctx, 4);
	y2 = duk_to_number(ctx, 5);
	x3 = duk_to_number(ctx, 6);
	y3 = duk_to_number(ctx, 7);
	if (num_args >= 10) {
		is_quadratic = false;
		x4 = duk_to_number(ctx, 8);
		y4 = duk_to_number(ctx, 9);
	}

	if (screen_skip_frame(g_screen))
		return 0;
	cp[0] = x1; cp[1] = y1;
	cp[2] = x2; cp[3] = y2;
	cp[4] = x3; cp[5] = y3;
	cp[6] = x4; cp[7] = y4;
	if (is_quadratic) {
		// convert quadratic Bezier curve to cubic
		cp[6] = x3; cp[7] = y3;
		cp[2] = x1 + (2.0 / 3.0) * (x2 - x1);
		cp[3] = y1 + (2.0 / 3.0) * (y2 - y1);
		cp[4] = x3 + (2.0 / 3.0) * (x2 - x3);
		cp[5] = y3 + (2.0 / 3.0) * (y2 - y3);
	}
	step_size = step_size < 0.001 ? 0.001 : step_size > 1.0 ? 1.0 : step_size;
	num_points = 1.0 / step_size;
	points = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	al_calculate_spline(&points[0].x, sizeof(ALLEGRO_VERTEX), cp, 0.0, num_points);
	for (i = 0; i < num_points; ++i)
		points[i].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(points, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	free(points);
	return 0;
}

static duk_ret_t
js_BindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_to_int(ctx, 0);
	int button = duk_to_int(ctx, 1);
	script_t* on_down_script = duk_require_sphere_script(ctx, 2, "[button-down script]");
	script_t* on_up_script = duk_require_sphere_script(ctx, 3, "[button-up script]");

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "joystick index `%d` out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "button index `%d` out of range", button);
	joy_bind_button(joy_index, button, on_down_script, on_up_script);
	return 0;
}

static duk_ret_t
js_BindKey(duk_context* ctx)
{
	int keycode = duk_to_int(ctx, 0);
	script_t* on_down_script = duk_require_sphere_script(ctx, 1, "[key-down script]");
	script_t* on_up_script = duk_require_sphere_script(ctx, 2, "[key-up script]");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid key constant");
	kb_bind_key(keycode, on_down_script, on_up_script);
	return 0;
}

static duk_ret_t
js_BlendColors(duk_context* ctx)
{
	color_t color1;
	color_t color2;
	int     num_args;
	float   w1 = 1.0;
	float   w2 = 1.0;

	num_args = duk_get_top(ctx);
	color1 = duk_require_sphere_color(ctx, 0);
	color2 = duk_require_sphere_color(ctx, 1);
	if (num_args > 2) {
		w1 = duk_to_number(ctx, 2);
		w2 = duk_to_number(ctx, 3);
	}

	if (w1 < 0.0 || w2 < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid weight(s)", w1, w2);

	duk_push_sphere_color(ctx, color_mix(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_CallDefaultMapScript(duk_context* ctx)
{
	int map_op;

	map_op = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (map_op < 0 || map_op >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid script type constant");
	map_call_default(map_op);
	return 0;
}

static duk_ret_t
js_CallDefaultPersonScript(duk_context* ctx)
{
	person_t*   actor = NULL;
	const char* name;
	person_t*   person;
	int         type;

	name = duk_require_string(ctx, 0);
	type = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		actor = person;
	person_call_default(person, type, actor);
	return 0;
}

static duk_ret_t
js_CallMapScript(duk_context* ctx)
{
	int type;

	type = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid script type constant");
	map_activate(type, false);
	return 0;
}

static duk_ret_t
js_CallPersonScript(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         type;

	name = duk_require_string(ctx, 0);
	type = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		person_activate(person, type, person, false);
	else
		person_activate(person, type, NULL, false);
	return 0;
}

static duk_ret_t
js_ChangeMap(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_pathname(ctx, 0, "maps", true, false);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (!map_engine_change_map(filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load map `%s`", filename);
	return 0;
}

static duk_ret_t
js_ClearPersonCommands(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_clear_queue(person);
	return 0;
}

static duk_ret_t
js_CreateByteArray(duk_context* ctx)
{
	bytearray_t* array;
	int          size;

	size = duk_to_int(ctx, 0);

	if (size < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid byte array size");
	if (!(array = bytearray_new(size)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create byte array");
	duk_push_sphere_bytearray(ctx, array);
	bytearray_unref(array);
	return 1;
}

static duk_ret_t
js_CreateByteArrayFromString(duk_context* ctx)
{
	bytearray_t* array;
	lstring_t*   string;

	string = duk_require_lstring_t(ctx, 0);

	if (!(array = bytearray_from_lstring(string)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create byte array");
	lstr_free(string);
	duk_push_sphere_bytearray(ctx, array);
	bytearray_unref(array);
	return 1;
}

static duk_ret_t
js_CreateColor(duk_context* ctx)
{
	int num_args = duk_get_top(ctx);
	int r = duk_to_int(ctx, 0);
	int g = duk_to_int(ctx, 1);
	int b = duk_to_int(ctx, 2);
	int a = num_args >= 4 ? duk_to_int(ctx, 3) : 255;

	// clamp components to 8-bit [0-255]
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	a = a < 0 ? 0 : a > 255 ? 255 : a;

	// construct a Color object
	duk_push_class_obj(ctx, "ssColor", NULL);
	duk_push_int(ctx, r); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, g); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, b); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, a); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

static duk_ret_t
js_CreateColorMatrix(duk_context* ctx)
{
	int rn = duk_to_int(ctx, 0);
	int rr = duk_to_int(ctx, 1);
	int rg = duk_to_int(ctx, 2);
	int rb = duk_to_int(ctx, 3);
	int gn = duk_to_int(ctx, 4);
	int gr = duk_to_int(ctx, 5);
	int gg = duk_to_int(ctx, 6);
	int gb = duk_to_int(ctx, 7);
	int bn = duk_to_int(ctx, 8);
	int br = duk_to_int(ctx, 9);
	int bg = duk_to_int(ctx, 10);
	int bb = duk_to_int(ctx, 11);

	// construct a ColorMatrix object
	duk_push_class_obj(ctx, "ssColorMatrix", NULL);
	duk_push_int(ctx, rn); duk_put_prop_string(ctx, -2, "rn");
	duk_push_int(ctx, rr); duk_put_prop_string(ctx, -2, "rr");
	duk_push_int(ctx, rg); duk_put_prop_string(ctx, -2, "rg");
	duk_push_int(ctx, rb); duk_put_prop_string(ctx, -2, "rb");
	duk_push_int(ctx, gn); duk_put_prop_string(ctx, -2, "gn");
	duk_push_int(ctx, gr); duk_put_prop_string(ctx, -2, "gr");
	duk_push_int(ctx, gg); duk_put_prop_string(ctx, -2, "gg");
	duk_push_int(ctx, gb); duk_put_prop_string(ctx, -2, "gb");
	duk_push_int(ctx, bn); duk_put_prop_string(ctx, -2, "bn");
	duk_push_int(ctx, br); duk_put_prop_string(ctx, -2, "br");
	duk_push_int(ctx, bg); duk_put_prop_string(ctx, -2, "bg");
	duk_push_int(ctx, bb); duk_put_prop_string(ctx, -2, "bb");
	return 1;
}

static duk_ret_t
js_CreateDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_pathname(ctx, 0, "save", true, true);

	if (!game_mkdir(g_game, name))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create directory `%s`", name);
	return 0;
}

static duk_ret_t
js_CreatePerson(duk_context* ctx)
{
	bool         destroy_with_map;
	const char*  filename;
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = duk_require_string(ctx, 0);
	destroy_with_map = duk_to_boolean(ctx, 2);

	if (duk_is_class_obj(ctx, 1, "ssSpriteset")) {
		// ref the spriteset so we can safely free it later. this avoids
		// having to check the argument type again.
		spriteset = duk_require_sphere_spriteset(ctx, 1);
	}
	else {
		filename = duk_require_pathname(ctx, 1, "spritesets", true, false);
		if (!(spriteset = spriteset_load(filename)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load spriteset `%s`", filename);
	}

	// create the person and its JS-side data object
	person = person_new(name, spriteset, !destroy_with_map, NULL);
	spriteset_unref(spriteset);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, name);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_CreateSpriteset(duk_context* ctx)
{
	int          height;
	image_t*     image;
	int          num_frames;
	int          num_images;
	int          num_poses;
	char         pose_name[32];
	spriteset_t* spriteset;
	int          width;

	int i, j;

	width = duk_to_int(ctx, 0);
	height = duk_to_int(ctx, 1);
	num_images = duk_to_int(ctx, 2);
	num_poses = duk_to_int(ctx, 3);
	num_frames = duk_to_int(ctx, 4);

	spriteset = spriteset_new();
	spriteset_set_base(spriteset, rect(0, 0, width, height));
	image = image_new(width, height);
	for (i = 0; i < num_images; ++i) {
		// use the same image in all slots to save memory.  this works because
		// images are read-only, so it doesn't matter that they all share the same
		// pixel data.
		spriteset_add_image(spriteset, image);
	}
	image_unref(image);
	for (i = 0; i < num_poses; ++i) {
		sprintf(pose_name, "unnamed %d", i + 1);
		spriteset_add_pose(spriteset, pose_name);
		for (j = 0; j < num_frames; ++j)
			spriteset_add_frame(spriteset, pose_name, 0, 8);
	}
	duk_push_sphere_spriteset(ctx, spriteset);
	spriteset_unref(spriteset);
	return 1;
}

static duk_ret_t
js_CreateStringFromByteArray(duk_context* ctx)
{
	bytearray_t* array;
	uint8_t*     buffer;
	size_t       size;

	array = duk_require_class_obj(ctx, 0, "ssByteArray");

	buffer = bytearray_buffer(array);
	size = bytearray_len(array);
	duk_push_lstring(ctx, (char*)buffer, size);
	return 1;
}

static duk_ret_t
js_CreateStringFromCode(duk_context* ctx)
{
	int  code;

	code = duk_to_int(ctx, 0);

	if (code < 0 || code > 255)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid ASCII character code");

	duk_push_sprintf(ctx, "%c", code);
	return 1;
}

static duk_ret_t
js_CreateSurface(duk_context* ctx)
{
	color_t     fill_color;
	int         height;
	image_t*    image;
	int         num_args;
	int         width;

	num_args = duk_get_top(ctx);
	width = duk_to_int(ctx, 0);
	height = duk_to_int(ctx, 1);
	fill_color = num_args >= 3 ? duk_require_sphere_color(ctx, 2) : color_new(0, 0, 0, 0);

	if (!(image = image_new(width, height)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create surface");
	image_fill(image, fill_color);
	duk_push_class_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_DeflateByteArray(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	bytearray_t* array = duk_require_class_obj(ctx, 0, "ssByteArray");
	int level = n_args >= 2 ? duk_to_int(ctx, 1) : -1;

	bytearray_t* new_array;

	if ((level < 0 || level > 9) && n_args >= 2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid compression level");
	if (!(new_array = bytearray_deflate(array, level)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't deflate byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double timeout;

	timeout = duk_to_number(ctx, 0);

	if (timeout < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid delay timeout");
	sphere_sleep(floor(timeout) / 1000);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_free(person);
	return 0;
}

static duk_ret_t
js_DetachCamera(duk_context* ctx)
{
	map_engine_set_subject(NULL);
	return 0;
}

static duk_ret_t
js_DetachInput(duk_context* ctx)
{
	map_engine_set_player(PLAYER_1, NULL);
	return 0;
}

static duk_ret_t
js_DetachPlayerInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (duk_get_top(ctx) < 1) {
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not a number or string");
	}
	else if (duk_is_string(ctx, 0)) {
		name = duk_get_string(ctx, 0);
		if (!(person = map_person_by_name(name)))
			duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
		player = -1;
		for (i = PLAYER_1; i < PLAYER_MAX; ++i) {
			if (person == map_engine_get_player(i)) {
				player = i;
				break;
			}
		}
		if (player == -1)
			return 0;
	}
	else if (duk_is_number(ctx, 0)) {
		player = duk_get_int(ctx, 0);
	}
	else {
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not a number or string");
	}
	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");
	map_engine_set_player(player, NULL);
	return 0;
}

static duk_ret_t
js_DoEvents(duk_context* ctx)
{
	sphere_run(true);
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_DoesFileExist(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_pathname(ctx, 0, NULL, true, false);

	duk_push_boolean(ctx, game_file_exists(g_game, filename));
	return 1;
}

static duk_ret_t
js_DoesPersonExist(duk_context* ctx)
{
	const char* name;

	name = duk_require_string(ctx, 0);

	duk_push_boolean(ctx, map_person_by_name(name) != NULL);
	return 1;
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_pathname(ctx, 0, "scripts", true, false);

	if (!game_file_exists(g_game, filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file `%s` not found", filename);
	if (!script_eval(filename, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char path[SPHERE_PATH_MAX];

	sprintf(path, "@/scripts/lib/%s", filename);
	if (!game_file_exists(g_game, path))
		sprintf(path, "#/scripts/%s", filename);
	if (!game_file_exists(g_game, path))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "system script `%s` not found", filename);
	if (!script_eval(path, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	path_t*     games_path;
	const char* filename;

	filename = duk_require_string(ctx, 0);

	// store the old game path so we can relaunch when the chained game exits
	g_last_game_path = path_dup(game_path(g_game));

	// if the passed-in path is relative, resolve it relative to <engine>/games.
	// this is done for compatibility with Sphere 1.x.
	g_game_path = path_new(filename);
	games_path = path_rebase(path_new("games/"), assets_path());
	path_rebase(g_game_path, games_path);
	path_free(games_path);

	sphere_restart();
}

static duk_ret_t
js_ExecuteTrigger(duk_context* ctx)
{
	int index;
	int layer;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if ((index = map_trigger_at(x, y, layer)) >= 0)
		trigger_activate(index);
	return 0;
}

static duk_ret_t
js_ExecuteZoneScript(duk_context* ctx)
{
	int zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	zone_activate(zone_index);
	return 0;
}

static duk_ret_t
js_ExecuteZones(duk_context* ctx)
{
	int index;
	int layer;
	int x;
	int y;

	int i;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	i = 0;
	while ((index = map_zone_at(x, y, layer, i++)) >= 0)
		zone_activate(index);
	return 0;
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	sphere_exit(false);
}

static duk_ret_t
js_ExitMapEngine(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	map_engine_exit();
	return 0;
}

static duk_ret_t
js_FilledCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int radius = duk_to_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	double phi;
	int    vcount;

	int i;

	if (screen_skip_frame(g_screen))
		return 0;
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_FilledComplex(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_FilledEllipse(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = trunc(duk_to_number(ctx, 0));
	int y = trunc(duk_to_number(ctx, 1));
	int rx = trunc(duk_to_number(ctx, 2));
	int ry = trunc(duk_to_number(ctx, 3));
	color_t color = duk_require_sphere_color(ctx, 4);

	double phi;
	int    vcount;

	int i;

	if (screen_skip_frame(g_screen))
		return 0;
	vcount = ceil(fmin(10 * sqrt((rx + ry) / 2), 126));
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * rx;
		s_vbuf[i + 1].y = y - sin(phi) * ry;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(color);
	}
	s_vbuf[i + 1].x = x + cos(0) * rx;
	s_vbuf[i + 1].y = y - sin(0) * ry;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	screen_flip(g_screen, s_frame_rate, true);
	return 0;
}

static duk_ret_t
js_FollowPerson(duk_context* ctx)
{
	int         distance = 0;
	person_t*   leader = NULL;
	const char* leader_name = "";
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	if (!duk_is_null(ctx, 1))
		leader_name = duk_require_string(ctx, 1);
	if (leader_name[0] != '\0')
		distance = duk_require_int(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!(leader_name[0] == '\0' || (leader = map_person_by_name(leader_name))))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", leader_name);
	if (distance <= 0 && leader_name[0] != '\0')
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid distance");
	if (!person_set_leader(person, leader, distance))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid circular follow chain");
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
js_GetActingPerson(duk_context* ctx)
{
	const person_t* person;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	person = map_engine_acting_person();
	if (person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no current person activation or actor unknown");
	duk_push_string(ctx, person_name(person));
	return 1;
}

static duk_ret_t
js_GetCameraPerson(duk_context* ctx)
{
	person_t* subject;

	subject = map_engine_get_subject();
	if (subject == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid operation, camera not attached");
	duk_push_string(ctx, person_name(subject));
	return 1;
}

static duk_ret_t
js_GetCameraX(duk_context* ctx)
{
	point2_t position;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	position = map_get_camera_xy();
	duk_push_int(ctx, position.x);
	return 1;
}

static duk_ret_t
js_GetCameraY(duk_context* ctx)
{
	point2_t position;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	position = map_get_camera_xy();
	duk_push_int(ctx, position.y);
	return 1;
}

static duk_ret_t
js_GetClippingRectangle(duk_context* ctx)
{
	rect_t clip;

	clip = image_get_scissor(screen_backbuffer(g_screen));

	duk_push_object(ctx);
	duk_push_int(ctx, clip.x1);
	duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, clip.y1);
	duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, clip.x2 - clip.x1);
	duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, clip.y2 - clip.y1);
	duk_put_prop_string(ctx, -2, "height");
	return 1;
}

static duk_ret_t
js_GetCurrentMap(duk_context* ctx)
{
	path_t* path;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	// GetCurrentMap() in Sphere 1.x returns the map path relative to the
	// 'maps' directory.
	path = game_relative_path(g_game, map_pathname(), "@/maps");
	duk_push_string(ctx, path_cstr(path));
	path_free(path);
	return 1;
}

static duk_ret_t
js_GetCurrentPerson(duk_context* ctx)
{
	const person_t* person;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	person = map_engine_active_person();
	if (person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no current person activation");
	duk_push_string(ctx, person_name(person));
	return 1;
}

static duk_ret_t
js_GetCurrentTrigger(duk_context* ctx)
{
	int trigger;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	trigger = map_engine_active_trigger();
	if (trigger == -1)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no current trigger activation");
	duk_push_int(ctx, trigger);
	return 1;
}

static duk_ret_t
js_GetCurrentZone(duk_context* ctx)
{
	int zone;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	zone = map_engine_active_zone();
	if (zone == -1)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no current zone activation");
	duk_push_int(ctx, zone);
	return 1;
}

static duk_ret_t
js_GetDirectoryList(duk_context* ctx)
{
	directory_t*  dir;
	const char*   dir_name = "@/";
	const path_t* entry;
	duk_uarridx_t index = 0;
	int           num_args;

	num_args = duk_get_top(ctx);
	if (num_args >= 1)
		dir_name = duk_require_pathname(ctx, 0, NULL, true, false);

	duk_push_array(ctx);
	if (!(dir = directory_open(g_game, dir_name)))
		return 1;
	while (entry = directory_next(dir)) {
		if (path_is_file(entry))
			continue;
		duk_push_string(ctx, path_hop(entry, path_num_hops(entry) - 1));
		duk_put_prop_index(ctx, -2, index++);
	}
	directory_close(dir);
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	directory_t*  dir;
	const char*   dir_name = "@/save";
	const path_t* entry;
	duk_uarridx_t index = 0;
	int           num_args;

	num_args = duk_get_top(ctx);
	if (num_args >= 1)
		dir_name = duk_require_pathname(ctx, 0, NULL, true, false);

	duk_push_array(ctx);
	if (!(dir = directory_open(g_game, dir_name)))
		return 1;
	while (entry = directory_next(dir)) {
		if (!path_is_file(entry))
			continue;
		duk_push_string(ctx, path_filename(entry));
		duk_put_prop_index(ctx, -2, index++);
	}
	directory_close(dir);
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_frame_rate);
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_FS_ENTRY* fse;
	game_t*           game;
	path_t*           path = NULL;
	path_t*           paths[2];

	int i, j = 0;

	// build search paths
	paths[0] = path_rebase(path_new("games/"), engine_path());
	paths[1] = path_rebase(path_new("miniSphere/Games/"), home_path());

	// search for supported games
	duk_push_array(ctx);
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				path = path_new(al_get_fs_entry_name(file_info));
				if (game = game_open(path_cstr(path))) {
					duk_push_lstring_t(ctx, game_manifest(game));
					duk_json_decode(ctx, -1);
					duk_push_string(ctx, path_cstr(path));
					duk_put_prop_string(ctx, -2, "directory");
					duk_put_prop_index(ctx, -2, j++);
					game_unref(game);
				}
				path_free(path);
			}
		}
		al_destroy_fs_entry(fse);
		path_free(paths[i]);
	}
	return 1;
}

static duk_ret_t
js_GetInputPerson(duk_context* ctx)
{
	int       num_args;
	person_t* person;
	int       player = PLAYER_1;

	num_args = duk_get_top(ctx);
	if (num_args >= 1)
		player = duk_to_int(ctx, 0);

	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");
	person = map_engine_get_player(player);
	if (person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "player %d not attached", player + 1);
	duk_push_string(ctx, person_name(person));
	return 1;
}

static duk_ret_t
js_GetJoystickAxis(duk_context* ctx)
{
	int joy_index = duk_to_int(ctx, 0);
	int axis_index = duk_to_int(ctx, 1);

	duk_push_number(ctx, joy_position(joy_index, axis_index));
	return 1;
}

static duk_ret_t
js_GetKey(duk_context* ctx)
{
	while (kb_queue_len() == 0) {
		sphere_sleep(0.05);
		sphere_run(false);
	}
	duk_push_int(ctx, kb_get_key());
	return 1;
}

static duk_ret_t
js_GetKeyString(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int keycode = duk_to_int(ctx, 0);
	bool shift = n_args >= 2 ? duk_to_boolean(ctx, 1) : false;

	switch (keycode) {
	case ALLEGRO_KEY_A: duk_push_string(ctx, shift ? "A" : "a"); break;
	case ALLEGRO_KEY_B: duk_push_string(ctx, shift ? "B" : "b"); break;
	case ALLEGRO_KEY_C: duk_push_string(ctx, shift ? "C" : "c"); break;
	case ALLEGRO_KEY_D: duk_push_string(ctx, shift ? "D" : "d"); break;
	case ALLEGRO_KEY_E: duk_push_string(ctx, shift ? "E" : "e"); break;
	case ALLEGRO_KEY_F: duk_push_string(ctx, shift ? "F" : "f"); break;
	case ALLEGRO_KEY_G: duk_push_string(ctx, shift ? "G" : "g"); break;
	case ALLEGRO_KEY_H: duk_push_string(ctx, shift ? "H" : "h"); break;
	case ALLEGRO_KEY_I: duk_push_string(ctx, shift ? "I" : "i"); break;
	case ALLEGRO_KEY_J: duk_push_string(ctx, shift ? "J" : "j"); break;
	case ALLEGRO_KEY_K: duk_push_string(ctx, shift ? "K" : "k"); break;
	case ALLEGRO_KEY_L: duk_push_string(ctx, shift ? "L" : "l"); break;
	case ALLEGRO_KEY_M: duk_push_string(ctx, shift ? "M" : "m"); break;
	case ALLEGRO_KEY_N: duk_push_string(ctx, shift ? "N" : "n"); break;
	case ALLEGRO_KEY_O: duk_push_string(ctx, shift ? "O" : "o"); break;
	case ALLEGRO_KEY_P: duk_push_string(ctx, shift ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: duk_push_string(ctx, shift ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: duk_push_string(ctx, shift ? "R" : "r"); break;
	case ALLEGRO_KEY_S: duk_push_string(ctx, shift ? "S" : "s"); break;
	case ALLEGRO_KEY_T: duk_push_string(ctx, shift ? "T" : "t"); break;
	case ALLEGRO_KEY_U: duk_push_string(ctx, shift ? "U" : "u"); break;
	case ALLEGRO_KEY_V: duk_push_string(ctx, shift ? "V" : "v"); break;
	case ALLEGRO_KEY_W: duk_push_string(ctx, shift ? "W" : "w"); break;
	case ALLEGRO_KEY_X: duk_push_string(ctx, shift ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: duk_push_string(ctx, shift ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: duk_push_string(ctx, shift ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: duk_push_string(ctx, shift ? "!" : "1"); break;
	case ALLEGRO_KEY_2: duk_push_string(ctx, shift ? "@" : "2"); break;
	case ALLEGRO_KEY_3: duk_push_string(ctx, shift ? "#" : "3"); break;
	case ALLEGRO_KEY_4: duk_push_string(ctx, shift ? "$" : "4"); break;
	case ALLEGRO_KEY_5: duk_push_string(ctx, shift ? "%" : "5"); break;
	case ALLEGRO_KEY_6: duk_push_string(ctx, shift ? "^" : "6"); break;
	case ALLEGRO_KEY_7: duk_push_string(ctx, shift ? "&" : "7"); break;
	case ALLEGRO_KEY_8: duk_push_string(ctx, shift ? "*" : "8"); break;
	case ALLEGRO_KEY_9: duk_push_string(ctx, shift ? "(" : "9"); break;
	case ALLEGRO_KEY_0: duk_push_string(ctx, shift ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: duk_push_string(ctx, shift ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: duk_push_string(ctx, shift ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: duk_push_string(ctx, shift ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: duk_push_string(ctx, shift ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: duk_push_string(ctx, shift ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: duk_push_string(ctx, shift ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: duk_push_string(ctx, shift ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: duk_push_string(ctx, shift ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: duk_push_string(ctx, shift ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: duk_push_string(ctx, shift ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: duk_push_string(ctx, " "); break;
	case ALLEGRO_KEY_TAB: duk_push_string(ctx, "\t"); break;
	case ALLEGRO_KEY_TILDE: duk_push_string(ctx, shift ? "~" : "`"); break;
	default:
		duk_push_string(ctx, "");
	}
	return 1;
}

static duk_ret_t
js_GetLayerAngle(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_GetLayerHeight(duk_context* ctx)
{
	int     layer;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_int(ctx, layer_size(layer).height);
	return 1;
}

static duk_ret_t
js_GetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_sphere_color(ctx, layer_get_color_mask(layer));
	return 1;
}

static duk_ret_t
js_GetLayerName(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_string(ctx, layer_name(layer));
	return 1;
}

static duk_ret_t
js_GetLayerWidth(duk_context* ctx)
{
	int layer;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_int(ctx, layer_size(layer).width);
	return 1;
}

static duk_ret_t
js_GetLocalAddress(duk_context* ctx)
{
	duk_push_string(ctx, "127.0.0.1");
	return 1;
}

static duk_ret_t
js_GetLocalName(duk_context* ctx)
{
	duk_push_string(ctx, "localhost");
	return 1;
}

static duk_ret_t
js_GetMapEngine(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 1;
}

static duk_ret_t
js_GetMapEngineFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, map_engine_get_framerate());
	return 1;
}

static duk_ret_t
js_GetMouseWheelEvent(duk_context* ctx)
{
	mouse_event_t event;

	do {
		while (mouse_queue_len() == 0) {
			sphere_sleep(0.05);
			sphere_run(false);
		}
		event = mouse_get_event();
	} while (event.key != MOUSE_KEY_WHEEL_UP && event.key != MOUSE_KEY_WHEEL_DOWN);
	duk_push_int(ctx, event.key);
	return 1;
}

static duk_ret_t
js_GetMouseX(duk_context* ctx)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetMouseY(duk_context* ctx)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetNextAnimatedTile(duk_context* ctx)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	duk_push_int(ctx, tileset_get_next(tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetNumJoysticks(duk_context* ctx)
{
	duk_push_int(ctx, joy_num_devices());
	return 1;
}

static duk_ret_t
js_GetNumJoystickAxes(duk_context* ctx)
{
	int joy_index;

	joy_index = duk_to_int(ctx, 0);

	duk_push_int(ctx, joy_num_axes(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumJoystickButtons(duk_context* ctx)
{
	int joy_index;

	joy_index = duk_to_int(ctx, 0);

	duk_push_int(ctx, joy_num_buttons(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumLayers(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_int(ctx, map_num_layers());
	return 1;
}

static duk_ret_t
js_GetNumMouseWheelEvents(duk_context* ctx)
{
	duk_push_int(ctx, mouse_queue_len());
	return 1;
}

static duk_ret_t
js_GetNumTiles(duk_context* ctx)
{
	tileset_t* tileset;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	tileset = map_tileset();
	duk_push_int(ctx, tileset_len(tileset));
	return 1;
}

static duk_ret_t
js_GetNumTriggers(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	duk_push_number(ctx, map_num_triggers());
	return 1;
}

static duk_ret_t
js_GetNumZones(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	duk_push_number(ctx, map_num_zones());
	return 1;
}

static duk_ret_t
js_GetObstructingPerson(duk_context* ctx)
{
	const char* name;
	person_t*   obs_person;
	person_t*   person;
	int         x;
	int         y;

	name = duk_require_string(ctx, 0);
	x = duk_require_int(ctx, 1);
	y = duk_require_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_obstructed_at(person, x, y, &obs_person, NULL);
	duk_push_string(ctx, obs_person != NULL ? person_name(obs_person) : "");
	return 1;
}

static duk_ret_t
js_GetObstructingTile(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         tile_index;
	int         x;
	int         y;

	name = duk_require_string(ctx, 0);
	x = duk_require_int(ctx, 1);
	y = duk_require_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_obstructed_at(person, x, y, NULL, &tile_index);
	duk_push_int(ctx, tile_index);
	return 1;
}

static duk_ret_t
js_GetPersonAngle(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_number(ctx, person_get_angle(person));
	return 1;
}

static duk_ret_t
js_GetPersonBase(duk_context* ctx)
{
	rect_t      base;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	base = spriteset_get_base(person_get_spriteset(person));
	duk_push_object(ctx);
	duk_push_int(ctx, base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, base.y2); duk_put_prop_string(ctx, -2, "y2");
	return 1;
}

static duk_ret_t
js_GetPersonData(duk_context* ctx)
{
	int          height;
	person_t*    leader;
	const char*  name;
	int          num_directions;
	int          num_frames;
	person_t*    person;
	spriteset_t* spriteset;
	int          width;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	leader = person_get_leader(person);
	spriteset = person_get_spriteset(person);
	width = spriteset_width(spriteset);
	height = spriteset_height(spriteset);
	num_directions = spriteset_num_poses(spriteset);
	num_frames = spriteset_num_frames(spriteset, person_get_pose(person));

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_get_prop_string(ctx, -1, name);
	duk_push_int(ctx, num_frames);
	duk_put_prop_string(ctx, -2, "num_frames");
	duk_push_int(ctx, num_directions);
	duk_put_prop_string(ctx, -2, "num_directions");
	duk_push_int(ctx, width);
	duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height);
	duk_put_prop_string(ctx, -2, "height");
	duk_push_string(ctx, person_name(leader));
	duk_put_prop_string(ctx, -2, "leader");
	return 1;
}

static duk_ret_t
js_GetPersonDirection(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_string(ctx, person_get_pose(person));
	return 1;
}

static duk_ret_t
js_GetPersonFollowDistance(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (person_get_leader(person) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "not a follower");
	duk_push_int(ctx, person_get_trailing(person));
	return 1;
}

static duk_ret_t
js_GetPersonFollowers(duk_context* ctx)
{
	vector_t*     all_persons;
	person_t*     candidate;
	duk_uarridx_t index = 0;
	const char*   name;
	person_t*     person;

	iter_t iter;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	all_persons = map_engine_persons();
	duk_push_array(ctx);
	iter = vector_enum(all_persons);
	while (iter_next(&iter)) {
		candidate = *(person_t**)iter.ptr;
		if (person_get_leader(candidate) == person) {
			duk_push_string(ctx, person_name(candidate));
			duk_put_prop_index(ctx, -2, index++);
		}
	}
	return 1;
}

static duk_ret_t
js_GetPersonFrame(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person_get_frame(person));
	return 1;
}

static duk_ret_t
js_GetPersonFrameNext(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person_get_frame_delay(person));
	return 1;
}

static duk_ret_t
js_GetPersonFrameRevert(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person_get_revert_delay(person));
	return 1;
}

static duk_ret_t
js_GetPersonIgnoreList(duk_context* ctx)
{
	vector_t*   ignore_list;
	const char* name;
	person_t*   person;

	iter_t iter;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	ignore_list = person_ignore_list(person);
	iter = vector_enum(ignore_list);
	duk_push_array(ctx);
	while (iter_next(&iter)) {
		duk_push_string(ctx, *(const char**)iter.ptr);
		duk_put_prop_index(ctx, -2, iter.index);
	}
	return 1;
}

static duk_ret_t
js_GetPersonLayer(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person_get_layer(person));
	return 1;
}

static duk_ret_t
js_GetPersonLeader(duk_context* ctx)
{
	person_t*   leader;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	leader = person_get_leader(person);
	duk_push_string(ctx, person_name(leader));
	return 1;
}

static duk_ret_t
js_GetPersonList(duk_context* ctx)
{
	vector_t* all_persons;
	person_t* person;

	iter_t iter;

	all_persons = map_engine_persons();
	iter = vector_enum(all_persons);
	duk_push_array(ctx);
	while (iter_next(&iter)) {
		person = *(person_t**)iter.ptr;
		duk_push_string(ctx, person_name(person));
		duk_put_prop_index(ctx, -2, iter.index);
	}
	return 1;
}

static duk_ret_t
js_GetPersonMask(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_sphere_color(ctx, person_get_color(person));
	return 1;
}

static duk_ret_t
js_GetPersonOffsetX(duk_context* ctx)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	offset = person_get_offset(person);
	duk_push_int(ctx, offset.x);
	return 1;
}

static duk_ret_t
js_GetPersonOffsetY(duk_context* ctx)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	offset = person_get_offset(person);
	duk_push_int(ctx, offset.y);
	return 1;
}

static duk_ret_t
js_GetPersonSpriteset(duk_context* ctx)
{
	const char*  name;
	spriteset_t* spriteset;
	person_t*    person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	spriteset = person_get_spriteset(person);
	duk_push_sphere_spriteset(ctx, spriteset);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedX(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x_speed;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_speed(person, &x_speed, NULL);
	duk_push_number(ctx, x_speed);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedY(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      y_speed;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_speed(person, NULL, &y_speed);
	duk_push_number(ctx, y_speed);
	return 1;
}

static duk_ret_t
js_GetPersonValue(duk_context* ctx)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	key = duk_to_string(ctx, 1);

	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_get_prop_string(ctx, -1, key);
	return 1;
}

static duk_ret_t
js_GetPersonX(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetPersonXFloat(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_number(ctx, x);
	return 1;
}

static duk_ret_t
js_GetPersonY(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetPersonYFloat(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_number(ctx, y);
	return 1;
}

static duk_ret_t
js_GetPlayerKey(duk_context* ctx)
{
	int player;
	int key_type;

	player = duk_to_int(ctx, 0);
	key_type = duk_to_int(ctx, 1);

	if (player < 0 || player >= 4)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player index out of range");
	if (key_type < 0 || key_type >= PLAYER_KEY_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid key type constant");
	duk_push_int(ctx, get_player_key(player, key_type));
	return 1;
}

static duk_ret_t
js_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, screen_size(g_screen).height);
	return 1;
}

static duk_ret_t
js_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, screen_size(g_screen).width);
	return 1;
}

static duk_ret_t
js_GetSystemArrow(duk_context* ctx)
{
	image_t* image;

	if (!(image = legacy_default_arrow_image()))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system arrow image");
	duk_push_class_obj(ctx, "ssImage", image_ref(image));
	return 1;
}

static duk_ret_t
js_GetSystemDownArrow(duk_context* ctx)
{
	image_t* image;

	if (!(image = legacy_default_arrow_down_image()))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system down arrow image");
	duk_push_class_obj(ctx, "ssImage", image_ref(image));
	return 1;
}

static duk_ret_t
js_GetSystemFont(duk_context* ctx)
{
	duk_push_sphere_font(ctx, legacy_default_font());
	return 1;
}

static duk_ret_t
js_GetSystemUpArrow(duk_context* ctx)
{
	image_t* image;

	if (!(image = legacy_default_arrow_up_image()))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system up arrow image");
	duk_push_class_obj(ctx, "ssImage", image_ref(image));
	return 1;
}

static duk_ret_t
js_GetSystemWindowStyle(duk_context* ctx)
{
	windowstyle_t* windowstyle;

	if (!(windowstyle = legacy_default_windowstyle()))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system windowstyle");
	duk_push_sphere_windowstyle(ctx, windowstyle);
	return 1;
}

static duk_ret_t
js_GetTalkActivationButton(duk_context* ctx)
{
	duk_push_int(ctx, map_engine_get_talk_button());
	return 1;
}

static duk_ret_t
js_GetTalkActivationKey(duk_context* ctx)
{
	int num_args;
	int player = PLAYER_1;

	num_args = duk_get_top(ctx);
	if (num_args >= 1)
		player = duk_to_int(ctx, 0);

	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");
	duk_push_int(ctx, map_engine_get_talk_key(player));
	return 1;
}

static duk_ret_t
js_GetTalkDistance(duk_context* ctx)
{
	duk_push_int(ctx, map_engine_get_talk_distance());
	return 1;
}

static duk_ret_t
js_GetTile(duk_context* ctx)
{
	int layer;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_int(ctx, map_tile_at(x, y, layer));
	return 1;
}

static duk_ret_t
js_GetTileDelay(duk_context* ctx)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	duk_push_int(ctx, tileset_get_delay(tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileHeight(duk_context* ctx)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	duk_push_int(ctx, height);
	return 1;
}

static duk_ret_t
js_GetTileImage(duk_context* ctx)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	if (!(image = image_clone(tileset_get_image(tileset, tile_index))))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create new surface image");
	duk_push_class_obj(ctx, "ssImage", image);
	return 1;
}

static duk_ret_t
js_GetTileName(duk_context* ctx)
{
	int              tile_index;
	const lstring_t* tile_name;
	tileset_t*       tileset;

	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tile_name = tileset_get_name(tileset, tile_index);
	duk_push_lstring_t(ctx, tile_name);
	return 1;
}

static duk_ret_t
js_GetTileSurface(duk_context* ctx)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	if (!(image = image_clone(tileset_get_image(tileset, tile_index))))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create new surface image");
	duk_push_class_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_GetTileWidth(duk_context* ctx)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	duk_push_int(ctx, width);
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	duk_push_number(ctx, floor(al_get_time() * 1000));
	return 1;
}

static duk_ret_t
js_GetToggleState(duk_context* ctx)
{
	int keycode;

	keycode = duk_to_int(ctx, 0);

	if (keycode != ALLEGRO_KEY_CAPSLOCK
		&& keycode != ALLEGRO_KEY_NUMLOCK
		&& keycode != ALLEGRO_KEY_SCROLLLOCK)
	{
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid toggle key constant");
	}

	duk_push_boolean(ctx, kb_is_toggled(keycode));
	return 1;
}

static duk_ret_t
js_GetTriggerLayer(duk_context* ctx)
{
	int layer;
	int trigger_index;

	trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, NULL, &layer);
	duk_push_int(ctx, layer);
	return 1;
}

static duk_ret_t
js_GetTriggerX(duk_context* ctx)
{
	int trigger_index;
	int x;

	trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, &x, NULL, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetTriggerY(duk_context* ctx)
{
	int trigger_index;
	int y;

	trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, &y, NULL);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, API_VERSION);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_string(ctx, API_VERSION_STRING);
	return 1;
}

static duk_ret_t
js_GetZoneHeight(duk_context* ctx)
{
	rect_t bounds;
	int    zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.y2 - bounds.y1);
	return 1;
}

static duk_ret_t
js_GetZoneLayer(duk_context* ctx)
{
	int zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	duk_push_int(ctx, zone_get_layer(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneSteps(duk_context* ctx)
{
	int zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	duk_push_int(ctx, zone_get_steps(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneWidth(duk_context* ctx)
{
	rect_t bounds;
	int    zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.x2 - bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneX(duk_context* ctx)
{
	rect_t bounds;
	int    zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneY(duk_context* ctx)
{
	rect_t bounds;
	int    zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.y1);
	return 1;
}

static duk_ret_t
js_GrabImage(duk_context* ctx)
{
	image_t* image;
	int      height;
	int      width;
	int      x;
	int      y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);

	if (!(image = screen_grab(g_screen, x, y, width, height)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't grab screen image");
	duk_push_class_obj(ctx, "ssImage", image);
	return 1;
}

static duk_ret_t
js_GrabSurface(duk_context* ctx)
{
	image_t* image;
	int      height;
	int      width;
	int      x;
	int      y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);

	if (!(image = screen_grab(g_screen, x, y, width, height)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't grab screen image");
	duk_push_class_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_GradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	color_t inner_color;
	int     num_verts;
	color_t outer_color;
	double  phi;
	float   radius;
	float   x;
	float   y;

	int i;

	x = trunc(duk_to_number(ctx, 0));
	y = trunc(duk_to_number(ctx, 1));
	radius = trunc(duk_to_number(ctx, 2));
	inner_color = duk_require_sphere_color(ctx, 3);
	outer_color = duk_require_sphere_color(ctx, 4);

	if (screen_skip_frame(g_screen))
		return 0;
	num_verts = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(inner_color);
	for (i = 0; i < num_verts; ++i) {
		phi = 2 * M_PI * i / num_verts;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(outer_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(outer_color);
	al_draw_prim(s_vbuf, NULL, NULL, 0, num_verts + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_GradientComplex(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_GradientEllipse(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	color_t inner_color;
	int     num_verts;
	color_t outer_color;
	double  phi;
	float   rx, ry;
	float   x;
	float   y;

	int i;

	x = trunc(duk_to_number(ctx, 0));
	y = trunc(duk_to_number(ctx, 1));
	rx = trunc(duk_to_number(ctx, 2));
	ry = trunc(duk_to_number(ctx, 3));
	inner_color = duk_require_sphere_color(ctx, 4);
	outer_color = duk_require_sphere_color(ctx, 5);

	if (screen_skip_frame(g_screen))
		return 0;
	num_verts = ceil(fmin(10 * sqrt((rx + ry) / 2), 126));
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(inner_color);
	for (i = 0; i < num_verts; ++i) {
		phi = 2 * M_PI * i / num_verts;
		s_vbuf[i + 1].x = x + cos(phi) * rx;
		s_vbuf[i + 1].y = y - sin(phi) * ry;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(outer_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * rx;
	s_vbuf[i + 1].y = y - sin(0) * ry;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(outer_color);
	galileo_reset();
	al_draw_prim(s_vbuf, NULL, NULL, 0, num_verts + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_GradientLine(duk_context* ctx)
{
	color_t color1;
	color_t color2;
	float   length;
	float   tx, ty;
	float   x1;
	float   x2;
	float   y1;
	float   y2;

	x1 = trunc(duk_to_number(ctx, 0)) + 0.5;
	y1 = trunc(duk_to_number(ctx, 1)) + 0.5;
	x2 = trunc(duk_to_number(ctx, 2)) + 0.5;
	y2 = trunc(duk_to_number(ctx, 3)) + 0.5;
	color1 = duk_require_sphere_color(ctx, 4);
	color2 = duk_require_sphere_color(ctx, 5);

	if (screen_skip_frame(g_screen))
		return 0;
	length = hypotf(x2 - x1, y2 - y1);
	tx = 0.5 * (y2 - y1) / length;
	ty = 0.5 * -(x2 - x1) / length;
	ALLEGRO_VERTEX verts[] = {
		{ x1 + tx, y1 + ty, 0, 0, 0, nativecolor(color1) },
		{ x1 - tx, y1 - ty, 0, 0, 0, nativecolor(color1) },
		{ x2 - tx, y2 - ty, 0, 0, 0, nativecolor(color2) },
		{ x2 + tx, y2 + ty, 0, 0, 0, nativecolor(color2) },
	};
	galileo_reset();
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_GradientRectangle(duk_context* ctx)
{
	color_t color_ul;
	color_t color_ur;
	color_t color_lr;
	color_t color_ll;
	float   x1, x2;
	float   y1, y2;

	x1 = trunc(duk_to_number(ctx, 0));
	y1 = trunc(duk_to_number(ctx, 1));
	x2 = x1 + trunc(duk_to_number(ctx, 2));
	y2 = y1 + trunc(duk_to_number(ctx, 3));
	color_ul = duk_require_sphere_color(ctx, 4);
	color_ur = duk_require_sphere_color(ctx, 5);
	color_lr = duk_require_sphere_color(ctx, 6);
	color_ll = duk_require_sphere_color(ctx, 7);

	if (screen_skip_frame(g_screen))
		return 0;
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	galileo_reset();
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_GradientTriangle(duk_context* ctx)
{
	color_t color1, color2, color3;
	float   x1, x2, x3;
	float   y1, y2, y3;

	x1 = trunc(duk_to_number(ctx, 0));
	y1 = trunc(duk_to_number(ctx, 1));
	x2 = trunc(duk_to_number(ctx, 2));
	y2 = trunc(duk_to_number(ctx, 3));
	x3 = trunc(duk_to_number(ctx, 4));
	y3 = trunc(duk_to_number(ctx, 5));
	color1 = duk_require_sphere_color(ctx, 6);
	color2 = duk_require_sphere_color(ctx, 7);
	color3 = duk_require_sphere_color(ctx, 8);

	if (screen_skip_frame(g_screen))
		return 0;
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color1) },
		{ x2, y2, 0, 0, 0, nativecolor(color2) },
		{ x3, y3, 0, 0, 0, nativecolor(color3) },
	};
	galileo_reset();
	al_draw_prim(verts, NULL, NULL, 0, 3, ALLEGRO_PRIM_TRIANGLE_LIST);
	return 0;
}

static duk_ret_t
js_HashByteArray(duk_context* ctx)
{
	bytearray_t* array;
	void*        data;
	size_t       size;

	array = duk_require_class_obj(ctx, 0, "ssByteArray");

	data = bytearray_buffer(array);
	size = bytearray_len(array);
	duk_push_string(ctx, md5sum(data, size));
	return 1;
}

static duk_ret_t
js_HashFromFile(duk_context* ctx)
{
	void*       data;
	size_t      file_size;
	const char* filename;

	filename = duk_require_pathname(ctx, 0, "other", true, false);

	if (!(data = game_read_file(g_game, filename, &file_size)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't read file");
	duk_push_string(ctx, md5sum(data, file_size));
	return 1;
}

static duk_ret_t
js_IgnorePersonObstructions(duk_context* ctx)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	ignoring = duk_to_boolean(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_ignore_persons(person, ignoring);
	return 0;
}

static duk_ret_t
js_IgnoreTileObstructions(duk_context* ctx)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	ignoring = duk_to_boolean(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_ignore_tiles(person, ignoring);
	return 0;
}

static duk_ret_t
js_InflateByteArray(duk_context* ctx)
{
	bytearray_t*  array;
	int           max_size;
	bytearray_t*  new_array;
	int           num_args;

	num_args = duk_get_top(ctx);
	array = duk_require_class_obj(ctx, 0, "ssByteArray");
	max_size = num_args >= 2 ? duk_to_int(ctx, 1) : 0;

	if (max_size < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "negative buffer size");
	if (!(new_array = bytearray_inflate(array, max_size)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't inflate byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_IsAnyKeyPressed(duk_context* ctx)
{
	duk_push_boolean(ctx, kb_is_any_key_down());
	return 1;
}

static duk_ret_t
js_IsCameraAttached(duk_context* ctx)
{
	duk_push_boolean(ctx, map_engine_get_subject() != NULL);
	return 1;
}

static duk_ret_t
js_IsCommandQueueEmpty(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, !person_moving(person));
	return 1;
}

static duk_ret_t
js_IsIgnoringPersonObstructions(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person_get_ignore_persons(person));
	return 1;
}

static duk_ret_t
js_IsIgnoringTileObstructions(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person_get_ignore_tiles(person));
	return 1;
}

static duk_ret_t
js_IsInputAttached(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (duk_get_top(ctx) < 1) {
		player = PLAYER_1;
	}
	else if (duk_is_string(ctx, 0)) {
		name = duk_get_string(ctx, 0);
		if (!(person = map_person_by_name(name)))
			duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
		player = -1;
		for (i = PLAYER_1; i < PLAYER_MAX; ++i) {
			if (person == map_engine_get_player(i)) {
				player = i;
				break;
			}
		}
		if (player == -1) {
			duk_push_false(ctx);
			return 1;
		}
	}
	else if (duk_is_number(ctx, 0)) {
		player = duk_get_int(ctx, 0);
	}
	else {
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "not a number or string");
	}
	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");
	duk_push_boolean(ctx, map_engine_get_player(player) != NULL);
	return 1;
}

static duk_ret_t
js_IsJoystickButtonPressed(duk_context* ctx)
{
	int joy_index = duk_to_int(ctx, 0);
	int button = duk_to_int(ctx, 1);

	duk_push_boolean(ctx, joy_is_button_down(joy_index, button));
	return 1;
}

static duk_ret_t
js_IsKeyPressed(duk_context* ctx)
{
	int keycode = duk_to_int(ctx, 0);

	duk_push_boolean(ctx, kb_is_key_down(keycode));
	return 1;
}

static duk_ret_t
js_IsLayerReflective(duk_context* ctx)
{
	int layer;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_boolean(ctx, layer_get_reflective(layer));
	return 1;
}

static duk_ret_t
js_IsLayerVisible(duk_context* ctx)
{
	int layer;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	duk_push_boolean(ctx, layer_get_visible(layer));
	return 1;
}

static duk_ret_t
js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_boolean(ctx, map_engine_running());
	return 1;
}

static duk_ret_t
js_IsMouseButtonPressed(duk_context* ctx)
{
	int                 button;
	int                 button_id;
	ALLEGRO_DISPLAY*    display;
	ALLEGRO_MOUSE_STATE mouse_state;

	button = duk_to_int(ctx, 0);
	button_id = button == MOUSE_BUTTON_RIGHT ? 2
		: button == MOUSE_BUTTON_MIDDLE ? 3
		: 1;
	al_get_mouse_state(&mouse_state);
	display = screen_display(g_screen);
	duk_push_boolean(ctx, mouse_state.display == display && al_mouse_button_down(&mouse_state, button_id));
	return 1;
}

static duk_ret_t
js_IsPersonObstructed(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         x;
	int         y;

	name = duk_require_string(ctx, 0);
	x = duk_require_int(ctx, 1);
	y = duk_require_int(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person_obstructed_at(person, x, y, NULL, NULL));
	return 1;
}

static duk_ret_t
js_IsPersonVisible(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person_get_visible(person));	return 1;
}

static duk_ret_t
js_IsTriggerAt(duk_context* ctx)
{
	int x;
	int y;
	int layer;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	duk_push_boolean(ctx, map_trigger_at(x, y, layer) >= 0);
	return 1;
}

static duk_ret_t
js_Line(duk_context* ctx)
{
	color_t color;
	float   x1, x2;
	float   y1, y2;

	x1 = trunc(duk_to_number(ctx, 0)) + 0.5;
	y1 = trunc(duk_to_number(ctx, 1)) + 0.5;
	x2 = trunc(duk_to_number(ctx, 2)) + 0.5;
	y2 = trunc(duk_to_number(ctx, 3)) + 0.5;
	color = duk_require_sphere_color(ctx, 4);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return 0;
}

static duk_ret_t
js_LineSeries(duk_context* ctx)
{
	color_t         color;
	int             num_args;
	size_t          num_points;
	int             type;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	num_args = duk_get_top(ctx);
	color = duk_require_sphere_color(ctx, 1);
	type = num_args >= 3 ? duk_to_int(ctx, 2) : LINE_MULTIPLE;

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "first argument must be an array");
	duk_get_prop_string(ctx, 0, "length");
	num_points = duk_get_uint(ctx, -1);
	duk_pop(ctx);
	if (num_points < 2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "two or more vertices required");
	if (num_points > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, -1, "x"); x = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "y"); y = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
		: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
		: ALLEGRO_PRIM_LINE_LIST
	);
	free(vertices);
	return 0;
}

static duk_ret_t
js_ListenOnPort(duk_context* ctx)
{
	int          port;
	socket_v1_t* socket;

	port = duk_to_int(ctx, 0);

	if (socket = socket_v1_new_server(port))
		duk_push_class_obj(ctx, "ssSocket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_LoadAnimation(duk_context* ctx)
{
	animation_t* anim;
	const char*  filename;

	filename = duk_require_pathname(ctx, 0, "animations", true, false);
	if (!(anim = animation_new(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load animation `%s`", filename);
	duk_push_class_obj(ctx, "ssAnimation", anim);
	return 1;
}

static duk_ret_t
js_LoadFont(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_pathname(ctx, 0, "fonts", true, false);
	if (!(font = font_load(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load font `%s`", filename);
	duk_push_sphere_font(ctx, font);
	font_unref(font);
	return 1;
}

static duk_ret_t
js_LoadImage(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	filename = duk_require_pathname(ctx, 0, "images", true, false);
	if (!(image = image_load(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load image `%s`", filename);
	duk_push_class_obj(ctx, "ssImage", image);
	return 1;
}

static duk_ret_t
js_LoadSound(duk_context* ctx)
{
	const char* filename;
	sound_t*    sound;

	filename = duk_require_pathname(ctx, 0, "sounds", true, false);

	if (!(sound = sound_new(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load sound `%s`", filename);
	duk_push_class_obj(ctx, "ssSound", sound);
	return 1;
}

static duk_ret_t
js_LoadSoundEffect(duk_context* ctx)
{
	const char* filename;
	int         mode;
	int         num_args;
	sample_t*   sample;

	num_args = duk_get_top(ctx);
	filename = duk_require_pathname(ctx, 0, "sounds", true, false);
	mode = num_args >= 2 ? duk_to_int(ctx, 1) : SE_SINGLE;

	if (!(sample = sample_new(filename, mode == SE_MULTIPLE)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to load sound effect `%s`", filename);
	duk_push_class_obj(ctx, "ssSoundEffect", sample);
	return 1;
}

static duk_ret_t
js_LoadSpriteset(duk_context* ctx)
{
	const char*  filename;
	spriteset_t* spriteset;

	filename = duk_require_pathname(ctx, 0, "spritesets", true, false);
	if ((spriteset = spriteset_load(filename)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load spriteset `%s`", filename);
	duk_push_sphere_spriteset(ctx, spriteset);
	spriteset_unref(spriteset);
	return 1;
}

static duk_ret_t
js_LoadSurface(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	filename = duk_require_pathname(ctx, 0, "images", true, false);
	if (!(image = image_load(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load image `%s`", filename);
	duk_push_class_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_LoadWindowStyle(duk_context* ctx)
{
	const char*    filename;
	windowstyle_t* winstyle;

	filename = duk_require_pathname(ctx, 0, "windowstyles", true, false);
	if (!(winstyle = winstyle_load(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot load windowstyle `%s`", filename);
	duk_push_sphere_windowstyle(ctx, winstyle);
	winstyle_unref(winstyle);
	return 1;
}

static duk_ret_t
js_MapEngine(duk_context* ctx)
{
	const char* filename;
	int         framerate;
	int         num_args;

	num_args = duk_get_top(ctx);
	filename = duk_require_pathname(ctx, 0, "maps", true, false);
	framerate = num_args >= 2 ? duk_to_int(ctx, 1)
		: s_frame_rate;

	if (!map_engine_start(filename, framerate))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load map `%s`", filename);
	return 0;
}

static duk_ret_t
js_MapToScreenX(duk_context* ctx)
{
	int      layer;
	point2_t offset;
	double   x;

	layer = duk_require_map_layer(ctx, 0);
	x = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	offset = map_xy_from_screen(point2(0, 0));
	duk_push_int(ctx, x - offset.x);
	return 1;
}

static duk_ret_t
js_MapToScreenY(duk_context* ctx)
{
	int      layer;
	point2_t offset;
	double   y;

	layer = duk_require_map_layer(ctx, 0);
	y = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	offset = map_xy_from_screen(point2(0, 0));
	duk_push_int(ctx, y - offset.y);
	return 1;
}

static duk_ret_t
js_OpenAddress(duk_context* ctx)
{
	const char*  hostname;
	int          port;
	socket_v1_t* socket;

	hostname = duk_require_string(ctx, 0);
	port = duk_to_int(ctx, 1);

	if (socket = socket_v1_new_client(hostname, port))
		duk_push_class_obj(ctx, "ssSocket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	kev_file_t* file;
	const char* filename;

	filename = duk_require_pathname(ctx, 0, "save", true, true);

	if (!(file = kev_open(g_game, filename, true)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to open file `%s`", filename);
	duk_push_class_obj(ctx, "ssFile", file);
	return 1;
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_pathname(ctx, 0, "logs", true, true);
	if (!(logger = logger_new(filename)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to open log `%s`", filename);
	duk_push_class_obj(ctx, "ssLogger", logger);
	return 1;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	file_t*     file;
	const char* filename;
	int         num_args;
	bool        writable;

	num_args = duk_get_top(ctx);
	writable = num_args >= 2 ? duk_to_boolean(ctx, 1) : false;
	filename = duk_require_pathname(ctx, 0, "other", true, writable);

	file = file_open(g_game, filename, writable ? "w+b" : "rb");
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot open file for %s",
			writable ? "write" : "read");
	duk_push_class_obj(ctx, "ssRawFile", file);
	return 1;
}

static duk_ret_t
js_OutlinedCircle(duk_context* ctx)
{
	float x = trunc(duk_to_number(ctx, 0)) + 0.5;
	float y = trunc(duk_to_number(ctx, 1)) + 0.5;
	float radius = trunc(duk_to_number(ctx, 2));
	color_t color = duk_require_sphere_color(ctx, 3);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	return 0;
}

static duk_ret_t
js_OutlinedComplex(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_OutlinedEllipse(duk_context* ctx)
{
	float x = duk_to_int(ctx, 0) + 0.5;
	float y = duk_to_int(ctx, 1) + 0.5;
	float rx = duk_to_int(ctx, 2);
	float ry = duk_to_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_ellipse(x, y, rx, ry, nativecolor(color), 1.0);
	return 0;
}

static duk_ret_t
js_OutlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_to_int(ctx, 0) + 0.5;
	float y1 = duk_to_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_to_int(ctx, 2) - 1;
	float y2 = y1 + duk_to_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_OutlinedRoundRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x = duk_to_int(ctx, 0) + 0.5;
	float y = duk_to_int(ctx, 1) + 0.5;
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	float radius = duk_to_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);
	int thickness = n_args >= 7 ? duk_to_int(ctx, 6) : 1;

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_rounded_rectangle(x, y, x + w - 1, y + h - 1, radius, radius, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_Point(duk_context* ctx)
{
	float x = duk_to_int(ctx, 0) + 0.5;
	float y = duk_to_int(ctx, 1) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 2);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_pixel(x, y, nativecolor(color));
	return 0;
}

static duk_ret_t
js_PointSeries(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 1);

	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "first argument must be an array");
	duk_get_prop_string(ctx, 0, "length");
	num_points = duk_get_uint(ctx, -1);
	duk_pop(ctx);
	if (num_points < 1)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "one or more vertices required");
	if (num_points > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, -1, "x"); x = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "y"); y = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	free(vertices);
	return 0;
}

static duk_ret_t
js_Polygon(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_Print(duk_context* ctx)
{
	int   num_items;
	char* text;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	text = strnewf("%s", duk_get_string(ctx, -1));
	debugger_log(text, PRINT_NORMAL, true);
	return 0;
}

static duk_ret_t
js_QueuePersonCommand(duk_context* ctx)
{
	int         command;
	const char* name;
	bool        immediate = false;
	int         num_args;
	person_t*   person;

	num_args = duk_get_top(ctx);
	name = duk_require_string(ctx, 0);
	command = duk_require_int(ctx, 1);
	if (num_args >= 3)
		immediate = duk_to_boolean(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (command < 0 || command >= COMMAND_RUN_SCRIPT)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid command type constant");
	if (command >= COMMAND_MOVE_NORTH && command <= COMMAND_MOVE_NORTHWEST) {
		if (!person_queue_command(person, COMMAND_ANIMATE, true))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to queue command");
	}
	if (!person_queue_command(person, command, immediate))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to queue command");
	return 0;
}

static duk_ret_t
js_QueuePersonScript(duk_context* ctx)
{
	bool        immediate = false;
	int         num_args;
	const char* name;
	script_t*   script;
	person_t*   person;

	num_args = duk_get_top(ctx);
	name = duk_require_string(ctx, 0);
	script = duk_require_sphere_script(ctx, 1, "%/queueScript.js");
	if (num_args >= 3)
		immediate = duk_to_boolean(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!person_queue_script(person, script, immediate))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to enqueue script");
	return 0;
}

static duk_ret_t
js_Rectangle(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	return 0;
}

static duk_ret_t
js_RemoveDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_pathname(ctx, 0, "save", true, true);
	if (!game_rmdir(g_game, name))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to remove directory `%s`", name);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_pathname(ctx, 0, "save", true, true);
	if (!game_unlink(g_game, filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to delete file `%s`", filename);
	return 0;
}

static duk_ret_t
js_RemoveTrigger(duk_context* ctx)
{
	int trigger_index;

	trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	map_remove_trigger(trigger_index);
	return 0;
}

static duk_ret_t
js_RemoveZone(duk_context* ctx)
{
	int zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	map_remove_zone(zone_index);
	return 0;
}

static duk_ret_t
js_Rename(duk_context* ctx)
{
	const char* name1;
	const char* name2;

	name1 = duk_require_pathname(ctx, 0, "save", true, true);
	name2 = duk_require_pathname(ctx, 1, "save", true, true);
	if (!game_rename(g_game, name1, name2))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to rename file `%s` to `%s`", name1, name2);
	return 0;
}

static duk_ret_t
js_RenderMap(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	map_engine_draw_map();
	return 0;
}

static duk_ret_t
js_ReplaceTilesOnLayer(duk_context* ctx)
{
	int        layer;
	int        new_index;
	int        old_index;
	tileset_t* tileset;

	layer = duk_require_map_layer(ctx, 0);
	old_index = duk_to_int(ctx, 1);
	new_index = duk_to_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (old_index < 0 || old_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid old tile index");
	if (new_index < 0 || new_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid new tile index");
	layer_replace_tiles(layer, old_index, new_index);
	return 0;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* filename;
	bool        is_required;

	filename = duk_require_pathname(ctx, 0, "scripts", true, false);
	if (!game_file_exists(g_game, filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file `%s` not found", filename);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, filename);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, filename);
		if (!script_eval(filename, false))
			duk_throw(ctx);
	}
	duk_pop_3(ctx);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	bool is_required;
	char path[SPHERE_PATH_MAX];

	sprintf(path, "@/scripts/lib/%s", filename);
	if (!game_file_exists(g_game, path))
		sprintf(path, "#/scripts/%s", filename);
	if (!game_file_exists(g_game, path))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "system script `%s` not found", filename);

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, path);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, path);
		if (!script_eval(path, false))
			duk_throw(ctx);
	}
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_RestartGame(duk_context* ctx)
{
	sphere_restart();
}

static duk_ret_t
js_RoundRectangle(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	float radius = duk_to_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_filled_rounded_rectangle(x, y, x + w, y + h, radius, radius, nativecolor(color));
	return 0;
}

static duk_ret_t
js_ScreenToMapX(duk_context* ctx)
{
	int      layer;
	point2_t map_xy;
	double   x;

	layer = duk_require_map_layer(ctx, 0);
	x = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	map_xy = map_xy_from_screen(point2(x, 0));
	duk_push_int(ctx, map_xy.x);
	return 1;
}

static duk_ret_t
js_ScreenToMapY(duk_context* ctx)
{
	int      layer;
	point2_t map_xy;
	double   y;

	layer = duk_require_map_layer(ctx, 0);
	y = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	map_xy = map_xy_from_screen(point2(0, y));
	duk_push_int(ctx, map_xy.y);
	return 1;
}

static duk_ret_t
js_SetCameraX(duk_context* ctx)
{
	int      new_x;
	point2_t position;

	new_x = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	position = map_get_camera_xy();
	map_set_camera_xy(point2(new_x, position.y));
	return 0;
}

static duk_ret_t
js_SetCameraY(duk_context* ctx)
{
	int      new_y;
	point2_t position;

	new_y = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	position = map_get_camera_xy();
	map_set_camera_xy(point2(position.x, new_y));
	return 0;
}

static duk_ret_t
js_SetClippingRectangle(duk_context* ctx)
{
	int height;
	int width;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);

	image_set_scissor(screen_backbuffer(g_screen),
		rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_SetColorMask(duk_context* ctx)
{
	color_t new_mask;
	int     num_args;
	int     num_frames;

	num_args = duk_get_top(ctx);
	new_mask = duk_require_sphere_color(ctx, 0);
	num_frames = num_args >= 2 ? duk_to_int(ctx, 1) : 0;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (num_frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid number of frames");
	map_engine_fade_to(new_mask, num_frames);
	return 0;
}

static duk_ret_t
js_SetDefaultMapScript(duk_context* ctx)
{
	int         map_op;
	script_t*   script;

	map_op = duk_to_int(ctx, 0);
	script = duk_require_sphere_script(ctx, 1, "%/mapScript.js");

	if (map_op < 0 || map_op >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid map script constant");
	map_engine_on_map_event(map_op, script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetDefaultPersonScript(duk_context* ctx)
{
	int       person_op;
	script_t* script;

	person_op = duk_require_int(ctx, 0);
	script = duk_require_sphere_script(ctx, 1, "%/personScript.js");

	if (person_op < 0 || person_op >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	map_engine_on_person_event(person_op, script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetDelayScript(duk_context* ctx)
{
	int num_frames;
	script_t* script;

	num_frames = duk_to_int(ctx, 0);
	script = duk_require_sphere_script(ctx, 1, "%/delayScript.js");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (num_frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame count");
	map_engine_defer(script, num_frames);
	return 0;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	int framerate = duk_to_int(ctx, 0);

	if (framerate < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame rate");
	s_frame_rate = framerate;
	return 0;
}

static duk_ret_t
js_SetLayerAngle(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_SetLayerHeight(duk_context* ctx)
{
	int layer;
	int new_height;

	layer = duk_require_map_layer(ctx, 0);
	new_height = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (new_height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "height must be positive and nonzero (got: %d)", new_height);
	if (!layer_resize(layer, layer_size(layer).width, new_height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to resize layer %d", layer);
	return 0;
}

static duk_ret_t
js_SetLayerMask(duk_context* ctx)
{
	int     layer;
	color_t mask;

	layer = duk_require_map_layer(ctx, 0);
	mask = duk_require_sphere_color(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	layer_set_color_mask(layer, mask);
	return 0;
}

static duk_ret_t
js_SetLayerReflective(duk_context* ctx)
{
	int layer;
	bool reflective;

	layer = duk_require_map_layer(ctx, 0);
	reflective = duk_to_boolean(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	layer_set_reflective(layer, reflective);
	return 0;
}

static duk_ret_t
js_SetLayerRenderer(duk_context* ctx)
{
	int       layer;
	char      script_name[50];
	script_t* script;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	sprintf(script_name, "[layer %d render script]", layer);
	script = duk_require_sphere_script(ctx, 1, script_name);
	layer_on_render(layer, script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetLayerScaleFactorX(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_SetLayerScaleFactorY(duk_context* ctx)
{
	duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	return 0;
}

static duk_ret_t
js_SetLayerSize(duk_context* ctx)
{
	int layer;
	int new_height;
	int new_width;

	layer = duk_require_map_layer(ctx, 0);
	new_width = duk_to_int(ctx, 1);
	new_height = duk_to_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (new_width <= 0 || new_height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid layer dimensions");
	if (!layer_resize(layer, new_width, new_height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't resize layer");
	return 0;
}

static duk_ret_t
js_SetLayerVisible(duk_context* ctx)
{
	bool visible;
	int  layer;

	layer = duk_require_map_layer(ctx, 0);
	visible = duk_to_boolean(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	layer_set_visible(layer, visible);
	return 0;
}

static duk_ret_t
js_SetLayerWidth(duk_context* ctx)
{
	int layer;
	int new_width;

	layer = duk_require_map_layer(ctx, 0);
	new_width = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (new_width <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "width must be positive and nonzero (got: %d)", new_width);
	if (!layer_resize(layer, new_width, layer_size(layer).height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to resize layer %d", layer);
	return 0;
}

static duk_ret_t
js_SetMapEngineFrameRate(duk_context* ctx)
{
	int framerate;

	framerate = duk_to_int(ctx, 0);

	if (framerate < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame rate");

	map_engine_set_framerate(framerate);
	return 0;
}

static duk_ret_t
js_SetMousePosition(duk_context* ctx)
{
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	screen_set_mouse_xy(g_screen, x, y);
	return 0;
}

static duk_ret_t
js_SetNextAnimatedTile(duk_context* ctx)
{
	int        next_index;
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	next_index = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	if (next_index < 0 || next_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index for next tile");
	tileset_set_next(tileset, tile_index, next_index);
	return 0;
}

static duk_ret_t
js_SetPersonAngle(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      theta;

	name = duk_require_string(ctx, 0);
	theta = duk_to_number(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_angle(person, theta);
	return 0;
}

static duk_ret_t
js_SetPersonData(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	duk_require_object_coercible(ctx, 1);
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_dup(ctx, 1); duk_put_prop_string(ctx, -2, name);
	return 0;
}

static duk_ret_t
js_SetPersonDirection(duk_context* ctx)
{
	const char* name;
	const char* new_dir;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	new_dir = duk_require_string(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_pose(person, new_dir);
	return 0;
}

static duk_ret_t
js_SetPersonFollowDistance(duk_context* ctx)
{
	int         distance;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	distance = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (person_get_leader(person) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "person has no leader");
	if (distance <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid distance");
	person_set_trailing(person, distance);
	return 0;
}

static duk_ret_t
js_SetPersonFrame(duk_context* ctx)
{
	int         frame_index;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	frame_index = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_frame(person, frame_index);
	return 0;
}

static duk_ret_t
js_SetPersonFrameNext(duk_context* ctx)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	num_frames = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (num_frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame count");
	person_set_frame_delay(person, num_frames);
	return 0;
}

static duk_ret_t
js_SetPersonFrameRevert(duk_context* ctx)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	num_frames = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (num_frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame count");
	person_set_revert_delay(person, num_frames);
	return 0;
}

static duk_ret_t
js_SetPersonIgnoreList(duk_context* ctx)
{
	const char* name;
	int         num_ignores;
	person_t*   person;

	int i;

	name = duk_require_string(ctx, 0);

	duk_require_object_coercible(ctx, 1);
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "not an array");
	person_clear_ignores(person);
	num_ignores = (int)duk_get_length(ctx, 1);
	for (i = 0; i < num_ignores; ++i) {
		duk_get_prop_index(ctx, 1, i);
		person_ignore_name(person, duk_require_string(ctx, -1));
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_SetPersonLayer(duk_context* ctx)
{
	int         layer;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	layer = duk_require_map_layer(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_layer(person, layer);
	return 0;
}

static duk_ret_t
js_SetPersonMask(duk_context* ctx)
{
	color_t     color;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	color = duk_require_sphere_color(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_color(person, color);
	return 0;
}

static duk_ret_t
js_SetPersonOffsetX(duk_context* ctx)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_x;

	name = duk_require_string(ctx, 0);
	new_x = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	offset = person_get_offset(person);
	person_set_offset(person, point2(new_x, offset.y));
	return 0;
}

static duk_ret_t
js_SetPersonOffsetY(duk_context* ctx)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_y;

	name = duk_require_string(ctx, 0);
	new_y = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	offset = person_get_offset(person);
	person_set_offset(person, point2(offset.x, new_y));
	return 0;
}

static duk_ret_t
js_SetPersonScaleAbsolute(duk_context* ctx)
{
	int          height;
	const char*  name;
	person_t*    person;
	int          sprite_h;
	int          sprite_w;
	spriteset_t* spriteset;
	int          width;

	name = duk_require_string(ctx, 0);
	width = duk_require_int(ctx, 1);
	height = duk_require_int(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (width < 0 || height < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid scale dimensions");
	spriteset = person_get_spriteset(person);
	sprite_w = spriteset_width(spriteset);
	sprite_h = spriteset_height(spriteset);
	person_set_scale(person, width / sprite_w, height / sprite_h);
	return 0;
}

static duk_ret_t
js_SetPersonScaleFactor(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      scale_x;
	double      scale_y;

	name = duk_require_string(ctx, 0);
	scale_x = duk_to_number(ctx, 1);
	scale_y = duk_to_number(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (scale_x < 0.0 || scale_y < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid scale factor(s)");
	person_set_scale(person, scale_x, scale_y);
	return 0;
}

static duk_ret_t
js_SetPersonScript(duk_context* ctx)
{
	lstring_t*  codestring;
	const char* name;
	person_t*   person;
	script_t*   script;
	int         type;

	name = duk_require_string(ctx, 0);
	type = duk_require_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	if (duk_is_string(ctx, 2)) {
		codestring = duk_require_lstring_t(ctx, 2);
		person_compile_script(person, type, codestring);
		lstr_free(codestring);
	}
	else {
		script = duk_require_sphere_script(ctx, 2, "[person script]");
		person_on_event(person, type, script);
	}
	return 0;
}

static duk_ret_t
js_SetPersonSpeed(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      speed;

	name = duk_require_string(ctx, 0);
	speed = duk_to_number(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_speed(person, speed, speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpeedXY(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x_speed;
	double      y_speed;

	name = duk_require_string(ctx, 0);
	x_speed = duk_to_number(ctx, 1);
	y_speed = duk_to_number(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_speed(person, x_speed, y_speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpriteset(duk_context* ctx)
{
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = duk_require_string(ctx, 0);
	spriteset = duk_require_sphere_spriteset(ctx, 1);

	if (!(person = map_person_by_name(name))) {
		spriteset_unref(spriteset);
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	}
	person_set_spriteset(person, spriteset);
	spriteset_unref(spriteset);
	return 0;
}

static duk_ret_t
js_SetPersonValue(duk_context* ctx)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	key = duk_to_string(ctx, 1);

	duk_require_valid_index(ctx, 2);
	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_dup(ctx, 2);
	duk_put_prop_string(ctx, -2, key);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_SetPersonVisible(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	bool        visible;

	name = duk_require_string(ctx, 0);
	visible = duk_to_boolean(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_visible(person, visible);
	return 0;
}

static duk_ret_t
js_SetPersonX(duk_context* ctx)
{
	int         layer;
	const char* name;
	int         new_x;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);
	new_x = duk_to_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, new_x, y, layer);
	return 0;
}

static duk_ret_t
js_SetPersonXYFloat(duk_context* ctx)
{
	int         layer;
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);
	x = duk_to_number(ctx, 1);
	y = duk_to_number(ctx, 2);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	layer = person_get_layer(person);
	person_set_xyz(person, x, y, layer);
	return 0;
}

static duk_ret_t
js_SetPersonY(duk_context* ctx)
{
	int         layer;
	const char* name;
	int         new_y;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);
	new_y = duk_to_int(ctx, 1);

	if (!(person = map_person_by_name(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, x, new_y, layer);
	return 0;
}

static duk_ret_t
js_SetRenderScript(duk_context* ctx)
{
	script_t* script;

	script = duk_require_sphere_script(ctx, 0, "%/renderScript.js");

	map_engine_on_render(script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetTalkActivationButton(duk_context* ctx)
{
	int button_id;

	button_id = duk_to_int(ctx, 0);

	map_engine_set_talk_button(button_id);
	return 0;
}

static duk_ret_t
js_SetTalkActivationKey(duk_context* ctx)
{
	int key;
	int num_args;
	int player = PLAYER_1;

	num_args = duk_get_top(ctx);
	key = duk_to_int(ctx, 0);
	if (num_args >= 2)
		player = duk_to_int(ctx, 1);

	if (player < 0 || player >= PLAYER_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid player constant");

	map_engine_set_talk_key(player, key);
	return 0;
}

static duk_ret_t
js_SetTalkDistance(duk_context* ctx)
{
	int pixels;

	pixels = duk_to_int(ctx, 0);

	if (pixels < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid talk distance");

	map_engine_set_talk_distance(pixels);
	return 0;
}

static duk_ret_t
js_SetTile(duk_context* ctx)
{
	int        layer;
	int        tile_index;
	tileset_t* tileset;
	int        x;
	int        y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);
	tile_index = duk_to_int(ctx, 3);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");

	layer_set_tile(layer, x, y, tile_index);
	return 0;
}

static duk_ret_t
js_SetTileDelay(duk_context* ctx)
{
	int        delay;
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	delay = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	if (delay < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame count");
	tileset_set_delay(tileset, tile_index, delay);
	return 0;
}

static duk_ret_t
js_SetTileImage(duk_context* ctx)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	image = duk_require_class_obj(ctx, 1, "ssImage");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "image/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTileName(duk_context* ctx)
{
	lstring_t* name;
	int        tile_index;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	name = duk_require_lstring_t(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_set_name(tileset, tile_index, name);
	lstr_free(name);
	return 0;
}

static duk_ret_t
js_SetTileSurface(duk_context* ctx)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	image = duk_require_class_obj(ctx, 1, "ssSurface");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "surface/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTriggerLayer(duk_context* ctx)
{
	int layer;
	int trigger_index;

	trigger_index = duk_to_int(ctx, 0);
	layer = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_set_layer(trigger_index, layer);
	return 0;
}

static duk_ret_t
js_SetTriggerScript(duk_context* ctx)
{
	script_t*  script;
	lstring_t* script_name;
	int        trigger_index;

	trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	script_name = lstr_newf("%s/trigger~%d/onStep", map_pathname(), trigger_index);
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	trigger_set_script(trigger_index, script);
	script_unref(script);
	lstr_free(script_name);
	return 0;
}

static duk_ret_t
js_SetTriggerXY(duk_context* ctx)
{
	int trigger_index;
	int x;
	int y;

	trigger_index = duk_to_int(ctx, 0);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_set_xy(trigger_index, x, y);
	return 0;
}

static duk_ret_t
js_SetUpdateScript(duk_context* ctx)
{
	script_t* script;

	script = duk_require_sphere_script(ctx, 0, "%/updateScript.js");

	map_engine_on_update(script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetZoneDimensions(duk_context* ctx)
{
	rect_t bounds;
	int    height;
	int    width;
	int    x;
	int    y;
	int    zone_index;

	zone_index = duk_to_int(ctx, 0);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	width = duk_to_int(ctx, 3);
	height = duk_to_int(ctx, 4);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone size");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "zone area out of bounds");
	zone_set_bounds(zone_index, rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_SetZoneLayer(duk_context* ctx)
{
	int layer;
	int zone_index;

	zone_index = duk_to_int(ctx, 0);
	layer = duk_require_map_layer(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	zone_set_layer(zone_index, layer);
	return 0;
}

static duk_ret_t
js_SetZoneScript(duk_context* ctx)
{
	script_t*  script;
	lstring_t* script_name;
	int        zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	if (!(script_name = lstr_newf("%s/zone%d", map_pathname(), zone_index)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "error compiling zone script");
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	lstr_free(script_name);
	zone_set_script(zone_index, script);
	script_unref(script);
	return 0;
}

static duk_ret_t
js_SetZoneSteps(duk_context* ctx)
{
	int zone_index;
	int steps;

	zone_index = duk_to_int(ctx, 0);
	steps = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	if (steps <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "steps must be positive (got: %d)", steps);
	zone_set_steps(zone_index, steps);
	return 0;
}

static duk_ret_t
js_Triangle(duk_context* ctx)
{
	int x1 = duk_to_int(ctx, 0);
	int y1 = duk_to_int(ctx, 1);
	int x2 = duk_to_int(ctx, 2);
	int y2 = duk_to_int(ctx, 3);
	int x3 = duk_to_int(ctx, 4);
	int y3 = duk_to_int(ctx, 5);
	color_t color = duk_require_sphere_color(ctx, 6);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, nativecolor(color));
	return 0;
}

static duk_ret_t
js_UnbindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_to_int(ctx, 0);
	int button = duk_to_int(ctx, 1);

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "joystick index `%d` out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "button index `%d` out of range", button);
	joy_bind_button(joy_index, button, NULL, NULL);
	return 0;
}

static duk_ret_t
js_UnbindKey(duk_context* ctx)
{
	int keycode = duk_to_int(ctx, 0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid key constant");
	kb_bind_key(keycode, NULL, NULL);
	return 0;
}

static duk_ret_t
js_UpdateMapEngine(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "no active map engine");
	map_engine_update();
	return 0;
}

static duk_ret_t
js_Animation_finalize(duk_context* ctx)
{
	animation_t* anim;

	anim = duk_require_class_obj(ctx, 0, "ssAnimation");
	animation_unref(anim);
	return 0;
}

static duk_ret_t
js_Animation_get_height(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_height(anim));
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_width(anim));
	return 1;
}

static duk_ret_t
js_Animation_drawFrame(duk_context* ctx)
{
	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);

	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");

	image_draw(animation_frame(anim), x, y);
	return 0;
}

static duk_ret_t
js_Animation_drawZoomedFrame(duk_context* ctx)
{
	animation_t* anim;
	int          height;
	double       scale;
	int          width;
	int          x;
	int          y;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");
	x = duk_to_number(ctx, 0);
	y = duk_to_number(ctx, 1);
	scale = duk_to_number(ctx, 2);

	if (scale < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "zoom must be positive");
	width = animation_width(anim);
	height = animation_height(anim);
	image_draw_scaled(animation_frame(anim), x, y, width * scale, height * scale);
	return 0;
}

static duk_ret_t
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");
	duk_pop(ctx);
	duk_push_int(ctx, animation_delay(anim));
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_num_frames(anim));
	return 1;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_class_obj(ctx, -1, "ssAnimation");
	duk_pop(ctx);
	animation_update(anim);
	return 0;
}

static duk_ret_t
js_ByteArray_finalize(duk_context* ctx)
{
	bytearray_t* array;

	array = duk_require_class_obj(ctx, 0, "ssByteArray");
	bytearray_unref(array);
	return 0;
}

static duk_ret_t
js_ByteArray_proxy_get(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	array = duk_require_class_obj(ctx, 0, "ssByteArray");
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = bytearray_len(array);
		if (index < 0 || index >= size)
			duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "byte index `%d` out of bounds", index);
		duk_push_uint(ctx, bytearray_get(array, index));
		return 1;
	}
	else {
		duk_dup(ctx, 1);
		duk_get_prop(ctx, 0);
		return 1;
	}
}

static duk_ret_t
js_ByteArray_proxy_set(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	array = duk_require_class_obj(ctx, 0, "ssByteArray");
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = bytearray_len(array);
		if (index < 0 || index >= size)
			duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "byte index `%d` out of bounds", index);
		bytearray_set(array, index, duk_require_uint(ctx, 2));
		return 0;
	}
	else {
		duk_dup(ctx, 1);
		duk_dup(ctx, 2);
		duk_put_prop(ctx, 0);
		return 0;
	}
}

static duk_ret_t
js_ByteArray_get_length(duk_context* ctx)
{
	bytearray_t* array;

	duk_push_this(ctx);
	array = duk_require_class_obj(ctx, -1, "ssByteArray");
	duk_pop(ctx);
	duk_push_int(ctx, bytearray_len(array));
	return 1;
}

static duk_ret_t
js_ByteArray_concat(duk_context* ctx)
{
	bytearray_t* array[2];
	bytearray_t* new_array;

	duk_push_this(ctx);
	array[0] = duk_require_class_obj(ctx, -1, "ssByteArray");
	array[1] = duk_require_class_obj(ctx, 0, "ssByteArray");

	if (!(new_array = bytearray_concat(array[0], array[1])))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to concatenate byte arrays");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_slice(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int start = duk_to_int(ctx, 0);
	int end = (n_args >= 2) ? duk_to_int(ctx, 1) : INT_MAX;

	bytearray_t* array;
	int          end_norm;
	bytearray_t* new_array;
	int          size;

	duk_push_this(ctx);
	array = duk_require_class_obj(ctx, -1, "ssByteArray");

	size = bytearray_len(array);
	end_norm = fmin(end >= 0 ? end : size + end, size);
	if (end_norm < start || end_norm > size)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "start and/or end is out of bounds");
	if (!(new_array = bytearray_slice(array, start, end_norm - start)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to slice byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object byte_array]");
	return 1;
}

static duk_ret_t
js_Color_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object color]");
	return 1;
}

static duk_ret_t
js_ColorMatrix_apply(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);

	colormatrix_t matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_colormatrix(ctx, -1);
	duk_pop(ctx);
	duk_push_sphere_color(ctx, color_transform(color, matrix));
	return 1;
}

static duk_ret_t
js_ColorMatrix_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object colormatrix]");
	return 1;
}

static duk_ret_t
js_File_finalize(duk_context* ctx)
{
	kev_file_t* file;

	file = duk_require_class_obj(ctx, 0, "ssFile");
	kev_close(file);
	return 0;
}

static duk_ret_t
js_File_close(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");

	duk_set_class_ptr(ctx, -1, NULL);
	kev_close(file);
	return 0;
}

static duk_ret_t
js_File_flush(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	kev_save(file);
	return 0;
}

static duk_ret_t
js_File_getKey(duk_context* ctx)
{
	int index = duk_to_int(ctx, 0);

	kev_file_t* file;
	const char* key;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	if (key = kev_get_key(file, index))
		duk_push_string(ctx, key);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_File_getNumKeys(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	duk_push_int(ctx, kev_num_keys(file));
	return 1;
}

static duk_ret_t
js_File_read(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);

	bool        def_bool;
	double      def_num;
	const char* def_string;
	kev_file_t* file;
	const char* value;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	switch (duk_get_type(ctx, 1)) {
	case DUK_TYPE_BOOLEAN:
		def_bool = duk_get_boolean(ctx, 1);
		duk_push_boolean(ctx, kev_read_bool(file, key, def_bool));
		break;
	case DUK_TYPE_NUMBER:
		def_num = duk_get_number(ctx, 1);
		duk_push_number(ctx, kev_read_float(file, key, def_num));
		break;
	default:
		def_string = duk_to_string(ctx, 1);
		value = kev_read_string(file, key, def_string);
		duk_push_string(ctx, value);
		break;
	}
	return 1;
}

static duk_ret_t
js_File_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object file]");
	return 1;
}

static duk_ret_t
js_File_write(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);

	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	kev_write_string(file, key, duk_to_string(ctx, 1));
	return 0;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_class_obj(ctx, 0, "ssFont");
	font_unref(font);
	return 0;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* dolly_font;
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_pop(ctx);
	if (!(dolly_font = font_clone(font)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to clone font");
	duk_push_sphere_font(ctx, dolly_font);
	return 1;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);

	font_t* font;
	color_t mask;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	mask = duk_require_sphere_color(ctx, -1);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	font_draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
	return 0;
}

static duk_ret_t
js_Font_drawTextBox(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	int offset = duk_to_int(ctx, 4);
	const char* text = duk_to_string(ctx, 5);

	font_t*     font;
	int         line_height;
	const char* line_text;
	color_t     mask;
	int         num_lines;

	int i;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	mask = duk_require_sphere_color(ctx, -1);

	if (screen_skip_frame(g_screen))
		return 0;
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
	duk_push_this(ctx);
	duk_push_string(ctx, text);
	duk_push_int(ctx, w);
	duk_call_method(ctx, 2);
	duk_get_prop_string(ctx, -1, "length");
	num_lines = duk_get_int(ctx, -1);
	duk_pop(ctx);
	line_height = font_height(font);
	galileo_reset();
	for (i = 0; i < num_lines; ++i) {
		duk_get_prop_index(ctx, -1, i); line_text = duk_get_string(ctx, -1); duk_pop(ctx);
		font_draw_text(font, mask, x + offset, y, TEXT_ALIGN_LEFT, line_text);
		y += line_height;
	}
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_drawZoomedText(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	font_t*         font;
	int             height;
	color_t         mask;
	ALLEGRO_BITMAP* old_target;
	float           scale;
	const char*     text;
	int             width;
	int             x;
	int             y;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	mask = duk_require_sphere_color(ctx, -1);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	scale = duk_to_number(ctx, 2);
	text = duk_to_string(ctx, 3);

	if (screen_skip_frame(g_screen))
		return 0;

	// render the text to a texture so we can scale it up.  not the most
	// efficient way to do it, sure, but it gets the job done.
	width = font_get_width(font, text);
	height = font_height(font);
	bitmap = al_create_bitmap(width, height);
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(bitmap);
	font_draw_text(font, mask, 0, 0, TEXT_ALIGN_LEFT, text);
	al_set_target_bitmap(old_target);

	galileo_reset();
	al_draw_scaled_bitmap(bitmap, 0, 0, width, height, x, y, width * scale, height * scale, 0x0);
	al_destroy_bitmap(bitmap);
	return 0;
}

static duk_ret_t
js_Font_getCharacterImage(duk_context* ctx)
{
	uint32_t cp;
	font_t*  font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	cp = duk_require_uint(ctx, 0);

	duk_push_class_obj(ctx, "ssImage", image_ref(font_glyph(font, cp)));
	return 1;
}

static duk_ret_t
js_Font_getColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	return 1;
}

static duk_ret_t
js_Font_getHeight(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font));
	return 1;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	font_t*     font;
	int         num_lines;
	const char* text;
	int         width;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	text = duk_to_string(ctx, 0);
	width = duk_to_int(ctx, 1);

	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
	duk_push_this(ctx);
	duk_push_string(ctx, text);
	duk_push_int(ctx, width);
	duk_call_method(ctx, 2);
	duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font) * num_lines);
	return 1;
}

static duk_ret_t
js_Font_getStringWidth(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	duk_pop(ctx);
	duk_push_int(ctx, font_get_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_setCharacterImage(duk_context* ctx)
{
	int cp = duk_to_int(ctx, 0);
	image_t* image = duk_require_class_obj(ctx, 1, "ssImage");

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");

	font_set_glyph(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_setColorMask(duk_context* ctx)
{
	font_t* font;
	color_t mask;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
	mask = duk_require_sphere_color(ctx, 0);

	duk_push_sphere_color(ctx, mask);
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	return 0;
}

static duk_ret_t
js_Font_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object font]");
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int         width = duk_to_int(ctx, 1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	duk_push_this(ctx);
	font = duk_require_class_obj(ctx, -1, "ssFont");
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
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_class_obj(ctx, 0, "ssImage");
	image_unref(image);
	return 0;
}

static duk_ret_t
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	duk_push_int(ctx, image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	duk_push_int(ctx, image_width(image));
	return 1;
}

static duk_ret_t
js_Image_blit(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_blitMask(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	color_t mask = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_tinted_bitmap(image_bitmap(image), nativecolor(mask), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_createSurface(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create new surface image");
	duk_push_class_obj(ctx, "ssSurface", new_image);
	return 1;
}

static duk_ret_t
js_Image_rotateBlit(duk_context* ctx)
{
	float    angle;
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	angle = duk_to_number(ctx, 2);

	if (screen_skip_frame(g_screen))
		return 0;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_rotated_bitmap(image_bitmap(image), width / 2, height / 2,
		x + width / 2, y + height / 2, angle, 0x0);
	return 0;
}

static duk_ret_t
js_Image_rotateBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;
	float    angle;
	color_t  mask;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	angle = duk_to_number(ctx, 2);
	mask = duk_require_sphere_color(ctx, 3);

	if (screen_skip_frame(g_screen))
		return 0;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_tinted_rotated_bitmap(image_bitmap(image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	return 0;
}

static duk_ret_t
js_Image_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object image]");
	return 1;
}

static duk_ret_t
js_Image_transformBlit(duk_context* ctx)
{
	int           height;
	image_t*      image;
	ALLEGRO_COLOR mask;
	int           width;
	int           x1, y1;
	int           x2, y2;
	int           x3, y3;
	int           x4, y4;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = duk_to_int(ctx, 2);
	y2 = duk_to_int(ctx, 3);
	x3 = duk_to_int(ctx, 4);
	y3 = duk_to_int(ctx, 5);
	x4 = duk_to_int(ctx, 6);
	y4 = duk_to_int(ctx, 7);

	width = image_width(image);
	height = image_height(image);
	mask = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, mask },
		{ x2 + 0.5, y2 + 0.5, 0, width, 0, mask },
		{ x4 + 0.5, y4 + 0.5, 0, 0, height, mask },
		{ x3 + 0.5, y3 + 0.5, 0, width, height, mask }
	};
	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_transformBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  mask;
	int      width;
	int      x1, y1;
	int      x2, y2;
	int      x3, y3;
	int      x4, y4;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = duk_to_int(ctx, 2);
	y2 = duk_to_int(ctx, 3);
	x3 = duk_to_int(ctx, 4);
	y3 = duk_to_int(ctx, 5);
	x4 = duk_to_int(ctx, 6);
	y4 = duk_to_int(ctx, 7);
	mask = duk_require_sphere_color(ctx, 8);

	width = image_width(image);
	height = image_height(image);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, nativecolor(mask) },
		{ x2 + 0.5, y2 + 0.5, 0, width, 0, nativecolor(mask) },
		{ x4 + 0.5, y4 + 0.5, 0, 0, height, nativecolor(mask) },
		{ x3 + 0.5, y3 + 0.5, 0, width, height, nativecolor(mask) }
	};
	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_zoomBlit(duk_context* ctx)
{
	int      height;
	image_t* image;
	float    scale;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	scale = duk_to_number(ctx, 2);

	if (screen_skip_frame(g_screen))
		return 0;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_scaled_bitmap(image_bitmap(image), 0, 0, width, height,
		x, y, width * scale, height * scale, 0x0);
	return 0;
}

static duk_ret_t
js_Image_zoomBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  mask;
	float    scale;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssImage");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	scale = duk_to_number(ctx, 2);
	mask = duk_require_sphere_color(ctx, 3);

	if (screen_skip_frame(g_screen))
		return 0;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_tinted_scaled_bitmap(image_bitmap(image), nativecolor(mask),
		0, 0, width, height, x, y, width * scale, height * scale, 0x0);
	return 0;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_class_obj(ctx, 0, "ssLogger");
	logger_unref(logger);
	return 0;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_class_obj(ctx, -1, "ssLogger");
	if (!logger_begin_block(logger, title))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_class_obj(ctx, -1, "ssLogger");
	logger_end_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object log]");
	return 1;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_class_obj(ctx, -1, "ssLogger");
	logger_write(logger, NULL, text);
	return 0;
}

static duk_ret_t
js_RawFile_finalize(duk_context* ctx)
{
	file_t* file;

	file = duk_require_class_obj(ctx, 0, "ssRawFile");
	if (file != NULL) file_close(file);
	return 0;
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file already closed");

	duk_set_class_ptr(ctx, -1, NULL);
	file_close(file);
	return 0;
}

static duk_ret_t
js_RawFile_getPosition(duk_context* ctx)
{
	file_t* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file was closed");
	duk_push_int(ctx, file_position(file));
	return 1;
}

static duk_ret_t
js_RawFile_getSize(duk_context* ctx)
{
	file_t* file;
	long    file_pos;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "RawFile:size: file was closed");
	file_pos = file_position(file);
	file_seek(file, 0, WHENCE_END);
	duk_push_int(ctx, file_position(file));
	file_seek(file, file_pos, WHENCE_SET);
	return 1;
}

static duk_ret_t
js_RawFile_read(duk_context* ctx)
{
	bytearray_t* array;
	file_t*      file;
	int          num_args;
	long long    num_bytes = 0;
	long long    pos;
	void*        read_buffer;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	if (num_args >= 1)
		num_bytes = duk_to_int(ctx, 0);

	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file is already closed");

	if (num_args < 1) {  // if no arguments, read entire file back to front
		pos = file_position(file);
		num_bytes = (file_seek(file, 0, WHENCE_END), file_position(file));
		file_seek(file, 0, WHENCE_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid read size");
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't read from file");
	num_bytes = (long)file_read(file, read_buffer, num_bytes, 1);
	if (num_args < 1)  // reset file position after whole-file read
		file_seek(file, pos, WHENCE_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't read from file");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_RawFile_setPosition(duk_context* ctx)
{
	file_t*   file;
	long long new_pos;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	new_pos = duk_to_int(ctx, 0);

	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file is already closed");

	if (!file_seek(file, new_pos, WHENCE_SET))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "RawFile:position: Failed to set read/write position");
	return 0;
}

static duk_ret_t
js_RawFile_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object rawfile]");
	return 1;
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	bytearray_t* array;
	const void*  data;
	file_t*      file;
	duk_size_t   write_size;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file was closed");
	if (duk_is_string(ctx, 0))
		data = duk_get_lstring(ctx, 0, &write_size);
	else if (duk_is_class_obj(ctx, 0, "ssByteArray")) {
		array = duk_require_class_obj(ctx, 0, "ssByteArray");
		data = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	else {
		data = duk_require_buffer_data(ctx, 0, &write_size);
	}
	if (file_write(file, data, write_size, 1) != write_size)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't write to file");
	return 0;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_v1_t* socket;

	socket = duk_require_class_obj(ctx, 0, "ssSocket");
	socket_v1_unref(socket);
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_v1_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "ssSocket");

	duk_set_class_ptr(ctx, -1, NULL);
	socket_v1_unref(socket);
	return 1;
}

static duk_ret_t
js_Socket_getPendingReadSize(duk_context* ctx)
{
	socket_v1_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "ssSocket");

	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	duk_push_uint(ctx, (duk_uint_t)socket_v1_peek(socket));
	return 1;
}

static duk_ret_t
js_Socket_isConnected(duk_context* ctx)
{
	socket_v1_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "ssSocket");

	if (socket != NULL)
		duk_push_boolean(ctx, socket_v1_connected(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	int length = duk_to_int(ctx, 0);

	bytearray_t* array;
	void*        read_buffer;
	socket_v1_t* socket;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "ssSocket");

	if (length <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid read size");
	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	if (!(read_buffer = malloc(length)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to allocate read buffer");
	socket_v1_read(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_Socket_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object socket]");
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_v1_t*   socket;
	size_t         write_size;

	duk_push_this(ctx);
	socket = duk_require_class_obj(ctx, -1, "ssSocket");

	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_class_obj(ctx, 0, "ssByteArray");
		payload = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	if (socket == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	socket_v1_write(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	sound = duk_require_class_obj(ctx, 0, "ssSound");
	sound_unref(sound);
	return 0;
}

static duk_ret_t
js_Sound_getLength(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, floor(sound_len(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_getPan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_int(ctx, sound_pan(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_getPitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, sound_speed(sound));
	return 1;
}

static duk_ret_t
js_Sound_getPosition(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, floor(sound_tell(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_getRepeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_boolean(ctx, sound_repeat(sound));
	return 1;
}

static duk_ret_t
js_Sound_getVolume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_int(ctx, sound_gain(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_isPlaying(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	duk_push_boolean(ctx, sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_isSeekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	sound_pause(sound, true);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	int      num_args;
	sound_t* sound;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	if (num_args >= 1) {
		sound_set_repeat(sound, duk_to_boolean(ctx, 0));
		sound_play(sound, s_sound_mixer);
	}
	else {
		sound_pause(sound, false);
		if (!sound_playing(sound))
			sound_play(sound, s_sound_mixer);
	}

	return 0;
}

static duk_ret_t
js_Sound_reset(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	sound_seek(sound, 0.0);
	return 0;
}

static duk_ret_t
js_Sound_setPan(duk_context* ctx)
{
	int      new_pan;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");
	new_pan = duk_to_int(ctx, 0);

	sound_set_pan(sound, (float)new_pan / 255);
	return 0;
}

static duk_ret_t
js_Sound_setPitch(duk_context* ctx)
{
	float    new_pitch;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");
	new_pitch = duk_to_number(ctx, 0);

	sound_set_speed(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_setPosition(duk_context* ctx)
{
	double   new_pos;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");
	new_pos = duk_to_number(ctx, 0);

	sound_seek(sound, floor(new_pos) / 1000000);
	return 0;
}

static duk_ret_t
js_Sound_setRepeat(duk_context* ctx)
{
	bool     is_looped;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");
	is_looped = duk_to_boolean(ctx, 0);

	sound_set_repeat(sound, is_looped);
	return 0;
}

static duk_ret_t
js_Sound_setVolume(duk_context* ctx)
{
	int volume;

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");
	volume = duk_to_int(ctx, 0);

	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	sound_set_gain(sound, (float)volume / 255);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_class_obj(ctx, -1, "ssSound");

	sound_stop(sound);
	return 0;
}

static duk_ret_t
js_Sound_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object sound]");
	return 1;
}

static duk_ret_t
js_SoundEffect_finalize(duk_context* ctx)
{
	sample_t* sample;

	sample = duk_require_class_obj(ctx, 0, "ssSoundEffect");

	sample_unref(sample);
	return 0;
}

static duk_ret_t
js_SoundEffect_getPan(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");

	duk_push_number(ctx, sample_get_pan(sample) * 255.0);
	return 1;
}

static duk_ret_t
js_SoundEffect_getPitch(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");

	duk_push_number(ctx, sample_get_speed(sample));
	return 1;
}

static duk_ret_t
js_SoundEffect_getVolume(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");

	duk_push_number(ctx, sample_get_gain(sample) * 255.0);
	return 1;
}

static duk_ret_t
js_SoundEffect_setPan(duk_context* ctx)
{
	sample_t* sample;
	float     pan;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");
	pan = duk_to_number(ctx, 0);

	sample_set_pan(sample, pan / 255.0);
	return 0;
}

static duk_ret_t
js_SoundEffect_setPitch(duk_context* ctx)
{
	sample_t* sample;
	float     pitch;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");
	pitch = duk_to_number(ctx, 0);

	sample_set_speed(sample, pitch);
	return 0;
}

static duk_ret_t
js_SoundEffect_setVolume(duk_context* ctx)
{
	sample_t* sample;
	float     volume;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");
	volume = duk_to_number(ctx, 0);

	sample_set_gain(sample, volume / 255.0);
	return 0;
}

static duk_ret_t
js_SoundEffect_play(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");

	sample_play(sample, s_sound_mixer);
	return 0;
}

static duk_ret_t
js_SoundEffect_stop(duk_context* ctx)
{
	sample_t* sample;

	duk_push_this(ctx);
	sample = duk_require_class_obj(ctx, -1, "ssSoundEffect");

	sample_stop_all(sample);
	return 0;
}

static duk_ret_t
js_SoundEffect_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object sound effect]");
	return 1;
}

static duk_ret_t
js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;

	spriteset = duk_require_class_obj(ctx, 0, "ssSpriteset");

	spriteset_unref(spriteset);
	return 0;
}

static duk_ret_t
js_Spriteset_get_filename(duk_context* ctx)
{
	path_t*      path;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_class_obj(ctx, -1, "ssSpriteset");

	// returned filename is relative to `@/`, even though Spriteset#save() is
	// rooted at `@/spritesets`.  I suspect this to be a bug in Sphere 1.x, but fixing
	// it here would break existing workarounds.
	path = game_relative_path(g_game, spriteset_pathname(spriteset), "@/");
	duk_push_string(ctx, path_cstr(path));
	path_free(path);
	return 1;
}

static duk_ret_t
js_Spriteset_clone(duk_context* ctx)
{
	spriteset_t* new_spriteset;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_class_obj(ctx, -1, "ssSpriteset");

	if ((new_spriteset = spriteset_clone(spriteset)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't clone spriteset");
	duk_push_sphere_spriteset(ctx, new_spriteset);
	spriteset_unref(new_spriteset);
	return 1;
}

static duk_ret_t
js_Spriteset_save(duk_context* ctx)
{
	const char*  filename;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_spriteset(ctx, -1);
	filename = duk_require_pathname(ctx, 0, "spritesets", true, true);

	if (!spriteset_save(spriteset, filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't save spriteset");
	spriteset_unref(spriteset);
	return 0;
}

static duk_ret_t
js_Spriteset_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object spriteset]");
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_class_obj(ctx, 0, "ssSurface");
	image_unref(image);
	return 0;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if (image != NULL)
		duk_push_int(ctx, image_height(image));
	else
		duk_push_int(ctx, screen_size(g_screen).height);
	return 1;
}

static duk_ret_t
js_Surface_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if (image != NULL)
		duk_push_int(ctx, image_width(image));
	else
		duk_push_int(ctx, screen_size(g_screen).width);
	return 1;
}

static duk_ret_t
js_Surface_applyColorFX(duk_context* ctx)
{
	colormatrix_t matrix;
	int           height;
	int           width;
	int           x;
	int           y;

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);
	matrix = duk_require_sphere_colormatrix(ctx, 4);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_colormat(image, matrix, x, y, width, height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't apply color FX");
	return 0;
}

static duk_ret_t
js_Surface_applyColorFX4(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	colormatrix_t ul_mat = duk_require_sphere_colormatrix(ctx, 4);
	colormatrix_t ur_mat = duk_require_sphere_colormatrix(ctx, 5);
	colormatrix_t ll_mat = duk_require_sphere_colormatrix(ctx, 6);
	colormatrix_t lr_mat = duk_require_sphere_colormatrix(ctx, 7);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_colormat_4(image, ul_mat, ur_mat, ll_mat, lr_mat, x, y, w, h))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't apply color FX");
	return 0;
}

static duk_ret_t
js_Surface_applyLookup(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	uint8_t* red_lu = duk_require_rgba_lut(ctx, 4);
	uint8_t* green_lu = duk_require_rgba_lut(ctx, 5);
	uint8_t* blue_lu = duk_require_rgba_lut(ctx, 6);
	uint8_t* alpha_lu = duk_require_rgba_lut(ctx, 7);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_lookup(image, x, y, w, h, red_lu, green_lu, blue_lu, alpha_lu))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't apply color FX");
	free(red_lu);
	free(green_lu);
	free(blue_lu);
	free(alpha_lu);
	return 0;
}

static duk_ret_t
js_Surface_bezierCurve(duk_context* ctx)
{
	int             blend_mode;
	color_t         color;
	float           cp[8];
	image_t*        image;
	bool            is_quadratic = true;
	int             num_args;
	int             num_points;
	double          step_size;
	ALLEGRO_VERTEX* vertices;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	color = duk_require_sphere_color(ctx, 0);
	step_size = duk_to_number(ctx, 1);
	x1 = duk_to_number(ctx, 2);
	y1 = duk_to_number(ctx, 3);
	x2 = duk_to_number(ctx, 4);
	y2 = duk_to_number(ctx, 5);
	x3 = duk_to_number(ctx, 6);
	y3 = duk_to_number(ctx, 7);
	if (num_args >= 10) {
		is_quadratic = false;
		x4 = duk_to_number(ctx, 8);
		y4 = duk_to_number(ctx, 9);
	}

	cp[0] = x1; cp[1] = y1;
	cp[2] = x2; cp[3] = y2;
	cp[4] = x3; cp[5] = y3;
	cp[6] = x4; cp[7] = y4;
	if (is_quadratic) {
		// convert quadratic Bezier curve to cubic
		cp[6] = x3; cp[7] = y3;
		cp[2] = x1 + (2.0 / 3.0) * (x2 - x1);
		cp[3] = y1 + (2.0 / 3.0) * (y2 - y1);
		cp[4] = x3 + (2.0 / 3.0) * (x2 - x3);
		cp[5] = y3 + (2.0 / 3.0) * (y2 - y3);
	}
	step_size = step_size < 0.001 ? 0.001 : step_size > 1.0 ? 1.0 : step_size;
	num_points = 1.0 / step_size;
	vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	al_calculate_spline(&vertices[0].x, sizeof(ALLEGRO_VERTEX), cp, 0.0, num_points);
	for (i = 0; i < num_points; ++i)
		vertices[i].color = nativecolor(color);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(vertices, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	reset_blend_modes();
	free(vertices);
	return 0;
}

static duk_ret_t
js_Surface_blit(duk_context* ctx)
{
	image_t* image;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);

	if (screen_skip_frame(g_screen))
		return 0;
	galileo_reset();
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_class_obj(ctx, 0, "ssSurface");
	int x = duk_to_int(ctx, 1);
	int y = duk_to_int(ctx, 2);
	color_t mask = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_bitmap(image_bitmap(src_image), nativecolor(mask), x, y, 0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_blitSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_class_obj(ctx, 0, "ssSurface");
	int x = duk_to_int(ctx, 1);
	int y = duk_to_int(ctx, 2);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_bitmap(image_bitmap(src_image), x, y, 0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_clone(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "Surface:clone() - Unable to create new surface image");
	duk_push_class_obj(ctx, "ssSurface", new_image);
	return 1;
}

static duk_ret_t
js_Surface_cloneSection(duk_context* ctx)
{
	int      height;
	image_t* image;
	image_t* new_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);

	if ((new_image = image_new(width, height)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap_region(image_bitmap(image), x, y, width, height, 0, 0, 0x0);
	duk_push_class_obj(ctx, "ssSurface", new_image);
	return 1;
}

static duk_ret_t
js_Surface_createImage(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create image");
	duk_push_class_obj(ctx, "ssImage", new_image);
	return 1;
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	int         blend_mode;
	color_t     color;
	font_t*     font;
	image_t*    image;
	const char* text;
	int         x;
	int         y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	font = duk_require_class_obj(ctx, 0, "ssFont");
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	text = duk_to_string(ctx, 3);

	duk_get_prop_string(ctx, 0, "\xFF" "color_mask");
	color = duk_require_sphere_color(ctx, -1);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	font_draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_filledCircle(duk_context* ctx)
{
	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int radius = duk_to_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_circle(x, y, radius, nativecolor(color));
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_filledEllipse(duk_context* ctx)
{
	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int rx = duk_to_number(ctx, 2);
	int ry = duk_to_number(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_ellipse(x, y, rx, ry, nativecolor(color));
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_flipHorizontally(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	image_flip(image, true, false);
	return 0;
}

static duk_ret_t
js_Surface_flipVertically(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	image_flip(image, false, true);
	return 0;
}

static duk_ret_t
js_Surface_getPixel(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  pixel;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);

	width = image_width(image);
	height = image_height(image);
	if (x < 0 || x >= width || y < 0 || y >= height)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "X/Y out of bounds");
	pixel = image_get_pixel(image, x, y);
	duk_push_sphere_color(ctx, pixel);
	return 1;
}

static duk_ret_t
js_Surface_gradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int radius = duk_to_number(ctx, 2);
	color_t in_color = duk_require_sphere_color(ctx, 3);
	color_t out_color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_gradientEllipse(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int rx = duk_to_number(ctx, 2);
	int ry = duk_to_number(ctx, 3);
	color_t in_color = duk_require_sphere_color(ctx, 4);
	color_t out_color = duk_require_sphere_color(ctx, 5);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	vcount = ceil(fmin(10 * sqrt((rx + ry) / 2), 126));
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * rx;
		s_vbuf[i + 1].y = y - sin(phi) * ry;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * rx;
	s_vbuf[i + 1].y = y - sin(0) * ry;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_gradientRectangle(duk_context* ctx)
{
	int x1 = duk_to_int(ctx, 0);
	int y1 = duk_to_int(ctx, 1);
	int x2 = x1 + duk_to_int(ctx, 2);
	int y2 = y1 + duk_to_int(ctx, 3);
	color_t color_ul = duk_require_sphere_color(ctx, 4);
	color_t color_ur = duk_require_sphere_color(ctx, 5);
	color_t color_lr = duk_require_sphere_color(ctx, 6);
	color_t color_ll = duk_require_sphere_color(ctx, 7);

	int           blend_mode;
	image_t*      image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	galileo_reset();
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_gradientLine(duk_context* ctx)
{
	int      blend_mode;
	color_t  color1;
	color_t  color2;
	image_t* image;
	float    length;
	float    tx, ty;
	float    x1;
	float    x2;
	float    y1;
	float    y2;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = duk_to_int(ctx, 2);
	y2 = duk_to_int(ctx, 3);
	color1 = duk_require_sphere_color(ctx, 4);
	color2 = duk_require_sphere_color(ctx, 5);

	length = hypotf(x2 - x1, y2 - y1);
	tx = 0.5 * (y2 - y1) / length;
	ty = 0.5 * -(x2 - x1) / length;
	ALLEGRO_VERTEX verts[] = {
		{ x1 + tx, y1 + ty, 0, 0, 0, nativecolor(color1) },
		{ x1 - tx, y1 - ty, 0, 0, 0, nativecolor(color1) },
		{ x2 - tx, y2 - ty, 0, 0, 0, nativecolor(color2) },
		{ x2 + tx, y2 + ty, 0, 0, 0, nativecolor(color2) }
	};
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_line(duk_context* ctx)
{
	float x1 = duk_to_int(ctx, 0) + 0.5;
	float y1 = duk_to_int(ctx, 1) + 0.5;
	float x2 = duk_to_int(ctx, 2) + 0.5;
	float y2 = duk_to_int(ctx, 3) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_lineSeries(duk_context* ctx)
{
	int             blend_mode;
	color_t         color;
	image_t*        image;
	int             num_args;
	size_t          num_points;
	int             type;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	color = duk_require_sphere_color(ctx, 1);
	type = num_args >= 3 ? duk_to_int(ctx, 2) : LINE_MULTIPLE;

	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "first argument must be an array");
	duk_get_prop_string(ctx, 0, "length");
	num_points = duk_get_uint(ctx, -1);
	duk_pop(ctx);
	if (num_points < 2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "two or more vertices required");
	if (num_points > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, -1, "x"); x = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "y"); y = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
		: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
		: ALLEGRO_PRIM_LINE_LIST
	);
	reset_blend_modes();
	free(vertices);
	return 0;
}

static duk_ret_t
js_Surface_outlinedCircle(duk_context* ctx)
{
	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int radius = duk_to_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_outlinedEllipse(duk_context* ctx)
{
	int x = duk_to_number(ctx, 0);
	int y = duk_to_number(ctx, 1);
	int rx = duk_to_number(ctx, 2);
	int ry = duk_to_number(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_ellipse(x, y, rx, ry, nativecolor(color), 1.0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_pointSeries(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 1);

	int             blend_mode;
	image_t*        image;
	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	unsigned int i;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!duk_is_array(ctx, 0))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "first argument must be an array");
	duk_get_prop_string(ctx, 0, "length");
	num_points = duk_get_uint(ctx, -1);
	duk_pop(ctx);
	if (num_points > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "too many vertices (%u)", num_points);
	vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, i);
		duk_get_prop_string(ctx, -1, "x"); x = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "y"); y = duk_to_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	reset_blend_modes();
	free(vertices);
	return 0;
}

static duk_ret_t
js_Surface_outlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_to_int(ctx, 0) + 0.5;
	float y1 = duk_to_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_to_int(ctx, 2) - 1;
	float y2 = y1 + duk_to_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_replaceColor(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);
	color_t new_color = duk_require_sphere_color(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	if (!image_replace_color(image, color, new_color))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "Surface:replaceColor() - Failed to perform replacement");
	return 0;
}

static duk_ret_t
js_Surface_rescale(duk_context* ctx)
{
	int width = duk_to_int(ctx, 0);
	int height = duk_to_int(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	if (!image_rescale(image, width, height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "Surface:rescale() - Failed to rescale image");
	duk_push_this(ctx);
	return 1;
}

static duk_ret_t
js_Surface_rotate(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float angle = duk_to_number(ctx, 0);
	bool want_resize = n_args >= 2 ? duk_to_boolean(ctx, 1) : true;

	image_t* image;
	image_t* new_image;
	int      new_w, new_h;
	int      w, h;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	w = new_w = image_width(image);
	h = new_h = image_height(image);
	if (want_resize) {
		// FIXME: implement in-place resizing for Surface#rotate()
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "not implemented");
	}
	if ((new_image = image_new(new_w, new_h)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't create new surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_rotated_bitmap(image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);

	// swap out the image pointer and free old image
	duk_set_class_ptr(ctx, -1, new_image);
	image_unref(image);
	return 1;
}

static duk_ret_t
js_Surface_rotateBlitMaskSurface(duk_context* ctx)
{
	float    angle;
	int      blend_mode;
	int      height;
	image_t* image;
	color_t  mask;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	angle = duk_to_number(ctx, 3);
	mask = duk_require_sphere_color(ctx, 4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_rotated_bitmap(image_bitmap(source_image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_rotateBlitSurface(duk_context* ctx)
{
	float    angle;
	int      blend_mode;
	int      height;
	image_t* image;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	angle = duk_to_number(ctx, 3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_rotated_bitmap(image_bitmap(source_image), width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_rectangle(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	image_t* image;
	int      blend_mode;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);

	galileo_reset();
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_save(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	filename = duk_require_pathname(ctx, 0, "images", true, false);
	image_save(image, filename);
	return 1;
}

static duk_ret_t
js_Surface_setAlpha(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int a = duk_to_int(ctx, 0);
	bool want_all = n_args >= 2 ? duk_to_boolean(ctx, 1) : true;

	image_t*      image;
	image_lock_t* lock;
	color_t*      pixel;
	int           w, h;

	int i_x, i_y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	if (!(lock = image_lock(image)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to lock surface");
	w = image_width(image);
	h = image_height(image);
	a = a < 0 ? 0 : a > 255 ? 255 : a;
	for (i_y = h - 1; i_y >= 0; --i_y) for (i_x = w - 1; i_x >= 0; --i_x) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		pixel->a = want_all || pixel->a != 0
			? a : pixel->a;
	}
	image_unlock(image, lock);
	return 0;
}

static duk_ret_t
js_Surface_setBlendMode(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, -2, "\xFF" "blend_mode");
	return 0;
}

static duk_ret_t
js_Surface_setPixel(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	color_t color = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");

	image_set_pixel(image, x, y, color);
	return 0;
}

static duk_ret_t
js_Surface_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object surface]");
	return 1;
}

static duk_ret_t
js_Surface_transformBlitMaskSurface(duk_context* ctx)
{
	int      blend_mode;
	int      height;
	image_t* image;
	color_t  mask;
	image_t* source_image;
	int      width;
	int      x1, x2, x3, x4;
	int      y1, y2, y3, y4;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x1 = duk_to_int(ctx, 1);
	y1 = duk_to_int(ctx, 2);
	x2 = duk_to_int(ctx, 3);
	y2 = duk_to_int(ctx, 4);
	x3 = duk_to_int(ctx, 5);
	y3 = duk_to_int(ctx, 6);
	x4 = duk_to_int(ctx, 7);
	y4 = duk_to_int(ctx, 8);
	mask = duk_require_sphere_color(ctx, 9);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, nativecolor(mask) },
		{ x2, y2, 0, width, 0, nativecolor(mask) },
		{ x4, y4, 0, 0, height, nativecolor(mask) },
		{ x3, y3, 0, width, height, nativecolor(mask) },
	};
	al_draw_prim(v, NULL, image_bitmap(source_image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_transformBlitSurface(duk_context* ctx)
{
	int      blend_mode;
	int      height;
	image_t* image;
	image_t* source_image;
	int      width;
	int      x1, x2, x3, x4;
	int      y1, y2, y3, y4;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x1 = duk_to_int(ctx, 1);
	y1 = duk_to_int(ctx, 2);
	x2 = duk_to_int(ctx, 3);
	y2 = duk_to_int(ctx, 4);
	x3 = duk_to_int(ctx, 5);
	y3 = duk_to_int(ctx, 6);
	x4 = duk_to_int(ctx, 7);
	y4 = duk_to_int(ctx, 8);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, al_map_rgba(255, 255, 255, 255) },
		{ x2, y2, 0, width, 0, al_map_rgba(255, 255, 255, 255) },
		{ x4, y4, 0, 0, height, al_map_rgba(255, 255, 255, 255) },
		{ x3, y3, 0, width, height, al_map_rgba(255, 255, 255, 255) },
	};
	al_draw_prim(v, NULL, image_bitmap(source_image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_zoomBlitMaskSurface(duk_context* ctx)
{
	int      blend_mode;
	int      height;
	image_t* image;
	color_t  mask;
	float    scale;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	scale = duk_to_number(ctx, 3);
	mask = duk_require_sphere_color(ctx, 4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_scaled_bitmap(image_bitmap(source_image),
		nativecolor(mask),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_Surface_zoomBlitSurface(duk_context* ctx)
{
	int      blend_mode;
	int      height;
	image_t* image;
	float    scale;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_class_obj(ctx, -1, "ssSurface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1);
	source_image = duk_require_class_obj(ctx, 0, "ssSurface");
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	scale = duk_to_number(ctx, 3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_scaled_bitmap(image_bitmap(source_image),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	reset_blend_modes();
	return 0;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	winstyle = duk_require_class_obj(ctx, 0, "ssWindowStyle");
	winstyle_unref(winstyle);
	return 0;
}

static duk_ret_t
js_WindowStyle_drawWindow(duk_context* ctx)
{
	int            height;
	color_t        mask;
	int            width;
	windowstyle_t* winstyle;
	int            x;
	int            y;

	duk_push_this(ctx);
	winstyle = duk_require_class_obj(ctx, -1, "ssWindowStyle");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	mask = duk_require_sphere_color(ctx, -1);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);

	winstyle_draw(winstyle, mask, x, y, width, height);
	return 0;
}

static duk_ret_t
js_WindowStyle_getColorMask(duk_context* ctx)
{
	windowstyle_t* winstyle;

	duk_push_this(ctx);
	winstyle = duk_require_class_obj(ctx, -1, "ssWindowStyle");

	duk_get_prop_string(ctx, -2, "\xFF" "color_mask");
	return 1;
}

static duk_ret_t
js_WindowStyle_setColorMask(duk_context* ctx)
{
	color_t        mask;
	windowstyle_t* winstyle;

	duk_push_this(ctx);
	winstyle = duk_require_class_obj(ctx, -1, "ssWindowStyle");
	mask = duk_require_sphere_color(ctx, 0);

	duk_push_sphere_color(ctx, mask);
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	return 0;
}

static duk_ret_t
js_WindowStyle_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object windowstyle]");
	return 1;
}
