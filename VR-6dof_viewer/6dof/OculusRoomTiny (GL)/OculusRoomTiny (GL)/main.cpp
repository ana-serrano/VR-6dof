/*****************************************************************************

Filename    :   main.cpp
Content     :   Simple minimal VR demo
Created     :   December 1, 2014
Author      :   Tom Heath
Copyright   :   Copyright 2012 Oculus, Inc. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

/*****************************************************************************/
/// This sample has not yet been fully assimiliated into the framework
/// and also the GL support is not quite fully there yet, hence the VR
/// is not that great!

#include <opencv2/opencv.hpp>

#include "Win32_GLAppUtil_main.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"

#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#endif

using namespace OVR;
#include <iostream>
#include <cstdio>
#include <ctime>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <fstream>
#include <irrKlang.h>
#include <string.h>
#include <mbctype.h>

#if defined(WIN32)
#include <conio.h>
#else
#include "../common/conio.h"
#endif
using namespace irrklang;
#pragma comment(lib, "irrKlang.lib")

cv::VideoCapture g_video;
cv::VideoCapture d_video;
cv::VideoCapture a_video;


char g1_filename[200];
char d1_filename[200];
char a1_filename[200];
char bg1_filename[200];
char bgd1_filename[200];
char bbg1_filename[200];
char bbgd1_filename[200];
char bga_filename[200];
char bga1_filename[200];
char g_filename[200];
char d_filename[200];
char a_filename[200];
char bg_filename[200];
char bgd_filename[200];
char bbg_filename[200];
char bbgd_filename[200];
char video_path[200];
char data_filename[200];
char data_path[200];
char userID[200];
char mode[200];
char testID[200];
char visID[200];

std::string audiofile;

bool poly_mesh = false;
bool positional_track = true;
bool stereo = true;
bool render_depth = false;
double layers = 3.0;
bool colored = false;
bool render_simple = false;
bool vis_fade = true;
bool vis_clamp = false;
bool auto_camera = false;
bool auto_results = false;

int frames, width, height;
float FPSvideo;
cv::Mat img1, img;
cv::Mat d_img1, d_img;
cv::Mat a_img;
cv::Mat bg_img;
cv::Mat bgd_img;
cv::Mat bbg_img;
cv::Mat bbgd_img;
cv::Mat black_img;
cv::Mat bga_img;

std::ofstream headpose;

void VideoThread(LPVOID pArgs_);
void AudioThread(LPVOID pArgs_);


static ovrGraphicsLuid GetDefaultAdapterLuid()
{
	ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
	IDXGIFactory* factory = nullptr;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
	{
		IDXGIAdapter* adapter = nullptr;

		if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
		{
			DXGI_ADAPTER_DESC desc;

			adapter->GetDesc(&desc);
			memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
			adapter->Release();
		}

		factory->Release();
	}
#endif

	return luid;
}


static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

double clockToMilliseconds(clock_t ticks) {
	// units/(units/time) => time (seconds) * 1000 = milliseconds
	return (ticks / (double)CLOCKS_PER_SEC)*1000.0;
}

