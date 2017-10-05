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
#include "jsal.h"
#include "legacy.h"
#include "logger.h"
#include "map_engine.h"
#include "spriteset.h"
#include "windowstyle.h"

#define API_VERSION        2.0
#define API_VERSION_STRING "v2.0"

static bool js_Abort                            (int num_args, bool is_ctor, int magic);
static bool js_AddTrigger                       (int num_args, bool is_ctor, int magic);
static bool js_AddZone                          (int num_args, bool is_ctor, int magic);
static bool js_ApplyColorMask                   (int num_args, bool is_ctor, int magic);
static bool js_AreKeysLeft                      (int num_args, bool is_ctor, int magic);
static bool js_AreZonesAt                       (int num_args, bool is_ctor, int magic);
static bool js_AttachCamera                     (int num_args, bool is_ctor, int magic);
static bool js_AttachInput                      (int num_args, bool is_ctor, int magic);
static bool js_AttachPlayerInput                (int num_args, bool is_ctor, int magic);
static bool js_BezierCurve                      (int num_args, bool is_ctor, int magic);
static bool js_BindJoystickButton               (int num_args, bool is_ctor, int magic);
static bool js_BindKey                          (int num_args, bool is_ctor, int magic);
static bool js_BlendColors                      (int num_args, bool is_ctor, int magic);
static bool js_CallDefaultMapScript             (int num_args, bool is_ctor, int magic);
static bool js_CallDefaultPersonScript          (int num_args, bool is_ctor, int magic);
static bool js_CallMapScript                    (int num_args, bool is_ctor, int magic);
static bool js_CallPersonScript                 (int num_args, bool is_ctor, int magic);
static bool js_ChangeMap                        (int num_args, bool is_ctor, int magic);
static bool js_ClearPersonCommands              (int num_args, bool is_ctor, int magic);
static bool js_CreatePerson                     (int num_args, bool is_ctor, int magic);
static bool js_CreateByteArray                  (int num_args, bool is_ctor, int magic);
static bool js_CreateByteArrayFromString        (int num_args, bool is_ctor, int magic);
static bool js_CreateColor                      (int num_args, bool is_ctor, int magic);
static bool js_CreateColorMatrix                (int num_args, bool is_ctor, int magic);
static bool js_CreateDirectory                  (int num_args, bool is_ctor, int magic);
static bool js_CreateSpriteset                  (int num_args, bool is_ctor, int magic);
static bool js_CreateStringFromByteArray        (int num_args, bool is_ctor, int magic);
static bool js_CreateStringFromCode             (int num_args, bool is_ctor, int magic);
static bool js_CreateSurface                    (int num_args, bool is_ctor, int magic);
static bool js_DeflateByteArray                 (int num_args, bool is_ctor, int magic);
static bool js_DestroyPerson                    (int num_args, bool is_ctor, int magic);
static bool js_DetachCamera                     (int num_args, bool is_ctor, int magic);
static bool js_DetachInput                      (int num_args, bool is_ctor, int magic);
static bool js_DetachPlayerInput                (int num_args, bool is_ctor, int magic);
static bool js_DoEvents                         (int num_args, bool is_ctor, int magic);
static bool js_DoesFileExist                    (int num_args, bool is_ctor, int magic);
static bool js_DoesPersonExist                  (int num_args, bool is_ctor, int magic);
static bool js_Delay                            (int num_args, bool is_ctor, int magic);
static bool js_EvaluateScript                   (int num_args, bool is_ctor, int magic);
static bool js_EvaluateSystemScript             (int num_args, bool is_ctor, int magic);
static bool js_ExecuteGame                      (int num_args, bool is_ctor, int magic);
static bool js_ExecuteTrigger                   (int num_args, bool is_ctor, int magic);
static bool js_ExecuteZoneScript                (int num_args, bool is_ctor, int magic);
static bool js_ExecuteZones                     (int num_args, bool is_ctor, int magic);
static bool js_Exit                             (int num_args, bool is_ctor, int magic);
static bool js_ExitMapEngine                    (int num_args, bool is_ctor, int magic);
static bool js_FilledCircle                     (int num_args, bool is_ctor, int magic);
static bool js_FilledComplex                    (int num_args, bool is_ctor, int magic);
static bool js_FilledEllipse                    (int num_args, bool is_ctor, int magic);
static bool js_FlipScreen                       (int num_args, bool is_ctor, int magic);
static bool js_FollowPerson                     (int num_args, bool is_ctor, int magic);
static bool js_GarbageCollect                   (int num_args, bool is_ctor, int magic);
static bool js_GetActingPerson                  (int num_args, bool is_ctor, int magic);
static bool js_GetCameraPerson                  (int num_args, bool is_ctor, int magic);
static bool js_GetCameraX                       (int num_args, bool is_ctor, int magic);
static bool js_GetCameraY                       (int num_args, bool is_ctor, int magic);
static bool js_GetClippingRectangle             (int num_args, bool is_ctor, int magic);
static bool js_GetCurrentMap                    (int num_args, bool is_ctor, int magic);
static bool js_GetCurrentPerson                 (int num_args, bool is_ctor, int magic);
static bool js_GetCurrentTrigger                (int num_args, bool is_ctor, int magic);
static bool js_GetCurrentZone                   (int num_args, bool is_ctor, int magic);
static bool js_GetDirectoryList                 (int num_args, bool is_ctor, int magic);
static bool js_GetFileList                      (int num_args, bool is_ctor, int magic);
static bool js_GetFrameRate                     (int num_args, bool is_ctor, int magic);
static bool js_GetGameList                      (int num_args, bool is_ctor, int magic);
static bool js_GetInputPerson                   (int num_args, bool is_ctor, int magic);
static bool js_GetJoystickAxis                  (int num_args, bool is_ctor, int magic);
static bool js_GetKey                           (int num_args, bool is_ctor, int magic);
static bool js_GetKeyString                     (int num_args, bool is_ctor, int magic);
static bool js_GetLayerAngle                    (int num_args, bool is_ctor, int magic);
static bool js_GetLayerHeight                   (int num_args, bool is_ctor, int magic);
static bool js_GetLayerMask                     (int num_args, bool is_ctor, int magic);
static bool js_GetLayerName                     (int num_args, bool is_ctor, int magic);
static bool js_GetLayerWidth                    (int num_args, bool is_ctor, int magic);
static bool js_GetLocalAddress                  (int num_args, bool is_ctor, int magic);
static bool js_GetLocalName                     (int num_args, bool is_ctor, int magic);
static bool js_GetMapEngine                     (int num_args, bool is_ctor, int magic);
static bool js_GetMapEngineFrameRate            (int num_args, bool is_ctor, int magic);
static bool js_GetMouseWheelEvent               (int num_args, bool is_ctor, int magic);
static bool js_GetMouseX                        (int num_args, bool is_ctor, int magic);
static bool js_GetMouseY                        (int num_args, bool is_ctor, int magic);
static bool js_GetNextAnimatedTile              (int num_args, bool is_ctor, int magic);
static bool js_GetNumJoysticks                  (int num_args, bool is_ctor, int magic);
static bool js_GetNumJoystickAxes               (int num_args, bool is_ctor, int magic);
static bool js_GetNumJoystickButtons            (int num_args, bool is_ctor, int magic);
static bool js_GetNumLayers                     (int num_args, bool is_ctor, int magic);
static bool js_GetNumMouseWheelEvents           (int num_args, bool is_ctor, int magic);
static bool js_GetNumTiles                      (int num_args, bool is_ctor, int magic);
static bool js_GetNumTriggers                   (int num_args, bool is_ctor, int magic);
static bool js_GetNumZones                      (int num_args, bool is_ctor, int magic);
static bool js_GetObstructingPerson             (int num_args, bool is_ctor, int magic);
static bool js_GetObstructingTile               (int num_args, bool is_ctor, int magic);
static bool js_GetPersonAngle                   (int num_args, bool is_ctor, int magic);
static bool js_GetPersonBase                    (int num_args, bool is_ctor, int magic);
static bool js_GetPersonData                    (int num_args, bool is_ctor, int magic);
static bool js_GetPersonDirection               (int num_args, bool is_ctor, int magic);
static bool js_GetPersonFollowDistance          (int num_args, bool is_ctor, int magic);
static bool js_GetPersonFollowers               (int num_args, bool is_ctor, int magic);
static bool js_GetPersonFrame                   (int num_args, bool is_ctor, int magic);
static bool js_GetPersonFrameNext               (int num_args, bool is_ctor, int magic);
static bool js_GetPersonFrameRevert             (int num_args, bool is_ctor, int magic);
static bool js_GetPersonIgnoreList              (int num_args, bool is_ctor, int magic);
static bool js_GetPersonLayer                   (int num_args, bool is_ctor, int magic);
static bool js_GetPersonLeader                  (int num_args, bool is_ctor, int magic);
static bool js_GetPersonList                    (int num_args, bool is_ctor, int magic);
static bool js_GetPersonMask                    (int num_args, bool is_ctor, int magic);
static bool js_GetPersonOffsetX                 (int num_args, bool is_ctor, int magic);
static bool js_GetPersonOffsetY                 (int num_args, bool is_ctor, int magic);
static bool js_GetPersonSpeedX                  (int num_args, bool is_ctor, int magic);
static bool js_GetPersonSpeedY                  (int num_args, bool is_ctor, int magic);
static bool js_GetPersonSpriteset               (int num_args, bool is_ctor, int magic);
static bool js_GetPersonValue                   (int num_args, bool is_ctor, int magic);
static bool js_GetPersonX                       (int num_args, bool is_ctor, int magic);
static bool js_GetPersonY                       (int num_args, bool is_ctor, int magic);
static bool js_GetPersonXFloat                  (int num_args, bool is_ctor, int magic);
static bool js_GetPersonYFloat                  (int num_args, bool is_ctor, int magic);
static bool js_GetPlayerKey                     (int num_args, bool is_ctor, int magic);
static bool js_GetScreenHeight                  (int num_args, bool is_ctor, int magic);
static bool js_GetScreenWidth                   (int num_args, bool is_ctor, int magic);
static bool js_GetSystemArrow                   (int num_args, bool is_ctor, int magic);
static bool js_GetSystemDownArrow               (int num_args, bool is_ctor, int magic);
static bool js_GetSystemFont                    (int num_args, bool is_ctor, int magic);
static bool js_GetSystemUpArrow                 (int num_args, bool is_ctor, int magic);
static bool js_GetSystemWindowStyle             (int num_args, bool is_ctor, int magic);
static bool js_GetTalkActivationButton          (int num_args, bool is_ctor, int magic);
static bool js_GetTalkActivationKey             (int num_args, bool is_ctor, int magic);
static bool js_GetTalkDistance                  (int num_args, bool is_ctor, int magic);
static bool js_GetTile                          (int num_args, bool is_ctor, int magic);
static bool js_GetTileDelay                     (int num_args, bool is_ctor, int magic);
static bool js_GetTileHeight                    (int num_args, bool is_ctor, int magic);
static bool js_GetTileImage                     (int num_args, bool is_ctor, int magic);
static bool js_GetTileName                      (int num_args, bool is_ctor, int magic);
static bool js_GetTileSurface                   (int num_args, bool is_ctor, int magic);
static bool js_GetTileWidth                     (int num_args, bool is_ctor, int magic);
static bool js_GetTime                          (int num_args, bool is_ctor, int magic);
static bool js_GetToggleState                   (int num_args, bool is_ctor, int magic);
static bool js_GetTriggerLayer                  (int num_args, bool is_ctor, int magic);
static bool js_GetTriggerX                      (int num_args, bool is_ctor, int magic);
static bool js_GetTriggerY                      (int num_args, bool is_ctor, int magic);
static bool js_GetVersion                       (int num_args, bool is_ctor, int magic);
static bool js_GetVersionString                 (int num_args, bool is_ctor, int magic);
static bool js_GetZoneHeight                    (int num_args, bool is_ctor, int magic);
static bool js_GetZoneLayer                     (int num_args, bool is_ctor, int magic);
static bool js_GetZoneSteps                     (int num_args, bool is_ctor, int magic);
static bool js_GetZoneWidth                     (int num_args, bool is_ctor, int magic);
static bool js_GetZoneX                         (int num_args, bool is_ctor, int magic);
static bool js_GetZoneY                         (int num_args, bool is_ctor, int magic);
static bool js_GrabImage                        (int num_args, bool is_ctor, int magic);
static bool js_GrabSurface                      (int num_args, bool is_ctor, int magic);
static bool js_GradientCircle                   (int num_args, bool is_ctor, int magic);
static bool js_GradientComplex                  (int num_args, bool is_ctor, int magic);
static bool js_GradientEllipse                  (int num_args, bool is_ctor, int magic);
static bool js_GradientLine                     (int num_args, bool is_ctor, int magic);
static bool js_GradientRectangle                (int num_args, bool is_ctor, int magic);
static bool js_GradientTriangle                 (int num_args, bool is_ctor, int magic);
static bool js_HashByteArray                    (int num_args, bool is_ctor, int magic);
static bool js_HashFromFile                     (int num_args, bool is_ctor, int magic);
static bool js_IgnorePersonObstructions         (int num_args, bool is_ctor, int magic);
static bool js_IgnoreTileObstructions           (int num_args, bool is_ctor, int magic);
static bool js_InflateByteArray                 (int num_args, bool is_ctor, int magic);
static bool js_IsAnyKeyPressed                  (int num_args, bool is_ctor, int magic);
static bool js_IsCameraAttached                 (int num_args, bool is_ctor, int magic);
static bool js_IsCommandQueueEmpty              (int num_args, bool is_ctor, int magic);
static bool js_IsIgnoringPersonObstructions     (int num_args, bool is_ctor, int magic);
static bool js_IsIgnoringTileObstructions       (int num_args, bool is_ctor, int magic);
static bool js_IsInputAttached                  (int num_args, bool is_ctor, int magic);
static bool js_IsJoystickButtonPressed          (int num_args, bool is_ctor, int magic);
static bool js_IsKeyPressed                     (int num_args, bool is_ctor, int magic);
static bool js_IsLayerReflective                (int num_args, bool is_ctor, int magic);
static bool js_IsLayerVisible                   (int num_args, bool is_ctor, int magic);
static bool js_IsMapEngineRunning               (int num_args, bool is_ctor, int magic);
static bool js_IsMouseButtonPressed             (int num_args, bool is_ctor, int magic);
static bool js_IsPersonObstructed               (int num_args, bool is_ctor, int magic);
static bool js_IsPersonVisible                  (int num_args, bool is_ctor, int magic);
static bool js_IsTriggerAt                      (int num_args, bool is_ctor, int magic);
static bool js_Line                             (int num_args, bool is_ctor, int magic);
static bool js_LineSeries                       (int num_args, bool is_ctor, int magic);
static bool js_ListenOnPort                     (int num_args, bool is_ctor, int magic);
static bool js_LoadAnimation                    (int num_args, bool is_ctor, int magic);
static bool js_LoadFont                         (int num_args, bool is_ctor, int magic);
static bool js_LoadImage                        (int num_args, bool is_ctor, int magic);
static bool js_LoadSound                        (int num_args, bool is_ctor, int magic);
static bool js_LoadSoundEffect                  (int num_args, bool is_ctor, int magic);
static bool js_LoadSpriteset                    (int num_args, bool is_ctor, int magic);
static bool js_LoadSurface                      (int num_args, bool is_ctor, int magic);
static bool js_LoadWindowStyle                  (int num_args, bool is_ctor, int magic);
static bool js_MapEngine                        (int num_args, bool is_ctor, int magic);
static bool js_MapToScreenX                     (int num_args, bool is_ctor, int magic);
static bool js_MapToScreenY                     (int num_args, bool is_ctor, int magic);
static bool js_OpenAddress                      (int num_args, bool is_ctor, int magic);
static bool js_OpenFile                         (int num_args, bool is_ctor, int magic);
static bool js_OpenLog                          (int num_args, bool is_ctor, int magic);
static bool js_OpenRawFile                      (int num_args, bool is_ctor, int magic);
static bool js_OutlinedCircle                   (int num_args, bool is_ctor, int magic);
static bool js_OutlinedComplex                  (int num_args, bool is_ctor, int magic);
static bool js_OutlinedEllipse                  (int num_args, bool is_ctor, int magic);
static bool js_OutlinedRectangle                (int num_args, bool is_ctor, int magic);
static bool js_OutlinedRoundRectangle           (int num_args, bool is_ctor, int magic);
static bool js_Point                            (int num_args, bool is_ctor, int magic);
static bool js_PointSeries                      (int num_args, bool is_ctor, int magic);
static bool js_Polygon                          (int num_args, bool is_ctor, int magic);
static bool js_Print                            (int num_args, bool is_ctor, int magic);
static bool js_QueuePersonCommand               (int num_args, bool is_ctor, int magic);
static bool js_QueuePersonScript                (int num_args, bool is_ctor, int magic);
static bool js_Rectangle                        (int num_args, bool is_ctor, int magic);
static bool js_RemoveDirectory                  (int num_args, bool is_ctor, int magic);
static bool js_RemoveFile                       (int num_args, bool is_ctor, int magic);
static bool js_RemoveTrigger                    (int num_args, bool is_ctor, int magic);
static bool js_RemoveZone                       (int num_args, bool is_ctor, int magic);
static bool js_Rename                           (int num_args, bool is_ctor, int magic);
static bool js_RenderMap                        (int num_args, bool is_ctor, int magic);
static bool js_ReplaceTilesOnLayer              (int num_args, bool is_ctor, int magic);
static bool js_RequireScript                    (int num_args, bool is_ctor, int magic);
static bool js_RequireSystemScript              (int num_args, bool is_ctor, int magic);
static bool js_RestartGame                      (int num_args, bool is_ctor, int magic);
static bool js_RoundRectangle                   (int num_args, bool is_ctor, int magic);
static bool js_ScreenToMapX                     (int num_args, bool is_ctor, int magic);
static bool js_ScreenToMapY                     (int num_args, bool is_ctor, int magic);
static bool js_SetCameraX                       (int num_args, bool is_ctor, int magic);
static bool js_SetCameraY                       (int num_args, bool is_ctor, int magic);
static bool js_SetClippingRectangle             (int num_args, bool is_ctor, int magic);
static bool js_SetColorMask                     (int num_args, bool is_ctor, int magic);
static bool js_SetDefaultMapScript              (int num_args, bool is_ctor, int magic);
static bool js_SetDefaultPersonScript           (int num_args, bool is_ctor, int magic);
static bool js_SetDelayScript                   (int num_args, bool is_ctor, int magic);
static bool js_SetFrameRate                     (int num_args, bool is_ctor, int magic);
static bool js_SetLayerAngle                    (int num_args, bool is_ctor, int magic);
static bool js_SetLayerHeight                   (int num_args, bool is_ctor, int magic);
static bool js_SetLayerMask                     (int num_args, bool is_ctor, int magic);
static bool js_SetLayerReflective               (int num_args, bool is_ctor, int magic);
static bool js_SetLayerRenderer                 (int num_args, bool is_ctor, int magic);
static bool js_SetLayerScaleFactorX             (int num_args, bool is_ctor, int magic);
static bool js_SetLayerScaleFactorY             (int num_args, bool is_ctor, int magic);
static bool js_SetLayerSize                     (int num_args, bool is_ctor, int magic);
static bool js_SetLayerVisible                  (int num_args, bool is_ctor, int magic);
static bool js_SetLayerWidth                    (int num_args, bool is_ctor, int magic);
static bool js_SetMapEngineFrameRate            (int num_args, bool is_ctor, int magic);
static bool js_SetMousePosition                 (int num_args, bool is_ctor, int magic);
static bool js_SetNextAnimatedTile              (int num_args, bool is_ctor, int magic);
static bool js_SetPersonAngle                   (int num_args, bool is_ctor, int magic);
static bool js_SetPersonData                    (int num_args, bool is_ctor, int magic);
static bool js_SetPersonDirection               (int num_args, bool is_ctor, int magic);
static bool js_SetPersonFollowDistance          (int num_args, bool is_ctor, int magic);
static bool js_SetPersonFrame                   (int num_args, bool is_ctor, int magic);
static bool js_SetPersonFrameNext               (int num_args, bool is_ctor, int magic);
static bool js_SetPersonFrameRevert             (int num_args, bool is_ctor, int magic);
static bool js_SetPersonIgnoreList              (int num_args, bool is_ctor, int magic);
static bool js_SetPersonLayer                   (int num_args, bool is_ctor, int magic);
static bool js_SetPersonMask                    (int num_args, bool is_ctor, int magic);
static bool js_SetPersonOffsetX                 (int num_args, bool is_ctor, int magic);
static bool js_SetPersonOffsetY                 (int num_args, bool is_ctor, int magic);
static bool js_SetPersonScaleAbsolute           (int num_args, bool is_ctor, int magic);
static bool js_SetPersonScaleFactor             (int num_args, bool is_ctor, int magic);
static bool js_SetPersonScript                  (int num_args, bool is_ctor, int magic);
static bool js_SetPersonSpeed                   (int num_args, bool is_ctor, int magic);
static bool js_SetPersonSpeedXY                 (int num_args, bool is_ctor, int magic);
static bool js_SetPersonSpriteset               (int num_args, bool is_ctor, int magic);
static bool js_SetPersonValue                   (int num_args, bool is_ctor, int magic);
static bool js_SetPersonVisible                 (int num_args, bool is_ctor, int magic);
static bool js_SetPersonX                       (int num_args, bool is_ctor, int magic);
static bool js_SetPersonXYFloat                 (int num_args, bool is_ctor, int magic);
static bool js_SetPersonY                       (int num_args, bool is_ctor, int magic);
static bool js_SetRenderScript                  (int num_args, bool is_ctor, int magic);
static bool js_SetTalkActivationButton          (int num_args, bool is_ctor, int magic);
static bool js_SetTalkActivationKey             (int num_args, bool is_ctor, int magic);
static bool js_SetTalkDistance                  (int num_args, bool is_ctor, int magic);
static bool js_SetTile                          (int num_args, bool is_ctor, int magic);
static bool js_SetTileDelay                     (int num_args, bool is_ctor, int magic);
static bool js_SetTileImage                     (int num_args, bool is_ctor, int magic);
static bool js_SetTileName                      (int num_args, bool is_ctor, int magic);
static bool js_SetTileSurface                   (int num_args, bool is_ctor, int magic);
static bool js_SetTriggerLayer                  (int num_args, bool is_ctor, int magic);
static bool js_SetTriggerScript                 (int num_args, bool is_ctor, int magic);
static bool js_SetTriggerXY                     (int num_args, bool is_ctor, int magic);
static bool js_SetUpdateScript                  (int num_args, bool is_ctor, int magic);
static bool js_SetZoneDimensions                (int num_args, bool is_ctor, int magic);
static bool js_SetZoneLayer                     (int num_args, bool is_ctor, int magic);
static bool js_SetZoneScript                    (int num_args, bool is_ctor, int magic);
static bool js_SetZoneSteps                     (int num_args, bool is_ctor, int magic);
static bool js_Triangle                         (int num_args, bool is_ctor, int magic);
static bool js_UnbindJoystickButton             (int num_args, bool is_ctor, int magic);
static bool js_UnbindKey                        (int num_args, bool is_ctor, int magic);
static bool js_UpdateMapEngine                  (int num_args, bool is_ctor, int magic);
static bool js_Animation_get_height             (int num_args, bool is_ctor, int magic);
static bool js_Animation_get_width              (int num_args, bool is_ctor, int magic);
static bool js_Animation_drawFrame              (int num_args, bool is_ctor, int magic);
static bool js_Animation_drawZoomedFrame        (int num_args, bool is_ctor, int magic);
static bool js_Animation_getDelay               (int num_args, bool is_ctor, int magic);
static bool js_Animation_getNumFrames           (int num_args, bool is_ctor, int magic);
static bool js_Animation_readNextFrame          (int num_args, bool is_ctor, int magic);
static bool js_ByteArray_get_length             (int num_args, bool is_ctor, int magic);
static bool js_ByteArray_concat                 (int num_args, bool is_ctor, int magic);
static bool js_ByteArray_slice                  (int num_args, bool is_ctor, int magic);
static bool js_ByteArray_toString               (int num_args, bool is_ctor, int magic);
static bool js_Color_get_alpha                  (int num_args, bool is_ctor, int magic);
static bool js_Color_get_blue                   (int num_args, bool is_ctor, int magic);
static bool js_Color_get_green                  (int num_args, bool is_ctor, int magic);
static bool js_Color_get_red                    (int num_args, bool is_ctor, int magic);
static bool js_Color_set_alpha                  (int num_args, bool is_ctor, int magic);
static bool js_Color_set_blue                   (int num_args, bool is_ctor, int magic);
static bool js_Color_set_green                  (int num_args, bool is_ctor, int magic);
static bool js_Color_set_red                    (int num_args, bool is_ctor, int magic);
static bool js_Color_toString                   (int num_args, bool is_ctor, int magic);
static bool js_ColorMatrix_toString             (int num_args, bool is_ctor, int magic);
static bool js_File_close                       (int num_args, bool is_ctor, int magic);
static bool js_File_flush                       (int num_args, bool is_ctor, int magic);
static bool js_File_getKey                      (int num_args, bool is_ctor, int magic);
static bool js_File_getNumKeys                  (int num_args, bool is_ctor, int magic);
static bool js_File_read                        (int num_args, bool is_ctor, int magic);
static bool js_File_toString                    (int num_args, bool is_ctor, int magic);
static bool js_File_write                       (int num_args, bool is_ctor, int magic);
static bool js_Font_clone                       (int num_args, bool is_ctor, int magic);
static bool js_Font_drawText                    (int num_args, bool is_ctor, int magic);
static bool js_Font_drawTextBox                 (int num_args, bool is_ctor, int magic);
static bool js_Font_drawZoomedText              (int num_args, bool is_ctor, int magic);
static bool js_Font_getCharacterImage           (int num_args, bool is_ctor, int magic);
static bool js_Font_getColorMask                (int num_args, bool is_ctor, int magic);
static bool js_Font_getHeight                   (int num_args, bool is_ctor, int magic);
static bool js_Font_getStringHeight             (int num_args, bool is_ctor, int magic);
static bool js_Font_getStringWidth              (int num_args, bool is_ctor, int magic);
static bool js_Font_setCharacterImage           (int num_args, bool is_ctor, int magic);
static bool js_Font_setColorMask                (int num_args, bool is_ctor, int magic);
static bool js_Font_toString                    (int num_args, bool is_ctor, int magic);
static bool js_Font_wordWrapString              (int num_args, bool is_ctor, int magic);
static bool js_Image_get_height                 (int num_args, bool is_ctor, int magic);
static bool js_Image_get_width                  (int num_args, bool is_ctor, int magic);
static bool js_Image_blit                       (int num_args, bool is_ctor, int magic);
static bool js_Image_blitMask                   (int num_args, bool is_ctor, int magic);
static bool js_Image_createSurface              (int num_args, bool is_ctor, int magic);
static bool js_Image_rotateBlit                 (int num_args, bool is_ctor, int magic);
static bool js_Image_rotateBlitMask             (int num_args, bool is_ctor, int magic);
static bool js_Image_toString                   (int num_args, bool is_ctor, int magic);
static bool js_Image_transformBlit              (int num_args, bool is_ctor, int magic);
static bool js_Image_transformBlitMask          (int num_args, bool is_ctor, int magic);
static bool js_Image_zoomBlit                   (int num_args, bool is_ctor, int magic);
static bool js_Image_zoomBlitMask               (int num_args, bool is_ctor, int magic);
static bool js_Logger_beginBlock                (int num_args, bool is_ctor, int magic);
static bool js_Logger_endBlock                  (int num_args, bool is_ctor, int magic);
static bool js_Logger_toString                  (int num_args, bool is_ctor, int magic);
static bool js_Logger_write                     (int num_args, bool is_ctor, int magic);
static bool js_RawFile_close                    (int num_args, bool is_ctor, int magic);
static bool js_RawFile_getPosition              (int num_args, bool is_ctor, int magic);
static bool js_RawFile_getSize                  (int num_args, bool is_ctor, int magic);
static bool js_RawFile_read                     (int num_args, bool is_ctor, int magic);
static bool js_RawFile_setPosition              (int num_args, bool is_ctor, int magic);
static bool js_RawFile_toString                 (int num_args, bool is_ctor, int magic);
static bool js_RawFile_write                    (int num_args, bool is_ctor, int magic);
static bool js_Socket_close                     (int num_args, bool is_ctor, int magic);
static bool js_Socket_getPendingReadSize        (int num_args, bool is_ctor, int magic);
static bool js_Socket_isConnected               (int num_args, bool is_ctor, int magic);
static bool js_Socket_read                      (int num_args, bool is_ctor, int magic);
static bool js_Socket_toString                  (int num_args, bool is_ctor, int magic);
static bool js_Socket_write                     (int num_args, bool is_ctor, int magic);
static bool js_Sound_getLength                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_getPan                     (int num_args, bool is_ctor, int magic);
static bool js_Sound_getPitch                   (int num_args, bool is_ctor, int magic);
static bool js_Sound_getPosition                (int num_args, bool is_ctor, int magic);
static bool js_Sound_getRepeat                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_getVolume                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_isPlaying                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_isSeekable                 (int num_args, bool is_ctor, int magic);
static bool js_Sound_pause                      (int num_args, bool is_ctor, int magic);
static bool js_Sound_play                       (int num_args, bool is_ctor, int magic);
static bool js_Sound_reset                      (int num_args, bool is_ctor, int magic);
static bool js_Sound_setPan                     (int num_args, bool is_ctor, int magic);
static bool js_Sound_setPitch                   (int num_args, bool is_ctor, int magic);
static bool js_Sound_setPosition                (int num_args, bool is_ctor, int magic);
static bool js_Sound_setRepeat                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_setVolume                  (int num_args, bool is_ctor, int magic);
static bool js_Sound_stop                       (int num_args, bool is_ctor, int magic);
static bool js_Sound_toString                   (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_getPan               (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_getPitch             (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_getVolume            (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_setPan               (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_setPitch             (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_setVolume            (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_play                 (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_stop                 (int num_args, bool is_ctor, int magic);
static bool js_SoundEffect_toString             (int num_args, bool is_ctor, int magic);
static bool js_Spriteset_get_filename           (int num_args, bool is_ctor, int magic);
static bool js_Spriteset_clone                  (int num_args, bool is_ctor, int magic);
static bool js_Spriteset_save                   (int num_args, bool is_ctor, int magic);
static bool js_Spriteset_toString               (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_height               (int num_args, bool is_ctor, int magic);
static bool js_Surface_get_width                (int num_args, bool is_ctor, int magic);
static bool js_Surface_applyColorFX             (int num_args, bool is_ctor, int magic);
static bool js_Surface_applyColorFX4            (int num_args, bool is_ctor, int magic);
static bool js_Surface_applyLookup              (int num_args, bool is_ctor, int magic);
static bool js_Surface_bezierCurve              (int num_args, bool is_ctor, int magic);
static bool js_Surface_blit                     (int num_args, bool is_ctor, int magic);
static bool js_Surface_blitMaskSurface          (int num_args, bool is_ctor, int magic);
static bool js_Surface_blitSurface              (int num_args, bool is_ctor, int magic);
static bool js_Surface_clone                    (int num_args, bool is_ctor, int magic);
static bool js_Surface_cloneSection             (int num_args, bool is_ctor, int magic);
static bool js_Surface_createImage              (int num_args, bool is_ctor, int magic);
static bool js_Surface_drawText                 (int num_args, bool is_ctor, int magic);
static bool js_Surface_filledCircle             (int num_args, bool is_ctor, int magic);
static bool js_Surface_filledEllipse            (int num_args, bool is_ctor, int magic);
static bool js_Surface_flipHorizontally         (int num_args, bool is_ctor, int magic);
static bool js_Surface_flipVertically           (int num_args, bool is_ctor, int magic);
static bool js_Surface_getPixel                 (int num_args, bool is_ctor, int magic);
static bool js_Surface_gradientCircle           (int num_args, bool is_ctor, int magic);
static bool js_Surface_gradientEllipse          (int num_args, bool is_ctor, int magic);
static bool js_Surface_gradientLine             (int num_args, bool is_ctor, int magic);
static bool js_Surface_gradientRectangle        (int num_args, bool is_ctor, int magic);
static bool js_Surface_line                     (int num_args, bool is_ctor, int magic);
static bool js_Surface_lineSeries               (int num_args, bool is_ctor, int magic);
static bool js_Surface_outlinedCircle           (int num_args, bool is_ctor, int magic);
static bool js_Surface_outlinedEllipse          (int num_args, bool is_ctor, int magic);
static bool js_Surface_outlinedRectangle        (int num_args, bool is_ctor, int magic);
static bool js_Surface_pointSeries              (int num_args, bool is_ctor, int magic);
static bool js_Surface_rotate                   (int num_args, bool is_ctor, int magic);
static bool js_Surface_rotateBlitMaskSurface    (int num_args, bool is_ctor, int magic);
static bool js_Surface_rotateBlitSurface        (int num_args, bool is_ctor, int magic);
static bool js_Surface_rectangle                (int num_args, bool is_ctor, int magic);
static bool js_Surface_replaceColor             (int num_args, bool is_ctor, int magic);
static bool js_Surface_rescale                  (int num_args, bool is_ctor, int magic);
static bool js_Surface_save                     (int num_args, bool is_ctor, int magic);
static bool js_Surface_setAlpha                 (int num_args, bool is_ctor, int magic);
static bool js_Surface_setBlendMode             (int num_args, bool is_ctor, int magic);
static bool js_Surface_setPixel                 (int num_args, bool is_ctor, int magic);
static bool js_Surface_toString                 (int num_args, bool is_ctor, int magic);
static bool js_Surface_transformBlitMaskSurface (int num_args, bool is_ctor, int magic);
static bool js_Surface_transformBlitSurface     (int num_args, bool is_ctor, int magic);
static bool js_Surface_zoomBlitMaskSurface      (int num_args, bool is_ctor, int magic);
static bool js_Surface_zoomBlitSurface          (int num_args, bool is_ctor, int magic);
static bool js_WindowStyle_drawWindow           (int num_args, bool is_ctor, int magic);
static bool js_WindowStyle_getColorMask         (int num_args, bool is_ctor, int magic);
static bool js_WindowStyle_setColorMask         (int num_args, bool is_ctor, int magic);
static bool js_WindowStyle_toString             (int num_args, bool is_ctor, int magic);

static void js_Animation_finalize   (void* host_ptr);
static void js_ByteArray_finalize   (void* host_ptr);
static void js_Color_finalize       (void* host_ptr);
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
vanilla_register_api(void)
{
	console_log(1, "initializing Sphere v1 API (%s)", API_VERSION_STRING);

	s_sound_mixer = mixer_new(44100, 16, 2);

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
	api_define_function(NULL, "Abort", js_Abort);
	api_define_function(NULL, "AddTrigger", js_AddTrigger);
	api_define_function(NULL, "AddZone", js_AddZone);
	api_define_function(NULL, "ApplyColorMask", js_ApplyColorMask);
	api_define_function(NULL, "AreKeysLeft", js_AreKeysLeft);
	api_define_function(NULL, "AreZonesAt", js_AreZonesAt);
	api_define_function(NULL, "AttachCamera", js_AttachCamera);
	api_define_function(NULL, "AttachInput", js_AttachInput);
	api_define_function(NULL, "AttachPlayerInput", js_AttachPlayerInput);
	api_define_function(NULL, "BezierCurve", js_BezierCurve);
	api_define_function(NULL, "BindJoystickButton", js_BindJoystickButton);
	api_define_function(NULL, "BindKey", js_BindKey);
	api_define_function(NULL, "BlendColors", js_BlendColors);
	api_define_function(NULL, "BlendColorsWeighted", js_BlendColors);
	api_define_function(NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	api_define_function(NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript);
	api_define_function(NULL, "CallMapScript", js_CallMapScript);
	api_define_function(NULL, "CallPersonScript", js_CallPersonScript);
	api_define_function(NULL, "ChangeMap", js_ChangeMap);
	api_define_function(NULL, "ClearPersonCommands", js_ClearPersonCommands);
	api_define_function(NULL, "CreateByteArray", js_CreateByteArray);
	api_define_function(NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString);
	api_define_function(NULL, "CreateColor", js_CreateColor);
	api_define_function(NULL, "CreateColorMatrix", js_CreateColorMatrix);
	api_define_function(NULL, "CreateDirectory", js_CreateDirectory);
	api_define_function(NULL, "CreatePerson", js_CreatePerson);
	api_define_function(NULL, "CreateSpriteset", js_CreateSpriteset);
	api_define_function(NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray);
	api_define_function(NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_define_function(NULL, "CreateSurface", js_CreateSurface);
	api_define_function(NULL, "DeflateByteArray", js_DeflateByteArray);
	api_define_function(NULL, "Delay", js_Delay);
	api_define_function(NULL, "DestroyPerson", js_DestroyPerson);
	api_define_function(NULL, "DetachCamera", js_DetachCamera);
	api_define_function(NULL, "DetachInput", js_DetachInput);
	api_define_function(NULL, "DetachPlayerInput", js_DetachPlayerInput);
	api_define_function(NULL, "DoEvents", js_DoEvents);
	api_define_function(NULL, "DoesFileExist", js_DoesFileExist);
	api_define_function(NULL, "DoesPersonExist", js_DoesPersonExist);
	api_define_function(NULL, "EvaluateScript", js_EvaluateScript);
	api_define_function(NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_define_function(NULL, "Exit", js_Exit);
	api_define_function(NULL, "ExitMapEngine", js_ExitMapEngine);
	api_define_function(NULL, "ExecuteGame", js_ExecuteGame);
	api_define_function(NULL, "ExecuteTrigger", js_ExecuteTrigger);
	api_define_function(NULL, "ExecuteZoneScript", js_ExecuteZoneScript);
	api_define_function(NULL, "ExecuteZones", js_ExecuteZones);
	api_define_function(NULL, "FilledCircle", js_FilledCircle);
	api_define_function(NULL, "FilledComplex", js_FilledComplex);
	api_define_function(NULL, "FilledEllipse", js_FilledEllipse);
	api_define_function(NULL, "FlipScreen", js_FlipScreen);
	api_define_function(NULL, "FollowPerson", js_FollowPerson);
	api_define_function(NULL, "GarbageCollect", js_GarbageCollect);
	api_define_function(NULL, "GetActingPerson", js_GetActingPerson);
	api_define_function(NULL, "GetCameraPerson", js_GetCameraPerson);
	api_define_function(NULL, "GetCameraX", js_GetCameraX);
	api_define_function(NULL, "GetCameraY", js_GetCameraY);
	api_define_function(NULL, "GetClippingRectangle", js_GetClippingRectangle);
	api_define_function(NULL, "GetCurrentMap", js_GetCurrentMap);
	api_define_function(NULL, "GetCurrentPerson", js_GetCurrentPerson);
	api_define_function(NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	api_define_function(NULL, "GetCurrentZone", js_GetCurrentZone);
	api_define_function(NULL, "GetDirectoryList", js_GetDirectoryList);
	api_define_function(NULL, "GetFileList", js_GetFileList);
	api_define_function(NULL, "GetFrameRate", js_GetFrameRate);
	api_define_function(NULL, "GetGameList", js_GetGameList);
	api_define_function(NULL, "GetInputPerson", js_GetInputPerson);
	api_define_function(NULL, "GetJoystickAxis", js_GetJoystickAxis);
	api_define_function(NULL, "GetLocalAddress", js_GetLocalAddress);
	api_define_function(NULL, "GetLocalName", js_GetLocalName);
	api_define_function(NULL, "GetKey", js_GetKey);
	api_define_function(NULL, "GetKeyString", js_GetKeyString);
	api_define_function(NULL, "GetLayerAngle", js_GetLayerAngle);
	api_define_function(NULL, "GetLayerHeight", js_GetLayerHeight);
	api_define_function(NULL, "GetLayerMask", js_GetLayerMask);
	api_define_function(NULL, "GetLayerName", js_GetLayerName);
	api_define_function(NULL, "GetLayerWidth", js_GetLayerWidth);
	api_define_function(NULL, "GetMapEngine", js_GetMapEngine);
	api_define_function(NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	api_define_function(NULL, "GetMouseWheelEvent", js_GetMouseWheelEvent);
	api_define_function(NULL, "GetMouseX", js_GetMouseX);
	api_define_function(NULL, "GetMouseY", js_GetMouseY);
	api_define_function(NULL, "GetNumJoysticks", js_GetNumJoysticks);
	api_define_function(NULL, "GetNumJoystickAxes", js_GetNumJoystickAxes);
	api_define_function(NULL, "GetNumJoystickButtons", js_GetNumJoystickButtons);
	api_define_function(NULL, "GetNumMouseWheelEvents", js_GetNumMouseWheelEvents);
	api_define_function(NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	api_define_function(NULL, "GetNumLayers", js_GetNumLayers);
	api_define_function(NULL, "GetNumTiles", js_GetNumTiles);
	api_define_function(NULL, "GetNumTriggers", js_GetNumTriggers);
	api_define_function(NULL, "GetNumZones", js_GetNumZones);
	api_define_function(NULL, "GetObstructingPerson", js_GetObstructingPerson);
	api_define_function(NULL, "GetObstructingTile", js_GetObstructingTile);
	api_define_function(NULL, "GetPersonAngle", js_GetPersonAngle);
	api_define_function(NULL, "GetPersonBase", js_GetPersonBase);
	api_define_function(NULL, "GetPersonData", js_GetPersonData);
	api_define_function(NULL, "GetPersonDirection", js_GetPersonDirection);
	api_define_function(NULL, "GetPersonFollowDistance", js_GetPersonFollowDistance);
	api_define_function(NULL, "GetPersonFollowers", js_GetPersonFollowers);
	api_define_function(NULL, "GetPersonFrame", js_GetPersonFrame);
	api_define_function(NULL, "GetPersonFrameNext", js_GetPersonFrameNext);
	api_define_function(NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert);
	api_define_function(NULL, "GetPersonIgnoreList", js_GetPersonIgnoreList);
	api_define_function(NULL, "GetPersonLayer", js_GetPersonLayer);
	api_define_function(NULL, "GetPersonLeader", js_GetPersonLeader);
	api_define_function(NULL, "GetPersonList", js_GetPersonList);
	api_define_function(NULL, "GetPersonMask", js_GetPersonMask);
	api_define_function(NULL, "GetPersonOffsetX", js_GetPersonOffsetX);
	api_define_function(NULL, "GetPersonOffsetY", js_GetPersonOffsetY);
	api_define_function(NULL, "GetPersonSpeedX", js_GetPersonSpeedX);
	api_define_function(NULL, "GetPersonSpeedY", js_GetPersonSpeedY);
	api_define_function(NULL, "GetPersonSpriteset", js_GetPersonSpriteset);
	api_define_function(NULL, "GetPersonValue", js_GetPersonValue);
	api_define_function(NULL, "GetPersonX", js_GetPersonX);
	api_define_function(NULL, "GetPersonXFloat", js_GetPersonXFloat);
	api_define_function(NULL, "GetPersonY", js_GetPersonY);
	api_define_function(NULL, "GetPersonYFloat", js_GetPersonYFloat);
	api_define_function(NULL, "GetPlayerKey", js_GetPlayerKey);
	api_define_function(NULL, "GetScreenHeight", js_GetScreenHeight);
	api_define_function(NULL, "GetScreenWidth", js_GetScreenWidth);
	api_define_function(NULL, "GetSystemArrow", js_GetSystemArrow);
	api_define_function(NULL, "GetSystemDownArrow", js_GetSystemDownArrow);
	api_define_function(NULL, "GetSystemFont", js_GetSystemFont);
	api_define_function(NULL, "GetSystemUpArrow", js_GetSystemUpArrow);
	api_define_function(NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	api_define_function(NULL, "GetTalkActivationButton", js_GetTalkActivationButton);
	api_define_function(NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	api_define_function(NULL, "GetTalkDistance", js_GetTalkDistance);
	api_define_function(NULL, "GetTile", js_GetTile);
	api_define_function(NULL, "GetTileDelay", js_GetTileDelay);
	api_define_function(NULL, "GetTileImage", js_GetTileImage);
	api_define_function(NULL, "GetTileHeight", js_GetTileHeight);
	api_define_function(NULL, "GetTileName", js_GetTileName);
	api_define_function(NULL, "GetTileSurface", js_GetTileSurface);
	api_define_function(NULL, "GetTileWidth", js_GetTileWidth);
	api_define_function(NULL, "GetTime", js_GetTime);
	api_define_function(NULL, "GetToggleState", js_GetToggleState);
	api_define_function(NULL, "GetTriggerLayer", js_GetTriggerLayer);
	api_define_function(NULL, "GetTriggerX", js_GetTriggerX);
	api_define_function(NULL, "GetTriggerY", js_GetTriggerY);
	api_define_function(NULL, "GetVersion", js_GetVersion);
	api_define_function(NULL, "GetVersionString", js_GetVersionString);
	api_define_function(NULL, "GetZoneHeight", js_GetZoneHeight);
	api_define_function(NULL, "GetZoneLayer", js_GetZoneLayer);
	api_define_function(NULL, "GetZoneSteps", js_GetZoneSteps);
	api_define_function(NULL, "GetZoneWidth", js_GetZoneWidth);
	api_define_function(NULL, "GetZoneX", js_GetZoneX);
	api_define_function(NULL, "GetZoneY", js_GetZoneY);
	api_define_function(NULL, "GrabImage", js_GrabImage);
	api_define_function(NULL, "GrabSurface", js_GrabSurface);
	api_define_function(NULL, "GradientCircle", js_GradientCircle);
	api_define_function(NULL, "GradientComplex", js_GradientComplex);
	api_define_function(NULL, "GradientEllipse", js_GradientEllipse);
	api_define_function(NULL, "GradientLine", js_GradientLine);
	api_define_function(NULL, "GradientRectangle", js_GradientRectangle);
	api_define_function(NULL, "GradientTriangle", js_GradientTriangle);
	api_define_function(NULL, "HashByteArray", js_HashByteArray);
	api_define_function(NULL, "HashFromFile", js_HashFromFile);
	api_define_function(NULL, "IgnorePersonObstructions", js_IgnorePersonObstructions);
	api_define_function(NULL, "IgnoreTileObstructions", js_IgnoreTileObstructions);
	api_define_function(NULL, "InflateByteArray", js_InflateByteArray);
	api_define_function(NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed);
	api_define_function(NULL, "IsCameraAttached", js_IsCameraAttached);
	api_define_function(NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty);
	api_define_function(NULL, "IsIgnoringPersonObstructions", js_IsIgnoringPersonObstructions);
	api_define_function(NULL, "IsIgnoringTileObstructions", js_IsIgnoringTileObstructions);
	api_define_function(NULL, "IsInputAttached", js_IsInputAttached);
	api_define_function(NULL, "IsJoystickButtonPressed", js_IsJoystickButtonPressed);
	api_define_function(NULL, "IsKeyPressed", js_IsKeyPressed);
	api_define_function(NULL, "IsLayerReflective", js_IsLayerReflective);
	api_define_function(NULL, "IsLayerVisible", js_IsLayerVisible);
	api_define_function(NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	api_define_function(NULL, "IsMouseButtonPressed", js_IsMouseButtonPressed);
	api_define_function(NULL, "IsPersonObstructed", js_IsPersonObstructed);
	api_define_function(NULL, "IsPersonVisible", js_IsPersonVisible);
	api_define_function(NULL, "IsTriggerAt", js_IsTriggerAt);
	api_define_function(NULL, "Line", js_Line);
	api_define_function(NULL, "LineSeries", js_LineSeries);
	api_define_function(NULL, "ListenOnPort", js_ListenOnPort);
	api_define_function(NULL, "LoadAnimation", js_LoadAnimation);
	api_define_function(NULL, "LoadFont", js_LoadFont);
	api_define_function(NULL, "LoadImage", js_LoadImage);
	api_define_function(NULL, "LoadSound", js_LoadSound);
	api_define_function(NULL, "LoadSoundEffect", js_LoadSoundEffect);
	api_define_function(NULL, "LoadSpriteset", js_LoadSpriteset);
	api_define_function(NULL, "LoadSurface", js_LoadSurface);
	api_define_function(NULL, "LoadWindowStyle", js_LoadWindowStyle);
	api_define_function(NULL, "MapEngine", js_MapEngine);
	api_define_function(NULL, "MapToScreenX", js_MapToScreenX);
	api_define_function(NULL, "MapToScreenY", js_MapToScreenY);
	api_define_function(NULL, "OpenAddress", js_OpenAddress);
	api_define_function(NULL, "OpenFile", js_OpenFile);
	api_define_function(NULL, "OpenLog", js_OpenLog);
	api_define_function(NULL, "OpenRawFile", js_OpenRawFile);
	api_define_function(NULL, "OutlinedCircle", js_OutlinedCircle);
	api_define_function(NULL, "OutlinedComplex", js_OutlinedComplex);
	api_define_function(NULL, "OutlinedEllipse", js_OutlinedEllipse);
	api_define_function(NULL, "OutlinedRectangle", js_OutlinedRectangle);
	api_define_function(NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle);
	api_define_function(NULL, "Point", js_Point);
	api_define_function(NULL, "PointSeries", js_PointSeries);
	api_define_function(NULL, "Polygon", js_Polygon);
	api_define_function(NULL, "Print", js_Print);
	api_define_function(NULL, "QueuePersonCommand", js_QueuePersonCommand);
	api_define_function(NULL, "QueuePersonScript", js_QueuePersonScript);
	api_define_function(NULL, "Rectangle", js_Rectangle);
	api_define_function(NULL, "RemoveDirectory", js_RemoveDirectory);
	api_define_function(NULL, "RemoveFile", js_RemoveFile);
	api_define_function(NULL, "RemoveTrigger", js_RemoveTrigger);
	api_define_function(NULL, "RemoveZone", js_RemoveZone);
	api_define_function(NULL, "RenderMap", js_RenderMap);
	api_define_function(NULL, "Rename", js_Rename);
	api_define_function(NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer);
	api_define_function(NULL, "RequireScript", js_RequireScript);
	api_define_function(NULL, "RequireSystemScript", js_RequireSystemScript);
	api_define_function(NULL, "RestartGame", js_RestartGame);
	api_define_function(NULL, "RoundRectangle", js_RoundRectangle);
	api_define_function(NULL, "ScreenToMapX", js_ScreenToMapX);
	api_define_function(NULL, "ScreenToMapY", js_ScreenToMapY);
	api_define_function(NULL, "SetCameraX", js_SetCameraX);
	api_define_function(NULL, "SetCameraY", js_SetCameraY);
	api_define_function(NULL, "SetClippingRectangle", js_SetClippingRectangle);
	api_define_function(NULL, "SetColorMask", js_SetColorMask);
	api_define_function(NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	api_define_function(NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript);
	api_define_function(NULL, "SetDelayScript", js_SetDelayScript);
	api_define_function(NULL, "SetFrameRate", js_SetFrameRate);
	api_define_function(NULL, "SetLayerAngle", js_SetLayerAngle);
	api_define_function(NULL, "SetLayerHeight", js_SetLayerHeight);
	api_define_function(NULL, "SetLayerMask", js_SetLayerMask);
	api_define_function(NULL, "SetLayerReflective", js_SetLayerReflective);
	api_define_function(NULL, "SetLayerRenderer", js_SetLayerRenderer);
	api_define_function(NULL, "SetLayerScaleFactorX", js_SetLayerScaleFactorX);
	api_define_function(NULL, "SetLayerScaleFactorY", js_SetLayerScaleFactorY);
	api_define_function(NULL, "SetLayerSize", js_SetLayerSize);
	api_define_function(NULL, "SetLayerVisible", js_SetLayerVisible);
	api_define_function(NULL, "SetLayerWidth", js_SetLayerWidth);
	api_define_function(NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	api_define_function(NULL, "SetMousePosition", js_SetMousePosition);
	api_define_function(NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	api_define_function(NULL, "SetPersonAngle", js_SetPersonAngle);
	api_define_function(NULL, "SetPersonData", js_SetPersonData);
	api_define_function(NULL, "SetPersonDirection", js_SetPersonDirection);
	api_define_function(NULL, "SetPersonFollowDistance", js_SetPersonFollowDistance);
	api_define_function(NULL, "SetPersonFrame", js_SetPersonFrame);
	api_define_function(NULL, "SetPersonFrameNext", js_SetPersonFrameNext);
	api_define_function(NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert);
	api_define_function(NULL, "SetPersonIgnoreList", js_SetPersonIgnoreList);
	api_define_function(NULL, "SetPersonLayer", js_SetPersonLayer);
	api_define_function(NULL, "SetPersonMask", js_SetPersonMask);
	api_define_function(NULL, "SetPersonOffsetX", js_SetPersonOffsetX);
	api_define_function(NULL, "SetPersonOffsetY", js_SetPersonOffsetY);
	api_define_function(NULL, "SetPersonScaleAbsolute", js_SetPersonScaleAbsolute);
	api_define_function(NULL, "SetPersonScaleFactor", js_SetPersonScaleFactor);
	api_define_function(NULL, "SetPersonScript", js_SetPersonScript);
	api_define_function(NULL, "SetPersonSpeed", js_SetPersonSpeed);
	api_define_function(NULL, "SetPersonSpeedXY", js_SetPersonSpeedXY);
	api_define_function(NULL, "SetPersonSpriteset", js_SetPersonSpriteset);
	api_define_function(NULL, "SetPersonValue", js_SetPersonValue);
	api_define_function(NULL, "SetPersonVisible", js_SetPersonVisible);
	api_define_function(NULL, "SetPersonX", js_SetPersonX);
	api_define_function(NULL, "SetPersonXYFloat", js_SetPersonXYFloat);
	api_define_function(NULL, "SetPersonY", js_SetPersonY);
	api_define_function(NULL, "SetRenderScript", js_SetRenderScript);
	api_define_function(NULL, "SetTalkActivationButton", js_SetTalkActivationButton);
	api_define_function(NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	api_define_function(NULL, "SetTalkDistance", js_SetTalkDistance);
	api_define_function(NULL, "SetTile", js_SetTile);
	api_define_function(NULL, "SetTileDelay", js_SetTileDelay);
	api_define_function(NULL, "SetTileImage", js_SetTileImage);
	api_define_function(NULL, "SetTileName", js_SetTileName);
	api_define_function(NULL, "SetTileSurface", js_SetTileSurface);
	api_define_function(NULL, "SetTriggerLayer", js_SetTriggerLayer);
	api_define_function(NULL, "SetTriggerScript", js_SetTriggerScript);
	api_define_function(NULL, "SetTriggerXY", js_SetTriggerXY);
	api_define_function(NULL, "SetUpdateScript", js_SetUpdateScript);
	api_define_function(NULL, "SetZoneDimensions", js_SetZoneDimensions);
	api_define_function(NULL, "SetZoneLayer", js_SetZoneLayer);
	api_define_function(NULL, "SetZoneScript", js_SetZoneScript);
	api_define_function(NULL, "SetZoneSteps", js_SetZoneSteps);
	api_define_function(NULL, "Triangle", js_Triangle);
	api_define_function(NULL, "UnbindJoystickButton", js_UnbindJoystickButton);
	api_define_function(NULL, "UnbindKey", js_UnbindKey);
	api_define_function(NULL, "UpdateMapEngine", js_UpdateMapEngine);

	api_define_class("v1Animation", SV1_ANIMATION, NULL, js_Animation_finalize);
	api_define_property("v1Animation", "width", false, js_Animation_get_width, NULL);
	api_define_property("v1Animation", "height", false, js_Animation_get_height, NULL);
	api_define_method("v1Animation", "getDelay", js_Animation_getDelay);
	api_define_method("v1Animation", "getNumFrames", js_Animation_getNumFrames);
	api_define_method("v1Animation", "drawFrame", js_Animation_drawFrame);
	api_define_method("v1Animation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	api_define_method("v1Animation", "readNextFrame", js_Animation_readNextFrame);

	api_define_class("v1ByteArray", SV1_BYTE_ARRAY, NULL, js_ByteArray_finalize);
	api_define_property("v1ByteArray", "length", false, js_ByteArray_get_length, NULL);
	api_define_method("v1ByteArray", "concat", js_ByteArray_concat);
	api_define_method("v1ByteArray", "slice", js_ByteArray_slice);
	api_define_method("v1ByteArray", "toString", js_ByteArray_toString);

	api_define_class("v1Color", SV1_COLOR, NULL, NULL);
	api_define_property("v1Color", "alpha", true, js_Color_get_alpha, js_Color_set_alpha);
	api_define_property("v1Color", "blue", true, js_Color_get_blue, js_Color_set_blue);
	api_define_property("v1Color", "green", true, js_Color_get_green, js_Color_set_green);
	api_define_property("v1Color", "red", true, js_Color_get_red, js_Color_set_red);
	api_define_method("v1Color", "toString", js_Color_toString);

	api_define_class("v1ColorMatrix", SV1_COLOR_MATRIX, NULL, NULL);
	api_define_method("v1ColorMatrix", "toString", js_ColorMatrix_toString);

	api_define_class("v1File", SV1_FILE, NULL, js_File_finalize);
	api_define_method("v1File", "close", js_File_close);
	api_define_method("v1File", "flush", js_File_flush);
	api_define_method("v1File", "getKey", js_File_getKey);
	api_define_method("v1File", "getNumKeys", js_File_getNumKeys);
	api_define_method("v1File", "read", js_File_read);
	api_define_method("v1File", "toString", js_File_toString);
	api_define_method("v1File", "write", js_File_write);

	api_define_class("v1Font", SV1_FONT, NULL, js_Font_finalize);
	api_define_method("v1Font", "clone", js_Font_clone);
	api_define_method("v1Font", "drawText", js_Font_drawText);
	api_define_method("v1Font", "drawTextBox", js_Font_drawTextBox);
	api_define_method("v1Font", "drawZoomedText", js_Font_drawZoomedText);
	api_define_method("v1Font", "getCharacterImage", js_Font_getCharacterImage);
	api_define_method("v1Font", "getColorMask", js_Font_getColorMask);
	api_define_method("v1Font", "getHeight", js_Font_getHeight);
	api_define_method("v1Font", "getStringHeight", js_Font_getStringHeight);
	api_define_method("v1Font", "getStringWidth", js_Font_getStringWidth);
	api_define_method("v1Font", "setCharacterImage", js_Font_setCharacterImage);
	api_define_method("v1Font", "setColorMask", js_Font_setColorMask);
	api_define_method("v1Font", "toString", js_Font_toString);
	api_define_method("v1Font", "wordWrapString", js_Font_wordWrapString);

	api_define_class("v1Image", SV1_IMAGE, NULL, js_Image_finalize);
	api_define_property("v1Image", "height", false, js_Image_get_height, NULL);
	api_define_property("v1Image", "width", false, js_Image_get_width, NULL);
	api_define_method("v1Image", "blit", js_Image_blit);
	api_define_method("v1Image", "blitMask", js_Image_blitMask);
	api_define_method("v1Image", "createSurface", js_Image_createSurface);
	api_define_method("v1Image", "rotateBlit", js_Image_rotateBlit);
	api_define_method("v1Image", "rotateBlitMask", js_Image_rotateBlitMask);
	api_define_method("v1Image", "toString", js_Image_toString);
	api_define_method("v1Image", "transformBlit", js_Image_transformBlit);
	api_define_method("v1Image", "transformBlitMask", js_Image_transformBlitMask);
	api_define_method("v1Image", "zoomBlit", js_Image_zoomBlit);
	api_define_method("v1Image", "zoomBlitMask", js_Image_zoomBlitMask);

	api_define_class("v1Logger", SV1_LOGGER, NULL, js_Logger_finalize);
	api_define_method("v1Logger", "toString", js_Logger_toString);
	api_define_method("v1Logger", "beginBlock", js_Logger_beginBlock);
	api_define_method("v1Logger", "endBlock", js_Logger_endBlock);
	api_define_method("v1Logger", "write", js_Logger_write);

	api_define_class("v1RawFile", SV1_RAW_FILE, NULL, js_RawFile_finalize);
	api_define_method("v1RawFile", "close", js_RawFile_close);
	api_define_method("v1RawFile", "getPosition", js_RawFile_getPosition);
	api_define_method("v1RawFile", "getSize", js_RawFile_getSize);
	api_define_method("v1RawFile", "read", js_RawFile_read);
	api_define_method("v1RawFile", "setPosition", js_RawFile_setPosition);
	api_define_method("v1RawFile", "toString", js_RawFile_toString);
	api_define_method("v1RawFile", "write", js_RawFile_write);

	api_define_class("v1Socket", SV1_SOCKET, NULL, js_Socket_finalize);
	api_define_method("v1Socket", "close", js_Socket_close);
	api_define_method("v1Socket", "getPendingReadSize", js_Socket_getPendingReadSize);
	api_define_method("v1Socket", "isConnected", js_Socket_isConnected);
	api_define_method("v1Socket", "read", js_Socket_read);
	api_define_method("v1Socket", "toString", js_Socket_toString);
	api_define_method("v1Socket", "write", js_Socket_write);

	api_define_class("v1Sound", SV1_SOUND, NULL, js_Sound_finalize);
	api_define_method("v1Sound", "getLength", js_Sound_getLength);
	api_define_method("v1Sound", "getPan", js_Sound_getPan);
	api_define_method("v1Sound", "getPitch", js_Sound_getPitch);
	api_define_method("v1Sound", "getPosition", js_Sound_getPosition);
	api_define_method("v1Sound", "getRepeat", js_Sound_getRepeat);
	api_define_method("v1Sound", "getVolume", js_Sound_getVolume);
	api_define_method("v1Sound", "isPlaying", js_Sound_isPlaying);
	api_define_method("v1Sound", "isSeekable", js_Sound_isSeekable);
	api_define_method("v1Sound", "pause", js_Sound_pause);
	api_define_method("v1Sound", "play", js_Sound_play);
	api_define_method("v1Sound", "reset", js_Sound_reset);
	api_define_method("v1Sound", "setPan", js_Sound_setPan);
	api_define_method("v1Sound", "setPitch", js_Sound_setPitch);
	api_define_method("v1Sound", "setPosition", js_Sound_setPosition);
	api_define_method("v1Sound", "setRepeat", js_Sound_setRepeat);
	api_define_method("v1Sound", "setVolume", js_Sound_setVolume);
	api_define_method("v1Sound", "stop", js_Sound_stop);
	api_define_method("v1Sound", "toString", js_Sound_toString);

	api_define_class("v1SoundEffect", SV1_SOUND_EFFECT, NULL, js_SoundEffect_finalize);
	api_define_method("v1SoundEffect", "getPan", js_SoundEffect_getPan);
	api_define_method("v1SoundEffect", "getPitch", js_SoundEffect_getPitch);
	api_define_method("v1SoundEffect", "getVolume", js_SoundEffect_getVolume);
	api_define_method("v1SoundEffect", "setPan", js_SoundEffect_setPan);
	api_define_method("v1SoundEffect", "setPitch", js_SoundEffect_setPitch);
	api_define_method("v1SoundEffect", "setVolume", js_SoundEffect_setVolume);
	api_define_method("v1SoundEffect", "play", js_SoundEffect_play);
	api_define_method("v1SoundEffect", "stop", js_SoundEffect_stop);
	api_define_method("v1SoundEffect", "toString", js_SoundEffect_toString);

	api_define_class("v1Spriteset", SV1_SPRITESET, NULL, js_Spriteset_finalize);
	api_define_property("v1Spriteset", "filename", false, js_Spriteset_get_filename, NULL);
	api_define_method("v1Spriteset", "clone", js_Spriteset_clone);
	api_define_method("v1Spriteset", "save", js_Spriteset_save);
	api_define_method("v1Spriteset", "toString", js_Spriteset_toString);

	api_define_class("v1Surface", SV1_SURFACE, NULL, js_Surface_finalize);
	api_define_property("v1Surface", "height", false, js_Surface_get_height, NULL);
	api_define_property("v1Surface", "width", false, js_Surface_get_width, NULL);
	api_define_method("v1Surface", "applyColorFX", js_Surface_applyColorFX);
	api_define_method("v1Surface", "applyColorFX4", js_Surface_applyColorFX4);
	api_define_method("v1Surface", "applyLookup", js_Surface_applyLookup);
	api_define_method("v1Surface", "bezierCurve", js_Surface_bezierCurve);
	api_define_method("v1Surface", "blit", js_Surface_blit);
	api_define_method("v1Surface", "blitMaskSurface", js_Surface_blitMaskSurface);
	api_define_method("v1Surface", "blitSurface", js_Surface_blitSurface);
	api_define_method("v1Surface", "clone", js_Surface_clone);
	api_define_method("v1Surface", "cloneSection", js_Surface_cloneSection);
	api_define_method("v1Surface", "createImage", js_Surface_createImage);
	api_define_method("v1Surface", "drawText", js_Surface_drawText);
	api_define_method("v1Surface", "filledCircle", js_Surface_filledCircle);
	api_define_method("v1Surface", "filledEllipse", js_Surface_filledEllipse);
	api_define_method("v1Surface", "flipHorizontally", js_Surface_flipHorizontally);
	api_define_method("v1Surface", "flipVertically", js_Surface_flipVertically);
	api_define_method("v1Surface", "getPixel", js_Surface_getPixel);
	api_define_method("v1Surface", "gradientCircle", js_Surface_gradientCircle);
	api_define_method("v1Surface", "gradientEllipse", js_Surface_gradientEllipse);
	api_define_method("v1Surface", "gradientLine", js_Surface_gradientLine);
	api_define_method("v1Surface", "gradientRectangle", js_Surface_gradientRectangle);
	api_define_method("v1Surface", "line", js_Surface_line);
	api_define_method("v1Surface", "lineSeries", js_Surface_lineSeries);
	api_define_method("v1Surface", "outlinedCircle", js_Surface_outlinedCircle);
	api_define_method("v1Surface", "outlinedEllipse", js_Surface_outlinedEllipse);
	api_define_method("v1Surface", "outlinedRectangle", js_Surface_outlinedRectangle);
	api_define_method("v1Surface", "pointSeries", js_Surface_pointSeries);
	api_define_method("v1Surface", "rotate", js_Surface_rotate);
	api_define_method("v1Surface", "rotateBlitMaskSurface", js_Surface_rotateBlitMaskSurface);
	api_define_method("v1Surface", "rotateBlitSurface", js_Surface_rotateBlitSurface);
	api_define_method("v1Surface", "rectangle", js_Surface_rectangle);
	api_define_method("v1Surface", "replaceColor", js_Surface_replaceColor);
	api_define_method("v1Surface", "rescale", js_Surface_rescale);
	api_define_method("v1Surface", "save", js_Surface_save);
	api_define_method("v1Surface", "setAlpha", js_Surface_setAlpha);
	api_define_method("v1Surface", "setBlendMode", js_Surface_setBlendMode);
	api_define_method("v1Surface", "setPixel", js_Surface_setPixel);
	api_define_method("v1Surface", "toString", js_Surface_toString);
	api_define_method("v1Surface", "transformBlitMaskSurface", js_Surface_transformBlitMaskSurface);
	api_define_method("v1Surface", "transformBlitSurface", js_Surface_transformBlitSurface);
	api_define_method("v1Surface", "zoomBlitMaskSurface", js_Surface_zoomBlitMaskSurface);
	api_define_method("v1Surface", "zoomBlitSurface", js_Surface_zoomBlitSurface);

	api_define_class("v1WindowStyle", SV1_WINDOW_STYLE, NULL, js_WindowStyle_finalize);
	api_define_method("v1WindowStyle", "drawWindow", js_WindowStyle_drawWindow);
	api_define_method("v1WindowStyle", "getColorMask", js_WindowStyle_getColorMask);
	api_define_method("v1WindowStyle", "setColorMask", js_WindowStyle_setColorMask);
	api_define_method("v1WindowStyle", "toString", js_WindowStyle_toString);

	// blend modes for Surfaces
	api_define_const(NULL, "BLEND", BLEND_BLEND);
	api_define_const(NULL, "REPLACE", BLEND_REPLACE);
	api_define_const(NULL, "RGB_ONLY", BLEND_RGB_ONLY);
	api_define_const(NULL, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
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
jsal_push_sphere_bytearray(bytearray_t* array)
{
	jsal_push_class_obj(SV1_BYTE_ARRAY, bytearray_ref(array), false);
	jsal_make_buffer(-1, JS_UINT8ARRAY, bytearray_buffer(array), bytearray_len(array));
}

void
jsal_push_sphere_color(color_t color)
{
	color_t* color_ptr;

	color_ptr = malloc(sizeof(color_t));
	*color_ptr = color;
	jsal_push_class_obj(SV1_COLOR, color_ptr, false);
}

void
jsal_push_sphere_font(font_t* font)
{
	jsal_push_class_obj(SV1_FONT, font_ref(font), false);
	jsal_push_sphere_color(color_new(255, 255, 255, 255));
	jsal_put_prop_string(-2, "\xFF" "color_mask");
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

	if (jsal_is_number(at_index)) {
		layer_index = jsal_get_int(at_index);
		goto have_index;
	}
	else if (jsal_is_string(at_index)) {
		// don't anyone ever say I'm not dedicated to compatibility!  there are a few
		// poorly written Sphere 1.5 games that pass layer IDs as strings.  usually this
		// would fail because miniSphere supports named layers, but here I go out of my
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
			jsal_error(JS_RANGE_ERROR, "layer name does not exist '%s'", name);
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
		jsal_error(JS_RANGE_ERROR, "layer index out of range");
	return layer_index;
}

color_t
jsal_require_sphere_color(int index)
{
	color_t* color_ptr;

	color_ptr = jsal_require_class_obj(index, SV1_COLOR);
	return *color_ptr;
}

colormatrix_t
jsal_require_sphere_colormatrix(int index)
{
	colormatrix_t matrix;

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
	}
	else if (jsal_is_string(index)) {
		// caller passed code string, compile it
		codestring = jsal_require_lstring_t(index);
		script = script_new(codestring, "%s", name);
		lstr_free(codestring);
	}
	else if (jsal_is_null(index) || jsal_is_undefined(index))
		return NULL;
	else
		jsal_error(JS_TYPE_ERROR, "expected string, function, or null/undefined");
	return script;
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
	num_images = (int)jsal_get_length(-1);
	for (i = 0; i < num_images; ++i) {
		jsal_get_prop_index(-1, i);
		image = jsal_require_class_obj(-1, SV1_IMAGE);
		spriteset_add_image(spriteset, image);
		jsal_pop(1);
	}
	jsal_pop(1);

	jsal_get_prop_string(index, "directions");
	num_poses = (int)jsal_get_length(-1);
	for (i = 0; i < num_poses; ++i) {
		jsal_get_prop_index(-1, i);
		jsal_get_prop_string(-1, "name");
		pose_name = jsal_require_string(-1);
		spriteset_add_pose(spriteset, pose_name);
		jsal_get_prop_string(-2, "frames");
		num_frames = (int)jsal_get_length(-1);
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
	jsal_push_sphere_color(color_new(255, 255, 255, 255));
	jsal_put_prop_string(-2, "\xFF" "color_mask");
}

static uint8_t*
jsal_require_rgba_lut(int index)
{
	uint8_t* lut;
	int      length;

	int i;

	index = jsal_normalize_index(index);
	jsal_require_object_coercible(index);
	lut = malloc(sizeof(uint8_t) * 256);
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

static bool
js_Abort(int num_args, bool is_ctor, int magic)
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
	filename = "the pig";
	line_number = 812;
	text = strnewf("%s:%d\nmanual abort\n\n%s", filename, line_number, message);
	sphere_abort(text);
}

static bool
js_AddTrigger(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	bounds = map_bounds();
	if (x < bounds.x1 || y < bounds.y1 || x >= bounds.x2 || y >= bounds.y2)
		jsal_error(JS_RANGE_ERROR, "X/Y out of bounds");

	if (!map_add_trigger(x, y, layer, script))
		jsal_error(JS_ERROR, "couldn't create trigger");
	jsal_push_number(map_num_triggers() - 1);
	return true;
}

static bool
js_AddZone(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid zone width/height");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		jsal_error(JS_RANGE_ERROR, "zone out of bounds");

	if (!map_add_zone(rect(x, y, width, height), layer, script, 8))
		jsal_error(JS_ERROR, "couldn't create zone");
	jsal_push_number(map_num_zones() - 1);
	script_unref(script);
	return true;
}

static bool
js_ApplyColorMask(int num_args, bool is_ctor, int magic)
{
	color_t color;
	size2_t resolution;

	color = jsal_require_sphere_color(0);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	resolution = screen_size(g_screen);
	al_draw_filled_rectangle(0, 0, resolution.width, resolution.height,
		nativecolor(color));
	return false;
}

static bool
js_AreKeysLeft(int num_args, bool is_ctor, int magic)
{
	update_input();
	jsal_push_boolean(kb_queue_len() > 0);
	return true;
}

static bool
js_AreZonesAt(int num_args, bool is_ctor, int magic)
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
js_AttachCamera(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_to_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	map_engine_set_subject(person);
	return false;
}

static bool
js_AttachInput(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	map_engine_set_player(PLAYER_1, person);
	return false;
}

static bool
js_AttachPlayerInput(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	player_id_t player;

	name = jsal_require_string(0);
	player = jsal_to_int(1);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	map_engine_set_player(player, person);
	return false;
}

static bool
js_BezierCurve(int num_args, bool is_ctor, int magic)
{
	color_t         color;
	float           cp[8];
	bool            is_quadratic = true;
	int             num_points;
	ALLEGRO_VERTEX* points;
	double          step_size;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	color = jsal_require_sphere_color(0);
	step_size = jsal_to_number(1);
	x1 = jsal_to_number(2);
	y1 = jsal_to_number(3);
	x2 = jsal_to_number(4);
	y2 = jsal_to_number(5);
	x3 = jsal_to_number(6);
	y3 = jsal_to_number(7);
	if (num_args >= 10) {
		is_quadratic = false;
		x4 = jsal_to_number(8);
		y4 = jsal_to_number(9);
	}

	if (screen_skip_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_BindJoystickButton(int num_args, bool is_ctor, int magic)
{
	int joy_index = jsal_to_int(0);
	int button = jsal_to_int(1);
	script_t* on_down_script = jsal_require_sphere_script(2, "[button-down script]");
	script_t* on_up_script = jsal_require_sphere_script(3, "[button-up script]");

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		jsal_error(JS_RANGE_ERROR, "joystick index '%d' out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		jsal_error(JS_RANGE_ERROR, "button index '%d' out of range", button);
	joy_bind_button(joy_index, button, on_down_script, on_up_script);
	return false;
}

static bool
js_BindKey(int num_args, bool is_ctor, int magic)
{
	int keycode = jsal_to_int(0);
	script_t* on_down_script = jsal_require_sphere_script(1, "[key-down script]");
	script_t* on_up_script = jsal_require_sphere_script(2, "[key-up script]");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid key constant");
	kb_bind_key(keycode, on_down_script, on_up_script);
	return false;
}

static bool
js_BlendColors(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_RANGE_ERROR, "invalid weight(s)", w1, w2);

	jsal_push_sphere_color(color_mix(color1, color2, w1, w2));
	return true;
}

static bool
js_CallDefaultMapScript(int num_args, bool is_ctor, int magic)
{
	int map_op;

	map_op = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (map_op < 0 || map_op >= MAP_SCRIPT_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid script type constant");
	map_call_default(map_op);
	return false;
}

static bool
js_CallDefaultPersonScript(int num_args, bool is_ctor, int magic)
{
	person_t*   actor = NULL;
	const char* name;
	person_t*   person;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		actor = person;
	person_call_default(person, type, actor);
	return false;
}

static bool
js_CallMapScript(int num_args, bool is_ctor, int magic)
{
	int type;

	type = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid script type constant");
	map_activate(type, false);
	return false;
}

static bool
js_CallPersonScript(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		jsal_error(JS_ERROR, "invalid script type constant");
	if (type == PERSON_SCRIPT_ON_TALK || type == PERSON_SCRIPT_ON_TOUCH)
		person_activate(person, type, person, false);
	else
		person_activate(person, type, NULL, false);
	return false;
}

static bool
js_ChangeMap(int num_args, bool is_ctor, int magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, "maps", true, false);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (!map_engine_change_map(filename))
		jsal_error(JS_ERROR, "couldn't load map '%s'", filename);
	return false;
}

static bool
js_ClearPersonCommands(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_clear_queue(person);
	return false;
}

static bool
js_CreateByteArray(int num_args, bool is_ctor, int magic)
{
	bytearray_t* array;
	int          size;

	size = jsal_to_int(0);

	if (size < 0)
		jsal_error(JS_RANGE_ERROR, "invalid byte array size");
	if (!(array = bytearray_new(size)))
		jsal_error(JS_ERROR, "couldn't create byte array");
	jsal_push_sphere_bytearray(array);
	bytearray_unref(array);
	return true;
}

static bool
js_CreateByteArrayFromString(int num_args, bool is_ctor, int magic)
{
	bytearray_t* array;
	lstring_t*   string;

	string = jsal_require_lstring_t(0);

	if (!(array = bytearray_from_lstring(string)))
		jsal_error(JS_ERROR, "couldn't create byte array");
	lstr_free(string);
	jsal_push_sphere_bytearray(array);
	bytearray_unref(array);
	return true;
}

static bool
js_CreateColor(int num_args, bool is_ctor, int magic)
{
	int r = jsal_to_int(0);
	int g = jsal_to_int(1);
	int b = jsal_to_int(2);
	int a = num_args >= 4 ? jsal_to_int(3) : 255;

	// clamp components to 8-bit [0-255]
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	a = a < 0 ? 0 : a > 255 ? 255 : a;

	// construct a Color object
	jsal_push_sphere_color(color_new(r, g, b, a));
	return true;
}

static bool
js_CreateColorMatrix(int num_args, bool is_ctor, int magic)
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
js_CreateDirectory(int num_args, bool is_ctor, int magic)
{
	const char* name;

	name = jsal_require_pathname(0, "save", true, true);

	if (!game_mkdir(g_game, name))
		jsal_error(JS_ERROR, "couldn't create directory '%s'", name);
	return false;
}

static bool
js_CreatePerson(int num_args, bool is_ctor, int magic)
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
			jsal_error(JS_ERROR, "couldn't load spriteset '%s'", filename);
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
js_CreateSpriteset(int num_args, bool is_ctor, int magic)
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
	jsal_push_sphere_spriteset(spriteset);
	spriteset_unref(spriteset);
	return true;
}

static bool
js_CreateStringFromByteArray(int num_args, bool is_ctor, int magic)
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
js_CreateStringFromCode(int num_args, bool is_ctor, int magic)
{
	int  code;

	code = jsal_to_int(0);

	if (code < 0 || code > 255)
		jsal_error(JS_RANGE_ERROR, "invalid ASCII character code");

	jsal_push_sprintf("%c", code);
	return true;
}

static bool
js_CreateSurface(int num_args, bool is_ctor, int magic)
{
	color_t     fill_color;
	int         height;
	image_t*    image;
	int         width;

	width = jsal_to_int(0);
	height = jsal_to_int(1);
	fill_color = num_args >= 3 ? jsal_require_sphere_color(2) : color_new(0, 0, 0, 0);

	if (!(image = image_new(width, height)))
		jsal_error(JS_ERROR, "couldn't create surface");
	image_fill(image, fill_color);
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_DeflateByteArray(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	bytearray_t* array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
	int level = n_args >= 2 ? jsal_to_int(1) : -1;

	bytearray_t* new_array;

	if ((level < 0 || level > 9) && n_args >= 2)
		jsal_error(JS_RANGE_ERROR, "invalid compression level");
	if (!(new_array = bytearray_deflate(array, level)))
		jsal_error(JS_ERROR, "couldn't deflate byte array");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_Delay(int num_args, bool is_ctor, int magic)
{
	double timeout;

	timeout = jsal_to_number(0);

	if (timeout < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid delay timeout");
	sphere_sleep(floor(timeout) / 1000);
	return false;
}

static bool
js_DestroyPerson(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_free(person);
	return false;
}

static bool
js_DetachCamera(int num_args, bool is_ctor, int magic)
{
	map_engine_set_subject(NULL);
	return false;
}

static bool
js_DetachInput(int num_args, bool is_ctor, int magic)
{
	map_engine_set_player(PLAYER_1, NULL);
	return false;
}

static bool
js_DetachPlayerInput(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (jsal_get_top() < 1) {
		jsal_error(JS_ERROR, "not a number or string");
	}
	else if (jsal_is_string(0)) {
		name = jsal_get_string(0);
		if (!(person = map_person_by_name(name)))
			jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
		jsal_error(JS_ERROR, "not a number or string");
	}
	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");
	map_engine_set_player(player, NULL);
	return false;
}

static bool
js_DoEvents(int num_args, bool is_ctor, int magic)
{
	sphere_run(true);
	jsal_push_boolean(true);
	return true;
}

static bool
js_DoesFileExist(int num_args, bool is_ctor, int magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, true, false);

	jsal_push_boolean(game_file_exists(g_game, filename));
	return true;
}

static bool
js_DoesPersonExist(int num_args, bool is_ctor, int magic)
{
	const char* name;

	name = jsal_require_string(0);

	jsal_push_boolean(map_person_by_name(name) != NULL);
	return true;
}

static bool
js_EvaluateScript(int num_args, bool is_ctor, int magic)
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
js_EvaluateSystemScript(int num_args, bool is_ctor, int magic)
{
	const char* filename = jsal_require_string(0);

	char path[SPHERE_PATH_MAX];

	sprintf(path, "@/scripts/lib/%s", filename);
	if (!game_file_exists(g_game, path))
		sprintf(path, "#/scripts/%s", filename);
	if (!game_file_exists(g_game, path))
		jsal_error(JS_ERROR, "system script '%s' not found", filename);
	if (!script_eval(path))
		jsal_throw();
	return true;
}

static bool
js_ExecuteGame(int num_args, bool is_ctor, int magic)
{
	path_t*     games_path;
	const char* filename;

	filename = jsal_require_string(0);

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

static bool
js_ExecuteTrigger(int num_args, bool is_ctor, int magic)
{
	int index;
	int layer;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if ((index = map_trigger_at(x, y, layer)) >= 0)
		trigger_activate(index);
	return false;
}

static bool
js_ExecuteZoneScript(int num_args, bool is_ctor, int magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	zone_activate(zone_index);
	return false;
}

static bool
js_ExecuteZones(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	i = 0;
	while ((index = map_zone_at(x, y, layer, i++)) >= 0)
		zone_activate(index);
	return false;
}

static bool
js_Exit(int num_args, bool is_ctor, int magic)
{
	sphere_exit(false);
}

static bool
js_ExitMapEngine(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	map_engine_exit();
	return false;
}

static bool
js_FilledCircle(int num_args, bool is_ctor, int magic)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int radius = jsal_to_number(2);
	color_t color = jsal_require_sphere_color(3);

	double phi;
	int    vcount;

	int i;

	if (screen_skip_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_FilledComplex(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_FilledEllipse(int num_args, bool is_ctor, int magic)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = trunc(jsal_to_number(0));
	int y = trunc(jsal_to_number(1));
	int rx = trunc(jsal_to_number(2));
	int ry = trunc(jsal_to_number(3));
	color_t color = jsal_require_sphere_color(4);

	double phi;
	int    vcount;

	int i;

	if (screen_skip_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_FlipScreen(int num_args, bool is_ctor, int magic)
{
	screen_flip(g_screen, s_frame_rate, true);
	return false;
}

static bool
js_FollowPerson(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (!(leader_name[0] == '\0' || (leader = map_person_by_name(leader_name))))
		jsal_error(JS_REF_ERROR, "no such person '%s'", leader_name);
	if (distance <= 0 && leader_name[0] != '\0')
		jsal_error(JS_RANGE_ERROR, "invalid distance");
	if (!person_set_leader(person, leader, distance))
		jsal_error(JS_ERROR, "invalid circular follow chain");
	return false;
}

static bool
js_GarbageCollect(int num_args, bool is_ctor, int magic)
{
	jsal_gc();
	return false;
}

static bool
js_GetActingPerson(int num_args, bool is_ctor, int magic)
{
	const person_t* person;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	person = map_engine_acting_person();
	if (person == NULL)
		jsal_error(JS_ERROR, "no current person activation or actor unknown");
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetCameraPerson(int num_args, bool is_ctor, int magic)
{
	person_t* subject;

	subject = map_engine_get_subject();
	if (subject == NULL)
		jsal_error(JS_ERROR, "invalid operation, camera not attached");
	jsal_push_string(person_name(subject));
	return true;
}

static bool
js_GetCameraX(int num_args, bool is_ctor, int magic)
{
	point2_t position;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	position = map_get_camera_xy();
	jsal_push_int(position.x);
	return true;
}

static bool
js_GetCameraY(int num_args, bool is_ctor, int magic)
{
	point2_t position;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	position = map_get_camera_xy();
	jsal_push_int(position.y);
	return true;
}

static bool
js_GetClippingRectangle(int num_args, bool is_ctor, int magic)
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
js_GetCurrentMap(int num_args, bool is_ctor, int magic)
{
	path_t* path;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	// GetCurrentMap() in Sphere 1.x returns the map path relative to the
	// 'maps' directory.
	path = game_relative_path(g_game, map_pathname(), "@/maps");
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_GetCurrentPerson(int num_args, bool is_ctor, int magic)
{
	const person_t* person;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	person = map_engine_active_person();
	if (person == NULL)
		jsal_error(JS_ERROR, "no current person activation");
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetCurrentTrigger(int num_args, bool is_ctor, int magic)
{
	int trigger;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	trigger = map_engine_active_trigger();
	if (trigger == -1)
		jsal_error(JS_ERROR, "no current trigger activation");
	jsal_push_int(trigger);
	return true;
}

static bool
js_GetCurrentZone(int num_args, bool is_ctor, int magic)
{
	int zone;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	zone = map_engine_active_zone();
	if (zone == -1)
		jsal_error(JS_ERROR, "no current zone activation");
	jsal_push_int(zone);
	return true;
}

static bool
js_GetDirectoryList(int num_args, bool is_ctor, int magic)
{
	directory_t*  dir;
	const char*   dir_name = "@/";
	const path_t* entry;
	int           index = 0;

	if (num_args >= 1)
		dir_name = jsal_require_pathname(0, NULL, true, false);

	jsal_push_new_array();
	if (!(dir = directory_open(g_game, dir_name)))
		return true;
	while (entry = directory_next(dir)) {
		if (path_is_file(entry))
			continue;
		jsal_push_string(path_hop(entry, path_num_hops(entry) - 1));
		jsal_put_prop_index(-2, index++);
	}
	directory_close(dir);
	return true;
}

static bool
js_GetFileList(int num_args, bool is_ctor, int magic)
{
	directory_t*  dir;
	const char*   dir_name = "@/save";
	const path_t* entry;
	int           index = 0;

	if (num_args >= 1)
		dir_name = jsal_require_pathname(0, NULL, true, false);

	jsal_push_new_array();
	if (!(dir = directory_open(g_game, dir_name)))
		return true;
	while (entry = directory_next(dir)) {
		if (!path_is_file(entry))
			continue;
		jsal_push_string(path_filename(entry));
		jsal_put_prop_index(-2, index++);
	}
	directory_close(dir);
	return true;
}

static bool
js_GetFrameRate(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(s_frame_rate);
	return true;
}

static bool
js_GetGameList(int num_args, bool is_ctor, int magic)
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
	jsal_push_new_array();
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				path = path_new(al_get_fs_entry_name(file_info));
				if (game = game_open(path_cstr(path))) {
					jsal_push_lstring_t(game_manifest(game));
					if (jsal_try_parse(-1)) {
						jsal_push_string(path_cstr(path));
						jsal_put_prop_string(-2, "directory");
						jsal_put_prop_index(-2, j++);
					}
					else {
						jsal_pop(1);
					}
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
js_GetInputPerson(int num_args, bool is_ctor, int magic)
{
	person_t* person;
	int       player = PLAYER_1;

	if (num_args >= 1)
		player = jsal_to_int(0);

	if (player < 0 || player >= PLAYER_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid player constant");
	person = map_engine_get_player(player);
	if (person == NULL)
		jsal_error(JS_ERROR, "player %d not attached", player + 1);
	jsal_push_string(person_name(person));
	return true;
}

static bool
js_GetJoystickAxis(int num_args, bool is_ctor, int magic)
{
	int joy_index = jsal_to_int(0);
	int axis_index = jsal_to_int(1);

	jsal_push_number(joy_position(joy_index, axis_index));
	return true;
}

static bool
js_GetKey(int num_args, bool is_ctor, int magic)
{
	while (kb_queue_len() == 0) {
		sphere_sleep(0.05);
		sphere_run(false);
	}
	jsal_push_int(kb_get_key());
	return true;
}

static bool
js_GetKeyString(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	int keycode = jsal_to_int(0);
	bool shift = n_args >= 2 ? jsal_to_boolean(1) : false;

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
js_GetLayerAngle(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_GetLayerHeight(int num_args, bool is_ctor, int magic)
{
	int     layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_int(layer_size(layer).height);
	return true;
}

static bool
js_GetLayerMask(int num_args, bool is_ctor, int magic)
{
	int layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_sphere_color(layer_get_color_mask(layer));
	return true;
}

static bool
js_GetLayerName(int num_args, bool is_ctor, int magic)
{
	int layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_string(layer_name(layer));
	return true;
}

static bool
js_GetLayerWidth(int num_args, bool is_ctor, int magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_int(layer_size(layer).width);
	return true;
}

static bool
js_GetLocalAddress(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("127.0.0.1");
	return true;
}

static bool
js_GetLocalName(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("localhost");
	return true;
}

static bool
js_GetMapEngine(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return true;
}

static bool
js_GetMapEngineFrameRate(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(map_engine_get_framerate());
	return true;
}

static bool
js_GetMouseWheelEvent(int num_args, bool is_ctor, int magic)
{
	mouse_event_t event;

	do {
		while (mouse_queue_len() == 0) {
			sphere_sleep(0.05);
			sphere_run(false);
		}
		event = mouse_get_event();
	} while (event.key != MOUSE_KEY_WHEEL_UP && event.key != MOUSE_KEY_WHEEL_DOWN);
	jsal_push_int(event.key);
	return true;
}

static bool
js_GetMouseX(int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(x);
	return true;
}

static bool
js_GetMouseY(int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	jsal_push_int(y);
	return true;
}

static bool
js_GetNextAnimatedTile(int num_args, bool is_ctor, int magic)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	jsal_push_int(tileset_get_next(tileset, tile_index));
	return true;
}

static bool
js_GetNumJoysticks(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(joy_num_devices());
	return true;
}

static bool
js_GetNumJoystickAxes(int num_args, bool is_ctor, int magic)
{
	int joy_index;

	joy_index = jsal_to_int(0);

	jsal_push_int(joy_num_axes(joy_index));
	return true;
}

static bool
js_GetNumJoystickButtons(int num_args, bool is_ctor, int magic)
{
	int joy_index;

	joy_index = jsal_to_int(0);

	jsal_push_int(joy_num_buttons(joy_index));
	return true;
}

static bool
js_GetNumLayers(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_int(map_num_layers());
	return true;
}

static bool
js_GetNumMouseWheelEvents(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(mouse_queue_len());
	return true;
}

static bool
js_GetNumTiles(int num_args, bool is_ctor, int magic)
{
	tileset_t* tileset;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	tileset = map_tileset();
	jsal_push_int(tileset_len(tileset));
	return true;
}

static bool
js_GetNumTriggers(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	jsal_push_number(map_num_triggers());
	return true;
}

static bool
js_GetNumZones(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	jsal_push_number(map_num_zones());
	return true;
}

static bool
js_GetObstructingPerson(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_obstructed_at(person, x, y, &obs_person, NULL);
	jsal_push_string(obs_person != NULL ? person_name(obs_person) : "");
	return true;
}

static bool
js_GetObstructingTile(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_obstructed_at(person, x, y, NULL, &tile_index);
	jsal_push_int(tile_index);
	return true;
}

static bool
js_GetPersonAngle(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_number(person_get_angle(person));
	return true;
}

static bool
js_GetPersonBase(int num_args, bool is_ctor, int magic)
{
	rect_t      base;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	base = spriteset_get_base(person_get_spriteset(person));
	jsal_push_new_object();
	jsal_push_int(base.x1); jsal_put_prop_string(-2, "x1");
	jsal_push_int(base.y1); jsal_put_prop_string(-2, "y1");
	jsal_push_int(base.x2); jsal_put_prop_string(-2, "x2");
	jsal_push_int(base.y2); jsal_put_prop_string(-2, "y2");
	return true;
}

static bool
js_GetPersonData(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_GetPersonDirection(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_string(person_get_pose(person));
	return true;
}

static bool
js_GetPersonFollowDistance(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (person_get_leader(person) == NULL)
		jsal_error(JS_TYPE_ERROR, "not a follower");
	jsal_push_int(person_get_trailing(person));
	return true;
}

static bool
js_GetPersonFollowers(int num_args, bool is_ctor, int magic)
{
	vector_t*     all_persons;
	person_t*     candidate;
	int           index = 0;
	const char*   name;
	person_t*     person;

	iter_t iter;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_GetPersonFrame(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_int(person_get_frame(person));
	return true;
}

static bool
js_GetPersonFrameNext(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_int(person_get_frame_delay(person));
	return true;
}

static bool
js_GetPersonFrameRevert(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_int(person_get_revert_delay(person));
	return true;
}

static bool
js_GetPersonIgnoreList(int num_args, bool is_ctor, int magic)
{
	vector_t*   ignore_list;
	const char* name;
	person_t*   person;

	iter_t iter;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_GetPersonLayer(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_int(person_get_layer(person));
	return true;
}

static bool
js_GetPersonLeader(int num_args, bool is_ctor, int magic)
{
	person_t*   leader;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	leader = person_get_leader(person);
	jsal_push_string(person_name(leader));
	return true;
}

static bool
js_GetPersonList(int num_args, bool is_ctor, int magic)
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
js_GetPersonMask(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_sphere_color(person_get_color(person));
	return true;
}

static bool
js_GetPersonOffsetX(int num_args, bool is_ctor, int magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	offset = person_get_offset(person);
	jsal_push_int(offset.x);
	return true;
}

static bool
js_GetPersonOffsetY(int num_args, bool is_ctor, int magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	offset = person_get_offset(person);
	jsal_push_int(offset.y);
	return true;
}

static bool
js_GetPersonSpriteset(int num_args, bool is_ctor, int magic)
{
	const char*  name;
	spriteset_t* spriteset;
	person_t*    person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	spriteset = person_get_spriteset(person);
	jsal_push_sphere_spriteset(spriteset);
	return true;
}

static bool
js_GetPersonSpeedX(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x_speed;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_speed(person, &x_speed, NULL);
	jsal_push_number(x_speed);
	return true;
}

static bool
js_GetPersonSpeedY(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      y_speed;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_speed(person, NULL, &y_speed);
	jsal_push_number(y_speed);
	return true;
}

static bool
js_GetPersonValue(int num_args, bool is_ctor, int magic)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	key = jsal_to_string(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_GetPersonX(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_int(x);
	return true;
}

static bool
js_GetPersonXFloat(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_number(x);
	return true;
}

static bool
js_GetPersonY(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_int(y);
	return true;
}

static bool
js_GetPersonYFloat(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xy(person, &x, &y, true);
	jsal_push_number(y);
	return true;
}

static bool
js_GetPlayerKey(int num_args, bool is_ctor, int magic)
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
js_GetScreenHeight(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(screen_size(g_screen).height);
	return true;
}

static bool
js_GetScreenWidth(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(screen_size(g_screen).width);
	return true;
}

static bool
js_GetSystemArrow(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	if (!(image = legacy_default_arrow_image()))
		jsal_error(JS_REF_ERROR, "missing system arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemDownArrow(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	if (!(image = legacy_default_arrow_down_image()))
		jsal_error(JS_REF_ERROR, "missing system down arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemFont(int num_args, bool is_ctor, int magic)
{
	jsal_push_sphere_font(legacy_default_font());
	return true;
}

static bool
js_GetSystemUpArrow(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	if (!(image = legacy_default_arrow_up_image()))
		jsal_error(JS_REF_ERROR, "missing system up arrow image");
	jsal_push_class_obj(SV1_IMAGE, image_ref(image), false);
	return true;
}

static bool
js_GetSystemWindowStyle(int num_args, bool is_ctor, int magic)
{
	windowstyle_t* windowstyle;

	if (!(windowstyle = legacy_default_windowstyle()))
		jsal_error(JS_REF_ERROR, "missing system windowstyle");
	jsal_push_sphere_windowstyle(windowstyle);
	return true;
}

static bool
js_GetTalkActivationButton(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(map_engine_get_talk_button());
	return true;
}

static bool
js_GetTalkActivationKey(int num_args, bool is_ctor, int magic)
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
js_GetTalkDistance(int num_args, bool is_ctor, int magic)
{
	jsal_push_int(map_engine_get_talk_distance());
	return true;
}

static bool
js_GetTile(int num_args, bool is_ctor, int magic)
{
	int layer;
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	layer = jsal_require_map_layer(2);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_int(map_tile_at(x, y, layer));
	return true;
}

static bool
js_GetTileDelay(int num_args, bool is_ctor, int magic)
{
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	jsal_push_int(tileset_get_delay(tileset, tile_index));
	return true;
}

static bool
js_GetTileHeight(int num_args, bool is_ctor, int magic)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	jsal_push_int(height);
	return true;
}

static bool
js_GetTileImage(int num_args, bool is_ctor, int magic)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (!(image = image_clone(tileset_get_image(tileset, tile_index))))
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_IMAGE, image, false);
	return true;
}

static bool
js_GetTileName(int num_args, bool is_ctor, int magic)
{
	int              tile_index;
	const lstring_t* tile_name;
	tileset_t*       tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tile_name = tileset_get_name(tileset, tile_index);
	jsal_push_lstring_t(tile_name);
	return true;
}

static bool
js_GetTileSurface(int num_args, bool is_ctor, int magic)
{
	image_t*   image;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (!(image = image_clone(tileset_get_image(tileset, tile_index))))
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_SURFACE, image, false);
	return true;
}

static bool
js_GetTileWidth(int num_args, bool is_ctor, int magic)
{
	int        height;
	tileset_t* tileset;
	int        width;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");

	tileset = map_tileset();
	tileset_get_size(tileset, &width, &height);
	jsal_push_int(width);
	return true;
}

static bool
js_GetTime(int num_args, bool is_ctor, int magic)
{
	jsal_push_number(floor(al_get_time() * 1000));
	return true;
}

static bool
js_GetToggleState(int num_args, bool is_ctor, int magic)
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
js_GetTriggerLayer(int num_args, bool is_ctor, int magic)
{
	int layer;
	int trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, NULL, &layer);
	jsal_push_int(layer);
	return true;
}

static bool
js_GetTriggerX(int num_args, bool is_ctor, int magic)
{
	int trigger_index;
	int x;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, &x, NULL, NULL);
	jsal_push_int(x);
	return true;
}

static bool
js_GetTriggerY(int num_args, bool is_ctor, int magic)
{
	int trigger_index;
	int y;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_get_xyz(trigger_index, NULL, &y, NULL);
	jsal_push_int(y);
	return true;
}

static bool
js_GetVersion(int num_args, bool is_ctor, int magic)
{
	jsal_push_number(API_VERSION);
	return true;
}

static bool
js_GetVersionString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string(API_VERSION_STRING);
	return true;
}

static bool
js_GetZoneHeight(int num_args, bool is_ctor, int magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.y2 - bounds.y1);
	return true;
}

static bool
js_GetZoneLayer(int num_args, bool is_ctor, int magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	jsal_push_int(zone_get_layer(zone_index));
	return true;
}

static bool
js_GetZoneSteps(int num_args, bool is_ctor, int magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	jsal_push_int(zone_get_steps(zone_index));
	return true;
}

static bool
js_GetZoneWidth(int num_args, bool is_ctor, int magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.x2 - bounds.x1);
	return true;
}

static bool
js_GetZoneX(int num_args, bool is_ctor, int magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.x1);
	return true;
}

static bool
js_GetZoneY(int num_args, bool is_ctor, int magic)
{
	rect_t bounds;
	int    zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = zone_get_bounds(zone_index);
	jsal_push_int(bounds.y1);
	return true;
}

static bool
js_GrabImage(int num_args, bool is_ctor, int magic)
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
js_GrabSurface(int num_args, bool is_ctor, int magic)
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
js_GradientCircle(int num_args, bool is_ctor, int magic)
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

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	radius = trunc(jsal_to_number(2));
	inner_color = jsal_require_sphere_color(3);
	outer_color = jsal_require_sphere_color(4);

	if (screen_skip_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_GradientComplex(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_GradientEllipse(int num_args, bool is_ctor, int magic)
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

	x = trunc(jsal_to_number(0));
	y = trunc(jsal_to_number(1));
	rx = trunc(jsal_to_number(2));
	ry = trunc(jsal_to_number(3));
	inner_color = jsal_require_sphere_color(4);
	outer_color = jsal_require_sphere_color(5);

	if (screen_skip_frame(g_screen))
		return false;
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
	return false;
}

static bool
js_GradientLine(int num_args, bool is_ctor, int magic)
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

	if (screen_skip_frame(g_screen))
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
js_GradientRectangle(int num_args, bool is_ctor, int magic)
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

	if (screen_skip_frame(g_screen))
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
js_GradientTriangle(int num_args, bool is_ctor, int magic)
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

	if (screen_skip_frame(g_screen))
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
js_HashByteArray(int num_args, bool is_ctor, int magic)
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
js_HashFromFile(int num_args, bool is_ctor, int magic)
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
js_IgnorePersonObstructions(int num_args, bool is_ctor, int magic)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	ignoring = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_ignore_persons(person, ignoring);
	return false;
}

static bool
js_IgnoreTileObstructions(int num_args, bool is_ctor, int magic)
{
	bool        ignoring;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	ignoring = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_ignore_tiles(person, ignoring);
	return false;
}

static bool
js_InflateByteArray(int num_args, bool is_ctor, int magic)
{
	bytearray_t*  array;
	int           max_size;
	bytearray_t*  new_array;

	array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
	max_size = num_args >= 2 ? jsal_to_int(1) : 0;

	if (max_size < 0)
		jsal_error(JS_RANGE_ERROR, "negative buffer size");
	if (!(new_array = bytearray_inflate(array, max_size)))
		jsal_error(JS_ERROR, "couldn't inflate byte array");
	jsal_push_sphere_bytearray(new_array);
	return true;
}

static bool
js_IsAnyKeyPressed(int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(kb_is_any_key_down());
	return true;
}

static bool
js_IsCameraAttached(int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(map_engine_get_subject() != NULL);
	return true;
}

static bool
js_IsCommandQueueEmpty(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_boolean(!person_moving(person));
	return true;
}

static bool
js_IsIgnoringPersonObstructions(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_boolean(person_get_ignore_persons(person));
	return true;
}

static bool
js_IsIgnoringTileObstructions(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_boolean(person_get_ignore_tiles(person));
	return true;
}

static bool
js_IsInputAttached(int num_args, bool is_ctor, int magic)
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
			jsal_error(JS_REF_ERROR, "no such person '%s'", name);
		player = -1;
		for (i = PLAYER_1; i < PLAYER_MAX; ++i) {
			if (person == map_engine_get_player(i)) {
				player = i;
				break;
			}
		}
		if (player == -1) {
			jsal_push_boolean(false);
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
js_IsJoystickButtonPressed(int num_args, bool is_ctor, int magic)
{
	int joy_index = jsal_to_int(0);
	int button = jsal_to_int(1);

	jsal_push_boolean(joy_is_button_down(joy_index, button));
	return true;
}

static bool
js_IsKeyPressed(int num_args, bool is_ctor, int magic)
{
	int keycode = jsal_to_int(0);

	jsal_push_boolean(kb_is_key_down(keycode));
	return true;
}

static bool
js_IsLayerReflective(int num_args, bool is_ctor, int magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_boolean(layer_get_reflective(layer));
	return true;
}

static bool
js_IsLayerVisible(int num_args, bool is_ctor, int magic)
{
	int layer;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	jsal_push_boolean(layer_get_visible(layer));
	return true;
}

static bool
js_IsMapEngineRunning(int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(map_engine_running());
	return true;
}

static bool
js_IsMouseButtonPressed(int num_args, bool is_ctor, int magic)
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
js_IsPersonObstructed(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	int         x;
	int         y;

	name = jsal_require_string(0);
	x = jsal_require_int(1);
	y = jsal_require_int(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_boolean(person_obstructed_at(person, x, y, NULL, NULL));
	return true;
}

static bool
js_IsPersonVisible(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_boolean(person_get_visible(person));	return true;
}

static bool
js_IsTriggerAt(int num_args, bool is_ctor, int magic)
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
js_Line(int num_args, bool is_ctor, int magic)
{
	color_t color;
	float   x1, x2;
	float   y1, y2;

	x1 = trunc(jsal_to_number(0)) + 0.5;
	y1 = trunc(jsal_to_number(1)) + 0.5;
	x2 = trunc(jsal_to_number(2)) + 0.5;
	y2 = trunc(jsal_to_number(3)) + 0.5;
	color = jsal_require_sphere_color(4);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return false;
}

static bool
js_LineSeries(int num_args, bool is_ctor, int magic)
{
	color_t         color;
	int             num_points;
	int             type;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	int i;

	color = jsal_require_sphere_color(1);
	type = num_args >= 3 ? jsal_to_int(2) : LINE_MULTIPLE;

	if (!jsal_is_array(0))
		jsal_error(JS_ERROR, "first argument must be an array");
	num_points = jsal_get_length(0);
	if (num_points < 2)
		jsal_error(JS_RANGE_ERROR, "two or more vertices required");
	if (num_points > INT_MAX)
		jsal_error(JS_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		jsal_error(JS_ERROR, "couldn't allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x"); x = jsal_to_int(-1); jsal_pop(1);
		jsal_get_prop_string(-1, "y"); y = jsal_to_int(-1); jsal_pop(1);
		jsal_pop(1);
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
	return false;
}

static bool
js_ListenOnPort(int num_args, bool is_ctor, int magic)
{
	int          port;
	socket_v1_t* socket;

	port = jsal_to_int(0);

	if (socket = socket_v1_new_server(port))
		jsal_push_class_obj(SV1_SOCKET, socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_LoadAnimation(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;
	const char*  filename;

	filename = jsal_require_pathname(0, "animations", true, false);
	if (!(anim = animation_new(filename)))
		jsal_error(JS_ERROR, "couldn't load animation '%s'", filename);
	jsal_push_class_obj(SV1_ANIMATION, anim, false);
	return true;
}

static bool
js_LoadFont(int num_args, bool is_ctor, int magic)
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
js_LoadImage(int num_args, bool is_ctor, int magic)
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
js_LoadSound(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	sound_t*    sound;

	filename = jsal_require_pathname(0, "sounds", true, false);

	if (!(sound = sound_new(filename)))
		jsal_error(JS_ERROR, "couldn't load sound '%s'", filename);
	jsal_push_class_obj(SV1_SOUND, sound, false);
	return true;
}

static bool
js_LoadSoundEffect(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	int         mode;
	sample_t*   sample;

	filename = jsal_require_pathname(0, "sounds", true, false);
	mode = num_args >= 2 ? jsal_to_int(1) : SE_SINGLE;

	if (!(sample = sample_new(filename, mode == SE_MULTIPLE)))
		jsal_error(JS_ERROR, "couldn't load sound effect '%s'", filename);
	jsal_push_class_obj(SV1_SOUND_EFFECT, sample, false);
	return true;
}

static bool
js_LoadSpriteset(int num_args, bool is_ctor, int magic)
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
js_LoadSurface(int num_args, bool is_ctor, int magic)
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
js_LoadWindowStyle(int num_args, bool is_ctor, int magic)
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
js_MapEngine(int num_args, bool is_ctor, int magic)
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
js_MapToScreenX(int num_args, bool is_ctor, int magic)
{
	int      layer;
	point2_t offset;
	double   x;

	layer = jsal_require_map_layer(0);
	x = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	offset = map_xy_from_screen(point2(0, 0));
	jsal_push_int(x - offset.x);
	return true;
}

static bool
js_MapToScreenY(int num_args, bool is_ctor, int magic)
{
	int      layer;
	point2_t offset;
	double   y;

	layer = jsal_require_map_layer(0);
	y = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	offset = map_xy_from_screen(point2(0, 0));
	jsal_push_int(y - offset.y);
	return true;
}

static bool
js_OpenAddress(int num_args, bool is_ctor, int magic)
{
	const char*  hostname;
	int          port;
	socket_v1_t* socket;

	hostname = jsal_require_string(0);
	port = jsal_to_int(1);

	if (socket = socket_v1_new_client(hostname, port))
		jsal_push_class_obj(SV1_SOCKET, socket, false);
	else
		jsal_push_null();
	return true;
}

static bool
js_OpenFile(int num_args, bool is_ctor, int magic)
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
js_OpenLog(int num_args, bool is_ctor, int magic)
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
js_OpenRawFile(int num_args, bool is_ctor, int magic)
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
js_OutlinedCircle(int num_args, bool is_ctor, int magic)
{
	float x = trunc(jsal_to_number(0)) + 0.5;
	float y = trunc(jsal_to_number(1)) + 0.5;
	float radius = trunc(jsal_to_number(2));
	color_t color = jsal_require_sphere_color(3);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	return false;
}

static bool
js_OutlinedComplex(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_OutlinedEllipse(int num_args, bool is_ctor, int magic)
{
	float x = jsal_to_int(0) + 0.5;
	float y = jsal_to_int(1) + 0.5;
	float rx = jsal_to_int(2);
	float ry = jsal_to_int(3);
	color_t color = jsal_require_sphere_color(4);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_ellipse(x, y, rx, ry, nativecolor(color), 1.0);
	return false;
}

static bool
js_OutlinedRectangle(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	float x1 = jsal_to_int(0) + 0.5;
	float y1 = jsal_to_int(1) + 0.5;
	float x2 = x1 + jsal_to_int(2) - 1;
	float y2 = y1 + jsal_to_int(3) - 1;
	color_t color = jsal_require_sphere_color(4);
	int thickness = n_args >= 6 ? jsal_to_int(5) : 1;

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return false;
}

static bool
js_OutlinedRoundRectangle(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	float x = jsal_to_int(0) + 0.5;
	float y = jsal_to_int(1) + 0.5;
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	float radius = jsal_to_number(4);
	color_t color = jsal_require_sphere_color(5);
	int thickness = n_args >= 7 ? jsal_to_int(6) : 1;

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_rounded_rectangle(x, y, x + w - 1, y + h - 1, radius, radius, nativecolor(color), thickness);
	return false;
}

static bool
js_Point(int num_args, bool is_ctor, int magic)
{
	float x = jsal_to_int(0) + 0.5;
	float y = jsal_to_int(1) + 0.5;
	color_t color = jsal_require_sphere_color(2);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_pixel(x, y, nativecolor(color));
	return false;
}

static bool
js_PointSeries(int num_args, bool is_ctor, int magic)
{
	color_t color = jsal_require_sphere_color(1);

	int             num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	int i;

	if (!jsal_is_array(0))
		jsal_error(JS_ERROR, "first argument must be an array");
	num_points = jsal_get_length(0);
	if (num_points < 1)
		jsal_error(JS_RANGE_ERROR, "one or more vertices required");
	if (num_points > INT_MAX)
		jsal_error(JS_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		jsal_error(JS_ERROR, "couldn't allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x"); x = jsal_to_int(-1); jsal_pop(1);
		jsal_get_prop_string(-1, "y"); y = jsal_to_int(-1); jsal_pop(1);
		jsal_pop(1);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	galileo_reset();
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	free(vertices);
	return false;
}

static bool
js_Polygon(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_Print(int num_args, bool is_ctor, int magic)
{
	const char* text;

	text = jsal_require_string(0);

	debugger_log(text, PRINT_NORMAL, true);
	return false;
}

static bool
js_QueuePersonCommand(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_QueuePersonScript(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (!person_queue_script(person, script, immediate))
		jsal_error(JS_ERROR, "couldn't enqueue script");
	return false;
}

static bool
js_Rectangle(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	color_t color = jsal_require_sphere_color(4);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	return false;
}

static bool
js_RemoveDirectory(int num_args, bool is_ctor, int magic)
{
	const char* name;

	name = jsal_require_pathname(0, "save", true, true);
	if (!game_rmdir(g_game, name))
		jsal_error(JS_ERROR, "couldn't remove directory '%s'", name);
	return false;
}

static bool
js_RemoveFile(int num_args, bool is_ctor, int magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, "save", true, true);
	if (!game_unlink(g_game, filename))
		jsal_error(JS_ERROR, "couldn't delete file '%s'", filename);
	return false;
}

static bool
js_RemoveTrigger(int num_args, bool is_ctor, int magic)
{
	int trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	map_remove_trigger(trigger_index);
	return false;
}

static bool
js_RemoveZone(int num_args, bool is_ctor, int magic)
{
	int zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	map_remove_zone(zone_index);
	return false;
}

static bool
js_Rename(int num_args, bool is_ctor, int magic)
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
js_RenderMap(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	map_engine_draw_map();
	return false;
}

static bool
js_ReplaceTilesOnLayer(int num_args, bool is_ctor, int magic)
{
	int        layer;
	int        new_index;
	int        old_index;
	tileset_t* tileset;

	layer = jsal_require_map_layer(0);
	old_index = jsal_to_int(1);
	new_index = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (old_index < 0 || old_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid old tile index");
	if (new_index < 0 || new_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid new tile index");
	layer_replace_tiles(layer, old_index, new_index);
	return false;
}

static bool
js_RequireScript(int num_args, bool is_ctor, int magic)
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
		jsal_push_boolean(true);
		jsal_put_prop_string(-2, filename);
		if (!script_eval(filename))
			jsal_throw();
	}
	jsal_pop(3);
	return false;
}

static bool
js_RequireSystemScript(int num_args, bool is_ctor, int magic)
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
		jsal_push_boolean(true);
		jsal_put_prop_string(-2, path);
		if (!script_eval(path))
			jsal_throw();
	}
	jsal_pop(2);
	return false;
}

static bool
js_RestartGame(int num_args, bool is_ctor, int magic)
{
	sphere_restart();
}

static bool
js_RoundRectangle(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	float radius = jsal_to_number(4);
	color_t color = jsal_require_sphere_color(5);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_rounded_rectangle(x, y, x + w, y + h, radius, radius, nativecolor(color));
	return false;
}

static bool
js_ScreenToMapX(int num_args, bool is_ctor, int magic)
{
	int      layer;
	point2_t map_xy;
	double   x;

	layer = jsal_require_map_layer(0);
	x = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	map_xy = map_xy_from_screen(point2(x, 0));
	jsal_push_int(map_xy.x);
	return true;
}

static bool
js_ScreenToMapY(int num_args, bool is_ctor, int magic)
{
	int      layer;
	point2_t map_xy;
	double   y;

	layer = jsal_require_map_layer(0);
	y = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	map_xy = map_xy_from_screen(point2(0, y));
	jsal_push_int(map_xy.y);
	return true;
}

static bool
js_SetCameraX(int num_args, bool is_ctor, int magic)
{
	int      new_x;
	point2_t position;

	new_x = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	position = map_get_camera_xy();
	map_set_camera_xy(point2(new_x, position.y));
	return false;
}

static bool
js_SetCameraY(int num_args, bool is_ctor, int magic)
{
	int      new_y;
	point2_t position;

	new_y = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	position = map_get_camera_xy();
	map_set_camera_xy(point2(position.x, new_y));
	return false;
}

static bool
js_SetClippingRectangle(int num_args, bool is_ctor, int magic)
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
		rect(x, y, x + width, y + height));
	return false;
}

static bool
js_SetColorMask(int num_args, bool is_ctor, int magic)
{
	color_t new_mask;
	int     num_frames;

	new_mask = jsal_require_sphere_color(0);
	num_frames = num_args >= 2 ? jsal_to_int(1) : 0;

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid number of frames");
	map_engine_fade_to(new_mask, num_frames);
	return false;
}

static bool
js_SetDefaultMapScript(int num_args, bool is_ctor, int magic)
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
js_SetDefaultPersonScript(int num_args, bool is_ctor, int magic)
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
js_SetDelayScript(int num_args, bool is_ctor, int magic)
{
	int num_frames;
	script_t* script;

	num_frames = jsal_to_int(0);
	script = jsal_require_sphere_script(1, "%/delayScript.js");

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	map_engine_defer(script, num_frames);
	return false;
}

static bool
js_SetFrameRate(int num_args, bool is_ctor, int magic)
{
	int framerate = jsal_to_int(0);

	if (framerate < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");
	s_frame_rate = framerate;
	return false;
}

static bool
js_SetLayerAngle(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerHeight(int num_args, bool is_ctor, int magic)
{
	int layer;
	int new_height;

	layer = jsal_require_map_layer(0);
	new_height = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (new_height <= 0)
		jsal_error(JS_ERROR, "height must be positive and nonzero (got: %d)", new_height);
	if (!layer_resize(layer, layer_size(layer).width, new_height))
		jsal_error(JS_ERROR, "couldn't resize layer '%d'", layer);
	return false;
}

static bool
js_SetLayerMask(int num_args, bool is_ctor, int magic)
{
	int     layer;
	color_t mask;

	layer = jsal_require_map_layer(0);
	mask = jsal_require_sphere_color(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	layer_set_color_mask(layer, mask);
	return false;
}

static bool
js_SetLayerReflective(int num_args, bool is_ctor, int magic)
{
	int layer;
	bool reflective;

	layer = jsal_require_map_layer(0);
	reflective = jsal_to_boolean(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	layer_set_reflective(layer, reflective);
	return false;
}

static bool
js_SetLayerRenderer(int num_args, bool is_ctor, int magic)
{
	int       layer;
	char      script_name[50];
	script_t* script;

	layer = jsal_require_map_layer(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	sprintf(script_name, "[layer %d render script]", layer);
	script = jsal_require_sphere_script(1, script_name);
	layer_on_render(layer, script);
	script_unref(script);
	return false;
}

static bool
js_SetLayerScaleFactorX(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerScaleFactorY(int num_args, bool is_ctor, int magic)
{
	jsal_error(JS_ERROR, "not implemented");
	return false;
}

static bool
js_SetLayerSize(int num_args, bool is_ctor, int magic)
{
	int layer;
	int new_height;
	int new_width;

	layer = jsal_require_map_layer(0);
	new_width = jsal_to_int(1);
	new_height = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (new_width <= 0 || new_height <= 0)
		jsal_error(JS_ERROR, "invalid layer dimensions");
	if (!layer_resize(layer, new_width, new_height))
		jsal_error(JS_ERROR, "couldn't resize layer");
	return false;
}

static bool
js_SetLayerVisible(int num_args, bool is_ctor, int magic)
{
	bool visible;
	int  layer;

	layer = jsal_require_map_layer(0);
	visible = jsal_to_boolean(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	layer_set_visible(layer, visible);
	return false;
}

static bool
js_SetLayerWidth(int num_args, bool is_ctor, int magic)
{
	int layer;
	int new_width;

	layer = jsal_require_map_layer(0);
	new_width = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (new_width <= 0)
		jsal_error(JS_ERROR, "width must be positive and nonzero (got: %d)", new_width);
	if (!layer_resize(layer, new_width, layer_size(layer).height))
		jsal_error(JS_ERROR, "couldn't resize layer '%d'", layer);
	return false;
}

static bool
js_SetMapEngineFrameRate(int num_args, bool is_ctor, int magic)
{
	int framerate;

	framerate = jsal_to_int(0);

	if (framerate < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame rate");

	map_engine_set_framerate(framerate);
	return false;
}

static bool
js_SetMousePosition(int num_args, bool is_ctor, int magic)
{
	int x;
	int y;

	x = jsal_to_int(0);
	y = jsal_to_int(1);
	screen_set_mouse_xy(g_screen, x, y);
	return false;
}

static bool
js_SetNextAnimatedTile(int num_args, bool is_ctor, int magic)
{
	int        next_index;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	next_index = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (next_index < 0 || next_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index for next tile");
	tileset_set_next(tileset, tile_index, next_index);
	return false;
}

static bool
js_SetPersonAngle(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      theta;

	name = jsal_require_string(0);
	theta = jsal_to_number(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_angle(person, theta);
	return false;
}

static bool
js_SetPersonData(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);

	jsal_require_object_coercible(1);
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "personData");
	jsal_dup(1); jsal_put_prop_string(-2, name);
	return false;
}

static bool
js_SetPersonDirection(int num_args, bool is_ctor, int magic)
{
	const char* name;
	const char* new_dir;
	person_t*   person;

	name = jsal_require_string(0);
	new_dir = jsal_require_string(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_pose(person, new_dir);
	return false;
}

static bool
js_SetPersonFollowDistance(int num_args, bool is_ctor, int magic)
{
	int         distance;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	distance = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (person_get_leader(person) == NULL)
		jsal_error(JS_TYPE_ERROR, "person has no leader");
	if (distance <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid distance");
	person_set_trailing(person, distance);
	return false;
}

static bool
js_SetPersonFrame(int num_args, bool is_ctor, int magic)
{
	int         frame_index;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	frame_index = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_frame(person, frame_index);
	return false;
}

static bool
js_SetPersonFrameNext(int num_args, bool is_ctor, int magic)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = jsal_require_string(0);
	num_frames = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	person_set_frame_delay(person, num_frames);
	return false;
}

static bool
js_SetPersonFrameRevert(int num_args, bool is_ctor, int magic)
{
	const char* name;
	int         num_frames;
	person_t*   person;

	name = jsal_require_string(0);
	num_frames = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (num_frames < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	person_set_revert_delay(person, num_frames);
	return false;
}

static bool
js_SetPersonIgnoreList(int num_args, bool is_ctor, int magic)
{
	const char* name;
	int         num_ignores;
	person_t*   person;

	int i;

	name = jsal_require_string(0);

	jsal_require_object_coercible(1);
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (!jsal_is_array(1))
		jsal_error(JS_RANGE_ERROR, "not an array");
	person_clear_ignores(person);
	num_ignores = (int)jsal_get_length(1);
	for (i = 0; i < num_ignores; ++i) {
		jsal_get_prop_index(1, i);
		person_ignore_name(person, jsal_require_string(-1));
		jsal_pop(1);
	}
	return false;
}

static bool
js_SetPersonLayer(int num_args, bool is_ctor, int magic)
{
	int         layer;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	layer = jsal_require_map_layer(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_layer(person, layer);
	return false;
}

static bool
js_SetPersonMask(int num_args, bool is_ctor, int magic)
{
	color_t     color;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	color = jsal_require_sphere_color(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_color(person, color);
	return false;
}

static bool
js_SetPersonOffsetX(int num_args, bool is_ctor, int magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_x;

	name = jsal_require_string(0);
	new_x = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	offset = person_get_offset(person);
	person_set_offset(person, point2(new_x, offset.y));
	return false;
}

static bool
js_SetPersonOffsetY(int num_args, bool is_ctor, int magic)
{
	const char* name;
	point2_t    offset;
	person_t*   person;
	int         new_y;

	name = jsal_require_string(0);
	new_y = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	offset = person_get_offset(person);
	person_set_offset(person, point2(offset.x, new_y));
	return false;
}

static bool
js_SetPersonScaleAbsolute(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (width < 0 || height < 0)
		jsal_error(JS_RANGE_ERROR, "invalid scale dimensions");
	spriteset = person_get_spriteset(person);
	sprite_w = spriteset_width(spriteset);
	sprite_h = spriteset_height(spriteset);
	person_set_scale(person, width / sprite_w, height / sprite_h);
	return false;
}

static bool
js_SetPersonScaleFactor(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      scale_x;
	double      scale_y;

	name = jsal_require_string(0);
	scale_x = jsal_to_number(1);
	scale_y = jsal_to_number(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	if (scale_x < 0.0 || scale_y < 0.0)
		jsal_error(JS_RANGE_ERROR, "invalid scale factor(s)");
	person_set_scale(person, scale_x, scale_y);
	return false;
}

static bool
js_SetPersonScript(int num_args, bool is_ctor, int magic)
{
	lstring_t*  codestring;
	const char* name;
	person_t*   person;
	script_t*   script;
	int         type;

	name = jsal_require_string(0);
	type = jsal_require_int(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_SetPersonSpeed(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      speed;

	name = jsal_require_string(0);
	speed = jsal_to_number(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_speed(person, speed, speed);
	return false;
}

static bool
js_SetPersonSpeedXY(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	double      x_speed;
	double      y_speed;

	name = jsal_require_string(0);
	x_speed = jsal_to_number(1);
	y_speed = jsal_to_number(2);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_speed(person, x_speed, y_speed);
	return false;
}

static bool
js_SetPersonSpriteset(int num_args, bool is_ctor, int magic)
{
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = jsal_require_string(0);
	spriteset = jsal_require_sphere_spriteset(1);

	if (!(person = map_person_by_name(name))) {
		spriteset_unref(spriteset);
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	}
	person_set_spriteset(person, spriteset);
	spriteset_unref(spriteset);
	return false;
}

static bool
js_SetPersonValue(int num_args, bool is_ctor, int magic)
{
	const char* key;
	const char* name;
	person_t*   person;

	name = jsal_require_string(0);
	key = jsal_to_string(1);

	jsal_normalize_index(2);
	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
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
js_SetPersonVisible(int num_args, bool is_ctor, int magic)
{
	const char* name;
	person_t*   person;
	bool        visible;

	name = jsal_require_string(0);
	visible = jsal_to_boolean(1);

	if (!(person = map_person_by_name(name)))
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_set_visible(person, visible);
	return false;
}

static bool
js_SetPersonX(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, new_x, y, layer);
	return false;
}

static bool
js_SetPersonXYFloat(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	layer = person_get_layer(person);
	person_set_xyz(person, x, y, layer);
	return false;
}

static bool
js_SetPersonY(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_REF_ERROR, "no such person '%s'", name);
	person_get_xyz(person, &x, &y, &layer, false);
	person_set_xyz(person, x, new_y, layer);
	return false;
}

static bool
js_SetRenderScript(int num_args, bool is_ctor, int magic)
{
	script_t* script;

	script = jsal_require_sphere_script(0, "%/renderScript.js");

	map_engine_on_render(script);
	script_unref(script);
	return false;
}

static bool
js_SetTalkActivationButton(int num_args, bool is_ctor, int magic)
{
	int button_id;

	button_id = jsal_to_int(0);

	map_engine_set_talk_button(button_id);
	return false;
}

static bool
js_SetTalkActivationKey(int num_args, bool is_ctor, int magic)
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
js_SetTalkDistance(int num_args, bool is_ctor, int magic)
{
	int pixels;

	pixels = jsal_to_int(0);

	if (pixels < 0)
		jsal_error(JS_RANGE_ERROR, "invalid talk distance");

	map_engine_set_talk_distance(pixels);
	return false;
}

static bool
js_SetTile(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");

	layer_set_tile(layer, x, y, tile_index);
	return false;
}

static bool
js_SetTileDelay(int num_args, bool is_ctor, int magic)
{
	int        delay;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	delay = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	if (delay < 0)
		jsal_error(JS_RANGE_ERROR, "invalid frame count");
	tileset_set_delay(tileset, tile_index, delay);
	return false;
}

static bool
js_SetTileImage(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
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
js_SetTileName(int num_args, bool is_ctor, int magic)
{
	lstring_t* name;
	int        tile_index;
	tileset_t* tileset;

	tile_index = jsal_to_int(0);
	name = jsal_require_lstring_t(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		jsal_error(JS_RANGE_ERROR, "invalid tile index");
	tileset_set_name(tileset, tile_index, name);
	lstr_free(name);
	return false;
}

static bool
js_SetTileSurface(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
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
js_SetTriggerLayer(int num_args, bool is_ctor, int magic)
{
	int layer;
	int trigger_index;

	trigger_index = jsal_to_int(0);
	layer = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_set_layer(trigger_index, layer);
	return false;
}

static bool
js_SetTriggerScript(int num_args, bool is_ctor, int magic)
{
	script_t*  script;
	lstring_t* script_name;
	int        trigger_index;

	trigger_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
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
js_SetTriggerXY(int num_args, bool is_ctor, int magic)
{
	int trigger_index;
	int x;
	int y;

	trigger_index = jsal_to_int(0);
	x = jsal_to_int(1);
	y = jsal_to_int(2);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		jsal_error(JS_RANGE_ERROR, "invalid trigger index");
	trigger_set_xy(trigger_index, x, y);
	return false;
}

static bool
js_SetUpdateScript(int num_args, bool is_ctor, int magic)
{
	script_t* script;

	script = jsal_require_sphere_script(0, "%/updateScript.js");

	map_engine_on_update(script);
	script_unref(script);
	return false;
}

static bool
js_SetZoneDimensions(int num_args, bool is_ctor, int magic)
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
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		jsal_error(JS_RANGE_ERROR, "invalid zone size");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		jsal_error(JS_RANGE_ERROR, "zone area out of bounds");
	zone_set_bounds(zone_index, rect(x, y, x + width, y + height));
	return false;
}

static bool
js_SetZoneLayer(int num_args, bool is_ctor, int magic)
{
	int layer;
	int zone_index;

	zone_index = jsal_to_int(0);
	layer = jsal_require_map_layer(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	zone_set_layer(zone_index, layer);
	return false;
}

static bool
js_SetZoneScript(int num_args, bool is_ctor, int magic)
{
	script_t*  script;
	lstring_t* script_name;
	int        zone_index;

	zone_index = jsal_to_int(0);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
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
js_SetZoneSteps(int num_args, bool is_ctor, int magic)
{
	int zone_index;
	int steps;

	zone_index = jsal_to_int(0);
	steps = jsal_to_int(1);

	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	if (zone_index < 0 || zone_index >= map_num_zones())
		jsal_error(JS_RANGE_ERROR, "invalid zone index");
	if (steps <= 0)
		jsal_error(JS_RANGE_ERROR, "steps must be positive (got: %d)", steps);
	zone_set_steps(zone_index, steps);
	return false;
}

static bool
js_Triangle(int num_args, bool is_ctor, int magic)
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

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, nativecolor(color));
	return false;
}

static bool
js_UnbindJoystickButton(int num_args, bool is_ctor, int magic)
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
js_UnbindKey(int num_args, bool is_ctor, int magic)
{
	int keycode = jsal_to_int(0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		jsal_error(JS_RANGE_ERROR, "invalid key constant");
	kb_bind_key(keycode, NULL, NULL);
	return false;
}

static bool
js_UpdateMapEngine(int num_args, bool is_ctor, int magic)
{
	if (!map_engine_running())
		jsal_error(JS_ERROR, "no active map engine");
	map_engine_update();
	return false;
}

static void
js_Animation_finalize(void* host_ptr)
{
	animation_unref(host_ptr);
}

static bool
js_Animation_get_height(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_height(anim));
	return true;
}

static bool
js_Animation_get_width(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_width(anim));
	return true;
}

static bool
js_Animation_drawFrame(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_number(0);
	int y = jsal_to_number(1);

	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	image_draw(animation_frame(anim), x, y);
	return false;
}

static bool
js_Animation_drawZoomedFrame(int num_args, bool is_ctor, int magic)
{
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
}

static bool
js_Animation_getDelay(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);
	jsal_pop(1);
	jsal_push_int(animation_delay(anim));
	return true;
}

static bool
js_Animation_getNumFrames(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);

	jsal_push_int(animation_num_frames(anim));
	return true;
}

static bool
js_Animation_readNextFrame(int num_args, bool is_ctor, int magic)
{
	animation_t* anim;

	jsal_push_this();
	anim = jsal_require_class_obj(-1, SV1_ANIMATION);
	jsal_pop(1);
	animation_update(anim);
	return false;
}

static void
js_ByteArray_finalize(void* host_ptr)
{
	bytearray_unref(host_ptr);
}

static bool
js_ByteArray_get_length(int num_args, bool is_ctor, int magic)
{
	bytearray_t* array;

	jsal_push_this();
	array = jsal_require_class_obj(-1, SV1_BYTE_ARRAY);
	jsal_pop(1);
	jsal_push_int(bytearray_len(array));
	return true;
}

static bool
js_ByteArray_concat(int num_args, bool is_ctor, int magic)
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
js_ByteArray_slice(int num_args, bool is_ctor, int magic)
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
js_ByteArray_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object byte_array]");
	return true;
}

static bool
js_Color_get_red(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->r);
	return true;
}

static bool
js_Color_get_green(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->g);
	return true;
}

static bool
js_Color_get_blue(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->b);
	return true;
}

static bool
js_Color_get_alpha(int num_args, bool is_ctor, int magic)
{
	color_t* color;

	jsal_push_this();
	color = jsal_require_class_obj(-1, SV1_COLOR);

	jsal_push_uint(color->a);
	return true;
}

static bool
js_Color_set_red(int num_args, bool is_ctor, int magic)
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
js_Color_set_green(int num_args, bool is_ctor, int magic)
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
js_Color_set_blue(int num_args, bool is_ctor, int magic)
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
js_Color_set_alpha(int num_args, bool is_ctor, int magic)
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
js_Color_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object color]");
	return true;
}

static bool
js_ColorMatrix_apply(int num_args, bool is_ctor, int magic)
{
	color_t color = jsal_require_sphere_color(0);

	colormatrix_t matrix;

	jsal_push_this();
	matrix = jsal_require_sphere_colormatrix(-1);
	jsal_pop(1);
	jsal_push_sphere_color(color_transform(color, matrix));
	return true;
}

static bool
js_ColorMatrix_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object colormatrix]");
	return true;
}

static void
js_Color_finalize(void* host_ptr)
{
	free(host_ptr);
}

static void
js_File_finalize(void* host_ptr)
{
	kev_close(host_ptr);
}

static bool
js_File_close(int num_args, bool is_ctor, int magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);

	jsal_set_class_ptr(-1, NULL);
	kev_close(file);
	return false;
}

static bool
js_File_flush(int num_args, bool is_ctor, int magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	kev_save(file);
	return false;
}

static bool
js_File_getKey(int num_args, bool is_ctor, int magic)
{
	int index = jsal_to_int(0);

	kev_file_t* file;
	const char* key;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	if (key = kev_get_key(file, index))
		jsal_push_string(key);
	else
		jsal_push_null();
	return true;
}

static bool
js_File_getNumKeys(int num_args, bool is_ctor, int magic)
{
	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	jsal_push_int(kev_num_keys(file));
	return true;
}

static bool
js_File_read(int num_args, bool is_ctor, int magic)
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
js_File_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object file]");
	return true;
}

static bool
js_File_write(int num_args, bool is_ctor, int magic)
{
	const char* key = jsal_to_string(0);

	kev_file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_FILE);
	jsal_pop(1);
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
js_Font_clone(int num_args, bool is_ctor, int magic)
{
	font_t* dolly_font;
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_pop(1);
	if (!(dolly_font = font_clone(font)))
		jsal_error(JS_ERROR, "couldn't clone font");
	jsal_push_sphere_font(dolly_font);
	return true;
}

static bool
js_Font_drawText(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	const char* text = jsal_to_string(2);

	font_t* font;
	color_t mask;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_get_prop_string(-1, "\xFF" "color_mask");
	mask = jsal_require_sphere_color(-1);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	font_draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
	return false;
}

static bool
js_Font_drawTextBox(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	int offset = jsal_to_int(4);
	const char* text = jsal_to_string(5);

	font_t*     font;
	int         line_height;
	const char* line_text;
	color_t     mask;
	int         num_lines;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_get_prop_string(-1, "\xFF" "color_mask");
	mask = jsal_require_sphere_color(-1);

	if (screen_skip_frame(g_screen))
		return false;
	jsal_push_new_function(js_Font_wordWrapString, "wordWrapString", 0, 0);
	jsal_push_this();
	jsal_push_string(text);
	jsal_push_int(w);
	jsal_call_method(2);
	jsal_get_prop_string(-1, "length");
	num_lines = jsal_get_int(-1);
	jsal_pop(1);
	line_height = font_height(font);
	galileo_reset();
	for (i = 0; i < num_lines; ++i) {
		jsal_get_prop_index(-1, i); line_text = jsal_get_string(-1); jsal_pop(1);
		font_draw_text(font, mask, x + offset, y, TEXT_ALIGN_LEFT, line_text);
		y += line_height;
	}
	jsal_pop(1);
	return false;
}

static bool
js_Font_drawZoomedText(int num_args, bool is_ctor, int magic)
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

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_get_prop_string(-1, "\xFF" "color_mask");
	mask = jsal_require_sphere_color(-1);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	scale = jsal_to_number(2);
	text = jsal_to_string(3);

	if (screen_skip_frame(g_screen))
		return false;

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
	return false;
}

static bool
js_Font_getCharacterImage(int num_args, bool is_ctor, int magic)
{
	uint32_t cp;
	font_t*  font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	cp = (uint32_t)jsal_require_number(0);

	jsal_push_class_obj(SV1_IMAGE, image_ref(font_glyph(font, cp)), false);
	return true;
}

static bool
js_Font_getColorMask(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_get_prop_string(-1, "\xFF" "color_mask");
	return true;
}

static bool
js_Font_getHeight(int num_args, bool is_ctor, int magic)
{
	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_pop(1);
	jsal_push_int(font_height(font));
	return true;
}

static bool
js_Font_getStringHeight(int num_args, bool is_ctor, int magic)
{
	font_t*     font;
	int         num_lines;
	const char* text;
	int         width;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	text = jsal_to_string(0);
	width = jsal_to_int(1);

	jsal_push_new_function(js_Font_wordWrapString, "wordWrapString", 0, 0);
	jsal_push_this();
	jsal_push_string(text);
	jsal_push_int(width);
	jsal_call_method(2);
	jsal_get_prop_string(-1, "length"); num_lines = jsal_get_int(-1); jsal_pop(1);
	jsal_pop(1);
	jsal_push_int(font_height(font) * num_lines);
	return true;
}

static bool
js_Font_getStringWidth(int num_args, bool is_ctor, int magic)
{
	const char* text = jsal_to_string(0);

	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	jsal_pop(1);
	jsal_push_int(font_get_width(font, text));
	return true;
}

static bool
js_Font_setCharacterImage(int num_args, bool is_ctor, int magic)
{
	int cp = jsal_to_int(0);
	image_t* image = jsal_require_class_obj(1, SV1_IMAGE);

	font_t* font;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);

	font_set_glyph(font, cp, image);
	return false;
}

static bool
js_Font_setColorMask(int num_args, bool is_ctor, int magic)
{
	font_t* font;
	color_t mask;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
	mask = jsal_require_sphere_color(0);

	jsal_push_sphere_color(mask);
	jsal_put_prop_string(-2, "\xFF" "color_mask");
	return false;
}

static bool
js_Font_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object font]");
	return true;
}

static bool
js_Font_wordWrapString(int num_args, bool is_ctor, int magic)
{
	const char* text = jsal_to_string(0);
	int         width = jsal_to_int(1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	jsal_push_this();
	font = jsal_require_class_obj(-1, SV1_FONT);
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

static void
js_Image_finalize(void* host_ptr)
{
	image_unref(host_ptr);
}

static bool
js_Image_get_height(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	jsal_pop(1);
	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Image_get_width(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	jsal_pop(1);
	jsal_push_int(image_width(image));
	return true;
}

static bool
js_Image_blit(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return false;
}

static bool
js_Image_blitMask(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	color_t mask = jsal_require_sphere_color(2);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_tinted_bitmap(image_bitmap(image), nativecolor(mask), x, y, 0x0);
	return false;
}

static bool
js_Image_createSurface(int num_args, bool is_ctor, int magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);

	if ((new_image = image_clone(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create new surface image");
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Image_rotateBlit(int num_args, bool is_ctor, int magic)
{
	float    angle;
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	angle = jsal_to_number(2);

	if (screen_skip_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_rotated_bitmap(image_bitmap(image), width / 2, height / 2,
		x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Image_rotateBlitMask(int num_args, bool is_ctor, int magic)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;
	float    angle;
	color_t  mask;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_IMAGE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	angle = jsal_to_number(2);
	mask = jsal_require_sphere_color(3);

	if (screen_skip_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_tinted_rotated_bitmap(image_bitmap(image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	return false;
}

static bool
js_Image_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object image]");
	return true;
}

static bool
js_Image_transformBlit(int num_args, bool is_ctor, int magic)
{
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

	width = image_width(image);
	height = image_height(image);
	mask = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, mask },
		{ x2, y2, 0, width, 0, mask },
		{ x4, y4, 0, 0, height, mask },
		{ x3, y3, 0, width, height, mask }
	};
	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Image_transformBlitMask(int num_args, bool is_ctor, int magic)
{
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

	width = image_width(image);
	height = image_height(image);
	color = nativecolor(mask);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, color },
		{ x2, y2, 0, width, 0, color },
		{ x4, y4, 0, 0, height, color },
		{ x3, y3, 0, width, height, color }
	};
	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return false;
}

static bool
js_Image_zoomBlit(int num_args, bool is_ctor, int magic)
{
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

	if (screen_skip_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
	al_draw_scaled_bitmap(image_bitmap(image), 0, 0, width, height,
		x, y, width * scale, height * scale, 0x0);
	return false;
}

static bool
js_Image_zoomBlitMask(int num_args, bool is_ctor, int magic)
{
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

	if (screen_skip_frame(g_screen))
		return false;
	width = image_width(image);
	height = image_height(image);
	galileo_reset();
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
js_Logger_beginBlock(int num_args, bool is_ctor, int magic)
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
js_Logger_endBlock(int num_args, bool is_ctor, int magic)
{
	logger_t* logger;

	jsal_push_this();
	logger = jsal_require_class_obj(-1, SV1_LOGGER);
	logger_end_block(logger);
	return false;
}

static bool
js_Logger_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object log]");
	return true;
}

static bool
js_Logger_write(int num_args, bool is_ctor, int magic)
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
js_RawFile_close(int num_args, bool is_ctor, int magic)
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
js_RawFile_getPosition(int num_args, bool is_ctor, int magic)
{
	file_t* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	jsal_push_int(file_position(file));
	return true;
}

static bool
js_RawFile_getSize(int num_args, bool is_ctor, int magic)
{
	file_t* file;
	long    file_pos;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	file_pos = file_position(file);
	file_seek(file, 0, WHENCE_END);
	jsal_push_int(file_position(file));
	file_seek(file, file_pos, WHENCE_SET);
	return true;
}

static bool
js_RawFile_read(int num_args, bool is_ctor, int magic)
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
	if (!(read_buffer = malloc(num_bytes)))
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
js_RawFile_setPosition(int num_args, bool is_ctor, int magic)
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
js_RawFile_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object rawfile]");
	return true;
}

static bool
js_RawFile_write(int num_args, bool is_ctor, int magic)
{
	bytearray_t* array;
	const void*  data;
	file_t*      file;
	size_t       write_size;

	jsal_push_this();
	file = jsal_require_class_obj(-1, SV1_RAW_FILE);
	jsal_pop(1);
	if (file == NULL)
		jsal_error(JS_ERROR, "file is already closed");
	if (jsal_is_string(0))
		data = jsal_get_lstring(0, &write_size);
	else if (jsal_is_class_obj(0, SV1_BYTE_ARRAY)) {
		array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
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
js_Socket_close(int num_args, bool is_ctor, int magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	jsal_set_class_ptr(-1, NULL);
	socket_v1_unref(socket);
	return true;
}

static bool
js_Socket_getPendingReadSize(int num_args, bool is_ctor, int magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (socket == NULL)
		jsal_error(JS_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		jsal_error(JS_ERROR, "socket is not connected");
	jsal_push_int((int)socket_v1_peek(socket));
	return true;
}

static bool
js_Socket_isConnected(int num_args, bool is_ctor, int magic)
{
	socket_v1_t* socket;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (socket != NULL)
		jsal_push_boolean(socket_v1_connected(socket));
	else
		jsal_push_boolean(false);
	return true;
}

static bool
js_Socket_read(int num_args, bool is_ctor, int magic)
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
	if (!(read_buffer = malloc(length)))
		jsal_error(JS_ERROR, "couldn't allocate read buffer");
	socket_v1_read(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		jsal_error(JS_ERROR, "couldn't create byte array");
	jsal_push_sphere_bytearray(array);
	return true;
}

static bool
js_Socket_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object socket]");
	return true;
}

static bool
js_Socket_write(int num_args, bool is_ctor, int magic)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_v1_t*   socket;
	size_t         write_size;

	jsal_push_this();
	socket = jsal_require_class_obj(-1, SV1_SOCKET);

	if (jsal_is_string(0))
		payload = (uint8_t*)jsal_get_lstring(0, &write_size);
	else {
		array = jsal_require_class_obj(0, SV1_BYTE_ARRAY);
		payload = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	if (socket == NULL)
		jsal_error(JS_ERROR, "socket has been closed");
	if (!socket_v1_connected(socket))
		jsal_error(JS_ERROR, "socket is not connected");
	socket_v1_write(socket, payload, write_size);
	return false;
}

static void
js_Sound_finalize(void* host_ptr)
{
	sound_unref(host_ptr);
}

static bool
js_Sound_getLength(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(floor(sound_len(sound) * 1000000));
	return true;
}

static bool
js_Sound_getPan(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_int(sound_pan(sound) * 255);
	return true;
}

static bool
js_Sound_getPitch(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(sound_speed(sound));
	return true;
}

static bool
js_Sound_getPosition(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_number(floor(sound_tell(sound) * 1000000));
	return true;
}

static bool
js_Sound_getRepeat(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_boolean(sound_repeat(sound));
	return true;
}

static bool
js_Sound_getVolume(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_int(sound_gain(sound) * 255);
	return true;
}

static bool
js_Sound_isPlaying(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	jsal_push_boolean(sound_playing(sound));
	return true;
}

static bool
js_Sound_isSeekable(int num_args, bool is_ctor, int magic)
{
	jsal_push_boolean(true);
	return true;
}

static bool
js_Sound_pause(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_pause(sound, true);
	return false;
}

static bool
js_Sound_play(int num_args, bool is_ctor, int magic)
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
js_Sound_reset(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_seek(sound, 0.0);
	return false;
}

static bool
js_Sound_setPan(int num_args, bool is_ctor, int magic)
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
js_Sound_setPitch(int num_args, bool is_ctor, int magic)
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
js_Sound_setPosition(int num_args, bool is_ctor, int magic)
{
	double   new_pos;
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);
	new_pos = jsal_to_number(0);

	sound_seek(sound, floor(new_pos) / 1000000);
	return false;
}

static bool
js_Sound_setRepeat(int num_args, bool is_ctor, int magic)
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
js_Sound_setVolume(int num_args, bool is_ctor, int magic)
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
js_Sound_stop(int num_args, bool is_ctor, int magic)
{
	sound_t* sound;

	jsal_push_this();
	sound = jsal_require_class_obj(-1, SV1_SOUND);

	sound_stop(sound);
	return false;
}

static bool
js_Sound_toString(int num_args, bool is_ctor, int magic)
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
js_SoundEffect_getPan(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_pan(sample) * 255.0);
	return true;
}

static bool
js_SoundEffect_getPitch(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_speed(sample));
	return true;
}

static bool
js_SoundEffect_getVolume(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	jsal_push_number(sample_get_gain(sample) * 255.0);
	return true;
}

static bool
js_SoundEffect_setPan(int num_args, bool is_ctor, int magic)
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
js_SoundEffect_setPitch(int num_args, bool is_ctor, int magic)
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
js_SoundEffect_setVolume(int num_args, bool is_ctor, int magic)
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
js_SoundEffect_play(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	sample_play(sample, s_sound_mixer);
	return false;
}

static bool
js_SoundEffect_stop(int num_args, bool is_ctor, int magic)
{
	sample_t* sample;

	jsal_push_this();
	sample = jsal_require_class_obj(-1, SV1_SOUND_EFFECT);

	sample_stop_all(sample);
	return false;
}

static bool
js_SoundEffect_toString(int num_args, bool is_ctor, int magic)
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
js_Spriteset_get_filename(int num_args, bool is_ctor, int magic)
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
js_Spriteset_clone(int num_args, bool is_ctor, int magic)
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
js_Spriteset_save(int num_args, bool is_ctor, int magic)
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
js_Spriteset_toString(int num_args, bool is_ctor, int magic)
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
js_Surface_get_height(int num_args, bool is_ctor, int magic)
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
js_Surface_get_width(int num_args, bool is_ctor, int magic)
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
js_Surface_applyColorFX(int num_args, bool is_ctor, int magic)
{
	colormatrix_t matrix;
	int           height;
	int           width;
	int           x;
	int           y;

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);
	matrix = jsal_require_sphere_colormatrix(4);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_colormat(image, matrix, x, y, width, height))
		jsal_error(JS_ERROR, "couldn't apply color FX");
	return false;
}

static bool
js_Surface_applyColorFX4(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	colormatrix_t ul_mat = jsal_require_sphere_colormatrix(4);
	colormatrix_t ur_mat = jsal_require_sphere_colormatrix(5);
	colormatrix_t ll_mat = jsal_require_sphere_colormatrix(6);
	colormatrix_t lr_mat = jsal_require_sphere_colormatrix(7);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_colormat_4(image, ul_mat, ur_mat, ll_mat, lr_mat, x, y, w, h))
		jsal_error(JS_ERROR, "couldn't apply color FX");
	return false;
}

static bool
js_Surface_applyLookup(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	uint8_t* red_lu = jsal_require_rgba_lut(4);
	uint8_t* green_lu = jsal_require_rgba_lut(5);
	uint8_t* blue_lu = jsal_require_rgba_lut(6);
	uint8_t* alpha_lu = jsal_require_rgba_lut(7);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		jsal_error(JS_RANGE_ERROR, "FX area of effect out of bounds");
	if (!image_apply_lookup(image, x, y, w, h, red_lu, green_lu, blue_lu, alpha_lu))
		jsal_error(JS_ERROR, "couldn't apply color FX");
	free(red_lu);
	free(green_lu);
	free(blue_lu);
	free(alpha_lu);
	return false;
}

static bool
js_Surface_bezierCurve(int num_args, bool is_ctor, int magic)
{
	int             blend_mode;
	color_t         color;
	float           cp[8];
	image_t*        image;
	bool            is_quadratic = true;
	int             num_points;
	double          step_size;
	ALLEGRO_VERTEX* vertices;
	float           x1, x2, x3, x4;
	float           y1, y2, y3, y4;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	color = jsal_require_sphere_color(0);
	step_size = jsal_to_number(1);
	x1 = jsal_to_number(2);
	y1 = jsal_to_number(3);
	x2 = jsal_to_number(4);
	y2 = jsal_to_number(5);
	x3 = jsal_to_number(6);
	y3 = jsal_to_number(7);
	if (num_args >= 10) {
		is_quadratic = false;
		x4 = jsal_to_number(8);
		y4 = jsal_to_number(9);
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
	return false;
}

static bool
js_Surface_blit(int num_args, bool is_ctor, int magic)
{
	image_t* image;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	x = jsal_to_int(0);
	y = jsal_to_int(1);

	if (screen_skip_frame(g_screen))
		return false;
	galileo_reset();
	al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return false;
}

static bool
js_Surface_blitMaskSurface(int num_args, bool is_ctor, int magic)
{
	image_t* src_image = jsal_require_class_obj(0, SV1_SURFACE);
	int x = jsal_to_int(1);
	int y = jsal_to_int(2);
	color_t mask = jsal_require_sphere_color(3);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1); jsal_pop(1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_bitmap(image_bitmap(src_image), nativecolor(mask), x, y, 0x0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_blitSurface(int num_args, bool is_ctor, int magic)
{
	image_t* src_image = jsal_require_class_obj(0, SV1_SURFACE);
	int x = jsal_to_int(1);
	int y = jsal_to_int(2);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_bitmap(image_bitmap(src_image), x, y, 0x0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_clone(int num_args, bool is_ctor, int magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if ((new_image = image_clone(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create new surface");
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Surface_cloneSection(int num_args, bool is_ctor, int magic)
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

	if ((new_image = image_new(width, height)) == NULL)
		jsal_error(JS_ERROR, "couldn't create surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap_region(image_bitmap(image), x, y, width, height, 0, 0, 0x0);
	jsal_push_class_obj(SV1_SURFACE, new_image, false);
	return true;
}

static bool
js_Surface_createImage(int num_args, bool is_ctor, int magic)
{
	image_t* image;
	image_t* new_image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	if ((new_image = image_clone(image)) == NULL)
		jsal_error(JS_ERROR, "couldn't create image");
	jsal_push_class_obj(SV1_IMAGE, new_image, false);
	return true;
}

static bool
js_Surface_drawText(int num_args, bool is_ctor, int magic)
{
	int         blend_mode;
	color_t     color;
	font_t*     font;
	image_t*    image;
	const char* text;
	int         x;
	int         y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	font = jsal_require_class_obj(0, SV1_FONT);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	text = jsal_to_string(3);

	jsal_get_prop_string(0, "\xFF" "color_mask");
	color = jsal_require_sphere_color(-1);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	font_draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_filledCircle(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int radius = jsal_to_number(2);
	color_t color = jsal_require_sphere_color(3);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_circle(x, y, radius, nativecolor(color));
	reset_blend_modes();
	return false;
}

static bool
js_Surface_filledEllipse(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int rx = jsal_to_number(2);
	int ry = jsal_to_number(3);
	color_t color = jsal_require_sphere_color(4);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_ellipse(x, y, rx, ry, nativecolor(color));
	reset_blend_modes();
	return false;
}

static bool
js_Surface_flipHorizontally(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	image_flip(image, true, false);
	return false;
}

static bool
js_Surface_flipVertically(int num_args, bool is_ctor, int magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	image_flip(image, false, true);
	return false;
}

static bool
js_Surface_getPixel(int num_args, bool is_ctor, int magic)
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
js_Surface_gradientCircle(int num_args, bool is_ctor, int magic)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int radius = jsal_to_number(2);
	color_t in_color = jsal_require_sphere_color(3);
	color_t out_color = jsal_require_sphere_color(4);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1); jsal_pop(1);
	jsal_pop(1);
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
	return false;
}

static bool
js_Surface_gradientEllipse(int num_args, bool is_ctor, int magic)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int rx = jsal_to_number(2);
	int ry = jsal_to_number(3);
	color_t in_color = jsal_require_sphere_color(4);
	color_t out_color = jsal_require_sphere_color(5);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

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
	return false;
}

static bool
js_Surface_gradientRectangle(int num_args, bool is_ctor, int magic)
{
	int x1 = jsal_to_int(0);
	int y1 = jsal_to_int(1);
	int x2 = x1 + jsal_to_int(2);
	int y2 = y1 + jsal_to_int(3);
	color_t color_ul = jsal_require_sphere_color(4);
	color_t color_ur = jsal_require_sphere_color(5);
	color_t color_lr = jsal_require_sphere_color(6);
	color_t color_ll = jsal_require_sphere_color(7);

	int           blend_mode;
	image_t*      image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

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
	return false;
}

static bool
js_Surface_gradientLine(int num_args, bool is_ctor, int magic)
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

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
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
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_line(int num_args, bool is_ctor, int magic)
{
	float x1 = jsal_to_int(0) + 0.5;
	float y1 = jsal_to_int(1) + 0.5;
	float x2 = jsal_to_int(2) + 0.5;
	float y2 = jsal_to_int(3) + 0.5;
	color_t color = jsal_require_sphere_color(4);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_lineSeries(int num_args, bool is_ctor, int magic)
{
	int             blend_mode;
	color_t         color;
	image_t*        image;
	int             num_points;
	int             type;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	color = jsal_require_sphere_color(1);
	type = num_args >= 3 ? jsal_to_int(2) : LINE_MULTIPLE;

	if (!jsal_is_array(0))
		jsal_error(JS_ERROR, "first argument must be an array");
	num_points = jsal_get_length(0);
	if (num_points < 2)
		jsal_error(JS_RANGE_ERROR, "two or more vertices required");
	if (num_points > INT_MAX)
		jsal_error(JS_RANGE_ERROR, "too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		jsal_error(JS_ERROR, "couldn't allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x"); x = jsal_to_int(-1); jsal_pop(1);
		jsal_get_prop_string(-1, "y"); y = jsal_to_int(-1); jsal_pop(1);
		jsal_pop(1);
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
	return false;
}

static bool
js_Surface_outlinedCircle(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int radius = jsal_to_number(2);
	color_t color = jsal_require_sphere_color(3);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_circle(x, y, radius, nativecolor(color), 1.0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_outlinedEllipse(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_number(0);
	int y = jsal_to_number(1);
	int rx = jsal_to_number(2);
	int ry = jsal_to_number(3);
	color_t color = jsal_require_sphere_color(4);

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_ellipse(x, y, rx, ry, nativecolor(color), 1.0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_pointSeries(int num_args, bool is_ctor, int magic)
{
	color_t color = jsal_require_sphere_color(1);

	int             blend_mode;
	image_t*        image;
	int             num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	int i;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1); jsal_pop(1);
	jsal_pop(1);
	if (!jsal_is_array(0))
		jsal_error(JS_ERROR, "first argument must be an array");
	num_points = jsal_get_length(0);
	if (num_points > INT_MAX)
		jsal_error(JS_RANGE_ERROR, "too many vertices (%u)", num_points);
	vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		jsal_get_prop_index(0, i);
		jsal_get_prop_string(-1, "x"); x = jsal_to_int(-1); jsal_pop(1);
		jsal_get_prop_string(-1, "y"); y = jsal_to_int(-1); jsal_pop(1);
		jsal_pop(1);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	reset_blend_modes();
	free(vertices);
	return false;
}

static bool
js_Surface_outlinedRectangle(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	float x1 = jsal_to_int(0) + 0.5;
	float y1 = jsal_to_int(1) + 0.5;
	float x2 = x1 + jsal_to_int(2) - 1;
	float y2 = y1 + jsal_to_int(3) - 1;
	color_t color = jsal_require_sphere_color(4);
	int thickness = n_args >= 6 ? jsal_to_int(5) : 1;

	int      blend_mode;
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_replaceColor(int num_args, bool is_ctor, int magic)
{
	color_t color = jsal_require_sphere_color(0);
	color_t new_color = jsal_require_sphere_color(1);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_pop(1);
	if (!image_replace_color(image, color, new_color))
		jsal_error(JS_ERROR, "couldn't perform color replacement");
	return false;
}

static bool
js_Surface_rescale(int num_args, bool is_ctor, int magic)
{
	int width = jsal_to_int(0);
	int height = jsal_to_int(1);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_pop(1);
	if (!image_rescale(image, width, height))
		jsal_error(JS_ERROR, "couldn't rescale image");
	jsal_push_this();
	return true;
}

static bool
js_Surface_rotate(int num_args, bool is_ctor, int magic)
{
	int n_args = jsal_get_top();
	float angle = jsal_to_number(0);
	bool want_resize = n_args >= 2 ? jsal_to_boolean(1) : true;

	image_t* image;
	image_t* new_image;
	int      new_w, new_h;
	int      w, h;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	w = new_w = image_width(image);
	h = new_h = image_height(image);
	if (want_resize) {
		// FIXME: implement in-place resizing for Surface#rotate()
		jsal_error(JS_ERROR, "not implemented");
	}
	if ((new_image = image_new(new_w, new_h)) == NULL)
		jsal_error(JS_ERROR, "couldn't create new surface");
	image_render_to(new_image, NULL);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_rotated_bitmap(image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);

	// swap out the image pointer and free old image
	jsal_set_class_ptr(-1, new_image);
	image_unref(image);
	return true;
}

static bool
js_Surface_rotateBlitMaskSurface(int num_args, bool is_ctor, int magic)
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

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	angle = jsal_to_number(3);
	mask = jsal_require_sphere_color(4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_rotated_bitmap(image_bitmap(source_image), nativecolor(mask),
		width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_rotateBlitSurface(int num_args, bool is_ctor, int magic)
{
	float    angle;
	int      blend_mode;
	int      height;
	image_t* image;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	angle = jsal_to_number(3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_rotated_bitmap(image_bitmap(source_image), width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_rectangle(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	int w = jsal_to_int(2);
	int h = jsal_to_int(3);
	color_t color = jsal_require_sphere_color(4);

	image_t* image;
	int      blend_mode;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);

	galileo_reset();
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	reset_blend_modes();
	return false;
}

static bool
js_Surface_save(int num_args, bool is_ctor, int magic)
{
	const char* filename;
	image_t*    image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_pop(1);
	filename = jsal_require_pathname(0, "images", true, false);
	image_save(image, filename);
	return true;
}

static bool
js_Surface_setAlpha(int num_args, bool is_ctor, int magic)
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
	jsal_pop(1);
	if (!(lock = image_lock(image)))
		jsal_error(JS_ERROR, "couldn't lock surface");
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
js_Surface_setBlendMode(int num_args, bool is_ctor, int magic)
{
	jsal_push_this();
	jsal_dup(0);
	jsal_put_prop_string(-2, "\xFF" "blend_mode");
	return false;
}

static bool
js_Surface_setPixel(int num_args, bool is_ctor, int magic)
{
	int x = jsal_to_int(0);
	int y = jsal_to_int(1);
	color_t color = jsal_require_sphere_color(2);

	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);

	image_set_pixel(image, x, y, color);
	return false;
}

static bool
js_Surface_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object surface]");
	return true;
}

static bool
js_Surface_transformBlitMaskSurface(int num_args, bool is_ctor, int magic)
{
	int      blend_mode;
	int      height;
	image_t* image;
	color_t  mask;
	image_t* source_image;
	int      width;
	int      x1, x2, x3, x4;
	int      y1, y2, y3, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x1 = jsal_to_int(1);
	y1 = jsal_to_int(2);
	x2 = jsal_to_int(3);
	y2 = jsal_to_int(4);
	x3 = jsal_to_int(5);
	y3 = jsal_to_int(6);
	x4 = jsal_to_int(7);
	y4 = jsal_to_int(8);
	mask = jsal_require_sphere_color(9);

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
	return false;
}

static bool
js_Surface_transformBlitSurface(int num_args, bool is_ctor, int magic)
{
	int      blend_mode;
	int      height;
	image_t* image;
	image_t* source_image;
	int      width;
	int      x1, x2, x3, x4;
	int      y1, y2, y3, y4;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x1 = jsal_to_int(1);
	y1 = jsal_to_int(2);
	x2 = jsal_to_int(3);
	y2 = jsal_to_int(4);
	x3 = jsal_to_int(5);
	y3 = jsal_to_int(6);
	x4 = jsal_to_int(7);
	y4 = jsal_to_int(8);

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
	return false;
}

static bool
js_Surface_zoomBlitMaskSurface(int num_args, bool is_ctor, int magic)
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

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	scale = jsal_to_number(3);
	mask = jsal_require_sphere_color(4);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_tinted_scaled_bitmap(image_bitmap(source_image),
		nativecolor(mask),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	reset_blend_modes();
	return false;
}

static bool
js_Surface_zoomBlitSurface(int num_args, bool is_ctor, int magic)
{
	int      blend_mode;
	int      height;
	image_t* image;
	float    scale;
	image_t* source_image;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, SV1_SURFACE);
	jsal_get_prop_string(-1, "\xFF" "blend_mode");
	blend_mode = jsal_get_int(-1);
	source_image = jsal_require_class_obj(0, SV1_SURFACE);
	x = jsal_to_int(1);
	y = jsal_to_int(2);
	scale = jsal_to_number(3);

	width = image_width(source_image);
	height = image_height(source_image);
	image_render_to(image, NULL);
	use_sphere_blend_mode(blend_mode);
	al_draw_scaled_bitmap(image_bitmap(source_image),
		0, 0, width, height, x, y, width * scale, height * scale,
		0x0);
	reset_blend_modes();
	return false;
}

static void
js_WindowStyle_finalize(void* host_ptr)
{
	winstyle_unref(host_ptr);
}

static bool
js_WindowStyle_drawWindow(int num_args, bool is_ctor, int magic)
{
	int            height;
	color_t        mask;
	int            width;
	windowstyle_t* winstyle;
	int            x;
	int            y;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);
	jsal_get_prop_string(-1, "\xFF" "color_mask");
	mask = jsal_require_sphere_color(-1);
	x = jsal_to_int(0);
	y = jsal_to_int(1);
	width = jsal_to_int(2);
	height = jsal_to_int(3);

	galileo_reset();
	winstyle_draw(winstyle, mask, x, y, width, height);
	return false;
}

static bool
js_WindowStyle_getColorMask(int num_args, bool is_ctor, int magic)
{
	windowstyle_t* winstyle;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);

	jsal_get_prop_string(-2, "\xFF" "color_mask");
	return true;
}

static bool
js_WindowStyle_setColorMask(int num_args, bool is_ctor, int magic)
{
	color_t        mask;
	windowstyle_t* winstyle;

	jsal_push_this();
	winstyle = jsal_require_class_obj(-1, SV1_WINDOW_STYLE);
	mask = jsal_require_sphere_color(0);

	jsal_push_sphere_color(mask);
	jsal_put_prop_string(-2, "\xFF" "color_mask");
	return false;
}

static bool
js_WindowStyle_toString(int num_args, bool is_ctor, int magic)
{
	jsal_push_string("[object windowstyle]");
	return true;
}
