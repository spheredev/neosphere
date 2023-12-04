/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#include "neosphere.h"
#include "vanilla.h"

#include "animation.h"
#include "api.h"
#include "audio.h"
#include "blend_op.h"
#include "byte_array.h"
#include "compress.h"
#include "debugger.h"
#include "dispatch.h"
#include "event_loop.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "jsal.h"
#include "kev_file.h"
#include "legacy.h"
#include "logger.h"
#include "map_engine.h"
#include "script.h"
#include "spriteset.h"
#include "windowstyle.h"

#define API_VERSION        2.0
#define API_VERSION_STRING "v2.0"

static bool js_Abort                            (int num_args, bool is_ctor, intptr_t magic);
static bool js_AddTrigger                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_AddZone                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_ApplyColorMask                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_AreKeysLeft                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_AreZonesAt                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_AttachCamera                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_AttachInput                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_AttachPlayerInput                (int num_args, bool is_ctor, intptr_t magic);
static bool js_BezierCurve                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_BindJoystickButton               (int num_args, bool is_ctor, intptr_t magic);
static bool js_BindKey                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_BlendColors                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_CallDefaultMapScript             (int num_args, bool is_ctor, intptr_t magic);
static bool js_CallDefaultPersonScript          (int num_args, bool is_ctor, intptr_t magic);
static bool js_CallMapScript                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_CallPersonScript                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_ChangeMap                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_ClearPersonCommands              (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreatePerson                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateByteArray                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateByteArrayFromString        (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateColor                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateColorMatrix                (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateDirectory                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateSpriteset                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateStringFromByteArray        (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateStringFromCode             (int num_args, bool is_ctor, intptr_t magic);
static bool js_CreateSurface                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_DeflateByteArray                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_DeflateFile                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_DestroyPerson                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_DetachCamera                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_DetachInput                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_DetachPlayerInput                (int num_args, bool is_ctor, intptr_t magic);
static bool js_DoEvents                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_DoesFileExist                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_DoesPersonExist                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Delay                            (int num_args, bool is_ctor, intptr_t magic);
static bool js_EvaluateScript                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_EvaluateSystemScript             (int num_args, bool is_ctor, intptr_t magic);
static bool js_ExecuteGame                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_ExecuteTrigger                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_ExecuteZoneScript                (int num_args, bool is_ctor, intptr_t magic);
static bool js_ExecuteZones                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Exit                             (int num_args, bool is_ctor, intptr_t magic);
static bool js_ExitMapEngine                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_FilledCircle                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_FilledComplex                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_FilledEllipse                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_FlipScreen                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_FollowPerson                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GarbageCollect                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetActingPerson                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCameraPerson                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCameraX                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCameraY                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetClippingRectangle             (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCurrentMap                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCurrentPerson                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCurrentTrigger                (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetCurrentZone                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetDirectoryList                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetFileList                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetFrameRate                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetGameList                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetInputPerson                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetJoystickAxis                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetKey                           (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetKeyString                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLayerAngle                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLayerHeight                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLayerMask                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLayerName                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLayerWidth                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLocalAddress                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetLocalName                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetMapEngine                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetMapEngineFrameRate            (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetMouseWheelEvent               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetMouseX                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetMouseY                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNextAnimatedTile              (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumJoysticks                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumJoystickAxes               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumJoystickButtons            (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumLayers                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumMouseWheelEvents           (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumTiles                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumTriggers                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetNumZones                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetObstructingPerson             (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetObstructingTile               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonAngle                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonBase                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonData                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonDirection               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonFollowDistance          (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonFollowers               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonFrame                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonFrameNext               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonFrameRevert             (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonIgnoreList              (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonLayer                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonLeader                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonList                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonMask                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonOffsetX                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonOffsetY                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonSpeedX                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonSpeedY                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonSpriteset               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonValue                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonX                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonY                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonXFloat                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPersonYFloat                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetPlayerKey                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetScreenHeight                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetScreenWidth                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetSystemArrow                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetSystemDownArrow               (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetSystemFont                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetSystemUpArrow                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetSystemWindowStyle             (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTalkActivationButton          (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTalkActivationKey             (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTalkDistance                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTile                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileDelay                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileHeight                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileImage                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileName                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileSurface                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTileWidth                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTime                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetToggleState                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTriggerLayer                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTriggerX                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetTriggerY                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetVersion                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetVersionString                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneHeight                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneLayer                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneSteps                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneWidth                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneX                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_GetZoneY                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_GrabImage                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_GrabSurface                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientCircle                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientComplex                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientEllipse                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientLine                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientRectangle                (int num_args, bool is_ctor, intptr_t magic);
static bool js_GradientTriangle                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_HashByteArray                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_HashFromFile                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_IgnorePersonObstructions         (int num_args, bool is_ctor, intptr_t magic);
static bool js_IgnoreTileObstructions           (int num_args, bool is_ctor, intptr_t magic);
static bool js_InflateByteArray                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_InflateFile                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsAnyKeyPressed                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsCameraAttached                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsCommandQueueEmpty              (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsIgnoringPersonObstructions     (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsIgnoringTileObstructions       (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsInputAttached                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsJoystickButtonPressed          (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsKeyPressed                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsLayerReflective                (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsLayerVisible                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsMapEngineRunning               (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsMouseButtonPressed             (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsPersonObstructed               (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsPersonVisible                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_IsTriggerAt                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Line                             (int num_args, bool is_ctor, intptr_t magic);
static bool js_LineIntersects                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_LineSeries                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_ListenOnPort                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadAnimation                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadFont                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadImage                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadSound                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadSoundEffect                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadSpriteset                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadSurface                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_LoadWindowStyle                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_MapEngine                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_MapToScreenX                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_MapToScreenY                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_OpenAddress                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_OpenFile                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_OpenLog                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_OpenRawFile                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_OutlinedCircle                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_OutlinedComplex                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_OutlinedEllipse                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_OutlinedRectangle                (int num_args, bool is_ctor, intptr_t magic);
static bool js_OutlinedRoundRectangle           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Point                            (int num_args, bool is_ctor, intptr_t magic);
static bool js_PointSeries                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Polygon                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Print                            (int num_args, bool is_ctor, intptr_t magic);
static bool js_QueuePersonCommand               (int num_args, bool is_ctor, intptr_t magic);
static bool js_QueuePersonScript                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Rectangle                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_RemoveDirectory                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_RemoveFile                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_RemoveTrigger                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_RemoveZone                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Rename                           (int num_args, bool is_ctor, intptr_t magic);
static bool js_RenderMap                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_ReplaceTilesOnLayer              (int num_args, bool is_ctor, intptr_t magic);
static bool js_RequireScript                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_RequireSystemScript              (int num_args, bool is_ctor, intptr_t magic);
static bool js_RestartGame                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_RoundRectangle                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_ScreenToMapX                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_ScreenToMapY                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetCameraX                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetCameraY                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetClippingRectangle             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetColorMask                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetDefaultMapScript              (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetDefaultPersonScript           (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetDelayScript                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetFrameRate                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerAngle                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerHeight                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerMask                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerReflective               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerRenderer                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerScaleFactorX             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerScaleFactorY             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerSize                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerVisible                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetLayerWidth                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetMapEngineFrameRate            (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetMousePosition                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetNextAnimatedTile              (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonAngle                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonData                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonDirection               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonFollowDistance          (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonFrame                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonFrameNext               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonFrameRevert             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonIgnoreList              (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonLayer                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonMask                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonOffsetX                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonOffsetY                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonScaleAbsolute           (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonScaleFactor             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonScript                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonSpeed                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonSpeedXY                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonSpriteset               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonValue                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonVisible                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonX                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonXYFloat                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetPersonY                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetRenderScript                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTalkActivationButton          (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTalkActivationKey             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTalkDistance                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTile                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTileDelay                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTileImage                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTileName                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTileSurface                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTriggerLayer                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTriggerScript                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetTriggerXY                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetUpdateScript                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetZoneDimensions                (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetZoneLayer                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetZoneScript                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_SetZoneSteps                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Triangle                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_UnbindJoystickButton             (int num_args, bool is_ctor, intptr_t magic);
static bool js_UnbindKey                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_UpdateMapEngine                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_get_height             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_get_width              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_drawFrame              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_drawZoomedFrame        (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_getDelay               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_getNumFrames           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Animation_readNextFrame          (int num_args, bool is_ctor, intptr_t magic);
static bool js_ByteArray_get_length             (int num_args, bool is_ctor, intptr_t magic);
static bool js_ByteArray_concat                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_ByteArray_slice                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_ByteArray_toString               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_get_alpha                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_get_blue                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_get_green                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_get_red                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_set_alpha                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_set_blue                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_set_green                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_set_red                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Color_toString                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_ColorMatrix_toString             (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_close                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_flush                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_getKey                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_getNumKeys                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_read                        (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_toString                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_File_write                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_clone                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_drawText                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_drawTextBox                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_drawZoomedText              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_getCharacterImage           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_getColorMask                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_getHeight                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_getStringHeight             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_getStringWidth              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_setCharacterImage           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_setColorMask                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_toString                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Font_wordWrapString              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_get_height                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_get_width                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_blit                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_blitMask                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_createSurface              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_rotateBlit                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_rotateBlitMask             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_toString                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_transformBlit              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_transformBlitMask          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_zoomBlit                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_zoomBlitMask               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Logger_beginBlock                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Logger_endBlock                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Logger_toString                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Logger_write                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_close                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_getPosition              (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_getSize                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_read                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_setPosition              (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_toString                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_RawFile_write                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_close                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_getPendingReadSize        (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_isConnected               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_read                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_toString                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Socket_write                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getLength                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getPan                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getPitch                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getPosition                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getRepeat                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_getVolume                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_isPlaying                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_isSeekable                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_pause                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_play                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_reset                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_setPan                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_setPitch                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_setPosition                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_setRepeat                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_setVolume                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_stop                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sound_toString                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_getPan               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_getPitch             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_getVolume            (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_setPan               (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_setPitch             (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_setVolume            (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_play                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_stop                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SoundEffect_toString             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Spriteset_get_filename           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Spriteset_clone                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Spriteset_save                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Spriteset_toString               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_get_height               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_get_width                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_applyColorFX             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_applyColorFX4            (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_applyLookup              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_bezierCurve              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_blit                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_blitMaskSurface          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_blitSurface              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_clone                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_cloneSection             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_createImage              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_drawText                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_filledCircle             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_filledEllipse            (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_flipHorizontally         (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_flipVertically           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_getPixel                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_gradientCircle           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_gradientEllipse          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_gradientLine             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_gradientRectangle        (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_line                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_lineSeries               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_outlinedCircle           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_outlinedEllipse          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_outlinedRectangle        (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_pointSeries              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_rotate                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_rotateBlitMaskSurface    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_rotateBlitSurface        (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_rectangle                (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_replaceColor             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_rescale                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_save                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_setAlpha                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_setBlendMode             (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_setPixel                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_toString                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_transformBlitMaskSurface (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_transformBlitSurface     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_zoomBlitMaskSurface      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Surface_zoomBlitSurface          (int num_args, bool is_ctor, intptr_t magic);
static bool js_WindowStyle_drawWindow           (int num_args, bool is_ctor, intptr_t magic);
static bool js_WindowStyle_getColorMask         (int num_args, bool is_ctor, intptr_t magic);
static bool js_WindowStyle_setColorMask         (int num_args, bool is_ctor, intptr_t magic);
static bool js_WindowStyle_toString             (int num_args, bool is_ctor, intptr_t magic);

static void js_Animation_finalize   (void* host_ptr);
static void js_ByteArray_finalize   (void* host_ptr);
static void js_File_finalize        (void* host_ptr);
static void js_Font_finalize        (void* host_ptr);
static void js_Image_finalize       (void* host_ptr);
static void js_Logger_finalize      (void* host_ptr);
static void js_RawFile_finalize     (void* host_ptr);
static void js_Socket_finalize      (void* host_ptr);
static void js_Sound_finalize       (void* host_ptr);
static void js_SoundEffect_finalize (void* host_ptr);
static void js_Spriteset_finalize   (void* host_ptr);
static void js_Surface_finalize     (void* host_ptr);
static void js_WindowStyle_finalize (void* host_ptr);

enum blend_mode
{
	// note: these are in the same order as their Sphere 1.x equivalents
	//       for maximum compatibility.
	BLEND_NORMAL,
	BLEND_REPLACE,
	BLEND_COPY_RGB,
	BLEND_COPY_ALPHA,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_MULTIPLY,
	BLEND_AVERAGE,
	BLEND_INVERT,
	BLEND_MAX,
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

static blend_op_t* s_blender_normal;
static blend_op_t* s_blender_null;
static blend_op_t* s_blender_add;
static blend_op_t* s_blender_average;
static blend_op_t* s_blender_copy;
static blend_op_t* s_blender_copy_alpha;
static blend_op_t* s_blender_copy_rgb;
static blend_op_t* s_blender_multiply;
static blend_op_t* s_blender_subtract;
static font_t*     s_default_font;
static int         s_frame_rate = 0;
static mixer_t*    s_sound_mixer;

void
vanilla_init(void)
{
	console_log(1, "initializing Sphere v1 API (%s)", API_VERSION_STRING);

	// note: `font_clone` is used purposely here to prevent the color mask from getting
	//       clobbered by internal text rendering (e.g. the FPS counter).
	s_default_font = font_clone(game_default_font(g_game));

	s_sound_mixer = mixer_new(44100, 16, 2);

	s_blender_normal = blend_op_new_sym(BLEND_OP_ADD, BLEND_ALPHA, BLEND_INV_ALPHA);
	s_blender_null = blend_op_new_sym(BLEND_OP_ADD, BLEND_ZERO, BLEND_ZERO);
	s_blender_add = blend_op_new_sym(BLEND_OP_ADD, BLEND_ONE, BLEND_ONE);
	s_blender_average = blend_op_new_sym(BLEND_OP_ADD, BLEND_CONST, BLEND_CONST);
	s_blender_copy = blend_op_new_sym(BLEND_OP_ADD, BLEND_ONE, BLEND_ZERO);
	s_blender_copy_alpha = blend_op_new_asym(BLEND_OP_ADD, BLEND_ZERO, BLEND_ONE, BLEND_OP_ADD, BLEND_ONE, BLEND_ZERO);
	s_blender_copy_rgb = blend_op_new_asym(BLEND_OP_ADD, BLEND_ONE, BLEND_ZERO, BLEND_OP_ADD, BLEND_ZERO, BLEND_ONE);
	s_blender_multiply = blend_op_new_sym(BLEND_OP_ADD, BLEND_DEST, BLEND_ZERO);
	s_blender_subtract = blend_op_new_sym(BLEND_OP_SUB, BLEND_ONE, BLEND_ONE);
	
	// there's no native "half" factor but we can fake it with a color constant.
	// as far as I can tell, this is the only way to implement AVERAGE blending properly.
	blend_op_set_const(s_blender_average, 0.5f, 0.5f, 0.5f, 0.5f);

	// set up a dictionary to track RequireScript() calls
	jsal_push_hidden_stash();
	jsal_push_new_bare_object();
	jsal_put_prop_string(-2, "RequireScript");
	jsal_pop(1);

	// stashed person data (used by Get/SetPersonData() APIs)
	jsal_push_hidden_stash();
	jsal_push_new_object();
	jsal_put_prop_string(-2, "personData");
	jsal_pop(1);

	// register the absolutely ginormous (and deprecated!) Sphere v1 API.
	api_define_func(NULL, "Abort", js_Abort, 0);
	api_define_func(NULL, "AddTrigger", js_AddTrigger, 0);
	api_define_func(NULL, "AddZone", js_AddZone, 0);
	api_define_func(NULL, "ApplyColorMask", js_ApplyColorMask, 0);
	api_define_func(NULL, "AreKeysLeft", js_AreKeysLeft, 0);
	api_define_func(NULL, "AreZonesAt", js_AreZonesAt, 0);
	api_define_func(NULL, "AttachCamera", js_AttachCamera, 0);
	api_define_func(NULL, "AttachInput", js_AttachInput, 0);
	api_define_func(NULL, "AttachPlayerInput", js_AttachPlayerInput, 0);
	api_define_func(NULL, "BezierCurve", js_BezierCurve, 0);
	api_define_func(NULL, "BindJoystickButton", js_BindJoystickButton, 0);
	api_define_func(NULL, "BindKey", js_BindKey, 0);
	api_define_func(NULL, "BlendColors", js_BlendColors, 0);
	api_define_func(NULL, "BlendColorsWeighted", js_BlendColors, 0);
	api_define_func(NULL, "CallDefaultMapScript", js_CallDefaultMapScript, 0);
	api_define_func(NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript, 0);
	api_define_func(NULL, "CallMapScript", js_CallMapScript, 0);
	api_define_func(NULL, "CallPersonScript", js_CallPersonScript, 0);
	api_define_func(NULL, "ChangeMap", js_ChangeMap, 0);
	api_define_func(NULL, "ClearPersonCommands", js_ClearPersonCommands, 0);
	api_define_func(NULL, "CreateByteArray", js_CreateByteArray, 0);
	api_define_func(NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString, 0);
	api_define_func(NULL, "CreateColor", js_CreateColor, 0);
	api_define_func(NULL, "CreateColorMatrix", js_CreateColorMatrix, 0);
	api_define_func(NULL, "CreateDirectory", js_CreateDirectory, 0);
	api_define_func(NULL, "CreatePerson", js_CreatePerson, 0);
	api_define_func(NULL, "CreateSpriteset", js_CreateSpriteset, 0);
	api_define_func(NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray, 0);
	api_define_func(NULL, "CreateStringFromCode", js_CreateStringFromCode, 0);
	api_define_func(NULL, "CreateSurface", js_CreateSurface, 0);
	api_define_func(NULL, "DeflateByteArray", js_DeflateByteArray, 0);
	api_define_func(NULL, "DeflateFile", js_DeflateFile, 0);
	api_define_func(NULL, "Delay", js_Delay, 0);
	api_define_func(NULL, "DestroyPerson", js_DestroyPerson, 0);
	api_define_func(NULL, "DetachCamera", js_DetachCamera, 0);
	api_define_func(NULL, "DetachInput", js_DetachInput, 0);
	api_define_func(NULL, "DetachPlayerInput", js_DetachPlayerInput, 0);
	api_define_func(NULL, "DoEvents", js_DoEvents, 0);
	api_define_func(NULL, "DoesFileExist", js_DoesFileExist, 0);
	api_define_func(NULL, "DoesPersonExist", js_DoesPersonExist, 0);
	api_define_func(NULL, "EvaluateScript", js_EvaluateScript, 0);
	api_define_func(NULL, "EvaluateSystemScript", js_EvaluateSystemScript, 0);
	api_define_func(NULL, "Exit", js_Exit, 0);
	api_define_func(NULL, "ExitMapEngine", js_ExitMapEngine, 0);
	api_define_func(NULL, "ExecuteGame", js_ExecuteGame, 0);
	api_define_func(NULL, "ExecuteTrigger", js_ExecuteTrigger, 0);
	api_define_func(NULL, "ExecuteZoneScript", js_ExecuteZoneScript, 0);
	api_define_func(NULL, "ExecuteZones", js_ExecuteZones, 0);
	api_define_func(NULL, "FilledCircle", js_FilledCircle, 0);
	api_define_func(NULL, "FilledComplex", js_FilledComplex, 0);
	api_define_func(NULL, "FilledEllipse", js_FilledEllipse, 0);
	api_define_func(NULL, "FlipScreen", js_FlipScreen, 0);
	api_define_func(NULL, "FollowPerson", js_FollowPerson, 0);
	api_define_func(NULL, "GarbageCollect", js_GarbageCollect, 0);
	api_define_func(NULL, "GetActingPerson", js_GetActingPerson, 0);
	api_define_func(NULL, "GetCameraPerson", js_GetCameraPerson, 0);
	api_define_func(NULL, "GetCameraX", js_GetCameraX, 0);
	api_define_func(NULL, "GetCameraY", js_GetCameraY, 0);
	api_define_func(NULL, "GetClippingRectangle", js_GetClippingRectangle, 0);
	api_define_func(NULL, "GetCurrentMap", js_GetCurrentMap, 0);
	api_define_func(NULL, "GetCurrentPerson", js_GetCurrentPerson, 0);
	api_define_func(NULL, "GetCurrentTrigger", js_GetCurrentTrigger, 0);
	api_define_func(NULL, "GetCurrentZone", js_GetCurrentZone, 0);
	api_define_func(NULL, "GetDirectoryList", js_GetDirectoryList, 0);
	api_define_func(NULL, "GetFileList", js_GetFileList, 0);
	api_define_func(NULL, "GetFrameRate", js_GetFrameRate, 0);
	api_define_func(NULL, "GetGameList", js_GetGameList, 0);
	api_define_func(NULL, "GetInputPerson", js_GetInputPerson, 0);
	api_define_func(NULL, "GetJoystickAxis", js_GetJoystickAxis, 0);
	api_define_func(NULL, "GetLocalAddress", js_GetLocalAddress, 0);
	api_define_func(NULL, "GetLocalName", js_GetLocalName, 0);
	api_define_func(NULL, "GetKey", js_GetKey, 0);
	api_define_func(NULL, "GetKeyString", js_GetKeyString, 0);
	api_define_func(NULL, "GetLayerAngle", js_GetLayerAngle, 0);
	api_define_func(NULL, "GetLayerHeight", js_GetLayerHeight, 0);
	api_define_func(NULL, "GetLayerMask", js_GetLayerMask, 0);
	api_define_func(NULL, "GetLayerName", js_GetLayerName, 0);
	api_define_func(NULL, "GetLayerWidth", js_GetLayerWidth, 0);
	api_define_func(NULL, "GetMapEngine", js_GetMapEngine, 0);
	api_define_func(NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate, 0);
	api_define_func(NULL, "GetMouseWheelEvent", js_GetMouseWheelEvent, 0);
	api_define_func(NULL, "GetMouseX", js_GetMouseX, 0);
	api_define_func(NULL, "GetMouseY", js_GetMouseY, 0);
	api_define_func(NULL, "GetNumJoysticks", js_GetNumJoysticks, 0);
	api_define_func(NULL, "GetNumJoystickAxes", js_GetNumJoystickAxes, 0);
	api_define_func(NULL, "GetNumJoystickButtons", js_GetNumJoystickButtons, 0);
	api_define_func(NULL, "GetNumMouseWheelEvents", js_GetNumMouseWheelEvents, 0);
	api_define_func(NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile, 0);
	api_define_func(NULL, "GetNumLayers", js_GetNumLayers, 0);
	api_define_func(NULL, "GetNumTiles", js_GetNumTiles, 0);
	api_define_func(NULL, "GetNumTriggers", js_GetNumTriggers, 0);
	api_define_func(NULL, "GetNumZones", js_GetNumZones, 0);
	api_define_func(NULL, "GetObstructingPerson", js_GetObstructingPerson, 0);
	api_define_func(NULL, "GetObstructingTile", js_GetObstructingTile, 0);
	api_define_func(NULL, "GetPersonAngle", js_GetPersonAngle, 0);
	api_define_func(NULL, "GetPersonBase", js_GetPersonBase, 0);
	api_define_func(NULL, "GetPersonData", js_GetPersonData, 0);
	api_define_func(NULL, "GetPersonDirection", js_GetPersonDirection, 0);
	api_define_func(NULL, "GetPersonFollowDistance", js_GetPersonFollowDistance, 0);
	api_define_func(NULL, "GetPersonFollowers", js_GetPersonFollowers, 0);
	api_define_func(NULL, "GetPersonFrame", js_GetPersonFrame, 0);
	api_define_func(NULL, "GetPersonFrameNext", js_GetPersonFrameNext, 0);
	api_define_func(NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert, 0);
	api_define_func(NULL, "GetPersonIgnoreList", js_GetPersonIgnoreList, 0);
	api_define_func(NULL, "GetPersonLayer", js_GetPersonLayer, 0);
	api_define_func(NULL, "GetPersonLeader", js_GetPersonLeader, 0);
	api_define_func(NULL, "GetPersonList", js_GetPersonList, 0);
	api_define_func(NULL, "GetPersonMask", js_GetPersonMask, 0);
	api_define_func(NULL, "GetPersonOffsetX", js_GetPersonOffsetX, 0);
	api_define_func(NULL, "GetPersonOffsetY", js_GetPersonOffsetY, 0);
	api_define_func(NULL, "GetPersonSpeedX", js_GetPersonSpeedX, 0);
	api_define_func(NULL, "GetPersonSpeedY", js_GetPersonSpeedY, 0);
	api_define_func(NULL, "GetPersonSpriteset", js_GetPersonSpriteset, 0);
	api_define_func(NULL, "GetPersonValue", js_GetPersonValue, 0);
	api_define_func(NULL, "GetPersonX", js_GetPersonX, 0);
	api_define_func(NULL, "GetPersonXFloat", js_GetPersonXFloat, 0);
	api_define_func(NULL, "GetPersonY", js_GetPersonY, 0);
	api_define_func(NULL, "GetPersonYFloat", js_GetPersonYFloat, 0);
	api_define_func(NULL, "GetPlayerKey", js_GetPlayerKey, 0);
	api_define_func(NULL, "GetScreenHeight", js_GetScreenHeight, 0);
	api_define_func(NULL, "GetScreenWidth", js_GetScreenWidth, 0);
	api_define_func(NULL, "GetSystemArrow", js_GetSystemArrow, 0);
	api_define_func(NULL, "GetSystemDownArrow", js_GetSystemDownArrow, 0);
	api_define_func(NULL, "GetSystemFont", js_GetSystemFont, 0);
	api_define_func(NULL, "GetSystemUpArrow", js_GetSystemUpArrow, 0);
	api_define_func(NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle, 0);
	api_define_func(NULL, "GetTalkActivationButton", js_GetTalkActivationButton, 0);
	api_define_func(NULL, "GetTalkActivationKey", js_GetTalkActivationKey, 0);
	api_define_func(NULL, "GetTalkDistance", js_GetTalkDistance, 0);
	api_define_func(NULL, "GetTile", js_GetTile, 0);
	api_define_func(NULL, "GetTileDelay", js_GetTileDelay, 0);
	api_define_func(NULL, "GetTileImage", js_GetTileImage, 0);
	api_define_func(NULL, "GetTileHeight", js_GetTileHeight, 0);
	api_define_func(NULL, "GetTileName", js_GetTileName, 0);
	api_define_func(NULL, "GetTileSurface", js_GetTileSurface, 0);
	api_define_func(NULL, "GetTileWidth", js_GetTileWidth, 0);
	api_define_func(NULL, "GetTime", js_GetTime, 0);
	api_define_func(NULL, "GetToggleState", js_GetToggleState, 0);
	api_define_func(NULL, "GetTriggerLayer", js_GetTriggerLayer, 0);
	api_define_func(NULL, "GetTriggerX", js_GetTriggerX, 0);
	api_define_func(NULL, "GetTriggerY", js_GetTriggerY, 0);
	api_define_func(NULL, "GetVersion", js_GetVersion, 0);
	api_define_func(NULL, "GetVersionString", js_GetVersionString, 0);
	api_define_func(NULL, "GetZoneHeight", js_GetZoneHeight, 0);
	api_define_func(NULL, "GetZoneLayer", js_GetZoneLayer, 0);
	api_define_func(NULL, "GetZoneSteps", js_GetZoneSteps, 0);
	api_define_func(NULL, "GetZoneWidth", js_GetZoneWidth, 0);
	api_define_func(NULL, "GetZoneX", js_GetZoneX, 0);
	api_define_func(NULL, "GetZoneY", js_GetZoneY, 0);
	api_define_func(NULL, "GrabImage", js_GrabImage, 0);
	api_define_func(NULL, "GrabSurface", js_GrabSurface, 0);
	api_define_func(NULL, "GradientCircle", js_GradientCircle, 0);
	api_define_func(NULL, "GradientComplex", js_GradientComplex, 0);
	api_define_func(NULL, "GradientEllipse", js_GradientEllipse, 0);
	api_define_func(NULL, "GradientLine", js_GradientLine, 0);
	api_define_func(NULL, "GradientRectangle", js_GradientRectangle, 0);
	api_define_func(NULL, "GradientTriangle", js_GradientTriangle, 0);
	api_define_func(NULL, "HashByteArray", js_HashByteArray, 0);
	api_define_func(NULL, "HashFromFile", js_HashFromFile, 0);
	api_define_func(NULL, "IgnorePersonObstructions", js_IgnorePersonObstructions, 0);
	api_define_func(NULL, "IgnoreTileObstructions", js_IgnoreTileObstructions, 0);
	api_define_func(NULL, "InflateByteArray", js_InflateByteArray, 0);
	api_define_func(NULL, "InflateFile", js_InflateFile, 0);
	api_define_func(NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed, 0);
	api_define_func(NULL, "IsCameraAttached", js_IsCameraAttached, 0);
	api_define_func(NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty, 0);
	api_define_func(NULL, "IsIgnoringPersonObstructions", js_IsIgnoringPersonObstructions, 0);
	api_define_func(NULL, "IsIgnoringTileObstructions", js_IsIgnoringTileObstructions, 0);
	api_define_func(NULL, "IsInputAttached", js_IsInputAttached, 0);
	api_define_func(NULL, "IsJoystickButtonPressed", js_IsJoystickButtonPressed, 0);
	api_define_func(NULL, "IsKeyPressed", js_IsKeyPressed, 0);
	api_define_func(NULL, "IsLayerReflective", js_IsLayerReflective, 0);
	api_define_func(NULL, "IsLayerVisible", js_IsLayerVisible, 0);
	api_define_func(NULL, "IsMapEngineRunning", js_IsMapEngineRunning, 0);
	api_define_func(NULL, "IsMouseButtonPressed", js_IsMouseButtonPressed, 0);
	api_define_func(NULL, "IsPersonObstructed", js_IsPersonObstructed, 0);
	api_define_func(NULL, "IsPersonVisible", js_IsPersonVisible, 0);
	api_define_func(NULL, "IsTriggerAt", js_IsTriggerAt, 0);
	api_define_func(NULL, "Line", js_Line, 0);
	api_define_func(NULL, "LineIntersects", js_LineIntersects, 0);
	api_define_func(NULL, "LineSeries", js_LineSeries, 0);
	api_define_func(NULL, "ListenOnPort", js_ListenOnPort, 0);
	api_define_func(NULL, "LoadAnimation", js_LoadAnimation, 0);
	api_define_func(NULL, "LoadFont", js_LoadFont, 0);
	api_define_func(NULL, "LoadImage", js_LoadImage, 0);
	api_define_func(NULL, "LoadSound", js_LoadSound, 0);
	api_define_func(NULL, "LoadSoundEffect", js_LoadSoundEffect, 0);
	api_define_func(NULL, "LoadSpriteset", js_LoadSpriteset, 0);
	api_define_func(NULL, "LoadSurface", js_LoadSurface, 0);
	api_define_func(NULL, "LoadWindowStyle", js_LoadWindowStyle, 0);
	api_define_func(NULL, "MapEngine", js_MapEngine, 0);
	api_define_func(NULL, "MapToScreenX", js_MapToScreenX, 0);
	api_define_func(NULL, "MapToScreenY", js_MapToScreenY, 0);
	api_define_func(NULL, "OpenAddress", js_OpenAddress, 0);
	api_define_func(NULL, "OpenFile", js_OpenFile, 0);
	api_define_func(NULL, "OpenLog", js_OpenLog, 0);
	api_define_func(NULL, "OpenRawFile", js_OpenRawFile, 0);
	api_define_func(NULL, "OutlinedCircle", js_OutlinedCircle, 0);
	api_define_func(NULL, "OutlinedComplex", js_OutlinedComplex, 0);
	api_define_func(NULL, "OutlinedEllipse", js_OutlinedEllipse, 0);
	api_define_func(NULL, "OutlinedRectangle", js_OutlinedRectangle, 0);
	api_define_func(NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle, 0);
	api_define_func(NULL, "Point", js_Point, 0);
	api_define_func(NULL, "PointSeries", js_PointSeries, 0);
	api_define_func(NULL, "Polygon", js_Polygon, 0);
	api_define_func(NULL, "Print", js_Print, 0);
	api_define_func(NULL, "QueuePersonCommand", js_QueuePersonCommand, 0);
	api_define_func(NULL, "QueuePersonScript", js_QueuePersonScript, 0);
	api_define_func(NULL, "Rectangle", js_Rectangle, 0);
	api_define_func(NULL, "RemoveDirectory", js_RemoveDirectory, 0);
	api_define_func(NULL, "RemoveFile", js_RemoveFile, 0);
	api_define_func(NULL, "RemoveTrigger", js_RemoveTrigger, 0);
	api_define_func(NULL, "RemoveZone", js_RemoveZone, 0);
	api_define_func(NULL, "RenderMap", js_RenderMap, 0);
	api_define_func(NULL, "Rename", js_Rename, 0);
	api_define_func(NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer, 0);
	api_define_func(NULL, "RequireScript", js_RequireScript, 0);
	api_define_func(NULL, "RequireSystemScript", js_RequireSystemScript, 0);
	api_define_func(NULL, "RestartGame", js_RestartGame, 0);
	api_define_func(NULL, "RoundRectangle", js_RoundRectangle, 0);
	api_define_func(NULL, "ScreenToMapX", js_ScreenToMapX, 0);
	api_define_func(NULL, "ScreenToMapY", js_ScreenToMapY, 0);
	api_define_func(NULL, "SetCameraX", js_SetCameraX, 0);
	api_define_func(NULL, "SetCameraY", js_SetCameraY, 0);
	api_define_func(NULL, "SetClippingRectangle", js_SetClippingRectangle, 0);
	api_define_func(NULL, "SetColorMask", js_SetColorMask, 0);
	api_define_func(NULL, "SetDefaultMapScript", js_SetDefaultMapScript, 0);
	api_define_func(NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript, 0);
	api_define_func(NULL, "SetDelayScript", js_SetDelayScript, 0);
	api_define_func(NULL, "SetFrameRate", js_SetFrameRate, 0);
	api_define_func(NULL, "SetLayerAngle", js_SetLayerAngle, 0);
	api_define_func(NULL, "SetLayerHeight", js_SetLayerHeight, 0);
	api_define_func(NULL, "SetLayerMask", js_SetLayerMask, 0);
	api_define_func(NULL, "SetLayerReflective", js_SetLayerReflective, 0);
	api_define_func(NULL, "SetLayerRenderer", js_SetLayerRenderer, 0);
	api_define_func(NULL, "SetLayerScaleFactorX", js_SetLayerScaleFactorX, 0);
	api_define_func(NULL, "SetLayerScaleFactorY", js_SetLayerScaleFactorY, 0);
	api_define_func(NULL, "SetLayerSize", js_SetLayerSize, 0);
	api_define_func(NULL, "SetLayerVisible", js_SetLayerVisible, 0);
	api_define_func(NULL, "SetLayerWidth", js_SetLayerWidth, 0);
	api_define_func(NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate, 0);
	api_define_func(NULL, "SetMousePosition", js_SetMousePosition, 0);
	api_define_func(NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile, 0);
	api_define_func(NULL, "SetPersonAngle", js_SetPersonAngle, 0);
	api_define_func(NULL, "SetPersonData", js_SetPersonData, 0);
	api_define_func(NULL, "SetPersonDirection", js_SetPersonDirection, 0);
	api_define_func(NULL, "SetPersonFollowDistance", js_SetPersonFollowDistance, 0);
	api_define_func(NULL, "SetPersonFrame", js_SetPersonFrame, 0);
	api_define_func(NULL, "SetPersonFrameNext", js_SetPersonFrameNext, 0);
	api_define_func(NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert, 0);
	api_define_func(NULL, "SetPersonIgnoreList", js_SetPersonIgnoreList, 0);
	api_define_func(NULL, "SetPersonLayer", js_SetPersonLayer, 0);
	api_define_func(NULL, "SetPersonMask", js_SetPersonMask, 0);
	api_define_func(NULL, "SetPersonOffsetX", js_SetPersonOffsetX, 0);
	api_define_func(NULL, "SetPersonOffsetY", js_SetPersonOffsetY, 0);
	api_define_func(NULL, "SetPersonScaleAbsolute", js_SetPersonScaleAbsolute, 0);
	api_define_func(NULL, "SetPersonScaleFactor", js_SetPersonScaleFactor, 0);
	api_define_func(NULL, "SetPersonScript", js_SetPersonScript, 0);
	api_define_func(NULL, "SetPersonSpeed", js_SetPersonSpeed, 0);
	api_define_func(NULL, "SetPersonSpeedXY", js_SetPersonSpeedXY, 0);
	api_define_func(NULL, "SetPersonSpriteset", js_SetPersonSpriteset, 0);
	api_define_func(NULL, "SetPersonValue", js_SetPersonValue, 0);
	api_define_func(NULL, "SetPersonVisible", js_SetPersonVisible, 0);
	api_define_func(NULL, "SetPersonX", js_SetPersonX, 0);
	api_define_func(NULL, "SetPersonXYFloat", js_SetPersonXYFloat, 0);
	api_define_func(NULL, "SetPersonY", js_SetPersonY, 0);
	api_define_func(NULL, "SetRenderScript", js_SetRenderScript, 0);
	api_define_func(NULL, "SetTalkActivationButton", js_SetTalkActivationButton, 0);
	api_define_func(NULL, "SetTalkActivationKey", js_SetTalkActivationKey, 0);
	api_define_func(NULL, "SetTalkDistance", js_SetTalkDistance, 0);
	api_define_func(NULL, "SetTile", js_SetTile, 0);
	api_define_func(NULL, "SetTileDelay", js_SetTileDelay, 0);
	api_define_func(NULL, "SetTileImage", js_SetTileImage, 0);
	api_define_func(NULL, "SetTileName", js_SetTileName, 0);
	api_define_func(NULL, "SetTileSurface", js_SetTileSurface, 0);
	api_define_func(NULL, "SetTriggerLayer", js_SetTriggerLayer, 0);
	api_define_func(NULL, "SetTriggerScript", js_SetTriggerScript, 0);
	api_define_func(NULL, "SetTriggerXY", js_SetTriggerXY, 0);
	api_define_func(NULL, "SetUpdateScript", js_SetUpdateScript, 0);
	api_define_func(NULL, "SetZoneDimensions", js_SetZoneDimensions, 0);
	api_define_func(NULL, "SetZoneLayer", js_SetZoneLayer, 0);
	api_define_func(NULL, "SetZoneScript", js_SetZoneScript, 0);
	api_define_func(NULL, "SetZoneSteps", js_SetZoneSteps, 0);
	api_define_func(NULL, "Triangle", js_Triangle, 0);
	api_define_func(NULL, "UnbindJoystickButton", js_UnbindJoystickButton, 0);
	api_define_func(NULL, "UnbindKey", js_UnbindKey, 0);
	api_define_func(NULL, "UpdateMapEngine", js_UpdateMapEngine, 0);

	api_define_class("v1Animation", SV1_ANIMATION, NULL, js_Animation_finalize, 0);
	api_define_prop("v1Animation", "width", false, js_Animation_get_width, NULL);
	api_define_prop("v1Animation", "height", false, js_Animation_get_height, NULL);
	api_define_method("v1Animation", "getDelay", js_Animation_getDelay, 0);
	api_define_method("v1Animation", "getNumFrames", js_Animation_getNumFrames, 0);
	api_define_method("v1Animation", "drawFrame", js_Animation_drawFrame, 0);
	api_define_method("v1Animation", "drawZoomedFrame", js_Animation_drawZoomedFrame, 0);
	api_define_method("v1Animation", "readNextFrame", js_Animation_readNextFrame, 0);

	api_define_class("v1ByteArray", SV1_BYTE_ARRAY, NULL, js_ByteArray_finalize, 0);
	api_define_prop("v1ByteArray", "length", false, js_ByteArray_get_length, NULL);
	api_define_method("v1ByteArray", "concat", js_ByteArray_concat, 0);
	api_define_method("v1ByteArray", "slice", js_ByteArray_slice, 0);
	api_define_method("v1ByteArray", "toString", js_ByteArray_toString, 0);

	api_define_class("v1Color", SV1_COLOR, NULL, NULL, 0);
	api_define_prop("v1Color", "alpha", true, js_Color_get_alpha, js_Color_set_alpha);
	api_define_prop("v1Color", "blue", true, js_Color_get_blue, js_Color_set_blue);
	api_define_prop("v1Color", "green", true, js_Color_get_green, js_Color_set_green);
	api_define_prop("v1Color", "red", true, js_Color_get_red, js_Color_set_red);
	api_define_method("v1Color", "toString", js_Color_toString, 0);

	api_define_class("v1ColorMatrix", SV1_COLOR_MATRIX, NULL, NULL, 0);
	api_define_method("v1ColorMatrix", "toString", js_ColorMatrix_toString, 0);

	api_define_class("v1File", SV1_FILE, NULL, js_File_finalize, 0);
	api_define_method("v1File", "close", js_File_close, 0);
	api_define_method("v1File", "flush", js_File_flush, 0);
	api_define_method("v1File", "getKey", js_File_getKey, 0);
	api_define_method("v1File", "getNumKeys", js_File_getNumKeys, 0);
	api_define_method("v1File", "read", js_File_read, 0);
	api_define_method("v1File", "toString", js_File_toString, 0);
	api_define_method("v1File", "write", js_File_write, 0);

	api_define_class("v1Font", SV1_FONT, NULL, js_Font_finalize, 0);
	api_define_method("v1Font", "clone", js_Font_clone, 0);
	api_define_method("v1Font", "drawText", js_Font_drawText, 0);
	api_define_method("v1Font", "drawTextBox", js_Font_drawTextBox, 0);
	api_define_method("v1Font", "drawZoomedText", js_Font_drawZoomedText, 0);
	api_define_method("v1Font", "getCharacterImage", js_Font_getCharacterImage, 0);
	api_define_method("v1Font", "getColorMask", js_Font_getColorMask, 0);
	api_define_method("v1Font", "getHeight", js_Font_getHeight, 0);
	api_define_method("v1Font", "getStringHeight", js_Font_getStringHeight, 0);
	api_define_method("v1Font", "getStringWidth", js_Font_getStringWidth, 0);
	api_define_method("v1Font", "setCharacterImage", js_Font_setCharacterImage, 0);
	api_define_method("v1Font", "setColorMask", js_Font_setColorMask, 0);
	api_define_method("v1Font", "toString", js_Font_toString, 0);
	api_define_method("v1Font", "wordWrapString", js_Font_wordWrapString, 0);

	api_define_class("v1Image", SV1_IMAGE, NULL, js_Image_finalize, 0);
	api_define_prop("v1Image", "height", false, js_Image_get_height, NULL);
	api_define_prop("v1Image", "width", false, js_Image_get_width, NULL);
	api_define_method("v1Image", "blit", js_Image_blit, 0);
	api_define_method("v1Image", "blitMask", js_Image_blitMask, 0);
	api_define_method("v1Image", "createSurface", js_Image_createSurface, 0);
	api_define_method("v1Image", "rotateBlit", js_Image_rotateBlit, 0);
	api_define_method("v1Image", "rotateBlitMask", js_Image_rotateBlitMask, 0);
	api_define_method("v1Image", "toString", js_Image_toString, 0);
	api_define_method("v1Image", "transformBlit", js_Image_transformBlit, 0);
	api_define_method("v1Image", "transformBlitMask", js_Image_transformBlitMask, 0);
	api_define_method("v1Image", "zoomBlit", js_Image_zoomBlit, 0);
	api_define_method("v1Image", "zoomBlitMask", js_Image_zoomBlitMask, 0);

	api_define_class("v1Logger", SV1_LOGGER, NULL, js_Logger_finalize, 0);
	api_define_method("v1Logger", "toString", js_Logger_toString, 0);
	api_define_method("v1Logger", "beginBlock", js_Logger_beginBlock, 0);
	api_define_method("v1Logger", "endBlock", js_Logger_endBlock, 0);
	api_define_method("v1Logger", "write", js_Logger_write, 0);

	api_define_class("v1RawFile", SV1_RAW_FILE, NULL, js_RawFile_finalize, 0);
	api_define_method("v1RawFile", "close", js_RawFile_close, 0);
	api_define_method("v1RawFile", "getPosition", js_RawFile_getPosition, 0);
	api_define_method("v1RawFile", "getSize", js_RawFile_getSize, 0);
	api_define_method("v1RawFile", "read", js_RawFile_read, 0);
	api_define_method("v1RawFile", "setPosition", js_RawFile_setPosition, 0);
	api_define_method("v1RawFile", "toString", js_RawFile_toString, 0);
	api_define_method("v1RawFile", "write", js_RawFile_write, 0);

	api_define_class("v1Socket", SV1_SOCKET, NULL, js_Socket_finalize, 0);
	api_define_method("v1Socket", "close", js_Socket_close, 0);
	api_define_method("v1Socket", "getPendingReadSize", js_Socket_getPendingReadSize, 0);
	api_define_method("v1Socket", "isConnected", js_Socket_isConnected, 0);
	api_define_method("v1Socket", "read", js_Socket_read, 0);
	api_define_method("v1Socket", "toString", js_Socket_toString, 0);
	api_define_method("v1Socket", "write", js_Socket_write, 0);

	api_define_class("v1Sound", SV1_SOUND, NULL, js_Sound_finalize, 0);
	api_define_method("v1Sound", "getLength", js_Sound_getLength, 0);
	api_define_method("v1Sound", "getPan", js_Sound_getPan, 0);
	api_define_method("v1Sound", "getPitch", js_Sound_getPitch, 0);
	api_define_method("v1Sound", "getPosition", js_Sound_getPosition, 0);
	api_define_method("v1Sound", "getRepeat", js_Sound_getRepeat, 0);
	api_define_method("v1Sound", "getVolume", js_Sound_getVolume, 0);
	api_define_method("v1Sound", "isPlaying", js_Sound_isPlaying, 0);
	api_define_method("v1Sound", "isSeekable", js_Sound_isSeekable, 0);
	api_define_method("v1Sound", "pause", js_Sound_pause, 0);
	api_define_method("v1Sound", "play", js_Sound_play, 0);
	api_define_method("v1Sound", "reset", js_Sound_reset, 0);
	api_define_method("v1Sound", "setPan", js_Sound_setPan, 0);
	api_define_method("v1Sound", "setPitch", js_Sound_setPitch, 0);
	api_define_method("v1Sound", "setPosition", js_Sound_setPosition, 0);
	api_define_method("v1Sound", "setRepeat", js_Sound_setRepeat, 0);
	api_define_method("v1Sound", "setVolume", js_Sound_setVolume, 0);
	api_define_method("v1Sound", "stop", js_Sound_stop, 0);
	api_define_method("v1Sound", "toString", js_Sound_toString, 0);

	api_define_class("v1SoundEffect", SV1_SOUND_EFFECT, NULL, js_SoundEffect_finalize, 0);
	api_define_method("v1SoundEffect", "getPan", js_SoundEffect_getPan, 0);
	api_define_method("v1SoundEffect", "getPitch", js_SoundEffect_getPitch, 0);
	api_define_method("v1SoundEffect", "getVolume", js_SoundEffect_getVolume, 0);
	api_define_method("v1SoundEffect", "setPan", js_SoundEffect_setPan, 0);
	api_define_method("v1SoundEffect", "setPitch", js_SoundEffect_setPitch, 0);
	api_define_method("v1SoundEffect", "setVolume", js_SoundEffect_setVolume, 0);
	api_define_method("v1SoundEffect", "play", js_SoundEffect_play, 0);
	api_define_method("v1SoundEffect", "stop", js_SoundEffect_stop, 0);
	api_define_method("v1SoundEffect", "toString", js_SoundEffect_toString, 0);

	api_define_class("v1Spriteset", SV1_SPRITESET, NULL, js_Spriteset_finalize, 0);
	api_define_prop("v1Spriteset", "filename", false, js_Spriteset_get_filename, NULL);
	api_define_method("v1Spriteset", "clone", js_Spriteset_clone, 0);
	api_define_method("v1Spriteset", "save", js_Spriteset_save, 0);
	api_define_method("v1Spriteset", "toString", js_Spriteset_toString, 0);

	api_define_class("v1Surface", SV1_SURFACE, NULL, js_Surface_finalize, 0);
	api_define_prop("v1Surface", "height", false, js_Surface_get_height, NULL);
	api_define_prop("v1Surface", "width", false, js_Surface_get_width, NULL);
	api_define_method("v1Surface", "applyColorFX", js_Surface_applyColorFX, 0);
	api_define_method("v1Surface", "applyColorFX4", js_Surface_applyColorFX4, 0);
	api_define_method("v1Surface", "applyLookup", js_Surface_applyLookup, 0);
	api_define_method("v1Surface", "bezierCurve", js_Surface_bezierCurve, 0);
	api_define_method("v1Surface", "blit", js_Surface_blit, 0);
	api_define_method("v1Surface", "blitMaskSurface", js_Surface_blitMaskSurface, 0);
	api_define_method("v1Surface", "blitSurface", js_Surface_blitSurface, 0);
	api_define_method("v1Surface", "clone", js_Surface_clone, 0);
	api_define_method("v1Surface", "cloneSection", js_Surface_cloneSection, 0);
	api_define_method("v1Surface", "createImage", js_Surface_createImage, 0);
	api_define_method("v1Surface", "drawText", js_Surface_drawText, 0);
	api_define_method("v1Surface", "filledCircle", js_Surface_filledCircle, 0);
	api_define_method("v1Surface", "filledEllipse", js_Surface_filledEllipse, 0);
	api_define_method("v1Surface", "flipHorizontally", js_Surface_flipHorizontally, 0);
	api_define_method("v1Surface", "flipVertically", js_Surface_flipVertically, 0);
	api_define_method("v1Surface", "getPixel", js_Surface_getPixel, 0);
	api_define_method("v1Surface", "gradientCircle", js_Surface_gradientCircle, 0);
	api_define_method("v1Surface", "gradientEllipse", js_Surface_gradientEllipse, 0);
	api_define_method("v1Surface", "gradientLine", js_Surface_gradientLine, 0);
	api_define_method("v1Surface", "gradientRectangle", js_Surface_gradientRectangle, 0);
	api_define_method("v1Surface", "line", js_Surface_line, 0);
	api_define_method("v1Surface", "lineSeries", js_Surface_lineSeries, 0);
	api_define_method("v1Surface", "outlinedCircle", js_Surface_outlinedCircle, 0);
	api_define_method("v1Surface", "outlinedEllipse", js_Surface_outlinedEllipse, 0);
	api_define_method("v1Surface", "outlinedRectangle", js_Surface_outlinedRectangle, 0);
	api_define_method("v1Surface", "pointSeries", js_Surface_pointSeries, 0);
	api_define_method("v1Surface", "rotate", js_Surface_rotate, 0);
	api_define_method("v1Surface", "rotateBlitMaskSurface", js_Surface_rotateBlitMaskSurface, 0);
	api_define_method("v1Surface", "rotateBlitSurface", js_Surface_rotateBlitSurface, 0);
	api_define_method("v1Surface", "rectangle", js_Surface_rectangle, 0);
	api_define_method("v1Surface", "replaceColor", js_Surface_replaceColor, 0);
	api_define_method("v1Surface", "rescale", js_Surface_rescale, 0);
	api_define_method("v1Surface", "save", js_Surface_save, 0);
	api_define_method("v1Surface", "setAlpha", js_Surface_setAlpha, 0);
	api_define_method("v1Surface", "setBlendMode", js_Surface_setBlendMode, 0);
	api_define_method("v1Surface", "setPixel", js_Surface_setPixel, 0);
	api_define_method("v1Surface", "toString", js_Surface_toString, 0);
	api_define_method("v1Surface", "transformBlitMaskSurface", js_Surface_transformBlitMaskSurface, 0);
	api_define_method("v1Surface", "transformBlitSurface", js_Surface_transformBlitSurface, 0);
	api_define_method("v1Surface", "zoomBlitMaskSurface", js_Surface_zoomBlitMaskSurface, 0);
	api_define_method("v1Surface", "zoomBlitSurface", js_Surface_zoomBlitSurface, 0);

	api_define_class("v1WindowStyle", SV1_WINDOW_STYLE, NULL, js_WindowStyle_finalize, 0);
	api_define_method("v1WindowStyle", "drawWindow", js_WindowStyle_drawWindow, 0);
	api_define_method("v1WindowStyle", "getColorMask", js_WindowStyle_getColorMask, 0);
	api_define_method("v1WindowStyle", "setColorMask", js_WindowStyle_setColorMask, 0);
	api_define_method("v1WindowStyle", "toString", js_WindowStyle_toString, 0);

	// blend modes for Surfaces
	api_define_const(NULL, "BLEND", BLEND_NORMAL);
	api_define_const(NULL, "REPLACE", BLEND_REPLACE);
	api_define_const(NULL, "RGB_ONLY", BLEND_COPY_RGB);
	api_define_const(NULL, "ALPHA_ONLY", BLEND_COPY_ALPHA);
	api_define_const(NULL, "ADD", BLEND_ADD);
	api_define_const(NULL, "SUBTRACT", BLEND_SUBTRACT);
	api_define_const(NULL, "MULTIPLY", BLEND_MULTIPLY);
	api_define_const(NULL, "AVERAGE", BLEND_AVERAGE);
	api_define_const(NULL, "INVERT", BLEND_INVERT);

	// person movement commands
	api_define_const(NULL, "COMMAND_WAIT", COMMAND_WAIT);
	api_define_const(NULL, "COMMAND_ANIMATE", COMMAND_ANIMATE);
	api_define_const(NULL, "COMMAND_FACE_NORTH", COMMAND_FACE_NORTH);
	api_define_const(NULL, "COMMAND_FACE_NORTHEAST", COMMAND_FACE_NORTHEAST);
	api_define_const(NULL, "COMMAND_FACE_EAST", COMMAND_FACE_EAST);
	api_define_const(NULL, "COMMAND_FACE_SOUTHEAST", COMMAND_FACE_SOUTHEAST);
	api_define_const(NULL, "COMMAND_FACE_SOUTH", COMMAND_FACE_SOUTH);
	api_define_const(NULL, "COMMAND_FACE_SOUTHWEST", COMMAND_FACE_SOUTHWEST);
	api_define_const(NULL, "COMMAND_FACE_WEST", COMMAND_FACE_WEST);
	api_define_const(NULL, "COMMAND_FACE_NORTHWEST", COMMAND_FACE_NORTHWEST);
	api_define_const(NULL, "COMMAND_MOVE_NORTH", COMMAND_MOVE_NORTH);
	api_define_const(NULL, "COMMAND_MOVE_NORTHEAST", COMMAND_MOVE_NORTHEAST);
	api_define_const(NULL, "COMMAND_MOVE_EAST", COMMAND_MOVE_EAST);
	api_define_const(NULL, "COMMAND_MOVE_SOUTHEAST", COMMAND_MOVE_SOUTHEAST);
	api_define_const(NULL, "COMMAND_MOVE_SOUTH", COMMAND_MOVE_SOUTH);
	api_define_const(NULL, "COMMAND_MOVE_SOUTHWEST", COMMAND_MOVE_SOUTHWEST);
	api_define_const(NULL, "COMMAND_MOVE_WEST", COMMAND_MOVE_WEST);
	api_define_const(NULL, "COMMAND_MOVE_NORTHWEST", COMMAND_MOVE_NORTHWEST);

	// joystick axes
	api_define_const(NULL, "JOYSTICK_AXIS_X", 0);
	api_define_const(NULL, "JOYSTICK_AXIS_Y", 1);
	api_define_const(NULL, "JOYSTICK_AXIS_Z", 2);
	api_define_const(NULL, "JOYSTICK_AXIS_R", 3);
	api_define_const(NULL, "JOYSTICK_AXIS_U", 4);
	api_define_const(NULL, "JOYSTICK_AXIS_V", 5);

	// LineSeries() modes
	api_define_const(NULL, "LINE_MULTIPLE", LINE_MULTIPLE);
	api_define_const(NULL, "LINE_STRIP", LINE_STRIP);
	api_define_const(NULL, "LINE_LOOP", LINE_LOOP);

	// keyboard keys
	api_define_const(NULL, "KEY_NONE", 0);
	api_define_const(NULL, "KEY_SHIFT", ALLEGRO_KEY_LSHIFT);
	api_define_const(NULL, "KEY_CTRL", ALLEGRO_KEY_LCTRL);
	api_define_const(NULL, "KEY_ALT", ALLEGRO_KEY_ALT);
	api_define_const(NULL, "KEY_UP", ALLEGRO_KEY_UP);
	api_define_const(NULL, "KEY_DOWN", ALLEGRO_KEY_DOWN);
	api_define_const(NULL, "KEY_LEFT", ALLEGRO_KEY_LEFT);
	api_define_const(NULL, "KEY_RIGHT", ALLEGRO_KEY_RIGHT);
	api_define_const(NULL, "KEY_APOSTROPHE", ALLEGRO_KEY_QUOTE);
	api_define_const(NULL, "KEY_BACKSLASH", ALLEGRO_KEY_BACKSLASH);
	api_define_const(NULL, "KEY_BACKSPACE", ALLEGRO_KEY_BACKSPACE);
	api_define_const(NULL, "KEY_CLOSEBRACE", ALLEGRO_KEY_CLOSEBRACE);
	api_define_const(NULL, "KEY_CAPSLOCK", ALLEGRO_KEY_CAPSLOCK);
	api_define_const(NULL, "KEY_COMMA", ALLEGRO_KEY_COMMA);
	api_define_const(NULL, "KEY_DELETE", ALLEGRO_KEY_DELETE);
	api_define_const(NULL, "KEY_END", ALLEGRO_KEY_END);
	api_define_const(NULL, "KEY_ENTER", ALLEGRO_KEY_ENTER);
	api_define_const(NULL, "KEY_EQUALS", ALLEGRO_KEY_EQUALS);
	api_define_const(NULL, "KEY_ESCAPE", ALLEGRO_KEY_ESCAPE);
	api_define_const(NULL, "KEY_HOME", ALLEGRO_KEY_HOME);
	api_define_const(NULL, "KEY_INSERT", ALLEGRO_KEY_INSERT);
	api_define_const(NULL, "KEY_MINUS", ALLEGRO_KEY_MINUS);
	api_define_const(NULL, "KEY_NUMLOCK", ALLEGRO_KEY_NUMLOCK);
	api_define_const(NULL, "KEY_OPENBRACE", ALLEGRO_KEY_OPENBRACE);
	api_define_const(NULL, "KEY_PAGEDOWN", ALLEGRO_KEY_PGDN);
	api_define_const(NULL, "KEY_PAGEUP", ALLEGRO_KEY_PGUP);
	api_define_const(NULL, "KEY_PERIOD", ALLEGRO_KEY_FULLSTOP);
	api_define_const(NULL, "KEY_SCROLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const(NULL, "KEY_SCROLLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_define_const(NULL, "KEY_SEMICOLON", ALLEGRO_KEY_SEMICOLON);
	api_define_const(NULL, "KEY_SPACE", ALLEGRO_KEY_SPACE);
	api_define_const(NULL, "KEY_SLASH", ALLEGRO_KEY_SLASH);
	api_define_const(NULL, "KEY_TAB", ALLEGRO_KEY_TAB);
	api_define_const(NULL, "KEY_TILDE", ALLEGRO_KEY_TILDE);
	api_define_const(NULL, "KEY_F1", ALLEGRO_KEY_F1);
	api_define_const(NULL, "KEY_F2", ALLEGRO_KEY_F2);
	api_define_const(NULL, "KEY_F3", ALLEGRO_KEY_F3);
	api_define_const(NULL, "KEY_F4", ALLEGRO_KEY_F4);
	api_define_const(NULL, "KEY_F5", ALLEGRO_KEY_F5);
	api_define_const(NULL, "KEY_F6", ALLEGRO_KEY_F6);
	api_define_const(NULL, "KEY_F7", ALLEGRO_KEY_F7);
	api_define_const(NULL, "KEY_F8", ALLEGRO_KEY_F8);
	api_define_const(NULL, "KEY_F9", ALLEGRO_KEY_F9);
	api_define_const(NULL, "KEY_F10", ALLEGRO_KEY_F10);
	api_define_const(NULL, "KEY_F11", ALLEGRO_KEY_F11);
	api_define_const(NULL, "KEY_F12", ALLEGRO_KEY_F12);
	api_define_const(NULL, "KEY_A", ALLEGRO_KEY_A);
	api_define_const(NULL, "KEY_B", ALLEGRO_KEY_B);
	api_define_const(NULL, "KEY_C", ALLEGRO_KEY_C);
	api_define_const(NULL, "KEY_D", ALLEGRO_KEY_D);
	api_define_const(NULL, "KEY_E", ALLEGRO_KEY_E);
	api_define_const(NULL, "KEY_F", ALLEGRO_KEY_F);
	api_define_const(NULL, "KEY_G", ALLEGRO_KEY_G);
	api_define_const(NULL, "KEY_H", ALLEGRO_KEY_H);
	api_define_const(NULL, "KEY_I", ALLEGRO_KEY_I);
	api_define_const(NULL, "KEY_J", ALLEGRO_KEY_J);
	api_define_const(NULL, "KEY_K", ALLEGRO_KEY_K);
	api_define_const(NULL, "KEY_L", ALLEGRO_KEY_L);
	api_define_const(NULL, "KEY_M", ALLEGRO_KEY_M);
	api_define_const(NULL, "KEY_N", ALLEGRO_KEY_N);
	api_define_const(NULL, "KEY_O", ALLEGRO_KEY_O);
	api_define_const(NULL, "KEY_P", ALLEGRO_KEY_P);
	api_define_const(NULL, "KEY_Q", ALLEGRO_KEY_Q);
	api_define_const(NULL, "KEY_R", ALLEGRO_KEY_R);
	api_define_const(NULL, "KEY_S", ALLEGRO_KEY_S);
	api_define_const(NULL, "KEY_T", ALLEGRO_KEY_T);
	api_define_const(NULL, "KEY_U", ALLEGRO_KEY_U);
	api_define_const(NULL, "KEY_V", ALLEGRO_KEY_V);
	api_define_const(NULL, "KEY_W", ALLEGRO_KEY_W);
	api_define_const(NULL, "KEY_X", ALLEGRO_KEY_X);
	api_define_const(NULL, "KEY_Y", ALLEGRO_KEY_Y);
	api_define_const(NULL, "KEY_Z", ALLEGRO_KEY_Z);
	api_define_const(NULL, "KEY_1", ALLEGRO_KEY_1);
	api_define_const(NULL, "KEY_2", ALLEGRO_KEY_2);
	api_define_const(NULL, "KEY_3", ALLEGRO_KEY_3);
	api_define_const(NULL, "KEY_4", ALLEGRO_KEY_4);
	api_define_const(NULL, "KEY_5", ALLEGRO_KEY_5);
	api_define_const(NULL, "KEY_6", ALLEGRO_KEY_6);
	api_define_const(NULL, "KEY_7", ALLEGRO_KEY_7);
	api_define_const(NULL, "KEY_8", ALLEGRO_KEY_8);
	api_define_const(NULL, "KEY_9", ALLEGRO_KEY_9);
	api_define_const(NULL, "KEY_0", ALLEGRO_KEY_0);
	api_define_const(NULL, "KEY_NUM_1", ALLEGRO_KEY_PAD_1);
	api_define_const(NULL, "KEY_NUM_2", ALLEGRO_KEY_PAD_2);
	api_define_const(NULL, "KEY_NUM_3", ALLEGRO_KEY_PAD_3);
	api_define_const(NULL, "KEY_NUM_4", ALLEGRO_KEY_PAD_4);
	api_define_const(NULL, "KEY_NUM_5", ALLEGRO_KEY_PAD_5);
	api_define_const(NULL, "KEY_NUM_6", ALLEGRO_KEY_PAD_6);
	api_define_const(NULL, "KEY_NUM_7", ALLEGRO_KEY_PAD_7);
	api_define_const(NULL, "KEY_NUM_8", ALLEGRO_KEY_PAD_8);
	api_define_const(NULL, "KEY_NUM_9", ALLEGRO_KEY_PAD_9);
	api_define_const(NULL, "KEY_NUM_0", ALLEGRO_KEY_PAD_0);

	api_define_const(NULL, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
	api_define_const(NULL, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);
	api_define_const(NULL, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
	api_define_const(NULL, "MOUSE_WHEEL_UP", MOUSE_KEY_WHEEL_UP);
	api_define_const(NULL, "MOUSE_WHEEL_DOWN", MOUSE_KEY_WHEEL_DOWN);

	api_define_const(NULL, "PLAYER_1", PLAYER_1);
	api_define_const(NULL, "PLAYER_2", PLAYER_2);
	api_define_const(NULL, "PLAYER_3", PLAYER_3);
	api_define_const(NULL, "PLAYER_4", PLAYER_4);

	api_define_const(NULL, "PLAYER_KEY_MENU", PLAYER_KEY_MENU);
	api_define_const(NULL, "PLAYER_KEY_UP", PLAYER_KEY_UP);
	api_define_const(NULL, "PLAYER_KEY_DOWN", PLAYER_KEY_DOWN);
	api_define_const(NULL, "PLAYER_KEY_LEFT", PLAYER_KEY_LEFT);
	api_define_const(NULL, "PLAYER_KEY_RIGHT", PLAYER_KEY_RIGHT);
	api_define_const(NULL, "PLAYER_KEY_A", PLAYER_KEY_A);
	api_define_const(NULL, "PLAYER_KEY_B", PLAYER_KEY_B);
	api_define_const(NULL, "PLAYER_KEY_X", PLAYER_KEY_X);
	api_define_const(NULL, "PLAYER_KEY_Y", PLAYER_KEY_Y);

	// map script types
	api_define_const(NULL, "SCRIPT_ON_ENTER_MAP", MAP_SCRIPT_ON_ENTER);
	api_define_const(NULL, "SCRIPT_ON_LEAVE_MAP", MAP_SCRIPT_ON_LEAVE);
	api_define_const(NULL, "SCRIPT_ON_LEAVE_MAP_NORTH", MAP_SCRIPT_ON_LEAVE_NORTH);
	api_define_const(NULL, "SCRIPT_ON_LEAVE_MAP_EAST", MAP_SCRIPT_ON_LEAVE_EAST);
	api_define_const(NULL, "SCRIPT_ON_LEAVE_MAP_SOUTH", MAP_SCRIPT_ON_LEAVE_SOUTH);
	api_define_const(NULL, "SCRIPT_ON_LEAVE_MAP_WEST", MAP_SCRIPT_ON_LEAVE_WEST);

	// person script types
	api_define_const(NULL, "SCRIPT_ON_CREATE", PERSON_SCRIPT_ON_CREATE);
	api_define_const(NULL, "SCRIPT_ON_DESTROY", PERSON_SCRIPT_ON_DESTROY);
	api_define_const(NULL, "SCRIPT_ON_ACTIVATE_TOUCH", PERSON_SCRIPT_ON_TOUCH);
	api_define_const(NULL, "SCRIPT_ON_ACTIVATE_TALK", PERSON_SCRIPT_ON_TALK);
	api_define_const(NULL, "SCRIPT_COMMAND_GENERATOR", PERSON_SCRIPT_GENERATOR);

	api_define_const(NULL, "SE_SINGLE", 0);
	api_define_const(NULL, "SE_MULTIPLE", 1);
}

void
vanilla_uninit()
{
	font_unref(s_default_font);
	mixer_unref(s_sound_mixer);
	blend_op_unref(s_blender_normal);
	blend_op_unref(s_blender_null);
	blend_op_unref(s_blender_add);
	blend_op_unref(s_blender_average);
	blend_op_unref(s_blender_copy);
	blend_op_unref(s_blender_copy_alpha);
	blend_op_unref(s_blender_copy_rgb);
	blend_op_unref(s_blender_multiply);
	blend_op_unref(s_blender_subtract);
}

void
jsal_push_sphere_bytearray(bytearray_t* array)
{
	jsal_push_class_obj(SV1_BYTE_ARRAY, bytearray_ref(array), false);
	jsal_make_buffer(-1, JS_UINT8ARRAY, bytearray_buffer(array), bytearray_len(array));
}

void
jsal_push_sphere_color(color_t color)
{
	color_t* color_ptr;

	jsal_push_class_fatobj(SV1_COLOR, false, sizeof(color_t), (void**)&color_ptr);
	*color_ptr = color;
}

void
jsal_push_sphere_font(font_t* font)
{
	jsal_push_class_obj(SV1_FONT, font_ref(font), false);
}

void
jsal_push_sphere_spriteset(spriteset_t* spriteset)
{
	rect_t      base;
	image_t*    image;
	const char* pose_name;

	int i, j;

	jsal_push_class_obj(SV1_SPRITESET, spriteset_ref(spriteset), false);

	// Spriteset:base
	base = spriteset_get_base(spriteset);
	jsal_push_new_object();
	jsal_push_int(base.x1); jsal_put_prop_string(-2, "x1");
	jsal_push_int(base.y1); jsal_put_prop_string(-2, "y1");
	jsal_push_int(base.x2); jsal_put_prop_string(-2, "x2");
	jsal_push_int(base.y2); jsal_put_prop_string(-2, "y2");
	jsal_put_prop_string(-2, "base");

	// Spriteset:images
	jsal_push_new_array();
	for (i = 0; i < spriteset_num_images(spriteset); ++i) {
		image = spriteset_image(spriteset, i);
		jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
		jsal_put_prop_index(-2, i);
	}
	jsal_put_prop_string(-2, "images");

	// Spriteset:directions
	jsal_push_new_array();
	for (i = 0; i < spriteset_num_poses(spriteset); ++i) {
		pose_name = spriteset_pose_name(spriteset, i);
		jsal_push_new_object();
		jsal_push_string(pose_name);
		jsal_put_prop_string(-2, "name");
		jsal_push_new_array();
		for (j = 0; j < spriteset_num_frames(spriteset, pose_name); ++j) {
			jsal_push_new_object();
			jsal_push_int(spriteset_frame_image_index(spriteset, pose_name, j));
			jsal_put_prop_string(-2, "index");
			jsal_push_int(spriteset_frame_delay(spriteset, pose_name, j));
			jsal_put_prop_string(-2, "delay");
			jsal_put_prop_index(-2, j);
		}
		jsal_put_prop_string(-2, "frames");
		jsal_put_prop_index(-2, i);
	}
	jsal_put_prop_string(-2, "directions");
}

static int
jsal_require_map_layer(int at_index)
{
	long        strtol_out;
	int         layer_index;
	const char* name;
	char*       p_end;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	if (jsal_is_number(at_index)) {
		layer_index = jsal_get_int(at_index);
		goto have_index;
	}
	else if (jsal_is_string(at_index)) {
		// don't anyone ever say I'm not dedicated to compatibility!  there are a few
		// poorly written Sphere 1.5 games that pass layer IDs as strings.  usually this
		// would fail because neoSphere supports named layers, but here I go out of my
		// way to support it anyway.
		name = jsal_get_string(at_index);
		strtol_out = strtol(name, &p_end, 0);
		if ((layer_index = map_layer_by_name(name)) >= 0) {
			goto have_index;
		}
		else if (name[0] != '\0' && *p_end == '\0') {
			layer_index = (int)strtol_out;
			goto have_index;
		}
		else {
			jsal_error(JS_RANGE_ERROR, "Layer name does not exist '%s'", name);
		}
	}
	else {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a map layer", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}

have_index:
	if (layer_index < 0 || layer_index >= map_num_layers())
		jsal_error(JS_RANGE_ERROR, "Layer index out of range '%d'", layer_index);
	return layer_index;
}

color_t
jsal_require_sphere_color(int index)
{
	color_t* color_ptr;

	color_ptr = jsal_require_class_obj(index, SV1_COLOR);
	return *color_ptr;
}

color_fx_t
jsal_require_sphere_color_fx(int index)
{
	color_fx_t matrix;

	jsal_require_class_obj(index, SV1_COLOR_MATRIX);
	jsal_get_prop_string(index, "rn"); matrix.rn = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "rr"); matrix.rr = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "rg"); matrix.rg = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "rb"); matrix.rb = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "gn"); matrix.gn = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "gr"); matrix.gr = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "gg"); matrix.gg = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "gb"); matrix.gb = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "bn"); matrix.bn = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "br"); matrix.br = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "bg"); matrix.bg = jsal_get_int(-1); jsal_pop(1);
	jsal_get_prop_string(index, "bb"); matrix.bb = jsal_get_int(-1); jsal_pop(1);
	return matrix;
}

script_t*
jsal_require_sphere_script(int index, const char* name)
{
	lstring_t* codestring;
	script_t*  script;

	index = jsal_normalize_index(index);

	if (jsal_is_function(index)) {
		// caller passed function directly
		script = script_new_function(index);
		return script;
	}
	else if (jsal_is_string(index)) {
		// caller passed code string, compile it
		codestring = jsal_require_lstring_t(index);
		script = script_new(codestring, "%s", name);
		lstr_free(codestring);
		return script;
	}
	else if (jsal_is_null(index) || jsal_is_undefined(index)) {
		return NULL;
	}
	else {
		jsal_error(JS_TYPE_ERROR, "Expected a string, function, or null/undefined");
	}
}

spriteset_t*
jsal_require_sphere_spriteset(int index)
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

	index = jsal_normalize_index(index);
	jsal_require_class_obj(index, SV1_SPRITESET);

	spriteset = spriteset_new();

	jsal_get_prop_string(index, "base");
	jsal_get_prop_string(-1, "x1");
	base.x1 = jsal_require_int(-1);
	jsal_get_prop_string(-2, "y1");
	base.y1 = jsal_require_int(-1);
	jsal_get_prop_string(-3, "x2");
	base.x2 = jsal_require_int(-1);
	jsal_get_prop_string(-4, "y2");
	base.y2 = jsal_require_int(-1);
	spriteset_set_base(spriteset, base);
	jsal_pop(5);

	jsal_get_prop_string(index, "images");
	num_images = jsal_get_length(-1);
	for (i = 0; i < num_images; ++i) {
		jsal_get_prop_index(-1, i);
		image = jsal_require_class_obj(-1, SV1_IMAGE);
		spriteset_add_image(spriteset, image);
		jsal_pop(1);
	}
	jsal_pop(1);

	jsal_get_prop_string(index, "directions");
	num_poses = jsal_get_length(-1);
	for (i = 0; i < num_poses; ++i) {
		jsal_get_prop_index(-1, i);
		jsal_get_prop_string(-1, "name");
		pose_name = jsal_require_string(-1);
		spriteset_add_pose(spriteset, pose_name);
		jsal_get_prop_string(-2, "frames");
		num_frames = jsal_get_length(-1);
		for (j = 0; j < num_frames; ++j) {
			jsal_get_prop_index(-1, j);
			jsal_get_prop_string(-1, "index");
			image_index = jsal_require_int(-1);
			jsal_get_prop_string(-2, "delay");
			delay = jsal_require_int(-1);
			spriteset_add_frame(spriteset, pose_name, image_index, delay);
			jsal_pop(3);
		}
		jsal_pop(3);
	}
	jsal_pop(1);

	return spriteset;
}

static void
jsal_push_sphere_windowstyle(windowstyle_t* winstyle)
{
	jsal_push_class_obj(SV1_WINDOW_STYLE, winstyle_ref(winstyle), false);
}

static uint8_t*
jsal_require_rgba_lut(int index)
{
	uint8_t* lut;
	int      length;

	int i;

	index = jsal_normalize_index(index);
	jsal_require_object_coercible(index);
	if (!(lut = malloc(sizeof(uint8_t) * 256)))
		return NULL;
	length = fmin(jsal_get_length(index), 256);
	for (i = length; i < 256; ++i) lut[i] = i;
	for (i = 0; i < length; ++i) {
		jsal_get_prop_index(index, i);
		lut[i] = fmin(fmax(jsal_to_int(-1), 0), 255);
		jsal_pop(1);
	}
	return lut;
}

static void
apply_blend_mode(image_t* image, enum blend_mode mode)
{
	blend_op_t* op;

	op = mode == BLEND_NORMAL ? s_blender_normal
		: mode == BLEND_ADD ? s_blender_add
		: mode == BLEND_AVERAGE ? s_blender_average
		: mode == BLEND_COPY_ALPHA ? s_blender_copy_alpha
		: mode == BLEND_COPY_RGB ? s_blender_copy_rgb
		: mode == BLEND_MULTIPLY ? s_blender_multiply
		: mode == BLEND_REPLACE ? s_blender_copy
		: mode == BLEND_SUBTRACT ? s_blender_subtract
		: s_blender_null;
	image_set_blend_op(image, op);
}

static bool
js_Abort(int num_args, bool is_ctor, intptr_t magic)
{
	const char* message;
	char*       text;

	message = num_args >= 1
		? jsal_to_string(0)
		: "Some type of eaty pig just ate your game\n\n\n...and you*munch*";

	text = strnewf("abort requested...\n\n%s\n", message);
	sphere_abort(text);
	return false;
}

static bool
js_AddTrigger(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t    bounds;
	int       layer;
	script_t* script;
	int       x;
	int       y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);
	script = jsal_require_sphere_script(3, "%/triggerScript.js");

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	bounds = map_bounds();
	if (x < bounds.x1 || y < bounds.y1 || x >= bounds.x2 || y >= bounds.y2)
		jsal_error(JS_RANGE_ERROR, "X/Y out of bounds (%d,%d)", x, y);

	if (!map_add_trigger(x, y, layer, script))
		jsal_error(JS_ERROR, "Couldn't allocate new trigger");
	jsal_push_number(map_num_triggers() - 1);
	return true;
}

static bool
js_AddZone(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t    bounds;
	int       height;
	int       layer;
	script_t* script;
	int       width;
	int       x;
	int       y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	layer = jsal_require_map_layer(4);
	script = jsal_require_sphere_script(5, "%/zoneScript.js");

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		jsal_error(JS_RANGE_ERROR, "Invalid zone width/height [%d,%d]", width, height);
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		jsal_error(JS_RANGE_ERROR, "X/Y out of bounds (%d,%d)", x, y);

	if (!map_add_zone(mk_rect(x, y, width, height), layer, script, 8))
		jsal_error(JS_ERROR, "Couldn't allocate new zone");
	jsal_push_number(map_num_zones() - 1);
	script_unref(script);
	return true;
}

static bool
js_ApplyColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	size2_t resolution;

	color = jsal_require_sphere_color(0);

	if (screen_skipping_frame(g_screen))
		return false;
	resolution = screen_size(g_screen);
	galileo_reset();
	al_draw_filled_rectangle(0, 0, resolution.width, resolution.height,
		nativecolor(color));
	return false;
}

static bool
js_AreKeysLeft(int num_args, bool is_ctor, intptr_t magic)
{
	update_input();
	jsal_push_boolean(kb_queue_len() > 0);
	return true;
}

static bool
js_AreZonesAt(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	jsal_push_boolean(map_zone_at(x, y, layer, 0) >= 0);
	return true;
}

static bool
js_AttachCamera(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_to_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	map_engine_set_subject(person);
	return false;
}

static bool
js_AttachInput(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	map_engine_set_player(PLAYER_1, person);
	return false;
}

static bool
js_AttachPlayerInput(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	player_id_t player;

	name = jsal_require_string(0);
	player = jsal_to_int(1);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid player constant");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	map_engine_set_player(player, person);
	return false;
}

static bool
js_BezierCurve(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	float           cp[8];
	int             num_points;
	ALLEGRO_VERTEX* points;
	bool            quadratic = true;
	double          step_size;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	color = jsal_require_sphere_color(0);
	step_size = jsal_to_number(1);
	x1 = trunc(jsal_to_number(2));
	y1 = trunc(jsal_to_number(3));
	x2 = trunc(jsal_to_number(4));
	y2 = trunc(jsal_to_number(5));
	x3 = trunc(jsal_to_number(6));
	y3 = trunc(jsal_to_number(7));
	if (num_args >= 10) {
		quadratic = false;
		x4 = trunc(jsal_to_number(8));
		y4 = trunc(jsal_to_number(9));
	}

	if (screen_skipping_frame(g_screen))
		return false;
	cp[0] = x1; cp[1] = y1;
	cp[2] = x2; cp[3] = y2;
	cp[4] = x3; cp[5] = y3;
	cp[6] = x4; cp[7] = y4;
	if (quadratic) {
		// convert quadratic Bezier curve to cubic
		cp[6] = x3; cp[7] = y3;
		cp[2] = x1 + (2.0 / 3.0) * (x2 - x1);
		cp[3] = y1 + (2.0 / 3.0) * (y2 - y1);
		cp[4] = x3 + (2.0 / 3.0) * (x2 - x3);
		cp[5] = y3 + (2.0 / 3.0) * (y2 - y3);
	}
	step_size = step_size < 0.001 ? 0.001
		: step_size > 1.0 ? 1.0
		: step_size;
	num_points = 1.0 / step_size;
	points = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	memset(points, 0, num_points * sizeof(ALLEGRO_VERTEX));
	al_calculate_spline(&points[0].x, sizeof(ALLEGRO_VERTEX), cp, 0.0, num_points);
	for (i = 0; i < num_points; ++i)
		points[i].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(points, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	return false;
}

static bool
js_BindJoystickButton(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index = jsal_to_int(0);
	int button = jsal_to_int(1);
	script_t* on_down_script = jsal_require_sphere_script(2, "[button-down script]");
	script_t* on_up_script = jsal_require_sphere_script(3, "[button-up script]");

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		jsal_error(JS_RANGE_ERROR, "Joystick index '%d' out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		jsal_error(JS_RANGE_ERROR, "Button index '%d' out of range", button);
	joy_bind_button(joy_index, button, on_down_script, on_up_script);
	return false;
}

static bool
js_BindKey(int num_args, bool is_ctor, intptr_t magic)
{
	int keycode = jsal_to_int(0);
	script_t* on_down_script = jsal_require_sphere_script(1, "[key-down script]");
	script_t* on_up_script = jsal_require_sphere_script(2, "[key-up script]");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid key constant");
	kb_bind_key(keycode, on_down_script, on_up_script);
	return false;
}

static bool
js_BlendColors(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color1;
	color_t color2;
	float   w1 = 1.0;
	float   w2 = 1.0;

	color1 = jsal_require_sphere_color(0);
	color2 = jsal_require_sphere_color(1);
	if (num_args > 2) {
		w1 = jsal_to_number(2);
		w2 = jsal_to_number(3);
	}

	if (w1 < 0.0 || w2 < 0.0)
		jsal_error(JS_RANGE_ERROR, "Invalid weight(s) '%g'/'%g'", w1, w2);

	jsal_push_sphere_color(color_mix(color1, color2, w1, w2));
	return true;
}

static bool
js_CallDefaultMapScript(int num_args, bool is_ctor, intptr_t magic)
{
	int map_op;

	map_op = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (map_op < 0 || map_op >= MAP_SCRIPT_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid script type constant");
	map_call_default(map_op);
	return false;
}

static bool
js_CallDefaultPersonScript(int num_args, bool is_ctor, intptr_t magic)
{
	person_t*   actor = NULL;
	const char* name;
	person_t*   person;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "Invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		actor = person;
	person_call_default(person, type, actor);
	return false;
}

static bool
js_CallMapScript(int num_args, bool is_ctor, intptr_t magic)
{
	int type;

	type = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid script type constant");
	map_activate(type, false);
	return false;
}

static bool
js_CallPersonScript(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "Invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		person_activate(person, type, person, false);
	else
		person_activate(person, type, NULL, false);
	return false;
}

static bool
js_ChangeMap(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, "maps", true, false);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (!map_engine_change_map(filename))
		jsal_error(JS_ERROR, "Couldn't load map file '%s'", filename);
	return false;
}

static bool
js_ClearPersonCommands(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_clear_queue(person);
	return false;
}

static bool
js_CreateByteArray(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	int          size;

	size = jsal_to_int(0);

	if (size < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid byte array size");
	if (!(array = bytearray_new(size)))
		jsal_error(JS_ERROR, "Couldn't allocate byte array");
	jsal_push_sphere_bytearray(array);
	bytearray_unref(array);
	return true;
}

static bool
js_CreateByteArrayFromString(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	lstring_t*   string;

	string = jsal_require_lstring_t(0);

	if (!(array = bytearray_from_lstring(string)))
		jsal_error(JS_ERROR, "Couldn't allocate byte array");
	lstr_free(string);
	jsal_push_sphere_bytearray(array);
	bytearray_unref(array);
	return true;
}

static bool
js_CreateColor(int num_args, bool is_ctor, intptr_t magic)
{
	int red;
	int green;
	int blue;
	int alpha = 255;

	red = jsal_to_int(0);
	green = jsal_to_int(1);
	blue = jsal_to_int(2);
	if (num_args >= 4)
		alpha = jsal_to_int(3);

	// clamp components to 8-bit [0-255]
	red = red < 0 ? 0 : red > 255 ? 255 : red;
	green = green < 0 ? 0 : green > 255 ? 255 : green;
	blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;

	// construct a Color object
	jsal_push_sphere_color(mk_color(red, green, blue, alpha));
	return true;
}

static bool
js_CreateColorMatrix(int num_args, bool is_ctor, intptr_t magic)
{
	int rn = jsal_to_int(0);
	int rr = jsal_to_int(1);
	int rg = jsal_to_int(2);
	int rb = jsal_to_int(3);
	int gn = jsal_to_int(4);
	int gr = jsal_to_int(5);
	int gg = jsal_to_int(6);
	int gb = jsal_to_int(7);
	int bn = jsal_to_int(8);
	int br = jsal_to_int(9);
	int bg = jsal_to_int(10);
	int bb = jsal_to_int(11);

	// construct a ColorMatrix object
	jsal_push_class_obj(SV1_COLOR_MATRIX, NULL, false);
	jsal_push_int(rn); jsal_put_prop_string(-2, "rn");
	jsal_push_int(rr); jsal_put_prop_string(-2, "rr");
	jsal_push_int(rg); jsal_put_prop_string(-2, "rg");
	jsal_push_int(rb); jsal_put_prop_string(-2, "rb");
	jsal_push_int(gn); jsal_put_prop_string(-2, "gn");
	jsal_push_int(gr); jsal_put_prop_string(-2, "gr");
	jsal_push_int(gg); jsal_put_prop_string(-2, "gg");
	jsal_push_int(gb); jsal_put_prop_string(-2, "gb");
	jsal_push_int(bn); jsal_put_prop_string(-2, "bn");
	jsal_push_int(br); jsal_put_prop_string(-2, "br");
	jsal_push_int(bg); jsal_put_prop_string(-2, "bg");
	jsal_push_int(bb); jsal_put_prop_string(-2, "bb");
	return true;
}

static bool
js_CreateDirectory(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;

	name = jsal_require_pathname(0, "save", true, true);

	if (!game_mkdir(g_game, name))
		jsal_error(JS_ERROR, "Couldn't create directory '%s'", name);
	return false;
}

static bool
js_CreatePerson(int num_args, bool is_ctor, intptr_t magic)
{
	bool         destroy_with_map;
	const char*  filename;
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = jsal_require_string(0);
	destroy_with_map = jsal_to_boolean(2);

	if (jsal_is_class_obj(1, SV1_SPRITESET)) {
		// ref the spriteset so we can safely free it later. this avoids
		// having to check the argument type again.
		spriteset = jsal_require_sphere_spriteset(1);
	}
	else {
		filename = jsal_require_pathname(1, "spritesets", true, false);
		if (!(spriteset = spriteset_load(filename)))
			jsal_error(JS_ERROR, "Couldn't load spriteset file '%s'", filename);
	}

	// create the person and its JS-side data object
	person = person_new(name, spriteset, !destroy_with_map, NULL);
	spriteset_unref(spriteset);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	jsal_push_new_object(); jsal_put_prop_string(-2, name);
	jsal_pop(2);
	return false;
}

static bool
js_CreateSpriteset(int num_args, bool is_ctor, intptr_t magic)
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

	width = jsal_to_int(0);
	height = jsal_to_int(1);
	num_images = jsal_to_int(2);
	num_poses = jsal_to_int(3);
	num_frames = jsal_to_int(4);

	spriteset = spriteset_new();
	spriteset_set_base(spriteset, mk_rect(0, 0, width, height));
	image = image_new(width, height, NULL);
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
	jsal_push_sphere_spriteset(spriteset);
	spriteset_unref(spriteset);
	return true;
}

static bool
js_CreateStringFromByteArray(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	uint8_t*     buffer;
	size_t       size;

	array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);

	buffer = bytearray_buffer(array);
	size = bytearray_len(array);
	jsal_push_lstring((char*)buffer, size);
	return true;
}

static bool
js_CreateStringFromCode(int num_args, bool is_ctor, intptr_t magic)
{
	int code;

	code = jsal_to_int(0);

	if (code < 0 || code > 255)
		jsal_error(JS_RANGE_ERROR, "Invalid ASCII character code '%d'", code);

	jsal_push_sprintf("%c", code);
	return true;
}

static bool
js_CreateSurface(int num_args, bool is_ctor, intptr_t magic)
{
	color_t     fill_color;
	int         height;
	image_t*    image;
	int         width;

	width = jsal_to_int(0);
	height = jsal_to_int(1);
	fill_color = num_args >= 3 ? jsal_require_sphere_color(2) : mk_color(0, 0, 0, 0);

	if (!(image = image_new(width, height, NULL)))
		jsal_error(JS_ERROR, "Couldn't create GPU texture");
	image_fill(image, fill_color, 1.0f);
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_DeflateByteArray(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* input_array;
	int          level = 6;
	bytearray_t* new_array;

	input_array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
	if (num_args >= 2)
		level = jsal_to_int(1);

	if (level < 0 || level > 9)
		jsal_error(JS_RANGE_ERROR, "Invalid compression level '%d'", level);

	if (!(new_array = bytearray_deflate(input_array, level)))
		jsal_error(JS_ERROR, "Couldn't deflate byte array");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_DeflateFile(int num_args, bool is_ctor, intptr_t magic)
{
	void*       input_data;
	size_t      input_size;
	const char* in_filename;
	int         level = 6;
	const char* out_filename;
	void*       output_data;
	size_t      output_size;

	in_filename = jsal_require_pathname(0, "other", true, false);
	out_filename = jsal_require_pathname(1, "other", true, true);
	if (num_args >= 3)
		level = jsal_to_int(2);

	if (level < 0 || level > 9)
		jsal_error(JS_RANGE_ERROR, "Invalid compression level '%d'", level);

	if (!(input_data = game_read_file(g_game, in_filename, &input_size)))
		jsal_error(JS_ERROR, "Couldn't read file '%s'", in_filename);
	if (!(output_data = z_deflate(input_data, input_size, level, &output_size)))
		jsal_error(JS_ERROR, "Couldn't deflate file contents");
	if (!game_write_file(g_game, out_filename, output_data, output_size))
		jsal_error(JS_ERROR, "Couldn't write deflated file to disk");
	free(input_data);
	free(output_data);
	jsal_push_int(0);
	return true;
}

static bool
js_Delay(int num_args, bool is_ctor, intptr_t magic)
{
	double timeout;

	timeout = jsal_to_number(0);

	if (timeout < 0.0)
		jsal_error(JS_RANGE_ERROR, "Invalid delay timeout '%g'", timeout);
	sphere_sleep(floor(timeout) / 1000.0);
	return false;
}

static bool
js_DestroyPerson(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_free(person);
	return false;
}

static bool
js_DetachCamera(int num_args, bool is_ctor, intptr_t magic)
{
	map_engine_set_subject(NULL);
	return false;
}

static bool
js_DetachInput(int num_args, bool is_ctor, intptr_t magic)
{
	map_engine_set_player(PLAYER_1, NULL);
	return false;
}

static bool
js_DetachPlayerInput(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (num_args < 1) {
		jsal_error(JS_ERROR, "Missing first argument");
	}
	else if (jsal_is_string(0)) {
		name = jsal_get_string(0);
		if (!(person = map_person_by_name(name)))
			jsal_error(JS_REF_ERROR, "No such person '%s'", name);
		player = -1;
		for (i = PLAYER_1; i < PLAYER_MAX; ++i) {
			if (person == map_engine_get_player(i)) {
				player = i;
				break;
			}
		}
		if (player == -1)
			return false;
	}
	else if (jsal_is_number(0)) {
		player = jsal_get_int(0);
	}
	else {
		jsal_error(JS_ERROR, "First argument is not a number or string");
	}
	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid player constant");
	map_engine_set_player(player, NULL);
	return false;
}

static bool
js_DoEvents(int num_args, bool is_ctor, intptr_t magic)
{
	sphere_heartbeat(true, 1);
	dispatch_run(JOB_ON_TICK);
	jsal_push_boolean_true();
	return true;
}

static bool
js_DoesFileExist(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, true, false);

	jsal_push_boolean(game_file_exists(g_game, filename));
	return true;
}

static bool
js_DoesPersonExist(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;

	name = jsal_require_string(0);

	jsal_push_boolean(map_person_by_name(name) != NULL);
	return true;
}

static bool
js_EvaluateScript(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, "scripts", true, false);

	if (!game_file_exists(g_game, filename))
		jsal_error(JS_ERROR, "file '%s' not found", filename);
	if (!script_eval(filename))
		jsal_throw();
	return true;
}

static bool
js_EvaluateSystemScript(int num_args, bool is_ctor, intptr_t magic)
{
	char        path[SPHERE_PATH_MAX];
	const char* pathname;

	pathname = jsal_require_string(0);

	sprintf(path, "@/scripts/lib/%s", pathname);
	if (!game_file_exists(g_game, path))
		sprintf(path, "#/scripts/%s", pathname);
	if (!game_file_exists(g_game, path))
		jsal_error(JS_ERROR, "System script '%s' not found", pathname);
	if (!script_eval(path))
		jsal_throw();
	return true;
}

static bool
js_ExecuteGame(int num_args, bool is_ctor, intptr_t magic)
{
	path_t*     base_path;
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_string(0);

	// if the passed-in path is relative, resolve it relative to <engine>/games.
	// this is done for compatibility with Sphere 1.x.
	path = path_new(pathname);
	base_path = path_rebase(path_new("games/"), assets_path());
	path_rebase(path, base_path);
	path_free(base_path);

	sphere_change_game(path_cstr(path));
	path_free(path);
	return false;
}

static bool
js_ExecuteTrigger(int num_args, bool is_ctor, intptr_t magic)
{
	int index;
	int layer;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if ((index = map_trigger_at(x, y, layer)) >= 0)
		trigger_activate(index);
	return false;
}

static bool
js_ExecuteZoneScript(int num_args, bool is_ctor, intptr_t magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "Invalid zone index '%d'", zone_index);
	zone_activate(zone_index);
	return false;
}

static bool
js_ExecuteZones(int num_args, bool is_ctor, intptr_t magic)
{
	int index;
	int layer;
	int x;
	int y;

	int i;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	i = 0;
	while ((index = map_zone_at(x, y, layer, i++)) >= 0)
		zone_activate(index);
	return false;
}

static bool
js_Exit(int num_args, bool is_ctor, intptr_t magic)
{
	sphere_exit(true);
	return false;
}

static bool
js_ExitMapEngine(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	map_engine_exit();
	return false;
}

static bool
js_FilledCircle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	int             num_points;
	double          phi;
	float           radius;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	color = jsal_require_sphere_color(3);

	if (screen_skipping_frame(g_screen))
		return false;
	num_points = fmin(radius, 126);
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		phi = 2 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius;
		vertices[i + 1].y = y - sinf(phi) * radius;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius;
	vertices[i + 1].y = y - sinf(0.0f) * radius;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_FilledComplex(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "Not implemented");
	return false;
}

static bool
js_FilledEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	int             num_points;
	double          phi;
	float           radius_x;
	float           radius_y;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius_x = trunc(jsal_to_number(2));
	radius_y = trunc(jsal_to_number(3));
	color = jsal_require_sphere_color(4);

	if (screen_skipping_frame(g_screen))
		return false;
	num_points = ceil(fmin(10 * sqrt((radius_x + radius_y) / 2), 126));
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		phi = 2 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius_x;
		vertices[i + 1].y = y - sinf(phi) * radius_y;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius_x;
	vertices[i + 1].y = y - sinf(0.0f) * radius_y;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(color);
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_FlipScreen(int num_args, bool is_ctor, intptr_t magic)
{
	events_tick(1, true, s_frame_rate);
	return false;
}

static bool
js_FollowPerson(int num_args, bool is_ctor, intptr_t magic)
{
	int         distance = 0;
	person_t*   leader = NULL;
	const char* leader_name = "";
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	if (!jsal_is_null(1))
		leader_name = jsal_require_string(1);
	if (leader_name[0] != '\0')
		distance = jsal_require_int(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (!(leader_name[0] == '\0' || (leader = map_person_by_name(leader_name))))
		jsal_error(JS_REF_ERROR, "No such person '%s'", leader_name);
	if (distance <= 0 && leader_name[0] != '\0')
		jsal_error(JS_RANGE_ERROR, "Invalid distance '%d'", distance);
	if (!person_set_leader(person, leader, distance))
		jsal_error(JS_ERROR, "Illegal cycle in FollowPerson train");
	return false;
}

static bool
js_GarbageCollect(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_gc();
	return false;
}

static bool
js_GetActingPerson(int num_args, bool is_ctor, intptr_t magic)
{
	const person_t* person;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	person = map_engine_acting_person();
	if (person == NULL)
		jsal_error(JS_ERROR, "No current person activation or actor unknown");
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetCameraPerson(int num_args, bool is_ctor, intptr_t magic)
{
	person_t* subject;

	subject = map_engine_get_subject();
	if (subject == NULL)
		jsal_error(JS_ERROR, "Camera is not attached");
	jsal_push_string(person_name(subject));
	return true;
}

static bool
js_GetCameraX(int num_args, bool is_ctor, intptr_t magic)
{
	point2_t position;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	position = map_get_camera_xy();
	jsal_push_int(position.x);
	return true;
}

static bool
js_GetCameraY(int num_args, bool is_ctor, intptr_t magic)
{
	point2_t position;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	position = map_get_camera_xy();
	jsal_push_int(position.y);
	return true;
}

static bool
js_GetClippingRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t clip;

	clip = image_get_scissor(screen_backbuffer(g_screen));

	jsal_push_new_object();
	jsal_push_int(clip.x1);
	jsal_put_prop_string(-2, "x");
	jsal_push_int(clip.y1);
	jsal_put_prop_string(-2, "y");
	jsal_push_int(clip.x2 - clip.x1);
	jsal_put_prop_string(-2, "width");
	jsal_push_int(clip.y2 - clip.y1);
	jsal_put_prop_string(-2, "height");
	return true;
}

static bool
js_GetCurrentMap(int num_args, bool is_ctor, intptr_t magic)
{
	path_t* path;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	// GetCurrentMap() in Sphere 1.x returns the map path relative to the
	// 'maps' directory.
	path = game_relative_path(g_game, map_pathname(), "@/maps");
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_GetCurrentPerson(int num_args, bool is_ctor, intptr_t magic)
{
	const person_t* person;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	person = map_engine_active_person();
	if (person == NULL)
		jsal_error(JS_ERROR, "No current person activation");
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetCurrentTrigger(int num_args, bool is_ctor, intptr_t magic)
{
	int trigger;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	trigger = map_engine_active_trigger();
	if (trigger == -1)
		jsal_error(JS_ERROR, "No current trigger activation");
	jsal_push_int(trigger);
	return true;
}

static bool
js_GetCurrentZone(int num_args, bool is_ctor, intptr_t magic)
{
	int zone;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	zone = map_engine_active_zone();
	if (zone == -1)
		jsal_error(JS_ERROR, "No current zone activation");
	jsal_push_int(zone);
	return true;
}

static bool
js_GetDirectoryList(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t*  dir;
	const char*   dir_name = "@/";
	const path_t* entry;
	int           index = 0;

	if (num_args >= 1)
		dir_name = jsal_require_pathname(0, NULL, true, false);

	jsal_push_new_array();
	if (!(dir = directory_open(g_game, dir_name, false)))
		return true;
	while ((entry = directory_next(dir))) {
		if (path_is_file(entry))
			continue;
		jsal_push_string(path_hop(entry, path_num_hops(entry) - 1));
		jsal_put_prop_index(-2, index++);
	}
	directory_close(dir);
	return true;
}

static bool
js_GetFileList(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t*  dir;
	const char*   dir_name = "@/save";
	const path_t* entry;
	int           index = 0;

	if (num_args >= 1)
		dir_name = jsal_require_pathname(0, NULL, true, false);

	jsal_push_new_array();
	if (!(dir = directory_open(g_game, dir_name, false)))
		return true;
	while ((entry = directory_next(dir))) {
		if (!path_is_file(entry))
			continue;
		jsal_push_string(path_filename(entry));
		jsal_put_prop_index(-2, index++);
	}
	directory_close(dir);
	return true;
}

static bool
js_GetFrameRate(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(s_frame_rate);
	return true;
}

static bool
js_GetGameList(int num_args, bool is_ctor, intptr_t magic)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_FS_ENTRY* fse;
	game_t*           game;
	path_t*           path = NULL;
	path_t*           paths[2];

	int i, j = 0;

	// build search paths
	paths[0] = path_rebase(path_new("games/"), engine_path());
	paths[1] = path_rebase(path_new("Sphere Games/"), home_path());

	// search for supported games
	jsal_push_new_array();
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while ((file_info = al_read_directory(fse))) {
				path = path_new(al_get_fs_entry_name(file_info));
				if ((game = game_open(path_cstr(path)))) {
					jsal_push_ref_weak(game_manifest(game));
					jsal_push_string(path_cstr(path));
					jsal_put_prop_string(-2, "directory");
					jsal_put_prop_index(-2, j++);
					game_unref(game);
				}
				path_free(path);
			}
		}
		al_destroy_fs_entry(fse);
		path_free(paths[i]);
	}
	return true;
}

static bool
js_GetInputPerson(int num_args, bool is_ctor, intptr_t magic)
{
	person_t* person;
	int       player = PLAYER_1;

	if (num_args >= 1)
		player = jsal_to_int(0);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid player constant");
	person = map_engine_get_player(player);
	if (person == NULL)
		jsal_error(JS_ERROR, "Input is not attached for Player %d", player + 1);
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetJoystickAxis(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index = jsal_to_int(0);
	int axis_index = jsal_to_int(1);

	jsal_push_number(joy_position(joy_index, axis_index));
	return true;
}

static bool
js_GetKey(int num_args, bool is_ctor, intptr_t magic)
{
	while (kb_queue_len() == 0)
		sphere_sleep(0.05);
	jsal_push_int(kb_get_key());
	return true;
}

static bool
js_GetKeyString(int num_args, bool is_ctor, intptr_t magic)
{
	int  keycode;
	bool shift = false;

	keycode = jsal_to_int(0);
	if (num_args >= 2)
		shift = jsal_to_boolean(1);

	switch (keycode) {
	case ALLEGRO_KEY_A: jsal_push_string(shift ? "A" : "a"); break;
	case ALLEGRO_KEY_B: jsal_push_string(shift ? "B" : "b"); break;
	case ALLEGRO_KEY_C: jsal_push_string(shift ? "C" : "c"); break;
	case ALLEGRO_KEY_D: jsal_push_string(shift ? "D" : "d"); break;
	case ALLEGRO_KEY_E: jsal_push_string(shift ? "E" : "e"); break;
	case ALLEGRO_KEY_F: jsal_push_string(shift ? "F" : "f"); break;
	case ALLEGRO_KEY_G: jsal_push_string(shift ? "G" : "g"); break;
	case ALLEGRO_KEY_H: jsal_push_string(shift ? "H" : "h"); break;
	case ALLEGRO_KEY_I: jsal_push_string(shift ? "I" : "i"); break;
	case ALLEGRO_KEY_J: jsal_push_string(shift ? "J" : "j"); break;
	case ALLEGRO_KEY_K: jsal_push_string(shift ? "K" : "k"); break;
	case ALLEGRO_KEY_L: jsal_push_string(shift ? "L" : "l"); break;
	case ALLEGRO_KEY_M: jsal_push_string(shift ? "M" : "m"); break;
	case ALLEGRO_KEY_N: jsal_push_string(shift ? "N" : "n"); break;
	case ALLEGRO_KEY_O: jsal_push_string(shift ? "O" : "o"); break;
	case ALLEGRO_KEY_P: jsal_push_string(shift ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: jsal_push_string(shift ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: jsal_push_string(shift ? "R" : "r"); break;
	case ALLEGRO_KEY_S: jsal_push_string(shift ? "S" : "s"); break;
	case ALLEGRO_KEY_T: jsal_push_string(shift ? "T" : "t"); break;
	case ALLEGRO_KEY_U: jsal_push_string(shift ? "U" : "u"); break;
	case ALLEGRO_KEY_V: jsal_push_string(shift ? "V" : "v"); break;
	case ALLEGRO_KEY_W: jsal_push_string(shift ? "W" : "w"); break;
	case ALLEGRO_KEY_X: jsal_push_string(shift ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: jsal_push_string(shift ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: jsal_push_string(shift ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: jsal_push_string(shift ? "!" : "1"); break;
	case ALLEGRO_KEY_2: jsal_push_string(shift ? "@" : "2"); break;
	case ALLEGRO_KEY_3: jsal_push_string(shift ? "#" : "3"); break;
	case ALLEGRO_KEY_4: jsal_push_string(shift ? "$" : "4"); break;
	case ALLEGRO_KEY_5: jsal_push_string(shift ? "%" : "5"); break;
	case ALLEGRO_KEY_6: jsal_push_string(shift ? "^" : "6"); break;
	case ALLEGRO_KEY_7: jsal_push_string(shift ? "&" : "7"); break;
	case ALLEGRO_KEY_8: jsal_push_string(shift ? "*" : "8"); break;
	case ALLEGRO_KEY_9: jsal_push_string(shift ? "(" : "9"); break;
	case ALLEGRO_KEY_0: jsal_push_string(shift ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: jsal_push_string(shift ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: jsal_push_string(shift ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: jsal_push_string(shift ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: jsal_push_string(shift ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: jsal_push_string(shift ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: jsal_push_string(shift ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: jsal_push_string(shift ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: jsal_push_string(shift ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: jsal_push_string(shift ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: jsal_push_string(shift ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: jsal_push_string(" "); break;
	case ALLEGRO_KEY_TAB: jsal_push_string("\t"); break;
	case ALLEGRO_KEY_TILDE: jsal_push_string(shift ? "~" : "`"); break;
	default:
		jsal_push_string("");
	}
	return true;
}

static bool
js_GetLayerAngle(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "Not implemented");
	return false;
}

static bool
js_GetLayerHeight(int num_args, bool is_ctor, intptr_t magic)
{
	int     layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_int(layer_size(layer).height);
	return true;
}

static bool
js_GetLayerMask(int num_args, bool is_ctor, intptr_t magic)
{
	int layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_sphere_color(layer_get_color_mask(layer));
	return true;
}

static bool
js_GetLayerName(int num_args, bool is_ctor, intptr_t magic)
{
	int layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_string(layer_name(layer));
	return true;
}

static bool
js_GetLayerWidth(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_int(layer_size(layer).width);
	return true;
}

static bool
js_GetLocalAddress(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("127.0.0.1");
	return true;
}

static bool
js_GetLocalName(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("localhost");
	return true;
}

static bool
js_GetMapEngine(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "Not implemented");
	return true;
}

static bool
js_GetMapEngineFrameRate(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(map_engine_get_framerate());
	return true;
}

static bool
js_GetMouseWheelEvent(int num_args, bool is_ctor, intptr_t magic)
{
	mouse_event_t event;

	do {
		while (mouse_queue_len() == 0)
			sphere_sleep(0.05);
		event = mouse_get_event();
	} while (event.key != MOUSE_KEY_WHEEL_UP && event.key != MOUSE_KEY_WHEEL_DOWN);
	jsal_push_int(event.key);
	return true;
}

static bool
js_GetMouseX(int num_args, bool is_ctor, intptr_t magic)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(x);
	return true;
}

static bool
js_GetMouseY(int num_args, bool is_ctor, intptr_t magic)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(y);
	return true;
}

static bool
js_GetNextAnimatedTile(int num_args, bool is_ctor, intptr_t magic)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	jsal_push_int(tileset_get_next(tileset, tile_index));
	return true;
}

static bool
js_GetNumJoysticks(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(joy_num_devices());
	return true;
}

static bool
js_GetNumJoystickAxes(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index;

	joy_index = jsal_to_int(0);

	jsal_push_int(joy_num_axes(joy_index));
	return true;
}

static bool
js_GetNumJoystickButtons(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index;

	joy_index = jsal_to_int(0);

	jsal_push_int(joy_num_buttons(joy_index));
	return true;
}

static bool
js_GetNumLayers(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_int(map_num_layers());
	return true;
}

static bool
js_GetNumMouseWheelEvents(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(mouse_queue_len());
	return true;
}

static bool
js_GetNumTiles(int num_args, bool is_ctor, intptr_t magic)
{
	tileset_t* tileset;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	tileset = map_tileset();
	jsal_push_int(tileset_len(tileset));
	return true;
}

static bool
js_GetNumTriggers(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	jsal_push_number(map_num_triggers());
	return true;
}

static bool
js_GetNumZones(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	jsal_push_number(map_num_zones());
	return true;
}

static bool
js_GetObstructingPerson(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   obs_person;
	person_t*   person;
	int         x;
	int         y;

	name = jsal_require_string(0);
	x = jsal_require_int(1);
	y = jsal_require_int(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_obstructed_at(person, x, y, &obs_person, NULL);
	jsal_push_string(obs_person != NULL ? person_name(obs_person) : "");
	return true;
}

static bool
js_GetObstructingTile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	int         tile_index;
	int         x;
	int         y;

	name = jsal_require_string(0);
	x = jsal_require_int(1);
	y = jsal_require_int(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_obstructed_at(person, x, y, NULL, &tile_index);
	jsal_push_int(tile_index);
	return true;
}

static bool
js_GetPersonAngle(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_number(person_get_angle(person));
	return true;
}

static bool
js_GetPersonBase(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t      base;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	base = spriteset_get_base(person_get_spriteset(person));
	jsal_push_new_object();
	jsal_push_int(base.x1); jsal_put_prop_string(-2, "x1");
	jsal_push_int(base.y1); jsal_put_prop_string(-2, "y1");
	jsal_push_int(base.x2); jsal_put_prop_string(-2, "x2");
	jsal_push_int(base.y2); jsal_put_prop_string(-2, "y2");
	return true;
}

static bool
js_GetPersonData(int num_args, bool is_ctor, intptr_t magic)
{
	int          height;
	person_t*    leader;
	const char*  name;
	int          num_directions;
	int          num_frames;
	person_t*    person;
	spriteset_t* spriteset;
	int          width;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	leader = person_get_leader(person);
	spriteset = person_get_spriteset(person);
	width = spriteset_width(spriteset);
	height = spriteset_height(spriteset);
	num_directions = spriteset_num_poses(spriteset);
	num_frames = spriteset_num_frames(spriteset, person_get_pose(person));

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	jsal_get_prop_string(-1, name);
	jsal_push_int(num_frames);
	jsal_put_prop_string(-2, "num_frames");
	jsal_push_int(num_directions);
	jsal_put_prop_string(-2, "num_directions");
	jsal_push_int(width);
	jsal_put_prop_string(-2, "width");
	jsal_push_int(height);
	jsal_put_prop_string(-2, "height");
	jsal_push_string(person_name(leader));
	jsal_put_prop_string(-2, "leader");
	return true;
}

static bool
js_GetPersonDirection(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_string(person_get_pose(person));
	return true;
}

static bool
js_GetPersonFollowDistance(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (person_get_leader(person) == NULL)
		jsal_error(JS_TYPE_ERROR, "not a follower");
	jsal_push_int(person_get_trailing(person));
	return true;
}

static bool
js_GetPersonFollowers(int num_args, bool is_ctor, intptr_t magic)
{
	vector_t*     all_persons;
	person_t*     candidate;
	int           index = 0;
	const char*   name;
	person_t*     person;

	iter_t iter;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	all_persons = map_engine_persons();
	jsal_push_new_array();
	iter = vector_enum(all_persons);
	while (iter_next(&iter)) {
		candidate = *(person_t**)iter.ptr;
		if (person_get_leader(candidate) == person) {
			jsal_push_string(person_name(candidate));
			jsal_put_prop_index(-2, index++);
		}
	}
	return true;
}

static bool
js_GetPersonFrame(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_int(person_get_frame(person));
	return true;
}

static bool
js_GetPersonFrameNext(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_int(person_get_frame_delay(person));
	return true;
}

static bool
js_GetPersonFrameRevert(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_int(person_get_revert_delay(person));
	return true;
}

static bool
js_GetPersonIgnoreList(int num_args, bool is_ctor, intptr_t magic)
{
	vector_t*   ignore_list;
	const char* name;
	person_t*   person;

	iter_t iter;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	ignore_list = person_ignore_list(person);
	iter = vector_enum(ignore_list);
	jsal_push_new_array();
	while (iter_next(&iter)) {
		jsal_push_string(*(const char**)iter.ptr);
		jsal_put_prop_index(-2, iter.index);
	}
	return true;
}

static bool
js_GetPersonLayer(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_int(person_get_layer(person));
	return true;
}

static bool
js_GetPersonLeader(int num_args, bool is_ctor, intptr_t magic)
{
	person_t*   leader;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	leader = person_get_leader(person);
	jsal_push_string(person_name(leader));
	return true;
}

static bool
js_GetPersonList(int num_args, bool is_ctor, intptr_t magic)
{
	vector_t* all_persons;
	person_t* person;

	iter_t iter;

	all_persons = map_engine_persons();
	iter = vector_enum(all_persons);
	jsal_push_new_array();
	while (iter_next(&iter)) {
		person = *(person_t**)iter.ptr;
		jsal_push_string(person_name(person));
		jsal_put_prop_index(-2, iter.index);
	}
	return true;
}

static bool
js_GetPersonMask(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_sphere_color(person_get_color(person));
	return true;
}

static bool
js_GetPersonOffsetX(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	offset = person_get_offset(person);
	jsal_push_int(offset.x);
	return true;
}

static bool
js_GetPersonOffsetY(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	offset = person_get_offset(person);
	jsal_push_int(offset.y);
	return true;
}

static bool
js_GetPersonSpriteset(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  name;
	spriteset_t* spriteset;
	person_t*    person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	spriteset = person_get_spriteset(person);
	jsal_push_sphere_spriteset(spriteset);
	return true;
}

static bool
js_GetPersonSpeedX(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x_speed;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_speed(person, &x_speed, NULL);
	jsal_push_number(x_speed);
	return true;
}

static bool
js_GetPersonSpeedY(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      y_speed;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_speed(person, NULL, &y_speed);
	jsal_push_number(y_speed);
	return true;
}

static bool
js_GetPersonValue(int num_args, bool is_ctor, intptr_t magic)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	key = jsal_to_string(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	if (!jsal_get_prop_string(-1, name)) {
		jsal_pop(1);
		jsal_push_new_object();
		jsal_put_prop_string(-2, name);
		jsal_get_prop_string(-1, name);
	}
	jsal_get_prop_string(-1, key);
	return true;
}

static bool
js_GetPersonX(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_int(x);
	return true;
}

static bool
js_GetPersonXFloat(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_number(x);
	return true;
}

static bool
js_GetPersonY(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_int(y);
	return true;
}

static bool
js_GetPersonYFloat(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_number(y);
	return true;
}

static bool
js_GetPlayerKey(int num_args, bool is_ctor, intptr_t magic)
{
	int player;
	int key_type;

	player = jsal_to_int(0);
	key_type = jsal_to_int(1);

	if (player < 0 || player >= 4)
		jsal_error(JS_RANGE_ERROR, "player index out of range");
	if (key_type < 0 || key_type >= PLAYER_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid key type constant");
	jsal_push_int(get_player_key(player, key_type));
	return true;
}

static bool
js_GetScreenHeight(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(screen_size(g_screen).height);
	return true;
}

static bool
js_GetScreenWidth(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(screen_size(g_screen).width);
	return true;
}

static bool
js_GetSystemArrow(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	if (!(image = game_default_arrow(g_game)))
		jsal_error(JS_REF_ERROR, "missing system arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemDownArrow(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	if (!(image = game_default_arrow_down(g_game)))
		jsal_error(JS_REF_ERROR, "missing system down arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemFont(int num_args, bool is_ctor, intptr_t magic)
{
	if (s_default_font == NULL)
		jsal_error(JS_REF_ERROR, "No default system font available");
	jsal_push_sphere_font(s_default_font);
	return true;
}

static bool
js_GetSystemUpArrow(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	if (!(image = game_default_arrow_up(g_game)))
		jsal_error(JS_REF_ERROR, "missing system up arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemWindowStyle(int num_args, bool is_ctor, intptr_t magic)
{
	windowstyle_t* windowstyle;

	if (!(windowstyle = game_default_windowstyle(g_game)))
		jsal_error(JS_REF_ERROR, "missing system windowstyle");
	jsal_push_sphere_windowstyle(windowstyle);
	return true;
}

static bool
js_GetTalkActivationButton(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(map_engine_get_talk_button());
	return true;
}

static bool
js_GetTalkActivationKey(int num_args, bool is_ctor, intptr_t magic)
{
	int player = PLAYER_1;

	if (num_args >= 1)
		player = jsal_to_int(0);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");
	jsal_push_int(map_engine_get_talk_key(player));
	return true;
}

static bool
js_GetTalkDistance(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(map_engine_get_talk_distance());
	return true;
}

static bool
js_GetTile(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_int(map_tile_at(x, y, layer));
	return true;
}

static bool
js_GetTileDelay(int num_args, bool is_ctor, intptr_t magic)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	jsal_push_int(tileset_get_delay(tileset, tile_index));
	return true;
}

static bool
js_GetTileHeight(int num_args, bool is_ctor, intptr_t magic)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	jsal_push_int(height);
	return true;
}

static bool
js_GetTileImage(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (!(image = image_dup(tileset_get_image(tileset, tile_index))))
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_IMAGE, image, false);
	return true;
}

static bool
js_GetTileName(int num_args, bool is_ctor, intptr_t magic)
{
	int              tile_index;
	const lstring_t* tile_name;
	tileset_t*       tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tile_name = tileset_get_name(tileset, tile_index);
	jsal_push_lstring_t(tile_name);
	return true;
}

static bool
js_GetTileSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (!(image = image_dup(tileset_get_image(tileset, tile_index))))
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_GetTileWidth(int num_args, bool is_ctor, intptr_t magic)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	jsal_push_int(width);
	return true;
}

static bool
js_GetTime(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_number(floor(al_get_time() * 1000));
	return true;
}

static bool
js_GetToggleState(int num_args, bool is_ctor, intptr_t magic)
{
	int keycode;

	keycode = jsal_to_int(0);

	if (keycode != ALLEGRO_KEY_CAPSLOCK
		&& keycode != ALLEGRO_KEY_NUMLOCK
		&& keycode != ALLEGRO_KEY_SCROLLLOCK)
	{
		jsal_error(JS_RANGE_ERROR, "invalid toggle key constant");
	}

	jsal_push_boolean(kb_is_toggled(keycode));
	return true;
}

static bool
js_GetTriggerLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, NULL, &layer);
	jsal_push_int(layer);
	return true;
}

static bool
js_GetTriggerX(int num_args, bool is_ctor, intptr_t magic)
{
	int trigger_index;
	int x;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, &x, NULL, NULL);
	jsal_push_int(x);
	return true;
}

static bool
js_GetTriggerY(int num_args, bool is_ctor, intptr_t magic)
{
	int trigger_index;
	int y;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, &y, NULL);
	jsal_push_int(y);
	return true;
}

static bool
js_GetVersion(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_number(API_VERSION);
	return true;
}

static bool
js_GetVersionString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string(API_VERSION_STRING);
	return true;
}

static bool
js_GetZoneHeight(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.y2 - bounds.y1);
	return true;
}

static bool
js_GetZoneLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	jsal_push_int(zone_get_layer(zone_index));
	return true;
}

static bool
js_GetZoneSteps(int num_args, bool is_ctor, intptr_t magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	jsal_push_int(zone_get_steps(zone_index));
	return true;
}

static bool
js_GetZoneWidth(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.x2 - bounds.x1);
	return true;
}

static bool
js_GetZoneX(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.x1);
	return true;
}

static bool
js_GetZoneY(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.y1);
	return true;
}

static bool
js_GrabImage(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	int      height;
	int      width;
	int      x;
	int      y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	if (!(image = screen_grab(g_screen, x, y, width, height)))
		jsal_error(JS_ERROR, "couldn't grab screen image");
	jsal_push_class_obj(SV1_IMAGE, image, false);
	return true;
}

static bool
js_GrabSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	int      height;
	int      width;
	int      x;
	int      y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	if (!(image = screen_grab(g_screen, x, y, width, height)))
		jsal_error(JS_ERROR, "couldn't grab screen image");
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_GradientCircle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         inner_color;
	int             num_points;
	color_t         outer_color;
	double          phi;
	float           radius;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	inner_color = jsal_require_sphere_color(3);
	outer_color = jsal_require_sphere_color(4);

	if (screen_skipping_frame(g_screen))
		return false;
	num_points = fmin(radius, 126);
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(inner_color);
	for (i = 0; i < num_points; ++i) {
		phi = 2.0 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius;
		vertices[i + 1].y = y - sinf(phi) * radius;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(outer_color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius;
	vertices[i + 1].y = y - sinf(0.0f) * radius;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(outer_color);
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_GradientComplex(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_GradientEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         inner_color;
	int             num_points;
	color_t         outer_color;
	double          phi;
	float           radius_x;
	float           radius_y;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius_x = trunc(jsal_to_number(2));
	radius_y = trunc(jsal_to_number(3));
	inner_color = jsal_require_sphere_color(4);
	outer_color = jsal_require_sphere_color(5);

	if (screen_skipping_frame(g_screen))
		return false;
	num_points = ceil(fmin(10 * sqrt((radius_x + radius_y) / 2), 126));
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(inner_color);
	for (i = 0; i < num_points; ++i) {
		phi = 2 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius_x;
		vertices[i + 1].y = y - sinf(phi) * radius_y;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(outer_color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius_x;
	vertices[i + 1].y = y - sinf(0.0f) * radius_y;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(outer_color);
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_GradientLine(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color1;
	color_t color2;
	float   length;
	float   tx, ty;
	float   x1;
	float   x2;
	float   y1;
	float   y2;

	x1 = trunc(jsal_to_number(0)) + 0.5;
	y1 = trunc(jsal_to_number(1)) + 0.5;
	x2 = trunc(jsal_to_number(2)) + 0.5;
	y2 = trunc(jsal_to_number(3)) + 0.5;
	color1 = jsal_require_sphere_color(4);
	color2 = jsal_require_sphere_color(5);

	if (screen_skipping_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_GradientRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color_ul;
	color_t color_ur;
	color_t color_lr;
	color_t color_ll;
	float   x1, x2;
	float   y1, y2;

	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = x1 + trunc(jsal_to_number(2));
	y2 = y1 + trunc(jsal_to_number(3));
	color_ul = jsal_require_sphere_color(4);
	color_ur = jsal_require_sphere_color(5);
	color_lr = jsal_require_sphere_color(6);
	color_ll = jsal_require_sphere_color(7);

	if (screen_skipping_frame(g_screen))
		return false;
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	galileo_reset();
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_GradientTriangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color1, color2, color3;
	float   x1, x2, x3;
	float   y1, y2, y3;

	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = trunc(jsal_to_number(2));
	y2 = trunc(jsal_to_number(3));
	x3 = trunc(jsal_to_number(4));
	y3 = trunc(jsal_to_number(5));
	color1 = jsal_require_sphere_color(6);
	color2 = jsal_require_sphere_color(7);
	color3 = jsal_require_sphere_color(8);

	if (screen_skipping_frame(g_screen))
		return false;
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color1) },
		{ x2, y2, 0, 0, 0, nativecolor(color2) },
		{ x3, y3, 0, 0, 0, nativecolor(color3) },
	};
	galileo_reset();
	al_draw_prim(verts, NULL, NULL, 0, 3, ALLEGRO_PRIM_TRIANGLE_LIST);
	return false;
}

static bool
js_HashByteArray(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	void*        data;
	size_t       size;

	array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);

	data = bytearray_buffer(array);
	size = bytearray_len(array);
	jsal_push_string(md5sum(data, size));
	return true;
}

static bool
js_HashFromFile(int num_args, bool is_ctor, intptr_t magic)
{
	void*       data;
	size_t      file_size;
	const char* filename;

	filename = jsal_require_pathname(0, "other", true, false);

	if (!(data = game_read_file(g_game, filename, &file_size)))
		jsal_error(JS_ERROR, "couldn't read file");
	jsal_push_string(md5sum(data, file_size));
	return true;
}

static bool
js_IgnorePersonObstructions(int num_args, bool is_ctor, intptr_t magic)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	ignoring = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_ignore_persons(person, ignoring);
	return false;
}

static bool
js_IgnoreTileObstructions(int num_args, bool is_ctor, intptr_t magic)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	ignoring = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_ignore_tiles(person, ignoring);
	return false;
}

static bool
js_InflateByteArray(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t*  array;
	int           max_size = 0;
	bytearray_t*  new_array;

	array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
	if (num_args >= 2)
		max_size = jsal_to_int(1);

	if (max_size < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid maximum size '%d'", max_size);
	if (!(new_array = bytearray_inflate(array, max_size)))
		jsal_error(JS_ERROR, "Couldn't inflate byte array");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_InflateFile(int num_args, bool is_ctor, intptr_t magic)
{
	void*       input_data;
	size_t      input_size;
	const char* in_filename;
	int         level = 6;
	const char* out_filename;
	void*       output_data;
	size_t      output_size;

	in_filename = jsal_require_pathname(0, "other", true, false);
	out_filename = jsal_require_pathname(1, "other", true, true);

	if (!(input_data = game_read_file(g_game, in_filename, &input_size)))
		jsal_error(JS_ERROR, "Couldn't read file '%s'", in_filename);
	if (!(output_data = z_inflate(input_data, input_size, 0, &output_size)))
		jsal_error(JS_ERROR, "Couldn't inflate file contents");
	if (!game_write_file(g_game, out_filename, output_data, output_size))
		jsal_error(JS_ERROR, "Couldn't write inflated file to disk");
	free(input_data);
	free(output_data);
	jsal_push_int(0);
	return true;
}

static bool
js_IsAnyKeyPressed(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_boolean(kb_is_any_key_down());
	return true;
}

static bool
js_IsCameraAttached(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_boolean(map_engine_get_subject() != NULL);
	return true;
}

static bool
js_IsCommandQueueEmpty(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_boolean(!person_moving(person));
	return true;
}

static bool
js_IsIgnoringPersonObstructions(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_boolean(person_get_ignore_persons(person));
	return true;
}

static bool
js_IsIgnoringTileObstructions(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_boolean(person_get_ignore_tiles(person));
	return true;
}

static bool
js_IsInputAttached(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (jsal_get_top() < 1) {
		player = PLAYER_1;
	}
	else if (jsal_is_string(0)) {
		name = jsal_get_string(0);
		if (!(person = map_person_by_name(name)))
			jsal_error(JS_REF_ERROR, "No such person '%s'", name);
		player = -1;
		for (i = PLAYER_1; i < PLAYER_MAX; ++i) {
			if (person == map_engine_get_player(i)) {
				player = i;
				break;
			}
		}
		if (player == -1) {
			jsal_push_boolean_false();
			return true;
		}
	}
	else if (jsal_is_number(0)) {
		player = jsal_get_int(0);
	}
	else {
		jsal_error(JS_TYPE_ERROR, "not a number or string");
	}
	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");
	jsal_push_boolean(map_engine_get_player(player) != NULL);
	return true;
}

static bool
js_IsJoystickButtonPressed(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index = jsal_to_int(0);
	int button = jsal_to_int(1);

	jsal_push_boolean(joy_is_button_down(joy_index, button));
	return true;
}

static bool
js_IsKeyPressed(int num_args, bool is_ctor, intptr_t magic)
{
	int keycode = jsal_to_int(0);

	jsal_push_boolean(kb_is_key_down(keycode));
	return true;
}

static bool
js_IsLayerReflective(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_boolean(layer_get_reflective(layer));
	return true;
}

static bool
js_IsLayerVisible(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	jsal_push_boolean(layer_get_visible(layer));
	return true;
}

static bool
js_IsMapEngineRunning(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_boolean(map_engine_running());
	return true;
}

static bool
js_IsMouseButtonPressed(int num_args, bool is_ctor, intptr_t magic)
{
	int                 button;
	int                 button_id;
	ALLEGRO_DISPLAY*    display;
	ALLEGRO_MOUSE_STATE mouse_state;

	button = jsal_to_int(0);
	button_id = button == MOUSE_BUTTON_RIGHT ? 2
		: button == MOUSE_BUTTON_MIDDLE ? 3
		: 1;
	al_get_mouse_state(&mouse_state);
	display = screen_display(g_screen);
	jsal_push_boolean(mouse_state.display == display && al_mouse_button_down(&mouse_state, button_id));
	return true;
}

static bool
js_IsPersonObstructed(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	int         x;
	int         y;

	name = jsal_require_string(0);
	x = jsal_require_int(1);
	y = jsal_require_int(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_boolean(person_obstructed_at(person, x, y, NULL, NULL));
	return true;
}

static bool
js_IsPersonVisible(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_boolean(person_get_visible(person));	return true;
}

static bool
js_IsTriggerAt(int num_args, bool is_ctor, intptr_t magic)
{
	int x;
	int y;
	int layer;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	jsal_push_boolean(map_trigger_at(x, y, layer) >= 0);
	return true;
}

static bool
js_Line(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	float   x1, x2;
	float   y1, y2;

	x1 = trunc(jsal_to_number(0)) + 0.5;
	y1 = trunc(jsal_to_number(1)) + 0.5;
	x2 = trunc(jsal_to_number(2)) + 0.5;
	y2 = trunc(jsal_to_number(3)) + 0.5;
	color = jsal_require_sphere_color(4);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return false;
}

static bool
js_LineIntersects(int num_args, bool is_ctor, intptr_t magic)
{
	bool overlapping;
	int  x1, x2, x3, x4;
	int  y1, y2, y3, y4;

	jsal_require_object(0);
	jsal_require_object(1);
	jsal_require_object(2);
	jsal_require_object(3);

	jsal_get_prop_string(0, "x");
	jsal_get_prop_string(0, "y");
	x1 = jsal_require_int(-2);
	y1 = jsal_require_int(-1);
	jsal_get_prop_string(1, "x");
	jsal_get_prop_string(1, "y");
	x2 = jsal_require_int(-2);
	y2 = jsal_require_int(-1);
	jsal_get_prop_string(2, "x");
	jsal_get_prop_string(2, "y");
	x3 = jsal_require_int(-2);
	y3 = jsal_require_int(-1);
	jsal_get_prop_string(3, "x");
	jsal_get_prop_string(3, "y");
	x4 = jsal_require_int(-2);
	y4 = jsal_require_int(-1);
	overlapping = do_lines_overlap(mk_rect(x1, y1, x2, y2), mk_rect(x3, y3, x4, y4));
	jsal_push_boolean(overlapping);
	return true;
}

static bool
js_LineSeries(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	int             num_points;
	int             type = LINE_MULTIPLE;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;
	float           x;
	float           y;

	int i;

	jsal_require_array(0);
	color = jsal_require_sphere_color(1);
	if (num_args >= 3)
		type = jsal_to_int(2);

	if ((num_points = jsal_get_length(0)) < 2)
		jsal_error(JS_RANGE_ERROR, "two or more vertices required");
	vertices = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x");
		jsal_get_prop_string(-2, "y");
		x = trunc(jsal_to_number(-2));
		y = trunc(jsal_to_number(-1));
		jsal_pop(3);
		vertices[i].x = x + 0.5f;
		vertices[i].y = y + 0.5f;
		vertices[i].z = 0.0f;
		vertices[i].u = 0.0f;
		vertices[i].v = 0.0f;
		vertices[i].color = vtx_color;
	}
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
			: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
			: ALLEGRO_PRIM_LINE_LIST);
	return false;
}

static bool
js_ListenOnPort(int num_args, bool is_ctor, intptr_t magic)
{
	int          port;
	socket_v1_t* socket;

	port = jsal_to_int(0);

	if ((socket = socket_v1_new_server(port)))
		jsal_push_class_obj(SV1_SOCKET, socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_LoadAnimation(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;
	const char*  filename;

	filename = jsal_require_pathname(0, "animations", true, false);
	
	if (!(anim = animation_new(filename)))
		jsal_error(JS_ERROR, "couldn't load animation '%s'", filename);
	jsal_push_class_obj(SV1_ANIMATION, anim, false);
	return true;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_LoadFont(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	font_t*     font;

	filename = jsal_require_pathname(0, "fonts", true, false);
	if (!(font = font_load(filename)))
		jsal_error(JS_ERROR, "couldn't load font '%s'", filename);
	jsal_push_sphere_font(font);
	font_unref(font);
	return true;
}

static bool
js_LoadImage(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	image_t*    image;

	filename = jsal_require_pathname(0, "images", true, false);
	if (!(image = image_load(filename)))
		jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	jsal_push_class_obj(SV1_IMAGE, image, false);
	return true;
}

static bool
js_LoadSound(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	sound_t*    sound;

	filename = jsal_require_pathname(0, "sounds", true, false);

	if (!(sound = sound_new(filename)))
		jsal_error(JS_ERROR, "Couldn't load sound '%s'", filename);
	jsal_push_class_obj(SV1_SOUND, sound, false);
	return true;
}

static bool
js_LoadSoundEffect(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	int         mode;
	sample_t*   sample;

	filename = jsal_require_pathname(0, "sounds", true, false);
	mode = num_args >= 2 ? jsal_to_int(1) : SE_SINGLE;

	if (!(sample = sample_new(filename, mode == SE_MULTIPLE)))
		jsal_error(JS_ERROR, "Couldn't load sound effect '%s'", filename);
	jsal_push_class_obj(SV1_SOUND_EFFECT, sample, false);
	return true;
}

static bool
js_LoadSpriteset(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  filename;
	spriteset_t* spriteset;

	filename = jsal_require_pathname(0, "spritesets", true, false);
	if ((spriteset = spriteset_load(filename)) == NULL)
		jsal_error(JS_ERROR, "couldn't load spriteset '%s'", filename);
	jsal_push_sphere_spriteset(spriteset);
	spriteset_unref(spriteset);
	return true;
}

static bool
js_LoadSurface(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	image_t*    image;

	filename = jsal_require_pathname(0, "images", true, false);
	if (!(image = image_load(filename)))
		jsal_error(JS_ERROR, "couldn't load image '%s'", filename);
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_LoadWindowStyle(int num_args, bool is_ctor, intptr_t magic)
{
	const char*    filename;
	windowstyle_t* winstyle;

	filename = jsal_require_pathname(0, "windowstyles", true, false);
	if (!(winstyle = winstyle_load(filename)))
		jsal_error(JS_ERROR, "couldn't load windowstyle '%s'", filename);
	jsal_push_sphere_windowstyle(winstyle);
	winstyle_unref(winstyle);
	return true;
}

static bool
js_MapEngine(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	int         framerate;

	filename = jsal_require_pathname(0, "maps", true, false);
	framerate = num_args >= 2 ? jsal_to_int(1)
		: s_frame_rate;

	if (!map_engine_start(filename, framerate))
		jsal_error(JS_ERROR, "couldn't load map '%s'", filename);
	return false;
}

static bool
js_MapToScreenX(int num_args, bool is_ctor, intptr_t magic)
{
	int      layer;
	point2_t offset;
	double   x;

	layer = jsal_require_map_layer(0);
	x = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	offset = map_xy_from_screen(mk_point2(0, 0));
	jsal_push_int(x - offset.x);
	return true;
}

static bool
js_MapToScreenY(int num_args, bool is_ctor, intptr_t magic)
{
	int      layer;
	point2_t offset;
	double   y;

	layer = jsal_require_map_layer(0);
	y = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	offset = map_xy_from_screen(mk_point2(0, 0));
	jsal_push_int(y - offset.y);
	return true;
}

static bool
js_OpenAddress(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  hostname;
	int          port;
	socket_v1_t* socket;

	hostname = jsal_require_string(0);
	port = jsal_to_int(1);

	if ((socket = socket_v1_new_client(hostname, port)))
		jsal_push_class_obj(SV1_SOCKET, socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_OpenFile(int num_args, bool is_ctor, intptr_t magic)
{
	kev_file_t* file;
	const char* filename;

	filename = jsal_require_pathname(0, "save", true, true);

	if (!(file = kev_open(g_game, filename, true)))
		jsal_error(JS_ERROR, "couldn't open file '%s'", filename);
	jsal_push_class_obj(SV1_FILE, file, false);
	return true;
}

static bool
js_OpenLog(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	logger_t*   logger;

	filename = jsal_require_pathname(0, "logs", true, true);
	if (!(logger = logger_new(filename)))
		jsal_error(JS_ERROR, "couldn't open log file '%s'", filename);
	jsal_push_class_obj(SV1_LOGGER, logger, false);
	return true;
}

static bool
js_OpenRawFile(int num_args, bool is_ctor, intptr_t magic)
{
	file_t*     file;
	const char* filename;
	bool        writable;

	writable = num_args >= 2 ? jsal_to_boolean(1) : false;
	filename = jsal_require_pathname(0, "other", true, writable);

	if (!(file = file_open(g_game, filename, writable ? "w+b" : "rb"))) {
		jsal_error(JS_ERROR, "couldn't open file '%s' for %s",
			filename, writable ? "write" : "read");
	}
	jsal_push_class_obj(SV1_RAW_FILE, file, false);
	return true;
}

static bool
js_OutlinedCircle(int num_args, bool is_ctor, intptr_t magic)
{
	float x = trunc(jsal_to_number(0)) + 0.5;
	float y = trunc(jsal_to_number(1)) + 0.5;
	float radius = trunc(jsal_to_number(2));
	color_t color = jsal_require_sphere_color(3);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	return false;
}

static bool
js_OutlinedComplex(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_OutlinedEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	float x = jsal_to_int(0) + 0.5;
	float y = jsal_to_int(1) + 0.5;
	float rx = jsal_to_int(2);
	float ry = jsal_to_int(3);
	color_t color = jsal_require_sphere_color(4);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_ellipse(x, y, rx, ry, nativecolor(color), 1.0);
	return false;
}

static bool
js_OutlinedRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	int     thickness = 1;
	float   x1, y1;
	float   x2, y2;

	x1 = jsal_to_int(0) + 0.5;
	y1 = jsal_to_int(1) + 0.5;
	x2 = x1 + jsal_to_int(2) - 1;
	y2 = y1 + jsal_to_int(3) - 1;
	color = jsal_require_sphere_color(4);
	if (num_args >= 6)
		thickness = jsal_to_int(5);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return false;
}

static bool
js_OutlinedRoundRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	double  height;
	double  radius;
	double  thickness = 1.0;
	double  width;
	double  x;
	double  y;

	x = trunc(jsal_to_number(0)) + 0.5;
	y = trunc(jsal_to_number(1)) + 0.5;
	width = trunc(jsal_to_number(2));
	height = trunc(jsal_to_number(3));
	radius = jsal_to_number(4);
	color = jsal_require_sphere_color(5);
	if (num_args >= 7)
		thickness = trunc(jsal_to_number(6));

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_rounded_rectangle(x, y, x + width - 1, y + height - 1, radius, radius, nativecolor(color), thickness);
	return false;
}

static bool
js_Point(int num_args, bool is_ctor, intptr_t magic)
{
	float x = jsal_to_int(0) + 0.5;
	float y = jsal_to_int(1) + 0.5;
	color_t color = jsal_require_sphere_color(2);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_pixel(x, y, nativecolor(color));
	return false;
}

static bool
js_PointSeries(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	int             num_points;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;
	float           x;
	float           y;

	int i;

	jsal_require_array(0);
	color = jsal_require_sphere_color(1);

	if ((num_points = jsal_get_length(0)) < 1)
		jsal_error(JS_RANGE_ERROR, "one or more vertices required");
	vertices = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x");
		jsal_get_prop_string(-2, "y");
		x = trunc(jsal_to_number(-2));
		y = trunc(jsal_to_number(-1));
		jsal_pop(3);
		vertices[i].x = x + 0.5f;
		vertices[i].y = y + 0.5f;
		vertices[i].z = 0.0f;
		vertices[i].u = 0.0f;
		vertices[i].v = 0.0f;
		vertices[i].color = vtx_color;
	}
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	return false;
}

static bool
js_Polygon(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_Print(int num_args, bool is_ctor, intptr_t magic)
{
	const char* text;

	if (jsal_is_object(0))
		jsal_stringify(0);
	text = jsal_to_string(0);

	debugger_log(text, KI_LOG_NORMAL, true);
	return false;
}

static bool
js_QueuePersonCommand(int num_args, bool is_ctor, intptr_t magic)
{
	int         command;
	const char* name;
	bool        immediate = false;
	person_t*   person;

	name = jsal_require_string(0);
	command = jsal_require_int(1);
	if (num_args >= 3)
		immediate = jsal_to_boolean(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (command < 0 || command >= COMMAND_RUN_SCRIPT)
		jsal_error(JS_RANGE_ERROR, "invalid command type constant");
	if (command >= COMMAND_MOVE_NORTH && command <= COMMAND_MOVE_NORTHWEST) {
		if (!person_queue_command(person, COMMAND_ANIMATE, true))
			jsal_error(JS_ERROR, "couldn't queue command");
	}
	if (!person_queue_command(person, command, immediate))
		jsal_error(JS_ERROR, "couldn't queue command");
	return false;
}

static bool
js_QueuePersonScript(int num_args, bool is_ctor, intptr_t magic)
{
	bool        immediate = false;
	const char* name;
	script_t*   script;
	person_t*   person;

	name = jsal_require_string(0);
	script = jsal_require_sphere_script(1, "%/queueScript.js");
	if (num_args >= 3)
		immediate = jsal_to_boolean(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (!person_queue_script(person, script, immediate))
		jsal_error(JS_ERROR, "couldn't enqueue script");
	return false;
}

static bool
js_Rectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	double  height;
	double  width;
	double  x;
	double  y;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	width = trunc(jsal_to_number(2));
	height = trunc(jsal_to_number(3));
	color = jsal_require_sphere_color(4);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_rectangle(x, y, x + width, y + height, nativecolor(color));
	return false;
}

static bool
js_RemoveDirectory(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;

	name = jsal_require_pathname(0, "save", true, true);

	if (!game_rmdir(g_game, name))
		jsal_error(JS_ERROR, "couldn't remove directory '%s'", name);
	return false;
}

static bool
js_RemoveFile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, "save", true, true);

	if (!game_unlink(g_game, filename))
		jsal_error(JS_ERROR, "couldn't delete file '%s'", filename);
	return false;
}

static bool
js_RemoveTrigger(int num_args, bool is_ctor, intptr_t magic)
{
	int trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	map_remove_trigger(trigger_index);
	return false;
}

static bool
js_RemoveZone(int num_args, bool is_ctor, intptr_t magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	map_remove_zone(zone_index);
	return false;
}

static bool
js_Rename(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name1;
	const char* name2;

	name1 = jsal_require_pathname(0, "save", true, true);
	name2 = jsal_require_pathname(1, "save", true, true);
	if (!game_rename(g_game, name1, name2))
		jsal_error(JS_ERROR, "couldn't rename file '%s' to '%s'", name1, name2);
	return false;
}

static bool
js_RenderMap(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	map_engine_draw_map();
	return false;
}

static bool
js_ReplaceTilesOnLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int        layer;
	int        new_index;
	int        old_index;
	tileset_t* tileset;

	layer = jsal_require_map_layer(0);
	old_index = jsal_to_int(1);
	new_index = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (old_index < 0 || old_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid old tile index");
	if (new_index < 0 || new_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid new tile index");
	layer_replace_tiles(layer, old_index, new_index);
	return false;
}

static bool
js_RequireScript(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	bool        is_required;

	filename = jsal_require_pathname(0, "scripts", true, false);
	if (!game_file_exists(g_game, filename))
		jsal_error(JS_ERROR, "file '%s' not found", filename);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "RequireScript");
	jsal_get_prop_string(-1, filename);
	is_required = jsal_get_boolean(-1);
	jsal_pop(1);
	if (!is_required) {
		jsal_push_boolean_true();
		jsal_put_prop_string(-2, filename);
		if (!script_eval(filename))
			jsal_throw();
	}
	jsal_pop(3);
	return false;
}

static bool
js_RequireSystemScript(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename = jsal_require_string(0);

	bool is_required;
	char path[SPHERE_PATH_MAX];

	sprintf(path, "@/scripts/lib/%s", filename);
	if (!game_file_exists(g_game, path))
		sprintf(path, "#/scripts/%s", filename);
	if (!game_file_exists(g_game, path))
		jsal_error(JS_ERROR, "system script '%s' not found", filename);

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "RequireScript");
	jsal_get_prop_string(-1, path);
	is_required = jsal_get_boolean(-1);
	jsal_pop(1);
	if (!is_required) {
		jsal_push_boolean_true();
		jsal_put_prop_string(-2, path);
		if (!script_eval(path))
			jsal_throw();
	}
	jsal_pop(2);
	return false;
}

static bool
js_RestartGame(int num_args, bool is_ctor, intptr_t magic)
{
	sphere_restart();
	return false;
}

static bool
js_RoundRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	double  height;
	double  radius;
	double  width;
	double  x;
	double  y;

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	width = trunc(jsal_to_number(2));
	height = trunc(jsal_to_number(3));
	radius = jsal_to_number(4);
	color = jsal_require_sphere_color(5);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_rounded_rectangle(x, y, x + width, y + height, radius, radius, nativecolor(color));
	return false;
}

static bool
js_ScreenToMapX(int num_args, bool is_ctor, intptr_t magic)
{
	int      layer;
	point2_t map_xy;
	double   x;

	layer = jsal_require_map_layer(0);
	x = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	map_xy = map_xy_from_screen(mk_point2(x, 0));
	jsal_push_int(map_xy.x);
	return true;
}

static bool
js_ScreenToMapY(int num_args, bool is_ctor, intptr_t magic)
{
	int      layer;
	point2_t map_xy;
	double   y;

	layer = jsal_require_map_layer(0);
	y = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	map_xy = map_xy_from_screen(mk_point2(0, y));
	jsal_push_int(map_xy.y);
	return true;
}

static bool
js_SetCameraX(int num_args, bool is_ctor, intptr_t magic)
{
	int      new_x;
	point2_t position;

	new_x = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	position = map_get_camera_xy();
	map_set_camera_xy(mk_point2(new_x, position.y));
	return false;
}

static bool
js_SetCameraY(int num_args, bool is_ctor, intptr_t magic)
{
	int      new_y;
	point2_t position;

	new_y = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	position = map_get_camera_xy();
	map_set_camera_xy(mk_point2(position.x, new_y));
	return false;
}

static bool
js_SetClippingRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	int height;
	int width;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	image_set_scissor(screen_backbuffer(g_screen),
		mk_rect(x, y, x + width, y + height));
	return false;
}

static bool
js_SetColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	color_t new_mask;
	int     num_frames;

	new_mask = jsal_require_sphere_color(0);
	num_frames = num_args >= 2 ? jsal_to_int(1) : 0;

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid number of frames");
	map_engine_fade_to(new_mask, num_frames);
	return false;
}

static bool
js_SetDefaultMapScript(int num_args, bool is_ctor, intptr_t magic)
{
	int         map_op;
	script_t*   script;

	map_op = jsal_to_int(0);
	script = jsal_require_sphere_script(1, "%/mapScript.js");

	if (map_op < 0 || map_op >= MAP_SCRIPT_MAX)
		jsal_error(JS_ERROR, "invalid map script constant");
	map_engine_on_map_event(map_op, script);
	script_unref(script);
	return false;
}

static bool
js_SetDefaultPersonScript(int num_args, bool is_ctor, intptr_t magic)
{
	int       person_op;
	script_t* script;

	person_op = jsal_require_int(0);
	script = jsal_require_sphere_script(1, "%/personScript.js");

	if (person_op < 0 || person_op >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "invalid script type constant");
	map_engine_on_person_event(person_op, script);
	script_unref(script);
	return false;
}

static bool
js_SetDelayScript(int num_args, bool is_ctor, intptr_t magic)
{
	int num_frames;
	script_t* script;

	num_frames = jsal_to_int(0);
	script = jsal_require_sphere_script(1, "%/delayScript.js");

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	map_engine_defer(script, num_frames);
	return false;
}

static bool
js_SetFrameRate(int num_args, bool is_ctor, intptr_t magic)
{
	int framerate = jsal_to_int(0);

	if (framerate < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");
	s_frame_rate = framerate;
	return false;
}

static bool
js_SetLayerAngle(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerHeight(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int new_height;

	layer = jsal_require_map_layer(0);
	new_height = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (new_height <= 0)
		jsal_error(JS_ERROR, "height must be positive and nonzero (got: %d)", new_height);
	if (!layer_resize(layer, layer_size(layer).width, new_height))
		jsal_error(JS_ERROR, "couldn't resize layer '%d'", layer);
	return false;
}

static bool
js_SetLayerMask(int num_args, bool is_ctor, intptr_t magic)
{
	int     layer;
	color_t mask;

	layer = jsal_require_map_layer(0);
	mask = jsal_require_sphere_color(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	layer_set_color_mask(layer, mask);
	return false;
}

static bool
js_SetLayerReflective(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	bool reflective;

	layer = jsal_require_map_layer(0);
	reflective = jsal_to_boolean(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	layer_set_reflective(layer, reflective);
	return false;
}

static bool
js_SetLayerRenderer(int num_args, bool is_ctor, intptr_t magic)
{
	int       layer;
	char      script_name[50];
	script_t* script;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	sprintf(script_name, "[layer %d render script]", layer);
	script = jsal_require_sphere_script(1, script_name);
	layer_on_render(layer, script);
	script_unref(script);
	return false;
}

static bool
js_SetLayerScaleFactorX(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerScaleFactorY(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerSize(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int new_height;
	int new_width;

	layer = jsal_require_map_layer(0);
	new_width = jsal_to_int(1);
	new_height = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (new_width <= 0 || new_height <= 0)
		jsal_error(JS_ERROR, "invalid layer dimensions");
	if (!layer_resize(layer, new_width, new_height))
		jsal_error(JS_ERROR, "couldn't resize layer");
	return false;
}

static bool
js_SetLayerVisible(int num_args, bool is_ctor, intptr_t magic)
{
	bool visible;
	int  layer;

	layer = jsal_require_map_layer(0);
	visible = jsal_to_boolean(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	layer_set_visible(layer, visible);
	return false;
}

static bool
js_SetLayerWidth(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int new_width;

	layer = jsal_require_map_layer(0);
	new_width = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (new_width <= 0)
		jsal_error(JS_ERROR, "width must be positive and nonzero (got: %d)", new_width);
	if (!layer_resize(layer, new_width, layer_size(layer).height))
		jsal_error(JS_ERROR, "couldn't resize layer '%d'", layer);
	return false;
}

static bool
js_SetMapEngineFrameRate(int num_args, bool is_ctor, intptr_t magic)
{
	int framerate;

	framerate = jsal_to_int(0);

	if (framerate < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");

	map_engine_set_framerate(framerate);
	return false;
}

static bool
js_SetMousePosition(int num_args, bool is_ctor, intptr_t magic)
{
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	screen_set_mouse_xy(g_screen, x, y);
	return false;
}

static bool
js_SetNextAnimatedTile(int num_args, bool is_ctor, intptr_t magic)
{
	int        next_index;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	next_index = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (next_index < 0 || next_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index for next tile");
	tileset_set_next(tileset, tile_index, next_index);
	return false;
}

static bool
js_SetPersonAngle(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      theta;

	name = jsal_require_string(0);
	theta = jsal_to_number(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_angle(person, theta);
	return false;
}

static bool
js_SetPersonData(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	jsal_require_object_coercible(1);
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	jsal_dup(1); jsal_put_prop_string(-2, name);
	return false;
}

static bool
js_SetPersonDirection(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	const char* new_dir;
	person_t*   person;

	name = jsal_require_string(0);
	new_dir = jsal_require_string(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_pose(person, new_dir);
	return false;
}

static bool
js_SetPersonFollowDistance(int num_args, bool is_ctor, intptr_t magic)
{
	int         distance;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	distance = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (person_get_leader(person) == NULL)
		jsal_error(JS_TYPE_ERROR, "person has no leader");
	if (distance <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid distance");
	person_set_trailing(person, distance);
	return false;
}

static bool
js_SetPersonFrame(int num_args, bool is_ctor, intptr_t magic)
{
	int         frame_index;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	frame_index = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_frame(person, frame_index);
	return false;
}

static bool
js_SetPersonFrameNext(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = jsal_require_string(0);
	num_frames = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	person_set_frame_delay(person, num_frames);
	return false;
}

static bool
js_SetPersonFrameRevert(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = jsal_require_string(0);
	num_frames = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	person_set_revert_delay(person, num_frames);
	return false;
}

static bool
js_SetPersonIgnoreList(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	int         num_ignores;
	person_t*   person;

	int i;

	name = jsal_require_string(0);
	jsal_require_array(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_clear_ignores(person);
	num_ignores = jsal_get_length(1);
	for (i = 0; i < num_ignores; ++i) {
		jsal_get_prop_index(1, i);
		person_ignore_name(person, jsal_require_string(-1));
		jsal_pop(1);
	}
	return false;
}

static bool
js_SetPersonLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int         layer;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	layer = jsal_require_map_layer(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_layer(person, layer);
	return false;
}

static bool
js_SetPersonMask(int num_args, bool is_ctor, intptr_t magic)
{
	color_t     color;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	color = jsal_require_sphere_color(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_color(person, color);
	return false;
}

static bool
js_SetPersonOffsetX(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_x;

	name = jsal_require_string(0);
	new_x = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	offset = person_get_offset(person);
	person_set_offset(person, mk_point2(new_x, offset.y));
	return false;
}

static bool
js_SetPersonOffsetY(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_y;

	name = jsal_require_string(0);
	new_y = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	offset = person_get_offset(person);
	person_set_offset(person, mk_point2(offset.x, new_y));
	return false;
}

static bool
js_SetPersonScaleAbsolute(int num_args, bool is_ctor, intptr_t magic)
{
	int          height;
	const char*  name;
	person_t*    person;
	int          sprite_h;
	int          sprite_w;
	spriteset_t* spriteset;
	int          width;

	name = jsal_require_string(0);
	width = jsal_require_int(1);
	height = jsal_require_int(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (width < 0 || height < 0)
		jsal_error(JS_RANGE_ERROR, "invalid scale dimensions");
	spriteset = person_get_spriteset(person);
	sprite_w = spriteset_width(spriteset);
	sprite_h = spriteset_height(spriteset);
	person_set_scale(person, width / sprite_w, height / sprite_h);
	return false;
}

static bool
js_SetPersonScaleFactor(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      scale_x;
	double      scale_y;

	name = jsal_require_string(0);
	scale_x = jsal_to_number(1);
	scale_y = jsal_to_number(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (scale_x < 0.0 || scale_y < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid scale factor(s)");
	person_set_scale(person, scale_x, scale_y);
	return false;
}

static bool
js_SetPersonScript(int num_args, bool is_ctor, intptr_t magic)
{
	lstring_t*  codestring;
	const char* name;
	person_t*   person;
	script_t*   script;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "invalid script type constant");
	if (jsal_is_string(2)) {
		codestring = jsal_require_lstring_t(2);
		person_compile_script(person, type, codestring);
		lstr_free(codestring);
	}
	else {
		script = jsal_require_sphere_script(2, "[person script]");
		person_on_event(person, type, script);
	}
	return false;
}

static bool
js_SetPersonSpeed(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      speed;

	name = jsal_require_string(0);
	speed = jsal_to_number(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_speed(person, speed, speed);
	return false;
}

static bool
js_SetPersonSpeedXY(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	double      x_speed;
	double      y_speed;

	name = jsal_require_string(0);
	x_speed = jsal_to_number(1);
	y_speed = jsal_to_number(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_speed(person, x_speed, y_speed);
	return false;
}

static bool
js_SetPersonSpriteset(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = jsal_require_string(0);
	spriteset = jsal_require_sphere_spriteset(1);

	if (!(person = map_person_by_name(name))) {
		spriteset_unref(spriteset);
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	}
	person_set_spriteset(person, spriteset);
	spriteset_unref(spriteset);
	return false;
}

static bool
js_SetPersonValue(int num_args, bool is_ctor, intptr_t magic)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	key = jsal_to_string(1);

	jsal_normalize_index(2);
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	if (!jsal_get_prop_string(-1, name)) {
		jsal_pop(1);
		jsal_push_new_object();
		jsal_put_prop_string(-2, name);
		jsal_get_prop_string(-1, name);
	}
	jsal_dup(2);
	jsal_put_prop_string(-2, key);
	jsal_pop(2);
	return false;
}

static bool
js_SetPersonVisible(int num_args, bool is_ctor, intptr_t magic)
{
	const char* name;
	person_t*   person;
	bool        visible;

	name = jsal_require_string(0);
	visible = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_set_visible(person, visible);
	return false;
}

static bool
js_SetPersonX(int num_args, bool is_ctor, intptr_t magic)
{
	int         layer;
	const char* name;
	int         new_x;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);
	new_x = jsal_to_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, new_x, y, layer);
	return false;
}

static bool
js_SetPersonXYFloat(int num_args, bool is_ctor, intptr_t magic)
{
	int         layer;
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);
	x = jsal_to_number(1);
	y = jsal_to_number(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	layer = person_get_layer(person);
	person_set_xyz(person, x, y, layer);
	return false;
}

static bool
js_SetPersonY(int num_args, bool is_ctor, intptr_t magic)
{
	int         layer;
	const char* name;
	int         new_y;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);
	new_y = jsal_to_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "No such person '%s'", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, x, new_y, layer);
	return false;
}

static bool
js_SetRenderScript(int num_args, bool is_ctor, intptr_t magic)
{
	script_t* script;

	script = jsal_require_sphere_script(0, "%/renderScript.js");

	map_engine_on_render(script);
	script_unref(script);
	return false;
}

static bool
js_SetTalkActivationButton(int num_args, bool is_ctor, intptr_t magic)
{
	int button_id;

	button_id = jsal_to_int(0);

	map_engine_set_talk_button(button_id);
	return false;
}

static bool
js_SetTalkActivationKey(int num_args, bool is_ctor, intptr_t magic)
{
	int key;
	int player = PLAYER_1;

	key = jsal_to_int(0);
	if (num_args >= 2)
		player = jsal_to_int(1);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");

	map_engine_set_talk_key(player, key);
	return false;
}

static bool
js_SetTalkDistance(int num_args, bool is_ctor, intptr_t magic)
{
	int pixels;

	pixels = jsal_to_int(0);

	if (pixels < 0)
		jsal_error(JS_RANGE_ERROR, "invalid talk distance");

	map_engine_set_talk_distance(pixels);
	return false;
}

static bool
js_SetTile(int num_args, bool is_ctor, intptr_t magic)
{
	int        layer;
	int        tile_index;
	tileset_t* tileset;
	int        x;
	int        y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);
	tile_index = jsal_to_int(3);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");

	layer_set_tile(layer, x, y, tile_index);
	return false;
}

static bool
js_SetTileDelay(int num_args, bool is_ctor, intptr_t magic)
{
	int        delay;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	delay = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (delay < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	tileset_set_delay(tileset, tile_index, delay);
	return false;
}

static bool
js_SetTileImage(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	image = jsal_require_class_obj(1, SV1_IMAGE);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		jsal_error(JS_TYPE_ERROR, "image/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return false;
}

static bool
js_SetTileName(int num_args, bool is_ctor, intptr_t magic)
{
	lstring_t* name;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	name = jsal_require_lstring_t(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tileset_set_name(tileset, tile_index, name);
	lstr_free(name);
	return false;
}

static bool
js_SetTileSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	image = jsal_require_class_obj(1, SV1_SURFACE);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		jsal_error(JS_TYPE_ERROR, "surface/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return false;
}

static bool
js_SetTriggerLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int trigger_index;

	trigger_index = jsal_to_int(0);
	layer = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_set_layer(trigger_index, layer);
	return false;
}

static bool
js_SetTriggerScript(int num_args, bool is_ctor, intptr_t magic)
{
	script_t*  script;
	lstring_t* script_name;
	int        trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	script_name = lstr_newf("%s/trigger~%d/onStep", map_pathname(), trigger_index);
	script = jsal_require_sphere_script(1, lstr_cstr(script_name));
	trigger_set_script(trigger_index, script);
	script_unref(script);
	lstr_free(script_name);
	return false;
}

static bool
js_SetTriggerXY(int num_args, bool is_ctor, intptr_t magic)
{
	int trigger_index;
	int x;
	int y;

	trigger_index = jsal_to_int(0);
	x = jsal_to_int(1);
	y = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_set_xy(trigger_index, x, y);
	return false;
}

static bool
js_SetUpdateScript(int num_args, bool is_ctor, intptr_t magic)
{
	script_t* script;

	script = jsal_require_sphere_script(0, "%/updateScript.js");

	map_engine_on_update(script);
	script_unref(script);
	return false;
}

static bool
js_SetZoneDimensions(int num_args, bool is_ctor, intptr_t magic)
{
	rect_t bounds;
	int    height;
	int    width;
	int    x;
	int    y;
	int    zone_index;

	zone_index = jsal_to_int(0);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	width = jsal_to_int(3);
	height = jsal_to_int(4);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid zone size");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		jsal_error(JS_RANGE_ERROR, "zone area out of bounds");
	zone_set_bounds(zone_index, mk_rect(x, y, x + width, y + height));
	return false;
}

static bool
js_SetZoneLayer(int num_args, bool is_ctor, intptr_t magic)
{
	int layer;
	int zone_index;

	zone_index = jsal_to_int(0);
	layer = jsal_require_map_layer(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	zone_set_layer(zone_index, layer);
	return false;
}

static bool
js_SetZoneScript(int num_args, bool is_ctor, intptr_t magic)
{
	script_t*  script;
	lstring_t* script_name;
	int        zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	if (!(script_name = lstr_newf("%s/zone%d", map_pathname(), zone_index)))
		jsal_error(JS_ERROR, "error compiling zone script");
	script = jsal_require_sphere_script(1, lstr_cstr(script_name));
	lstr_free(script_name);
	zone_set_script(zone_index, script);
	script_unref(script);
	return false;
}

static bool
js_SetZoneSteps(int num_args, bool is_ctor, intptr_t magic)
{
	int zone_index;
	int steps;

	zone_index = jsal_to_int(0);
	steps = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	if (steps <= 0)
		jsal_error(JS_RANGE_ERROR, "steps must be positive (got: %d)", steps);
	zone_set_steps(zone_index, steps);
	return false;
}

static bool
js_Triangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t color;
	float   x1, x2, x3;
	float   y1, y2, y3;

	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = trunc(jsal_to_number(2));
	y2 = trunc(jsal_to_number(3));
	x3 = trunc(jsal_to_number(4));
	y3 = trunc(jsal_to_number(5));
	color = jsal_require_sphere_color(6);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, nativecolor(color));
	return false;
}

static bool
js_UnbindJoystickButton(int num_args, bool is_ctor, intptr_t magic)
{
	int joy_index = jsal_to_int(0);
	int button = jsal_to_int(1);

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		jsal_error(JS_RANGE_ERROR, "joystick index '%d' out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		jsal_error(JS_RANGE_ERROR, "button index '%d' out of range", button);
	joy_bind_button(joy_index, button, NULL, NULL);
	return false;
}

static bool
js_UnbindKey(int num_args, bool is_ctor, intptr_t magic)
{
	int keycode = jsal_to_int(0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid key constant");
	kb_bind_key(keycode, NULL, NULL);
	return false;
}

static bool
js_UpdateMapEngine(int num_args, bool is_ctor, intptr_t magic)
{
	if (!map_engine_running())
		jsal_error(JS_RANGE_ERROR, "Map engine not running");
	map_engine_update();
	return false;
}

static void
js_Animation_finalize(void* host_ptr)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_unref(host_ptr);
#endif
}

static bool
js_Animation_get_height(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_height(anim));
	return true;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_get_width(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_width(anim));
	return true;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_drawFrame(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;
	int          x;
	int          y;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);
	x = jsal_to_int(0);
	y = jsal_to_int(1);

	image_draw(animation_frame(anim), x, y);
	return false;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_drawZoomedFrame(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;
	int          height;
	double       scale;
	int          width;
	int          x;
	int          y;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);
	x = jsal_to_number(0);
	y = jsal_to_number(1);
	scale = jsal_to_number(2);

	if (scale < 0.0)
		jsal_error(JS_RANGE_ERROR, "zoom must be positive");
	width = animation_width(anim);
	height = animation_height(anim);
	image_draw_scaled(animation_frame(anim), x, y, width * scale, height * scale);
	return false;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_getDelay(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_delay(anim));
	return true;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_getNumFrames(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_num_frames(anim));
	return true;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static bool
js_Animation_readNextFrame(int num_args, bool is_ctor, intptr_t magic)
{
#if defined(NEOSPHERE_MNG_SUPPORT)
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	animation_update(anim);
	return false;
#else
	jsal_error(JS_ERROR, "MNG animation support is not available");
#endif
}

static void
js_ByteArray_finalize(void* host_ptr)
{
	bytearray_unref(host_ptr);
}

static bool
js_ByteArray_get_length(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;

	jsal_push_this();
	array = jsal_require_class_obj(-1, SV1_BYTE_ARRAY);

	jsal_push_int(bytearray_len(array));
	return true;
}

static bool
js_ByteArray_concat(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array[2];
	bytearray_t* new_array;

	jsal_push_this();
	array[0] = jsal_require_class_obj(-1, SV1_BYTE_ARRAY);
	array[1] = jsal_require_class_obj(0, SV1_BYTE_ARRAY);

	if (!(new_array = bytearray_concat(array[0], array[1])))
		jsal_error(JS_ERROR, "couldn't concatenate byte arrays");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_ByteArray_slice(int num_args, bool is_ctor, intptr_t magic)
{
	int n_args = jsal_get_top();
	int start = jsal_to_int(0);
	int end = (n_args >= 2) ? jsal_to_int(1) : INT_MAX;

	bytearray_t* array;
	int          end_norm;
	bytearray_t* new_array;
	int          size;

	jsal_push_this();
	array = jsal_require_class_obj(-1, SV1_BYTE_ARRAY);

	size = bytearray_len(array);
	end_norm = fmin(end >= 0 ? end : size + end, size);
	if (end_norm < start || end_norm > size)
		jsal_error(JS_RANGE_ERROR, "start and/or end is out of bounds");
	if (!(new_array = bytearray_slice(array, start, end_norm - start)))
		jsal_error(JS_ERROR, "couldn't slice byte array");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_ByteArray_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object byte_array]");
	return true;
}

static bool
js_Color_get_red(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->r);
	return true;
}

static bool
js_Color_get_green(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->g);
	return true;
}

static bool
js_Color_get_blue(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->b);
	return true;
}

static bool
js_Color_get_alpha(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->a);
	return true;
}

static bool
js_Color_set_red(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);
	value = jsal_require_int(0);

	color->r = value < 0 ? 0 : value > 255 ? 255 : value;
	return false;
}

static bool
js_Color_set_green(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);
	value = jsal_require_int(0);

	color->g = value < 0 ? 0 : value > 255 ? 255 : value;
	return false;
}

static bool
js_Color_set_blue(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);
	value = jsal_require_int(0);

	color->b = value < 0 ? 0 : value > 255 ? 255 : value;
	return false;
}

static bool
js_Color_set_alpha(int num_args, bool is_ctor, intptr_t magic)
{
	color_t* color;
	double   value;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);
	value = jsal_require_int(0);

	color->a = value < 0 ? 0 : value > 255 ? 255 : value;
	return false;
}

static bool
js_Color_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object color]");
	return true;
}

static bool
js_ColorMatrix_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object colormatrix]");
	return true;
}

static void
js_File_finalize(void* host_ptr)
{
	kev_close(host_ptr);
}

static bool
js_File_close(int num_args, bool is_ctor, intptr_t magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	jsal_set_class_ptr(-1, NULL);
	kev_close(file);
	return false;
}

static bool
js_File_flush(int num_args, bool is_ctor, intptr_t magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	kev_save(file);
	return false;
}

static bool
js_File_getKey(int num_args, bool is_ctor, intptr_t magic)
{
	int index = jsal_to_int(0);

	kev_file_t* file;
	const char* key;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	if ((key = kev_get_key(file, index)))
		jsal_push_string(key);
	else
		jsal_push_null();
	return true;
}

static bool
js_File_getNumKeys(int num_args, bool is_ctor, intptr_t magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	jsal_push_int(kev_num_keys(file));
	return true;
}

static bool
js_File_read(int num_args, bool is_ctor, intptr_t magic)
{
	const char* key = jsal_to_string(0);

	bool        def_bool;
	double      def_num;
	const char* def_string;
	kev_file_t* file;
	const char* value;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, SV1_FILE)))
		jsal_error(JS_ERROR, "file is already closed");

	if (jsal_is_boolean(1)) {
		def_bool = jsal_get_boolean(1);
		jsal_push_boolean(kev_read_bool(file, key, def_bool));
	}
	else if (jsal_is_number(1)) {
		def_num = jsal_get_number(1);
		jsal_push_number(kev_read_float(file, key, def_num));
	}
	else {
		def_string = jsal_to_string(1);
		value = kev_read_string(file, key, def_string);
		jsal_push_string(value);
	}
	return true;
}

static bool
js_File_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object file]");
	return true;
}

static bool
js_File_write(int num_args, bool is_ctor, intptr_t magic)
{
	const char* key = jsal_to_string(0);

	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	kev_write_string(file, key, jsal_to_string(1));
	return false;
}

static void
js_Font_finalize(void* host_ptr)
{
	font_unref(host_ptr);
}

static bool
js_Font_clone(int num_args, bool is_ctor, intptr_t magic)
{
	font_t* dolly_font;
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);

	if (!(dolly_font = font_clone(font)))
		jsal_error(JS_ERROR, "couldn't clone font");
	jsal_push_sphere_font(dolly_font);
	return true;
}

static bool
js_Font_drawText(int num_args, bool is_ctor, intptr_t magic)
{
	font_t*     font;
	const char* text = jsal_to_string(2);
	int         x = jsal_to_int(0);
	int         y = jsal_to_int(1);

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	font_draw_text(font, x, y, TEXT_ALIGN_LEFT, text);
	return false;
}

static bool
js_Font_drawTextBox(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*    backbuffer;
	font_t*     font;
	int         height;
	int         line_height;
	const char* line_text;
	rect_t      old_clip_box;
	int         width;
	int         x;
	int         y;
	int         y_offset;
	const char* text;
	wraptext_t* wraptext;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	y_offset = jsal_to_int(4);
	text = jsal_to_string(5);

	if (screen_skipping_frame(g_screen))
		return false;

	// intersect our own clipping box with the one set by the user to ensure we
	// don't accidentally draw outside of it
	backbuffer = screen_backbuffer(g_screen);
	old_clip_box = image_get_scissor(backbuffer);
	image_set_scissor(backbuffer,
		rect_intersect(mk_rect(x, y, x + width, y + height), old_clip_box));

	wraptext = font_wrap(font, text, width);
	line_height = font_height(font);
	galileo_reset();
	y += y_offset;
	for (i = 0; i < wraptext_len(wraptext); ++i) {
		line_text = wraptext_line(wraptext, i);
		font_draw_text(font, x, y, TEXT_ALIGN_LEFT, line_text);
		y += line_height;
	}
	wraptext_free(wraptext);
	image_set_scissor(backbuffer, old_clip_box);
	return false;
}

static bool
js_Font_drawZoomedText(int num_args, bool is_ctor, intptr_t magic)
{
	ALLEGRO_BITMAP* bitmap;
	font_t*         font;
	int             height;
	ALLEGRO_BITMAP* old_target;
	float           scale;
	const char*     text;
	int             width;
	int             x;
	int             y;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	scale = jsal_to_number(2);
	text = jsal_to_string(3);

	if (screen_skipping_frame(g_screen))
		return false;

	// render the text to a texture so we can scale it up.  not the most
	// efficient way to do it, sure, but it gets the job done.
	width = font_get_width(font, text);
	height = font_height(font);
	bitmap = al_create_bitmap(width, height);
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(bitmap);
	font_draw_text(font, 0, 0, TEXT_ALIGN_LEFT, text);
	al_set_target_bitmap(old_target);

	galileo_reset();
	al_draw_scaled_bitmap(bitmap, 0, 0, width, height, x, y, width * scale, height * scale, 0x0);
	al_destroy_bitmap(bitmap);
	return false;
}

static bool
js_Font_getCharacterImage(int num_args, bool is_ctor, intptr_t magic)
{
	uint32_t cp;
	font_t*  font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	cp = jsal_to_uint(0);

	jsal_push_class_obj(SV1_IMAGE, image_ref(font_glyph(font, cp)), false);
	return true;
}

static bool
js_Font_getColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);

	jsal_push_sphere_color(font_get_mask(font));
	return true;
}

static bool
js_Font_getHeight(int num_args, bool is_ctor, intptr_t magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);

	jsal_push_int(font_height(font));
	return true;
}

static bool
js_Font_getStringHeight(int num_args, bool is_ctor, intptr_t magic)
{
	font_t*     font;
	const char* text;
	int         width;
	wraptext_t* wraptext;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	text = jsal_to_string(0);
	width = jsal_to_int(1);

	wraptext = font_wrap(font, text, width);
	jsal_push_int(font_height(font) * wraptext_len(wraptext));
	wraptext_free(wraptext);
	return true;
}

static bool
js_Font_getStringWidth(int num_args, bool is_ctor, intptr_t magic)
{
	font_t*     font;
	const char* text;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	text = jsal_to_string(0);

	jsal_push_int(font_get_width(font, text));
	return true;
}

static bool
js_Font_setCharacterImage(int num_args, bool is_ctor, intptr_t magic)
{
	uint32_t cp;
	font_t*  font;
	image_t* image;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	cp = jsal_to_uint(0);
	image = jsal_require_class_obj(1, SV1_IMAGE);

	font_set_glyph(font, cp, image);
	return false;
}

static bool
js_Font_setColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	font_t* font;
	color_t mask;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	mask = jsal_require_sphere_color(0);

	font_set_mask(font, mask);
	return false;
}

static bool
js_Font_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object font]");
	return true;
}

static bool
js_Font_wordWrapString(int num_args, bool is_ctor, intptr_t magic)
{
	font_t*     font;
	int         num_lines;
	const char* text;
	int         width;
	wraptext_t* wraptext;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	text = jsal_to_string(0);
	width = jsal_to_int(1);

	wraptext = font_wrap(font, text, width);
	num_lines = wraptext_len(wraptext);
	jsal_push_new_array();
	for (i = 0; i < num_lines; ++i) {
		jsal_push_string(wraptext_line(wraptext, i));
		jsal_put_prop_index(-2, i);
	}
	wraptext_free(wraptext);
	return true;
}

static void
js_Image_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Image_get_height(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Image_get_width(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_Image_blit(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*    backbuffer;
	int         blend_mode = BLEND_NORMAL;
	blend_op_t* blend_op;
	image_t*    image;
	blend_op_t* prev_blend_op;
	int         x;
	int         y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	if (num_args >= 3)
		blend_mode = jsal_to_int(2);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	backbuffer = screen_backbuffer(g_screen);
	blend_op = blend_mode == BLEND_NORMAL ? s_blender_normal
		: blend_mode == BLEND_REPLACE ? s_blender_copy
		: s_blender_null;
	galileo_reset();
	prev_blend_op = blend_op_ref(image_get_blend_op(backbuffer));
	image_set_blend_op(backbuffer, blend_op);
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	image_set_blend_op(backbuffer, prev_blend_op);
	blend_op_unref(prev_blend_op);
	return false;
}

static bool
js_Image_blitMask(int num_args, bool is_ctor, intptr_t magic)
{
	int      blend_mode = BLEND_NORMAL;
	image_t* image;
	color_t  mask;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	mask = jsal_require_sphere_color(2);
	if (num_args >= 4)
		blend_mode = jsal_to_int(3);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_tinted_bitmap(image_bitmap(image), nativecolor(mask), x, y, 0x0);
	return false;
}

static bool
js_Image_createSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	if ((new_image = image_dup(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Image_rotateBlit(int num_args, bool is_ctor, intptr_t magic)
{
	float    angle;
	int      blend_mode = BLEND_NORMAL;
	float    height;
	image_t* image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	angle = jsal_to_number(2);
	if (num_args >= 4)
		blend_mode = jsal_to_int(3);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_rotated_bitmap(image_bitmap(image), width / 2, height / 2,
		x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Image_rotateBlitMask(int num_args, bool is_ctor, intptr_t magic)
{
	float    angle;
	int      blend_mode = BLEND_NORMAL;
	float    height;
	image_t* image;
	color_t  mask;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	angle = jsal_to_number(2);
	mask = jsal_require_sphere_color(3);
	if (num_args >= 5)
		blend_mode = jsal_to_int(4);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_tinted_rotated_bitmap(image_bitmap(image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Image_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object image]");
	return true;
}

static bool
js_Image_transformBlit(int num_args, bool is_ctor, intptr_t magic)
{
	int           blend_mode = BLEND_NORMAL;
	int           height;
	image_t*      image;
	ALLEGRO_COLOR mask;
	int           width;
	float         x1, y1;
	float         x2, y2;
	float         x3, y3;
	float         x4, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = trunc(jsal_to_number(2));
	y2 = trunc(jsal_to_number(3));
	x3 = trunc(jsal_to_number(4));
	y3 = trunc(jsal_to_number(5));
	x4 = trunc(jsal_to_number(6));
	y4 = trunc(jsal_to_number(7));
	if (num_args >= 9)
		blend_mode = jsal_to_int(8);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	width = image_width(image);
	height = image_height(image);
	mask = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, mask },
		{ x2, y2, 0, width, 0, mask },
		{ x4, y4, 0, 0, height, mask },
		{ x3, y3, 0, width, height, mask }
	};
	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Image_transformBlitMask(int num_args, bool is_ctor, intptr_t magic)
{
	int           blend_mode = BLEND_NORMAL;
	ALLEGRO_COLOR color;
	int           height;
	image_t*      image;
	color_t       mask;
	int           width;
	float         x1, y1;
	float         x2, y2;
	float         x3, y3;
	float         x4, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = trunc(jsal_to_number(2));
	y2 = trunc(jsal_to_number(3));
	x3 = trunc(jsal_to_number(4));
	y3 = trunc(jsal_to_number(5));
	x4 = trunc(jsal_to_number(6));
	y4 = trunc(jsal_to_number(7));
	mask = jsal_require_sphere_color(8);
	if (num_args >= 10)
		blend_mode = jsal_to_int(9);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	width = image_width(image);
	height = image_height(image);
	color = nativecolor(mask);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, color },
		{ x2, y2, 0, width, 0, color },
		{ x4, y4, 0, 0, height, color },
		{ x3, y3, 0, width, height, color }
	};
	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Image_zoomBlit(int num_args, bool is_ctor, intptr_t magic)
{
	int      blend_mode = BLEND_NORMAL;
	int      height;
	image_t* image;
	float    scale;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	scale = jsal_to_number(2);
	if (num_args >= 4)
		blend_mode = jsal_to_int(3);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_scaled_bitmap(image_bitmap(image), 0, 0, width, height,
		x, y, width * scale, height * scale, 0x0);
	return false;
}

static bool
js_Image_zoomBlitMask(int num_args, bool is_ctor, intptr_t magic)
{
	int      blend_mode = BLEND_NORMAL;
	int      height;
	image_t* image;
	color_t  mask;
	float    scale;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	scale = jsal_to_number(2);
	mask = jsal_require_sphere_color(3);
	if (num_args >= 5)
		blend_mode = jsal_to_int(4);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	if (screen_skipping_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	apply_blend_mode(screen_backbuffer(g_screen), blend_mode);
	al_draw_tinted_scaled_bitmap(image_bitmap(image), nativecolor(mask),
		0, 0, width, height, x, y, width * scale, height * scale, 0x0);
	return false;
}

static void
js_Logger_finalize(void* host_ptr)
{
	logger_unref(host_ptr);
}

static bool
js_Logger_beginBlock(int num_args, bool is_ctor, intptr_t magic)
{
	const char* title = jsal_to_string(0);

	logger_t* logger;

	jsal_push_this();
	logger = jsal_require_class_obj(-1, SV1_LOGGER);
	if (!logger_begin_block(logger, title))
		jsal_error(JS_ERROR, "couldn't create new log block");
	return false;
}

static bool
js_Logger_endBlock(int num_args, bool is_ctor, intptr_t magic)
{
	logger_t* logger;

	jsal_push_this();
	logger = jsal_require_class_obj(-1, SV1_LOGGER);
	logger_end_block(logger);
	return false;
}

static bool
js_Logger_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object log]");
	return true;
}

static bool
js_Logger_write(int num_args, bool is_ctor, intptr_t magic)
{
	const char* text = jsal_to_string(0);

	logger_t* logger;

	jsal_push_this();
	logger = jsal_require_class_obj(-1, SV1_LOGGER);
	logger_write(logger, NULL, text);
	return false;
}

static void
js_RawFile_finalize(void* host_ptr)
{
	file_close(host_ptr);
}

static bool
js_RawFile_close(int num_args, bool is_ctor, intptr_t magic)
{
	file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	if (file == NULL)
		jsal_error(JS_ERROR, "file already closed");

	jsal_set_class_ptr(-1, NULL);
	file_close(file);
	return false;
}

static bool
js_RawFile_getPosition(int num_args, bool is_ctor, intptr_t magic)
{
	file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	jsal_push_int(file_position(file));
	return true;
}

static bool
js_RawFile_getSize(int num_args, bool is_ctor, intptr_t magic)
{
	file_t* file;
	long    file_pos;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	file_pos = file_position(file);
	file_seek(file, 0, WHENCE_END);
	jsal_push_int(file_position(file));
	file_seek(file, file_pos, WHENCE_SET);
	return true;
}

static bool
js_RawFile_read(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	file_t*      file;
	long long    num_bytes = 0;
	long long    pos;
	void*        read_buffer;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	if (num_args >= 1)
		num_bytes = jsal_to_int(0);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");

	if (num_args < 1) {  // if no arguments, read entire file back to front
		pos = file_position(file);
		num_bytes = (file_seek(file, 0, WHENCE_END), file_position(file));
		file_seek(file, 0, WHENCE_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid read size");
	if (!(read_buffer = alloca(num_bytes)))
		jsal_error(JS_ERROR, "couldn't read from file");
	num_bytes = (long)file_read(file, read_buffer, num_bytes, 1);
	if (num_args < 1)  // reset file position after whole-file read
		file_seek(file, pos, WHENCE_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		jsal_error(JS_ERROR, "couldn't read from file");
	jsal_push_sphere_bytearray(array);
	return true;
}

static bool
js_RawFile_setPosition(int num_args, bool is_ctor, intptr_t magic)
{
	file_t*   file;
	long long new_pos;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	new_pos = jsal_to_int(0);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");

	if (!file_seek(file, new_pos, WHENCE_SET))
		jsal_error(JS_ERROR, "couldn't set read/write position");
	return false;
}

static bool
js_RawFile_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object rawfile]");
	return true;
}

static bool
js_RawFile_write(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t* array;
	const void*  data;
	file_t*      file;
	size_t       write_size;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);

	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	if (jsal_is_string(0)) {
		data = jsal_get_lstring(0, &write_size);
	}
	else if ((array = jsal_get_class_obj(0, SV1_BYTE_ARRAY))) {
		data = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	else {
		data = jsal_require_buffer_ptr(0, &write_size);
	}
	if (file_write(file, data, write_size, 1) != write_size)
		jsal_error(JS_ERROR, "couldn't write to file");
	return false;
}

static void
js_Socket_finalize(void* host_ptr)
{
	socket_v1_unref(host_ptr);
}

static bool
js_Socket_close(int num_args, bool is_ctor, intptr_t magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	jsal_set_class_ptr(-1, NULL);
	socket_v1_unref(socket);
	return true;
}

static bool
js_Socket_getPendingReadSize(int num_args, bool is_ctor, intptr_t magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		jsal_error(JS_ERROR, "socket is not connected");
	jsal_push_int(socket_v1_bytes_avail(socket));
	return true;
}

static bool
js_Socket_isConnected(int num_args, bool is_ctor, intptr_t magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (socket != NULL)
		jsal_push_boolean(socket_v1_connected(socket));
	else
		jsal_push_boolean_false();
	return true;
}

static bool
js_Socket_read(int num_args, bool is_ctor, intptr_t magic)
{
	int length = jsal_to_int(0);

	bytearray_t* array;
	void*        read_buffer;
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (length <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid read size");
	if (socket == NULL)
		jsal_error(JS_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		jsal_error(JS_ERROR, "socket is not connected");
	if (!(read_buffer = alloca(length)))
		jsal_error(JS_ERROR, "couldn't allocate read buffer");
	socket_v1_read(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		jsal_error(JS_ERROR, "couldn't create byte array");
	jsal_push_sphere_bytearray(array);
	return true;
}

static bool
js_Socket_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object socket]");
	return true;
}

static bool
js_Socket_write(int num_args, bool is_ctor, intptr_t magic)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_v1_t*   socket;
	size_t         write_size;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (jsal_is_string(0)) {
		payload = (uint8_t*)jsal_get_lstring(0, &write_size);
	}
	else {
		array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
		payload = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	if (socket == NULL)
		jsal_error(JS_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		jsal_error(JS_ERROR, "socket is not connected");
	socket_v1_write(socket, payload, (int)write_size);
	return false;
}

static void
js_Sound_finalize(void* host_ptr)
{
	sound_unref(host_ptr);
}

static bool
js_Sound_getLength(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(floor(sound_len(sound) * 1.0e6));
	return true;
}

static bool
js_Sound_getPan(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_int(sound_pan(sound) * 255);
	return true;
}

static bool
js_Sound_getPitch(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(sound_speed(sound));
	return true;
}

static bool
js_Sound_getPosition(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(floor(sound_tell(sound) * 1.0e6));
	return true;
}

static bool
js_Sound_getRepeat(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_boolean(sound_repeat(sound));
	return true;
}

static bool
js_Sound_getVolume(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_int(sound_gain(sound) * 255);
	return true;
}

static bool
js_Sound_isPlaying(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_boolean(sound_playing(sound));
	return true;
}

static bool
js_Sound_isSeekable(int num_args, bool is_ctor, intptr_t magic)
{
	// all supported formats are seekable, so just return true.
	jsal_push_boolean_true();
	return true;
}

static bool
js_Sound_pause(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_pause(sound, true);
	return false;
}

static bool
js_Sound_play(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	if (num_args >= 1) {
		sound_set_repeat(sound, jsal_to_boolean(0));
		sound_play(sound, s_sound_mixer);
	}
	else {
		sound_pause(sound, false);
		if (!sound_playing(sound))
			sound_play(sound, s_sound_mixer);
	}

	return false;
}

static bool
js_Sound_reset(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_seek(sound, 0.0);
	return false;
}

static bool
js_Sound_setPan(int num_args, bool is_ctor, intptr_t magic)
{
	int      new_pan;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	new_pan = jsal_to_int(0);

	sound_set_pan(sound, (float)new_pan / 255);
	return false;
}

static bool
js_Sound_setPitch(int num_args, bool is_ctor, intptr_t magic)
{
	float    new_pitch;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	new_pitch = jsal_to_number(0);

	sound_set_speed(sound, new_pitch);
	return false;
}

static bool
js_Sound_setPosition(int num_args, bool is_ctor, intptr_t magic)
{
	double   new_pos;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	new_pos = trunc(jsal_to_number(0));

	sound_seek(sound, new_pos / 1.0e6);
	return false;
}

static bool
js_Sound_setRepeat(int num_args, bool is_ctor, intptr_t magic)
{
	bool     is_looped;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	is_looped = jsal_to_boolean(0);

	sound_set_repeat(sound, is_looped);
	return false;
}

static bool
js_Sound_setVolume(int num_args, bool is_ctor, intptr_t magic)
{
	int volume;

	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	volume = jsal_to_int(0);

	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	sound_set_gain(sound, (float)volume / 255);
	return false;
}

static bool
js_Sound_stop(int num_args, bool is_ctor, intptr_t magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_stop(sound);
	return false;
}

static bool
js_Sound_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object sound]");
	return true;
}

static void
js_SoundEffect_finalize(void* host_ptr)
{
	sample_unref(host_ptr);
}

static bool
js_SoundEffect_getPan(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_pan(sample) * 255.0);
	return true;
}

static bool
js_SoundEffect_getPitch(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_speed(sample));
	return true;
}

static bool
js_SoundEffect_getVolume(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_gain(sample) * 255.0);
	return true;
}

static bool
js_SoundEffect_setPan(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;
	float     pan;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);
	pan = jsal_to_number(0);

	sample_set_pan(sample, pan / 255.0);
	return false;
}

static bool
js_SoundEffect_setPitch(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;
	float     pitch;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);
	pitch = jsal_to_number(0);

	sample_set_speed(sample, pitch);
	return false;
}

static bool
js_SoundEffect_setVolume(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;
	float     volume;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);
	volume = jsal_to_number(0);

	sample_set_gain(sample, volume / 255.0);
	return false;
}

static bool
js_SoundEffect_play(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	sample_play(sample, s_sound_mixer);
	return false;
}

static bool
js_SoundEffect_stop(int num_args, bool is_ctor, intptr_t magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	sample_stop_all(sample);
	return false;
}

static bool
js_SoundEffect_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object sound effect]");
	return true;
}

static void
js_Spriteset_finalize(void* host_ptr)
{
	spriteset_unref(host_ptr);
}

static bool
js_Spriteset_get_filename(int num_args, bool is_ctor, intptr_t magic)
{
	path_t*      path;
	spriteset_t* spriteset;

	jsal_push_this();
	spriteset = jsal_require_class_obj(-1, SV1_SPRITESET);

	// returned filename is relative to '@/', even though Spriteset#save() is
	// rooted at '@/spritesets'.  I suspect this to be a bug in Sphere 1.x, but fixing
	// it here would break existing workarounds.
	path = game_relative_path(g_game, spriteset_pathname(spriteset), "@/");
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_Spriteset_clone(int num_args, bool is_ctor, intptr_t magic)
{
	spriteset_t* new_spriteset;
	spriteset_t* spriteset;

	jsal_push_this();
	spriteset = jsal_require_class_obj(-1, SV1_SPRITESET);

	if ((new_spriteset = spriteset_clone(spriteset)) == NULL)
		jsal_error(JS_ERROR, "couldn't clone spriteset");
	jsal_push_sphere_spriteset(new_spriteset);
	spriteset_unref(new_spriteset);
	return true;
}

static bool
js_Spriteset_save(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  filename;
	spriteset_t* spriteset;

	jsal_push_this();
	spriteset = jsal_require_sphere_spriteset(-1);
	filename = jsal_require_pathname(0, "spritesets", true, true);

	if (!spriteset_save(spriteset, filename))
		jsal_error(JS_ERROR, "couldn't save spriteset");
	spriteset_unref(spriteset);
	return false;
}

static bool
js_Spriteset_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object spriteset]");
	return true;
}

static void
js_Surface_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Surface_get_height(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if (image != NULL)
		jsal_push_int(image_height(image));
	else
		jsal_push_int(screen_size(g_screen).height);
	return true;
}

static bool
js_Surface_get_width(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if (image != NULL)
		jsal_push_int(image_width(image));
	else
		jsal_push_int(screen_size(g_screen).width);
	return true;
}

static bool
js_Surface_applyColorFX(int num_args, bool is_ctor, intptr_t magic)
{
	int        height;
	image_t*   image;
	color_fx_t matrix;
	int        width;
	int        x;
	int        y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	matrix = jsal_require_sphere_color_fx(4);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_color_fx(image, matrix, x, y, width, height))
		jsal_error(JS_ERROR, "Couldn't apply color FX");
	return false;
}

static bool
js_Surface_applyColorFX4(int num_args, bool is_ctor, intptr_t magic)
{
	int        height;
	image_t*   image;
	color_fx_t ll_mat;
	color_fx_t lr_mat;
	color_fx_t ul_mat;
	color_fx_t ur_mat;
	int        width;
	int        x;
	int        y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	ul_mat = jsal_require_sphere_color_fx(4);
	ur_mat = jsal_require_sphere_color_fx(5);
	ll_mat = jsal_require_sphere_color_fx(6);
	lr_mat = jsal_require_sphere_color_fx(7);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_color_fx_4(image, ul_mat, ur_mat, ll_mat, lr_mat, x, y, width, height))
		jsal_error(JS_ERROR, "Couldn't apply color FX");
	return false;
}

static bool
js_Surface_applyLookup(int num_args, bool is_ctor, intptr_t magic)
{
	uint8_t* alpha_lu;
	uint8_t* blue_lu;
	uint8_t* green_lu;
	int      height;
	image_t* image;
	uint8_t* red_lu;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	red_lu = jsal_require_rgba_lut(4);
	green_lu = jsal_require_rgba_lut(5);
	blue_lu = jsal_require_rgba_lut(6);
	alpha_lu = jsal_require_rgba_lut(7);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_lookup(image, x, y, width, height, red_lu, green_lu, blue_lu, alpha_lu))
		jsal_error(JS_ERROR, "Couldn't apply color FX");
	free(red_lu);
	free(green_lu);
	free(blue_lu);
	free(alpha_lu);
	return false;
}

static bool
js_Surface_bezierCurve(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	float           cp[8];
	image_t*        image;
	int             num_points;
	bool            quadratic = true;
	double          step_size;
	ALLEGRO_VERTEX* vertices;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	color = jsal_require_sphere_color(0);
	step_size = jsal_to_number(1);
	x1 = jsal_to_number(2);
	y1 = jsal_to_number(3);
	x2 = jsal_to_number(4);
	y2 = jsal_to_number(5);
	x3 = jsal_to_number(6);
	y3 = jsal_to_number(7);
	if (num_args >= 10) {
		quadratic = false;
		x4 = jsal_to_number(8);
		y4 = jsal_to_number(9);
	}

	cp[0] = x1; cp[1] = y1;
	cp[2] = x2; cp[3] = y2;
	cp[4] = x3; cp[5] = y3;
	cp[6] = x4; cp[7] = y4;
	if (quadratic) {
		// convert quadratic Bezier curve to cubic
		cp[6] = x3; cp[7] = y3;
		cp[2] = x1 + (2.0 / 3.0) * (x2 - x1);
		cp[3] = y1 + (2.0 / 3.0) * (y2 - y1);
		cp[4] = x3 + (2.0 / 3.0) * (x2 - x3);
		cp[5] = y3 + (2.0 / 3.0) * (y2 - y3);
	}
	step_size = step_size < 0.001 ? 0.001 : step_size > 1.0 ? 1.0 : step_size;
	num_points = 1.0 / step_size;
	vertices = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	memset(vertices, 0, num_points * sizeof(ALLEGRO_VERTEX));
	al_calculate_spline(&vertices[0].x, sizeof(ALLEGRO_VERTEX), cp, 0.0, num_points);
	for (i = 0; i < num_points; ++i)
		vertices[i].color = nativecolor(color);
	image_render_to(image, NULL);
	al_draw_prim(vertices, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	return false;
}

static bool
js_Surface_blit(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));

	if (screen_skipping_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return false;
}

static bool
js_Surface_blitMaskSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	color_t  mask;
	image_t* src_image;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	src_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));
	mask = jsal_require_sphere_color(3);

	image_render_to(image, NULL);
	al_draw_tinted_bitmap(image_bitmap(src_image), nativecolor(mask), x, y, 0x0);
	return false;
}

static bool
js_Surface_blitSurface(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	image_t* src_image;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	src_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));

	image_render_to(image, NULL);
	al_draw_bitmap(image_bitmap(src_image), x, y, 0x0);
	return false;
}

static bool
js_Surface_clone(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if ((new_image = image_dup(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create new surface");
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Surface_cloneSection(int num_args, bool is_ctor, intptr_t magic)
{
	int      height;
	image_t* image;
	image_t* new_image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	if (!(new_image = image_new(width, height, NULL)))
		jsal_error(JS_ERROR, "couldn't create surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap_region(image_bitmap(image), x, y, width, height, 0, 0, 0x0);
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Surface_createImage(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if ((new_image = image_dup(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create image");
	jsal_push_class_obj(SV1_IMAGE, new_image, false);
	return true;
}

static bool
js_Surface_drawText(int num_args, bool is_ctor, intptr_t magic)
{
	font_t*     font;
	image_t*    image;
	const char* text;
	int         x;
	int         y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	font = jsal_require_class_obj(0, SV1_FONT);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	text = jsal_to_string(3);

	image_render_to(image, NULL);
	font_draw_text(font, x, y, TEXT_ALIGN_LEFT, text);
	return false;
}

static bool
js_Surface_filledCircle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    radius;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	color = jsal_require_sphere_color(3);

	image_render_to(image, NULL);
	al_draw_filled_circle(x, y, radius, nativecolor(color));
	return false;
}

static bool
js_Surface_filledEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    radius_y;
	float    radius_x;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius_x = trunc(jsal_to_number(2));
	radius_y = trunc(jsal_to_number(3));
	color = jsal_require_sphere_color(4);

	image_render_to(image, NULL);
	al_draw_filled_ellipse(x, y, radius_x, radius_y, nativecolor(color));
	return false;
}

static bool
js_Surface_flipHorizontally(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	image_flip(image, true, false);
	return false;
}

static bool
js_Surface_flipVertically(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	image_flip(image, false, true);
	return false;
}

static bool
js_Surface_getPixel(int num_args, bool is_ctor, intptr_t magic)
{
	int      height;
	image_t* image;
	color_t  pixel;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);

	width = image_width(image);
	height = image_height(image);
	if (x < 0 || x >= width || y < 0 || y >= height)
		jsal_error(JS_RANGE_ERROR, "X/Y out of bounds");
	pixel = image_get_pixel(image, x, y);
	jsal_push_sphere_color(pixel);
	return true;
}

static bool
js_Surface_gradientCircle(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*        image;
	color_t         inner_color;
	int             num_points;
	color_t         outer_color;
	double          phi;
	float           radius;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	inner_color = jsal_require_sphere_color(3);
	outer_color = jsal_require_sphere_color(4);

	num_points = fmin(radius, 126);
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(inner_color);
	for (i = 0; i < num_points; ++i) {
		phi = 2.0 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius;
		vertices[i + 1].y = y - sinf(phi) * radius;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(outer_color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius;
	vertices[i + 1].y = y - sinf(0.0f) * radius;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(outer_color);
	image_render_to(image, NULL);
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_Surface_gradientEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	image_t*        image;
	color_t         inner_color;
	int             num_points;
	color_t         outer_color;
	double          phi;
	float           radius_x;
	float           radius_y;
	ALLEGRO_VERTEX* vertices;
	float           x;
	float           y;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius_x = trunc(jsal_to_number(2));
	radius_y = trunc(jsal_to_number(3));
	inner_color = jsal_require_sphere_color(4);
	outer_color = jsal_require_sphere_color(5);

	num_points = ceil(fmin(10 * sqrt((radius_x + radius_y) / 2), 126));
	vertices = alloca((num_points + 2) * sizeof(ALLEGRO_VERTEX));
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0.0f;
	vertices[0].color = nativecolor(inner_color);
	for (i = 0; i < num_points; ++i) {
		phi = 2.0 * M_PI * i / num_points;
		vertices[i + 1].x = x + cosf(phi) * radius_x;
		vertices[i + 1].y = y - sinf(phi) * radius_y;
		vertices[i + 1].z = 0.0f;
		vertices[i + 1].color = nativecolor(outer_color);
	}
	vertices[i + 1].x = x + cosf(0.0f) * radius_x;
	vertices[i + 1].y = y - sinf(0.0f) * radius_y;
	vertices[i + 1].z = 0.0f;
	vertices[i + 1].color = nativecolor(outer_color);
	image_render_to(image, NULL);
	al_draw_prim(vertices, NULL, NULL, 0, num_points + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_Surface_gradientRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color_ll;
	color_t  color_lr;
	color_t  color_ul;
	color_t  color_ur;
	image_t* image;
	float    x1;
	float    x2;
	float    y1;
	float    y2;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x1 = trunc(jsal_to_number(0));
	y1 = trunc(jsal_to_number(1));
	x2 = x1 + trunc(jsal_to_number(2));
	y2 = y1 + trunc(jsal_to_number(3));
	color_ul = jsal_require_sphere_color(4);
	color_ur = jsal_require_sphere_color(5);
	color_lr = jsal_require_sphere_color(6);
	color_ll = jsal_require_sphere_color(7);

	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	image_render_to(image, NULL);
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Surface_gradientLine(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color1;
	color_t  color2;
	image_t* image;
	float    length;
	float    tx, ty;
	float    x1;
	float    x2;
	float    y1;
	float    y2;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x1 = jsal_to_int(0);
	y1 = jsal_to_int(1);
	x2 = jsal_to_int(2);
	y2 = jsal_to_int(3);
	color1 = jsal_require_sphere_color(4);
	color2 = jsal_require_sphere_color(5);

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
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
	return false;
}

static bool
js_Surface_line(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    x1;
	float    x2;
	float    y1;
	float    y2;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x1 = trunc(jsal_to_number(0)) + 0.5;
	y1 = trunc(jsal_to_number(1)) + 0.5;
	x2 = trunc(jsal_to_number(2)) + 0.5;
	y2 = trunc(jsal_to_number(3)) + 0.5;
	color = jsal_require_sphere_color(4);

	image_render_to(image, NULL);
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return false;
}

static bool
js_Surface_lineSeries(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	image_t*        image;
	int             num_points;
	int             type = LINE_MULTIPLE;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;
	float           x;
	float           y;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_require_array(0);
	color = jsal_require_sphere_color(1);
	if (num_args >= 3)
		type = jsal_to_int(2);

	if ((num_points = jsal_get_length(0)) < 2)
		jsal_error(JS_RANGE_ERROR, "two or more vertices required");
	vertices = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x");
		jsal_get_prop_string(-2, "y");
		x = trunc(jsal_to_number(-2));
		y = trunc(jsal_to_number(-1));
		jsal_pop(3);
		vertices[i].x = x + 0.5f;
		vertices[i].y = y + 0.5f;
		vertices[i].z = 0.0f;
		vertices[i].u = 0.0f;
		vertices[i].v = 0.0f;
		vertices[i].color = vtx_color;
	}
	image_render_to(image, NULL);
	al_draw_prim(vertices, NULL, NULL, 0, num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
			: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
			: ALLEGRO_PRIM_LINE_LIST);
	return false;
}

static bool
js_Surface_outlinedCircle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    radius;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	color = jsal_require_sphere_color(3);

	image_render_to(image, NULL);
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	return false;
}

static bool
js_Surface_outlinedEllipse(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    radius_x;
	float    radius_y;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius_x = trunc(jsal_to_number(2));
	radius_y = trunc(jsal_to_number(3));
	color = jsal_require_sphere_color(4);

	image_render_to(image, NULL);
	al_draw_ellipse(x, y, radius_x, radius_y, nativecolor(color), 1.0);
	return false;
}

static bool
js_Surface_pointSeries(int num_args, bool is_ctor, intptr_t magic)
{
	color_t         color;
	image_t*        image;
	int             num_points;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;
	float           x;
	float           y;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_require_array(0);
	color = jsal_require_sphere_color(1);

	num_points = jsal_get_length(0);
	vertices = alloca(num_points * sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x");
		jsal_get_prop_string(-2, "y");
		x = trunc(jsal_to_number(-2));
		y = trunc(jsal_to_number(-1));
		jsal_pop(3);
		vertices[i].x = x + 0.5f;
		vertices[i].y = y + 0.5f;
		vertices[i].z = 0.0f;
		vertices[i].u = 0.0f;
		vertices[i].v = 0.0f;
		vertices[i].color = vtx_color;
	}
	image_render_to(image, NULL);
	al_draw_prim(vertices, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	return false;
}

static bool
js_Surface_outlinedRectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	float    thickness = 1.0f;
	float    x1;
	float    y1;
	float    x2;
	float    y2;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x1 = trunc(jsal_to_number(0)) + 0.5;
	y1 = trunc(jsal_to_number(1)) + 0.5;
	x2 = x1 + trunc(jsal_to_number(2)) - 1.0;
	y2 = y1 + trunc(jsal_to_number(3)) - 1.0;
	color = jsal_require_sphere_color(4);
	if (num_args >= 6)
		thickness = trunc(jsal_to_number(5));

	image_render_to(image, NULL);
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return false;
}

static bool
js_Surface_replaceColor(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	color_t  new_color;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	color = jsal_require_sphere_color(0);
	new_color = jsal_require_sphere_color(1);

	if (!image_replace_color(image, color, new_color))
		jsal_error(JS_ERROR, "couldn't perform color replacement");
	return false;
}

static bool
js_Surface_rescale(int num_args, bool is_ctor, intptr_t magic)
{
	int      height;
	image_t* image;
	int      width;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	width = jsal_to_int(0);
	height = jsal_to_int(1);

	if (!image_rescale(image, width, height))
		jsal_error(JS_ERROR, "couldn't rescale image");
	jsal_push_this();
	return true;
}

static bool
js_Surface_rotate(int num_args, bool is_ctor, intptr_t magic)
{
	float    angle;
	float    height;
	image_t* image;
	float    new_height;
	image_t* new_image;
	float    new_width;
	float    width;
	bool     want_resize = false;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	angle = jsal_to_number(0);
	if (num_args >= 2)
		want_resize = jsal_to_boolean(1);

	width = new_width = image_width(image);
	height = new_height = image_height(image);
	if (want_resize) {
		// FIXME: implement in-place resizing for Surface#rotate()
		jsal_error(JS_ERROR, "Resizing not implemented for Surface#rotate()");
	}
	if (!(new_image = image_new(new_width, new_height, NULL)))
		jsal_error(JS_ERROR, "Couldn't create new surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_rotated_bitmap(image_bitmap(image), width / 2, height / 2, new_width / 2, new_height / 2, angle, 0x0);

	// swap out the image pointer and free old image
	jsal_set_class_ptr(-1, new_image);
	image_unref(image);
	return true;
}

static bool
js_Surface_rotateBlitMaskSurface(int num_args, bool is_ctor, intptr_t magic)
{
	float    angle;
	float    height;
	image_t* image;
	color_t  mask;
	image_t* source_image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));
	angle = jsal_to_number(3);
	mask = jsal_require_sphere_color(4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	al_draw_tinted_rotated_bitmap(image_bitmap(source_image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Surface_rotateBlitSurface(int num_args, bool is_ctor, intptr_t magic)
{
	float    angle;
	float    height;
	image_t* image;
	image_t* source_image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));
	angle = jsal_to_number(3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	al_draw_rotated_bitmap(image_bitmap(source_image), width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Surface_rectangle(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	float    height;
	image_t* image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	width = trunc(jsal_to_number(2));
	height = trunc(jsal_to_number(3));
	color = jsal_require_sphere_color(4);

	image_render_to(image, NULL);
	al_draw_filled_rectangle(x, y, x + width, y + height, nativecolor(color));
	return false;
}

static bool
js_Surface_save(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	image_t*    image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	filename = jsal_require_pathname(0, "images", true, false);
	image_save(image, filename);
	return true;
}

static bool
js_Surface_setAlpha(int num_args, bool is_ctor, intptr_t magic)
{
	int n_args = jsal_get_top();
	int a = jsal_to_int(0);
	bool want_all = n_args >= 2 ? jsal_to_boolean(1) : true;

	image_t*      image;
	image_lock_t* lock;
	color_t*      pixel;
	int           w, h;

	int i_x, i_y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if (!(lock = image_lock(image, true, true)))
		jsal_error(JS_ERROR, "Couldn't lock surface");
	w = image_width(image);
	h = image_height(image);
	a = a < 0 ? 0 : a > 255 ? 255 : a;
	for (i_y = h - 1; i_y >= 0; --i_y) for (i_x = w - 1; i_x >= 0; --i_x) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		pixel->a = want_all || pixel->a != 0
			? a : pixel->a;
	}
	image_unlock(image, lock);
	return false;
}

static bool
js_Surface_setBlendMode(int num_args, bool is_ctor, intptr_t magic)
{
	int         blend_mode;
	image_t*    image;
	blend_op_t* op;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	blend_mode = jsal_to_int(0);

	if (blend_mode < 0 || blend_mode >= BLEND_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid blend mode constant '%d'", blend_mode);

	op = blend_mode == BLEND_NORMAL ? s_blender_normal
		: blend_mode == BLEND_ADD ? s_blender_add
		: blend_mode == BLEND_COPY_ALPHA ? s_blender_copy_alpha
		: blend_mode == BLEND_COPY_RGB ? s_blender_copy_rgb
		: blend_mode == BLEND_MULTIPLY ? s_blender_multiply
		: blend_mode == BLEND_REPLACE ? s_blender_copy
		: blend_mode == BLEND_SUBTRACT ? s_blender_subtract
		: s_blender_null;
	image_set_blend_op(image, op);
	return false;
}

static bool
js_Surface_setPixel(int num_args, bool is_ctor, intptr_t magic)
{
	color_t  color;
	image_t* image;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	color = jsal_require_sphere_color(2);

	image_set_pixel(image, x, y, color);
	return false;
}

static bool
js_Surface_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object surface]");
	return true;
}

static bool
js_Surface_transformBlitMaskSurface(int num_args, bool is_ctor, intptr_t magic)
{
	int      height;
	image_t* image;
	color_t  mask;
	image_t* source_image;
	int      width;
	float    x1, x2, x3, x4;
	float    y1, y2, y3, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x1 = trunc(jsal_to_number(1));
	y1 = trunc(jsal_to_number(2));
	x2 = trunc(jsal_to_number(3));
	y2 = trunc(jsal_to_number(4));
	x3 = trunc(jsal_to_number(5));
	y3 = trunc(jsal_to_number(6));
	x4 = trunc(jsal_to_number(7));
	y4 = trunc(jsal_to_number(8));
	mask = jsal_require_sphere_color(9);

	width = image_width(source_image);
	height = image_height(source_image);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, nativecolor(mask) },
		{ x2, y2, 0, width, 0, nativecolor(mask) },
		{ x4, y4, 0, 0, height, nativecolor(mask) },
		{ x3, y3, 0, width, height, nativecolor(mask) },
	};
	image_render_to(image, NULL);
	al_draw_prim(v, NULL, image_bitmap(source_image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Surface_transformBlitSurface(int num_args, bool is_ctor, intptr_t magic)
{
	float    height;
	image_t* image;
	image_t* source_image;
	float    width;
	float    x1, x2, x3, x4;
	float    y1, y2, y3, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x1 = trunc(jsal_to_number(1));
	y1 = trunc(jsal_to_number(2));
	x2 = trunc(jsal_to_number(3));
	y2 = trunc(jsal_to_number(4));
	x3 = trunc(jsal_to_number(5));
	y3 = trunc(jsal_to_number(6));
	x4 = trunc(jsal_to_number(7));
	y4 = trunc(jsal_to_number(8));

	width = image_width(source_image);
	height = image_height(source_image);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, al_map_rgba(255, 255, 255, 255) },
		{ x2, y2, 0, width, 0, al_map_rgba(255, 255, 255, 255) },
		{ x4, y4, 0, 0, height, al_map_rgba(255, 255, 255, 255) },
		{ x3, y3, 0, width, height, al_map_rgba(255, 255, 255, 255) },
	};
	image_render_to(image, NULL);
	al_draw_prim(v, NULL, image_bitmap(source_image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Surface_zoomBlitMaskSurface(int num_args, bool is_ctor, intptr_t magic)
{
	float    height;
	image_t* image;
	color_t  mask;
	float    scale;
	image_t* source_image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));
	scale = jsal_to_number(3);
	mask = jsal_require_sphere_color(4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	al_draw_tinted_scaled_bitmap(image_bitmap(source_image),
		nativecolor(mask),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	return false;
}

static bool
js_Surface_zoomBlitSurface(int num_args, bool is_ctor, intptr_t magic)
{
	float    height;
	image_t* image;
	float    scale;
	image_t* source_image;
	float    width;
	float    x;
	float    y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = trunc(jsal_to_number(1));
	y = trunc(jsal_to_number(2));
	scale = jsal_to_number(3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	al_draw_scaled_bitmap(image_bitmap(source_image),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	return false;
}

static void
js_WindowStyle_finalize(void* host_ptr)
{
	winstyle_unref(host_ptr);
}

static bool
js_WindowStyle_drawWindow(int num_args, bool is_ctor, intptr_t magic)
{
	int            height;
	int            width;
	windowstyle_t* winstyle;
	int            x;
	int            y;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	galileo_reset();
	winstyle_draw(winstyle, x, y, width, height);
	return false;
}

static bool
js_WindowStyle_getColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	windowstyle_t* winstyle;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);

	jsal_push_sphere_color(winstyle_get_mask(winstyle));
	return true;
}

static bool
js_WindowStyle_setColorMask(int num_args, bool is_ctor, intptr_t magic)
{
	color_t        mask;
	windowstyle_t* winstyle;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);
	mask = jsal_require_sphere_color(0);

	winstyle_set_mask(winstyle, mask);
	return false;
}

static bool
js_WindowStyle_toString(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_string("[object windowstyle]");
	return true;
}