// return true to retry later (e.g. after display lost)
static bool MainLoop(bool retryCreate)
{
	TextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
	DepthBuffer   * eyeDepthBuffer[2] = { nullptr, nullptr };
	ovrMirrorTexture mirrorTexture = nullptr;
	GLuint          mirrorFBO = 0;
	Scene         * roomScene = nullptr;
	long long frameIndex = 0;

	ovrSession session;
	ovrGraphicsLuid luid;
	ovrResult result = ovr_Create(&session, &luid);
	if (!OVR_SUCCESS(result))
		return retryCreate;

	if (Compare(luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
	{
		VALIDATE(false, "OpenGL supports only the default graphics adapter.");
	}
	
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

	// Setup Window and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
	//ovrSizei windowSize = { hmdDesc.Resolution.w, hmdDesc.Resolution.h};
	ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
	if (!Platform.InitDevice(windowSize.w, windowSize.h, reinterpret_cast<LUID*>(&luid)))
		goto Done;

	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	//CREATE VIDEO THREAD
	GLuint mFront_left, mFront_dleft, mFront_aleft, mBack_left, mBack_dleft, mBack_bg, mFront_bg, mFront_bgd, mBack_bgd, mFront_bbg, mBack_bbg, mFront_bbgd, mBack_bbgd, mBack_aleft, black_text, bga_text;
	bool fl_write = false;
	bool fl_terminate = false;
	bool pause_all = false;
	ARGS args = { &mFront_left, &mFront_dleft, &mFront_aleft, &mFront_bg, &mFront_bgd, &mFront_bbg, &mFront_bbgd, &mBack_left, &mBack_dleft, &mBack_aleft, &mBack_bg, &mBack_bgd, &mBack_bbg, &mBack_bbgd, &black_text, &bga_text, &fl_write, &fl_terminate, &pause_all};
	
	HANDLE threadDecoding;
	threadDecoding = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)VideoThread, &args, 0, NULL);
	///////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//CREATE AUDIO THREAD
	ARGS_aud args_aud = { &fl_write, &fl_terminate, &audiofile, &pause_all};
	HANDLE threadAudioPlaying;
	threadAudioPlaying = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioThread, &args_aud, 0, NULL);
	///////////////////////////////////////////////////////////////////////////////////////////////////

	// Make eye render buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
		eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);
		eyeDepthBuffer[eye] = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

		if (!eyeRenderTexture[eye]->TextureChain)
		{
			if (retryCreate) goto Done;
			VALIDATE(false, "Failed to create texture.");
		}
	}

	ovrMirrorTextureDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = windowSize.w;
	desc.Height = windowSize.h;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
	if (!OVR_SUCCESS(result))
	{
		if (retryCreate) goto Done;
		VALIDATE(false, "Failed to create mirror texture.");
	}

	// Configure the mirror read buffer
	GLuint texId;
	ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);

	glGenFramebuffers(1, &mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	// Turn off vsync to let the compositor do its magic
	wglSwapIntervalEXT(0);

	
	// FloorLevel will give tracking poses where the floor height is 0
	ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
	ovrTrackingState TrackingState;
	TrackingState = ovr_GetTrackingState(session, 0, ovrTrue);

	Vector2i SphereSize;
	//Currently fixed (can take the size of the video as input, but very large meshes are not supported) 
	SphereSize.x = 2048;
	SphereSize.y = 1024;
	Vector3f spherecenter = TrackingState.HeadPose.ThePose.Position;
	// Make scene
	roomScene = new Scene(false, TrackingState.HeadPose.ThePose.Position, SphereSize);
	Vector2f ScreenSize(hmdDesc.Resolution.w, hmdDesc.Resolution.h);
	
	//Pass the video texture each frame to the render call
	//For now we are only passing the first image	
	clock_t beginFrame = clock();
	clock_t deltaClock = 0;
	double deltaTime = 0;
	unsigned int frames = 0;
	double  frameRate = 30;
	double  averageFrameTimeMilliseconds = 33.333;
	// Main loop
	clock_t timestamp = 0;
	clock_t t_origin = 0;
	bool write = false;
	double DistX = 0.0;
	double DistY = 0.0;
	double DistZ = 0.0;
	double circle = 0.0;
	double radius = 100.0;
	double th = 0.17*0.17;
	clock_t StartTime = clock();
	clock_t SpentTime = 0;
	bool starting = true;
	while (Platform.HandleMessages())
	{ 
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);		
		
		if (starting == true & sessionStatus.HmdMounted)
		{
			Sleep(1500.0f);
			TrackingState = ovr_GetTrackingState(session, frameIndex, ovrFalse);
			roomScene->Models[0]->Pos = TrackingState.HeadPose.ThePose.Position;
			spherecenter = TrackingState.HeadPose.ThePose.Position;
			starting = false;

		}
	
		/*if (fl_terminate == true)
		{
			//headpose << std::endl;
			delete roomScene;
			if (mirrorFBO) glDeleteFramebuffers(1, &mirrorFBO);
			if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
			for (int eye = 0; eye < 2; ++eye)
			{
				delete eyeRenderTexture[eye];
				delete eyeDepthBuffer[eye];
			}
			Platform.ReleaseDevice();
			ovr_Destroy(session);
			//exit(0);
			return 0;
		}
		*/

		/////////////////////////////////////////////////////////////////////////////////////////////
		//FPS COUNTER
		clock_t endFrame = clock();
		deltaClock = endFrame - beginFrame;
		deltaTime = clockToMilliseconds(deltaClock) / 1000.0;
		frames++;
		if (deltaTime > 1.0) { //every second
			frameRate = frames / deltaTime;
			//frameRate = frameRate / 5.0;
			beginFrame = endFrame;
			frames = 0;
			averageFrameTimeMilliseconds = 1000.0 / (frameRate == 0 ? 0.001 : frameRate);
			std::cout << printf("fps=%02.2f   mspf=%02.2f", frameRate, averageFrameTimeMilliseconds) << std::endl;
		}
		/////////////////////////////////////////////////////////////////////////////////////////////
		//ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);
		if (sessionStatus.ShouldQuit)
		{
			// Because the application is requested to quit, should not request retry
			retryCreate = false;
			break;
		}
		if (sessionStatus.ShouldRecenter)
			ovr_RecenterTrackingOrigin(session);

		if (sessionStatus.IsVisible)
		{

			if (Platform.Key['T'])
			{				
				TrackingState = ovr_GetTrackingState(session, frameIndex, ovrFalse);
				roomScene->Models[0]->Pos = TrackingState.HeadPose.ThePose.Position;
				spherecenter = TrackingState.HeadPose.ThePose.Position;
				positional_track = true;
			}
			if (Platform.Key['Y'])
			{				
				TrackingState = ovr_GetTrackingState(session, frameIndex, ovrFalse);
				roomScene->Models[0]->Pos = TrackingState.HeadPose.ThePose.Position;
				spherecenter = TrackingState.HeadPose.ThePose.Position;
				positional_track = false;
			}
			
			if (Platform.Key['C']) {
				colored = !colored;
				Platform.Key['C'] = false;
			}
						
			if (Platform.Key['R'])
			{
					TrackingState = ovr_GetTrackingState(session, frameIndex, ovrFalse);
					roomScene->Models[0]->Pos = TrackingState.HeadPose.ThePose.Position;
					spherecenter = TrackingState.HeadPose.ThePose.Position;
			}
			

			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			ovrVector3f               HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset,
				eyeRenderDesc[1].HmdToEyeOffset };

			double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
			TrackingState = ovr_GetTrackingState(session, displayMidpointSeconds, ovrTrue);

			ovrPosef FinalEyePos[2];
			ovrPosef FinalEyePosCentered[2];
			ovrPosef FinalEyePosDefCam[2];
			ovr_CalcEyePoses(TrackingState.HeadPose.ThePose, HmdToEyeOffset, FinalEyePos);//Output: FinalEyePose --> Final Orientation and Position (head & IPD offset)
			
			
			// Render Scene to Eye Buffers
			for (int eye = 0; eye < 2; ++eye)
			{
				Matrix4f rollPitchYawDef;
				Vector3f finalUpDef;
				Vector3f finalForwardDef;
				// Switch to eye render target
				eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);
				 
							
				Matrix4f rollPitchYaw = Matrix4f(FinalEyePos[eye].Orientation);				
				Vector3f EyePos = FinalEyePos[eye].Position;
				Vector3f finalUp = rollPitchYaw.Transform(Vector3f(0, 1, 0));//
				Vector3f finalForward = rollPitchYaw.Transform(Vector3f(0, 0, -1));//				
				Matrix4f view = Matrix4f::LookAtRH(EyePos, EyePos + finalForward, finalUp);
				Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
				Vector3f HeadPos = TrackingState.HeadPose.ThePose.Position;

				// Render world
				float desat = 0.0;
				if (vis_fade == true)
				{
					DistX = (spherecenter.x - TrackingState.HeadPose.ThePose.Position.x);
					DistZ = (spherecenter.z - TrackingState.HeadPose.ThePose.Position.z);
					DistY = (spherecenter.y - TrackingState.HeadPose.ThePose.Position.y);
					circle = DistX*DistX + DistZ*DistZ;
					double th_mult = 5;
					if (circle > th)
					{
						desat = float(th_mult*circle);
						desat = fmin(desat, 1.0);
					}
				}				
				
				if (positional_track == true & render_simple == false)
				{
					roomScene->Render(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, desat, args);
				}
				if (positional_track == false)
				{
					ovrPosef centered;
					centered.Position = spherecenter;
					centered.Orientation = TrackingState.HeadPose.ThePose.Orientation;
					ovr_CalcEyePoses(centered, HmdToEyeOffset, FinalEyePosCentered);
					Vector3f EyePosCentered = FinalEyePosCentered[eye].Position;
					Matrix4f viewCentered = Matrix4f::LookAtRH(EyePosCentered, EyePosCentered + finalForward, finalUp);
					roomScene->RenderSimple(ScreenSize, spherecenter, EyePos, HeadPos, viewCentered, proj, eye, poly_mesh, stereo, render_depth, colored, layers, desat, args);
				}				

				if (positional_track == true & render_simple == true)
				{
					roomScene->RenderSimple(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, desat, args);

				}
				
				
				if (vis_fade == true )
				{
					double DistX = (spherecenter.x - TrackingState.HeadPose.ThePose.Position.x);
					double DistZ = (spherecenter.z - TrackingState.HeadPose.ThePose.Position.z);
					double DistY = (spherecenter.y - TrackingState.HeadPose.ThePose.Position.y);
					double circle = DistX*DistX + DistZ*DistZ + DistY*DistY;
					double th_mult = 10;
					float radius = 0.35;
					bool isblack = false;
					if (circle > th)
					{
						roomScene->RenderBlack(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers,th_mult*circle, radius, isblack, args);
						if (circle > (1.0 / th_mult))
						{
							isblack = true;
							radius = 0.8;
							roomScene->RenderBlack(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, th_mult*circle, radius, isblack, args);
						}
					}

				}

				// Avoids an error when calling SetAndClearRenderSurface during next iteration.
				// Without this, during the next while loop iteration SetAndClearRenderSurface
				// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
				// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
				eyeRenderTexture[eye]->UnsetRenderSurface();

				// Commit changes to the textures so they get picked up frame
				eyeRenderTexture[eye]->Commit();
			}

			// Do distortion rendering, Present and flush/sync
			ovrLayerEyeFov ld;
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

			for (int eye = 0; eye < 2; ++eye)
			{
				ld.ColorTexture[eye] = eyeRenderTexture[eye]->TextureChain;
				ld.Viewport[eye] = Recti(eyeRenderTexture[eye]->GetSize());
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = FinalEyePos[eye];
				ld.SensorSampleTime = 0;
			}

			ovrLayerHeader* layers = &ld.Header;
			result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
			// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
			if (!OVR_SUCCESS(result))
				goto Done;

			//frameIndex++;
		}

		// Blit mirror texture to back buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLint w = windowSize.w;
		GLint h = windowSize.h;
		glBlitFramebuffer(0, windowSize.h, windowSize.w, 0,
			0, 0, Platform.WinSizeW, Platform.WinSizeH,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		SwapBuffers(Platform.hDC);
	}

Done:
	delete roomScene;
	if (mirrorFBO) glDeleteFramebuffers(1, &mirrorFBO);
	if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
	for (int eye = 0; eye < 2; ++eye)
	{
		delete eyeRenderTexture[eye];
		delete eyeDepthBuffer[eye];
	}
	Platform.ReleaseDevice();
	ovr_Destroy(session);

	// Retry on ovrError_DisplayLost
	return retryCreate || (result == ovrError_DisplayLost);
}

//Thread for handling audio
void AudioThread(LPVOID pArgs_)
{
	ARGS_aud *pArgs = (ARGS_aud*)pArgs_;
	bool *fl_write = pArgs->fl_write;
	bool *fl_terminate = pArgs->fl_terminate;
	std::string *audiofile = pArgs->audiofile;
	bool *pause_all = pArgs->pause_all;
	std::string audiofilename = *audiofile;
	const char *cstr = audiofilename.c_str();
	bool pause_sound;
	

	ISoundEngine* engine = createIrrKlangDevice();
	if (!engine)
	{
		printf("Could not startup engine\n");
	}

	bool played = false;
	while (Platform.HandleMessages())
	{
		

		if (*fl_write == true & played == false)
		{
			Sleep(1500.0f);
			engine->play2D(cstr, true);
			played = true;
		}
		if (*fl_terminate == true)
		{
			*fl_terminate = false;
			engine->stopAllSounds();
			engine->play2D(cstr, true);
			played = true;
			//ExitThread(0);
		}

		engine->setAllSoundsPaused(*pause_all);
	}
}

//Thread for handling video decoding
void VideoThread(LPVOID pArgs_)
{
/*
Decode video frame
Upload frame to back texture
Swap front and back textures
*/
wglMakeCurrent(Platform.hDC, Platform.WglContext_VideoThread);
OVR::GLEContext::SetCurrentContext(&Platform.GLEContext);
Platform.GLEContext.Init();

ARGS *pArgs = (ARGS*)pArgs_;
GLuint *mFront_left = pArgs->mFront_left;
GLuint *mFront_dleft = pArgs->mFront_dleft;
GLuint *mFront_aleft = pArgs->mFront_aleft;
GLuint *mFront_bg = pArgs->mFront_bg;
GLuint *mFront_bgd = pArgs->mFront_bgd;
GLuint *mFront_bbg = pArgs->mFront_bbg;
GLuint *mFront_bbgd = pArgs->mFront_bbgd;

GLuint *mBack_left = pArgs->mBack_left;
GLuint *mBack_dleft = pArgs->mBack_dleft;
GLuint *mBack_aleft = pArgs->mBack_aleft;
GLuint *mBack_bg = pArgs->mBack_bg;
GLuint *mBack_bgd = pArgs->mBack_bgd;
GLuint *mBack_bbg = pArgs->mBack_bbg;
GLuint *mBack_bbgd = pArgs->mBack_bbgd;

GLuint *black_text = pArgs->black_text;

GLuint *bga_text = pArgs->bga_text;


bool *fl_terminate = pArgs->fl_terminate;
bool *fl_write = pArgs->fl_write;
bool *pause_all = pArgs->pause_all;


std::swap(*mFront_dleft, *mBack_dleft);
std::swap(*mFront_left, *mBack_left);
std::swap(*mFront_aleft, *mBack_aleft);
std::swap(*mFront_bg, *mBack_bg);
std::swap(*mFront_bgd, *mBack_bgd);
std::swap(*mFront_bbg, *mBack_bbg);
std::swap(*mFront_bbgd, *mBack_bbgd);


//Get number of frames (duration)
a_video.open(a_filename);
if (!a_video.isOpened()) {
	std::cout << "cannot read depth video!\n";
}
frames = int(a_video.get(CV_CAP_PROP_FRAME_COUNT));
width = int(a_video.get(CV_CAP_PROP_FRAME_WIDTH));
height = int(a_video.get(CV_CAP_PROP_FRAME_HEIGHT));
FPSvideo = float(a_video.get(CV_CAP_PROP_FPS));
//FPSvideo = 10.0f;
a_video.release();

//GRAMMA CORRECTION LOOKUP TABLE
double inverse_gamma = 1.02;
cv::Mat lut_matrix(1, 256, CV_8UC1);
uchar * ptr = lut_matrix.ptr();
for (int i = 0; i < 256; i++)
	ptr[i] = (int)(pow((double)i / 255.0, inverse_gamma) * 255.0);


bbgd_img = cv::imread(bbgd_filename);
cv::flip(bbgd_img, bbgd_img, 0);

bbg_img = cv::imread(bbg_filename);
cv::flip(bbg_img, bbg_img, 0);
LUT(bbg_img, lut_matrix, bbg_img);


//Open videos, read first images
g_video.open(g_filename);
if (!g_video.isOpened()) {
	std::cout << "cannot read rgb video!\n";
}
g_video.read(img);
img1 = img;
cv::flip(img1, img1, 0);

//
d_video.open(d_filename);
if (!d_video.isOpened()) {
	std::cout << "cannot read video!\n";
}
d_video.read(d_img1);
cv::flip(d_img1, d_img1, 0);
//
a_video.open(a_filename);
if (!a_video.isOpened()) {
	std::cout << "cannot read video!\n";
}
a_video.read(a_img);
cv::flip(a_img, a_img, 0);

bg_img = cv::imread(bg_filename);
cv::flip(bg_img, bg_img, 0);
LUT(bg_img, lut_matrix, bg_img);

bgd_img = cv::imread(bgd_filename);
cv::flip(bgd_img, bgd_img, 0);

bga_img = cv::imread(bga_filename);
cv::flip(bga_img, bga_img, 0);


black_img = cv::imread("Resources/black.png");


//Textures
glGenTextures(1, black_text);
glBindTexture(GL_TEXTURE_2D, *black_text);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, black_img.cols, black_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, black_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_left);
glBindTexture(GL_TEXTURE_2D, *mBack_left);
glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, img1.cols, img1.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img1.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_dleft);
glBindTexture(GL_TEXTURE_2D, *mBack_dleft);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, d_img1.cols, d_img1.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, d_img1.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_aleft);
glBindTexture(GL_TEXTURE_2D, *mBack_aleft);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, a_img.cols, a_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, a_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_bg);
glBindTexture(GL_TEXTURE_2D, *mBack_bg);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bg_img.cols, bg_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, bg_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_bgd);
glBindTexture(GL_TEXTURE_2D, *mBack_bgd);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bgd_img.cols, bgd_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, bgd_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, bga_text);
glBindTexture(GL_TEXTURE_2D, *bga_text);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bga_img.cols, bga_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, bga_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mBack_bbgd);
glBindTexture(GL_TEXTURE_2D, *mBack_bbgd);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bbgd_img.cols, bbgd_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, bbgd_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);

glGenTextures(1, mFront_bbg);
glBindTexture(GL_TEXTURE_2D, *mFront_bbg);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bbg_img.cols, bbg_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, bbg_img.data);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glBindTexture(GL_TEXTURE_2D, 0);


std::swap(*mFront_dleft, *mBack_dleft);
std::swap(*mFront_left, *mBack_left);
std::swap(*mFront_aleft, *mBack_aleft);
std::swap(*mFront_bg, *mBack_bg);
std::swap(*mFront_bgd, *mBack_bgd);
//std::swap(*mFront_bbg, *mBack_bbg);
std::swap(*mFront_bbgd, *mBack_bbgd);

bool pause = false;
int framecount = 1;
int refresh = 0;

clock_t eF = clock();
clock_t DTime = 0;

Sleep(1000);
*fl_write = true;

//Read new frames, update textures
while (Platform.HandleMessages())
{	

	//PAUSE - CONTINUE	
	if (Platform.Key[VK_SPACE]) {
		pause = !pause;
		Platform.Key[VK_SPACE] = false;
 		*pause_all = !(*pause_all);
	}

	DTime = clock() - eF;
	//FPSvideo = 1;
	if ((clockToMilliseconds(DTime) > (1.0/ (FPSvideo))*1000.0) || refresh)  //every second
	{
		eF = clock();
		if (pause && refresh==0)
			continue;
		
		if (pause && refresh > 0)
		{
			refresh--;
			framecount--;
		}

		//VIDEO IMG
		g_video.read(img);	
		//LUT(img, lut_matrix, img);
		img1 = img;
		cv::flip(img1, img1, 0);

		//DEPTH IMG
		d_video.read(d_img1);
		cv::flip(d_img1, d_img1, 0);

		//ALPHA IMG
		a_video.read(a_img);
		cv::flip(a_img, a_img, 0);
		

		glBindTexture(GL_TEXTURE_2D, *mBack_left);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img1.cols, img1.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img1.data);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, *mBack_dleft);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, d_img1.cols, d_img1.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, d_img1.data);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, *mBack_aleft);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, a_img.cols, a_img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, a_img.data);
		glBindTexture(GL_TEXTURE_2D, 0);


		std::swap(mFront_left, mBack_left);
		std::swap(mFront_dleft, mBack_dleft);
		std::swap(mFront_aleft, mBack_aleft);

		framecount++;
		
		if (framecount == frames)
		{
			*fl_terminate = true;
			//ExitThread(0);
			framecount = 1;
			g_video.set(CV_CAP_PROP_POS_FRAMES, framecount);
			d_video.set(CV_CAP_PROP_POS_FRAMES, framecount);
			a_video.set(CV_CAP_PROP_POS_FRAMES, framecount);			
		}
	}
}
}

LPSTR* CommandLineToArgvA(_In_opt_ LPCSTR lpCmdLine, _Out_ int *pNumArgs)
{
	if (!pNumArgs)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	*pNumArgs = 0;
	/*follow CommandLinetoArgvW and if lpCmdLine is NULL return the path to the executable.
	Use 'programname' so that we don't have to allocate MAX_PATH * sizeof(CHAR) for argv
	every time. Since this is ANSI the return can't be greater than MAX_PATH (260
	characters)*/
	CHAR programname[MAX_PATH] = {};
	/*pnlength = the length of the string that is copied to the buffer, in characters, not
	including the terminating null character*/
	DWORD pnlength = GetModuleFileNameA(NULL, programname, MAX_PATH);
	if (pnlength == 0) //error getting program name
	{
		//GetModuleFileNameA will SetLastError
		return NULL;
	}
	if (*lpCmdLine == NULL)
	{

		/*In keeping with CommandLineToArgvW the caller should make a single call to HeapFree
		to release the memory of argv. Allocate a single block of memory with space for two
		pointers (representing argv[0] and argv[1]). argv[0] will contain a pointer to argv+2
		where the actual program name will be stored. argv[1] will be nullptr per the C++
		specifications for argv. Hence space required is the size of a LPSTR (char*) multiplied
		by 2 [pointers] + the length of the program name (+1 for null terminating character)
		multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS flag,
		so if there is a failure on allocating memory an exception will be generated.*/
		LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
			(sizeof(LPSTR) * 2) + ((pnlength + 1) * sizeof(CHAR))));
		memcpy(argv + 2, programname, pnlength + 1); //add 1 for the terminating null character
		argv[0] = reinterpret_cast<LPSTR>(argv + 2);
		argv[1] = nullptr;
		*pNumArgs = 1;
		return argv;
	}
	/*We need to determine the number of arguments and the number of characters so that the
	proper amount of memory can be allocated for argv. Our argument count starts at 1 as the
	first "argument" is the program name even if there are no other arguments per specs.*/
	int argc = 1;
	int numchars = 0;
	LPCSTR templpcl = lpCmdLine;
	bool in_quotes = false;  //'in quotes' mode is off (false) or on (true)
							 /*first scan the program name and copy it. The handling is much simpler than for other
							 arguments. Basically, whatever lies between the leading double-quote and next one, or a
							 terminal null character is simply accepted. Fancier handling is not required because the
							 program name must be a legal NTFS/HPFS file name. Note that the double-quote characters are
							 not copied.*/
	do {
		if (*templpcl == '"')
		{
			//don't add " to character count
			in_quotes = !in_quotes;
			templpcl++; //move to next character
			continue;
		}
		++numchars; //count character
		templpcl++; //move to next character
		if (_ismbblead(*templpcl) != 0) //handle MBCS
		{
			++numchars;
			templpcl++; //skip over trail byte
		}
	} while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
	//parsed first argument
	if (*templpcl == '\0')
	{
		/*no more arguments, rewind and the next for statement will handle*/
		templpcl--;
	}
	//loop through the remaining arguments
	int slashcount = 0; //count of backslashes
	bool countorcopychar = true; //count the character or not
	for (;;)
	{
		if (*templpcl)
		{
			//next argument begins with next non-whitespace character
			while (*templpcl == ' ' || *templpcl == '\t')
				++templpcl;
		}
		if (*templpcl == '\0')
			break; //end of arguments

		++argc; //next argument - increment argument count
				//loop through this argument
		for (;;)
		{
			/*Rules:
			2N     backslashes   + " ==> N backslashes and begin/end quote
			2N + 1 backslashes   + " ==> N backslashes + literal "
			N      backslashes       ==> N backslashes*/
			slashcount = 0;
			countorcopychar = true;
			while (*templpcl == '\\')
			{
				//count the number of backslashes for use below
				++templpcl;
				++slashcount;
			}
			if (*templpcl == '"')
			{
				//if 2N backslashes before, start/end quote, otherwise count.
				if (slashcount % 2 == 0) //even number of backslashes
				{
					if (in_quotes && *(templpcl + 1) == '"')
					{
						in_quotes = !in_quotes; //NB: parse_cmdline omits this line
						templpcl++; //double quote inside quoted string
					}
					else
					{
						//skip first quote character and count second
						countorcopychar = false;
						in_quotes = !in_quotes;
					}
				}
				slashcount /= 2;
			}
			//count slashes
			while (slashcount--)
			{
				++numchars;
			}
			if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
			{
				//at the end of the argument - break
				break;
			}
			if (countorcopychar)
			{
				if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
				{
					++templpcl; //skip over trail byte
					++numchars;
				}
				++numchars;
			}
			++templpcl;
		}
		//add a count for the null-terminating character
		++numchars;
	}
	/*allocate memory for argv. Allocate a single block of memory with space for argc number of
	pointers. argv[0] will contain a pointer to argv+argc where the actual program name will be
	stored. argv[argc] will be nullptr per the C++ specifications. Hence space required is the
	size of a LPSTR (char*) multiplied by argc + 1 pointers + the number of characters counted
	above multiplied by the sizeof CHAR. HeapAlloc is called with HEAP_GENERATE_EXCEPTIONS
	flag, so if there is a failure on allocating memory an exception will be generated.*/
	LPSTR *argv = static_cast<LPSTR*>(HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
		(sizeof(LPSTR) * (argc + 1)) + (numchars * sizeof(CHAR))));
	//now loop through the commandline again and split out arguments
	in_quotes = false;
	templpcl = lpCmdLine;
	argv[0] = reinterpret_cast<LPSTR>(argv + argc + 1);
	LPSTR tempargv = reinterpret_cast<LPSTR>(argv + argc + 1);
	do {
		if (*templpcl == '"')
		{
			in_quotes = !in_quotes;
			templpcl++; //move to next character
			continue;
		}
		*tempargv++ = *templpcl;
		templpcl++; //move to next character
		if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
		{
			*tempargv++ = *templpcl; //copy second byte
			templpcl++; //skip over trail byte
		}
	} while (*templpcl != '\0' && (in_quotes || (*templpcl != ' ' && *templpcl != '\t')));
	//parsed first argument
	if (*templpcl == '\0')
	{
		//no more arguments, rewind and the next for statement will handle
		templpcl--;
	}
	else
	{
		//end of program name - add null terminator
		*tempargv = '\0';
	}
	int currentarg = 1;
	argv[currentarg] = ++tempargv;
	//loop through the remaining arguments
	slashcount = 0; //count of backslashes
	countorcopychar = true; //count the character or not
	for (;;)
	{
		if (*templpcl)
		{
			//next argument begins with next non-whitespace character
			while (*templpcl == ' ' || *templpcl == '\t')
				++templpcl;
		}
		if (*templpcl == '\0')
			break; //end of arguments
		argv[currentarg] = ++tempargv; //copy address of this argument string
									   //next argument - loop through it's characters
		for (;;)
		{
			/*Rules:
			2N     backslashes   + " ==> N backslashes and begin/end quote
			2N + 1 backslashes   + " ==> N backslashes + literal "
			N      backslashes       ==> N backslashes*/
			slashcount = 0;
			countorcopychar = true;
			while (*templpcl == '\\')
			{
				//count the number of backslashes for use below
				++templpcl;
				++slashcount;
			}
			if (*templpcl == '"')
			{
				//if 2N backslashes before, start/end quote, otherwise copy literally.
				if (slashcount % 2 == 0) //even number of backslashes
				{
					if (in_quotes && *(templpcl + 1) == '"')
					{
						in_quotes = !in_quotes; //NB: parse_cmdline omits this line
						templpcl++; //double quote inside quoted string
					}
					else
					{
						//skip first quote character and count second
						countorcopychar = false;
						in_quotes = !in_quotes;
					}
				}
				slashcount /= 2;
			}
			//copy slashes
			while (slashcount--)
			{
				*tempargv++ = '\\';
			}
			if (*templpcl == '\0' || (!in_quotes && (*templpcl == ' ' || *templpcl == '\t')))
			{
				//at the end of the argument - break
				break;
			}
			if (countorcopychar)
			{
				*tempargv++ = *templpcl;
				if (_ismbblead(*templpcl) != 0) //should copy another character for MBCS
				{
					++templpcl; //skip over trail byte
					*tempargv++ = *templpcl;
				}
			}
			++templpcl;
		}
		//null-terminate the argument
		*tempargv = '\0';
		++currentarg;
	}
	argv[argc] = nullptr;
	*pNumArgs = argc;
	return argv;
}

void WaitForEnter()
{
	// if enter is already pressed, wait for
	// it to be released
	while (GetAsyncKeyState(VK_RETURN) & 0x8000) {}

	// wait for enter to be pressed
	while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {}
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR lpCmdLine, int)
{

	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	//printf("Hello console on\n");
	std::cout.clear();


	int threadDecodingID = 0;
	std::ifstream is("settings.txt");
	char buffer[200];
	char buffer_name[200];
	while (is >> buffer) {
		if (strcmp(buffer, "Path") == 0) {
			is >> buffer_name;
			sprintf(video_path, "%s", buffer_name);
		}
		if (strcmp(buffer, "DataPath") == 0) {
			is >> buffer_name;
			sprintf(data_path, "%s", buffer_name);
		}
		if (strcmp(buffer, "UserID") == 0) {
			is >> buffer_name;
			sprintf(userID, "%s", buffer_name);
		}
		if (strcmp(buffer, "Test") == 0) {
			is >> buffer_name;
			sprintf(testID, "%s", buffer_name);
		}
		if (strcmp(buffer, "RenderingMode") == 0) {
			is >> buffer_name;
			if (strcmp(buffer_name, "ours") == 0)
			{
				positional_track = true;
				render_simple = false;
				sprintf(mode, "%s", buffer_name);
			}
			if (strcmp(buffer_name, "static") == 0)
			{
				positional_track = false;
				render_simple = false;
				sprintf(mode, "%s", buffer_name);
			}
			if (strcmp(buffer_name, "simple") == 0)
			{
				positional_track = true;
				render_simple = true;
				sprintf(mode, "%s", buffer_name);
			}

		}
		if (strcmp(buffer, "VisualConstraint") == 0) {
			is >> buffer_name;
			sprintf(visID, "%s", "None");
			if (strcmp(buffer_name, "fade") == 0)
			{
				vis_fade = true;
				sprintf(visID, "%s", buffer_name);

			}
			if (strcmp(buffer_name, "Clamp") == 0)
			{
				vis_clamp = true;
				sprintf(visID, "%s", buffer_name);
			}
		}
		if (strcmp(buffer, "VideoName") == 0) {
			is >> buffer_name;
			sprintf(g1_filename, "%s.mp4", buffer_name);
			sprintf(d1_filename, "%s_depth.mp4", buffer_name);
			sprintf(bg1_filename, "%s_BG.png", buffer_name);
			sprintf(bgd1_filename, "%s_BGD.png", buffer_name);
			sprintf(bbg1_filename, "%s_BG_inp.png", buffer_name);
			sprintf(bbgd1_filename, "%s_BGD_inp.png", buffer_name);
			sprintf(a1_filename, "%s_alphaproc.mp4", buffer_name);
			sprintf(bga1_filename, "%s_BGA.png", buffer_name);
			sprintf(buffer, "%s%s_audio.mp3", video_path, buffer_name);
			audiofile = buffer;
			sprintf(data_filename, "%s%s-%s-%s-%s-%s.txt", data_path, userID, testID, mode, visID, buffer_name);
		}
	}

	//By default start with the first one
	sprintf(g_filename, "%s%s", video_path, g1_filename);
	sprintf(d_filename, "%s%s", video_path, d1_filename);
	sprintf(bg_filename, "%s%s", video_path, bg1_filename);
	sprintf(bgd_filename, "%s%s", video_path, bgd1_filename);
	sprintf(bbg_filename, "%s%s", video_path, bbg1_filename);
	sprintf(bbgd_filename, "%s%s", video_path, bbgd1_filename);
	sprintf(bga_filename, "%s%s", video_path, bga1_filename);
	sprintf(a_filename, "%s%s", video_path, a1_filename);

	//Head position stream
	//headpose.open(data_filename);

	// Initializes LibOVR, and the Rift
	ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

	VALIDATE(Platform.InitWindow(hinst, L"Oculus Room Tiny (GL)"), "Failed to open window.");

	Platform.Run(MainLoop);

	ovr_Shutdown();

	return(0);

}