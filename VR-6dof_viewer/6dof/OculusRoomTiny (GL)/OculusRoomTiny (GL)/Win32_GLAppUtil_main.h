/************************************************************************************
 Filename    :   Win32_GLAppUtil.h
 Content     :   OpenGL and Application/Window setup functionality for RoomTiny
 Created     :   October 20th, 2014
 Author      :   Tom Heath
 Copyright   :   Copyright 2014 Oculus, LLC. All Rights reserved.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 *************************************************************************************/

#ifndef OVR_Win32_GLAppUtil_h
#define OVR_Win32_GLAppUtil_h

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"
#include <assert.h>

using namespace OVR;

#ifndef VALIDATE
    #define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }
#endif

#ifndef OVR_DEBUG_LOG
    #define OVR_DEBUG_LOG(x)
#endif

const double M_PI = 3.14159265358979323846;
const double M_PI_2 = 1.57079632679489661923;


struct ARGS
{
	GLuint *mFront_left, *mFront_dleft, *mFront_aleft, *mFront_bg, *mFront_bgd, *mFront_bbg, *mFront_bbgd, *mBack_left, *mBack_dleft, *mBack_aleft, *mBack_bg, *mBack_bgd, *mBack_bbg, *mBack_bbgd, *black_text, *bga_text;
	bool *fl_write, *fl_terminate, *pause_all;
};

struct ARGS_aud
{
	bool *fl_write, *fl_terminate;
	std::string *audiofile;
	bool *pause_all;
};


std::string loadShader(const char* filepath)
{
	if (filepath) {

		std::ifstream ifs(filepath, std::ifstream::in);
		std::ostringstream oss;
		std::string temp;

		while (ifs.good()) {

			getline(ifs, temp);
			oss << temp << '\n';
			//std::cout << temp << std::endl;

		}
		ifs.close();
		return oss.str();

	}
	else {
		exit(EXIT_FAILURE);
	}
}


struct Shader
{
	GLuint program;

	Shader(const char* vertexsrc, const char* fragsrc)
	{
		//Read and load shaders				
		const std::string vertexShaderStr = loadShader(vertexsrc);
		const GLchar *vertexShader = vertexShaderStr.c_str();
		GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vshader, 1, &vertexShader, NULL);
		glCompileShader(vshader);
		GLint r;
		glGetShaderiv(vshader, GL_COMPILE_STATUS, &r);
		if (!r)
		{
			GLchar msg[1024];
			glGetShaderInfoLog(vshader, sizeof(msg), 0, msg);
			if (msg[0]) {
				OVR_DEBUG_LOG(("Compiling shader failed: %s\n", msg));
			}
		}

		const std::string FragmentShaderStr = loadShader(fragsrc);
		const GLchar *fragShader = FragmentShaderStr.c_str();
		GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fshader, 1, &fragShader, NULL);
		glCompileShader(fshader);
		glGetShaderiv(fshader, GL_COMPILE_STATUS, &r);
		if (!r)
		{
			GLchar msg[1024];
			glGetShaderInfoLog(fshader, sizeof(msg), 0, msg);
			if (msg[0]) {
				OVR_DEBUG_LOG(("Compiling shader failed: %s\n", msg));
			}
		}

		program = glCreateProgram();

		glAttachShader(program, vshader);
		glAttachShader(program, fshader);

		glLinkProgram(program);

		glDetachShader(program, vshader);
		glDetachShader(program, fshader);

		glGetProgramiv(program, GL_LINK_STATUS, &r);
		if (!r)
		{
			GLchar msg[1024];
			glGetProgramInfoLog(program, sizeof(msg), 0, msg);
			OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
		}

		glDeleteShader(vshader);
		glDeleteShader(fshader);
	}
};


//---------------------------------------------------------------------------------------
struct DepthBuffer
{
    GLuint        texId;

    DepthBuffer(Sizei size, int sampleCount)
    {
        UNREFERENCED_PARAMETER(sampleCount);

        assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFormat = GL_DEPTH_COMPONENT24;
        GLenum type = GL_UNSIGNED_INT;
        if (GLE_ARB_depth_buffer_float)
        {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
    }
    ~DepthBuffer()
    {
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
    }
};

//--------------------------------------------------------------------------
struct TextureBuffer
{
    ovrSession          Session;
    ovrTextureSwapChain  TextureChain;
    GLuint              texId;
    GLuint              fboId;
    Sizei               texSize;

    TextureBuffer(ovrSession session, bool rendertarget, bool displayableOnHmd, Sizei size, int mipLevels, unsigned char * data, int sampleCount) :
        Session(session),
        TextureChain(nullptr),
        texId(0),
        fboId(0),
        texSize(0, 0)
    {
        UNREFERENCED_PARAMETER(sampleCount);

        assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        texSize = size;

        if (displayableOnHmd)
        {
            // This texture isn't necessarily going to be a rendertarget, but it usually is.
            assert(session); // No HMD? A little odd.
            assert(sampleCount == 1); // ovr_CreateSwapTextureSetD3D11 doesn't support MSAA.

            ovrTextureSwapChainDesc desc = {};
            desc.Type = ovrTexture_2D;
            desc.ArraySize = 1;
            desc.Width = size.w;
            desc.Height = size.h;
            desc.MipLevels = 1;
            desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
            desc.SampleCount = 1;
            desc.StaticImage = ovrFalse;

            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, TextureChain, &length);

            if(OVR_SUCCESS(result))
            {
                for (int i = 0; i < length; ++i)
                {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    if (rendertarget)
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    }
                    else
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    }
                }
            }
        }
        else
        {
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);

            if (rendertarget)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        if (mipLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glGenFramebuffers(1, &fboId);
    }

    ~TextureBuffer()
    {
        if (TextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, TextureChain);
            TextureChain = nullptr;
        }
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
    }

    Sizei GetSize() const
    {
        return texSize;
    }

    void SetAndClearRenderSurface(DepthBuffer* dbuffer)
    {
        GLuint curTexId;
        if (TextureChain)
        {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
        }
        else
        {
            curTexId = texId;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
		glClearColor(0.5, 0.5, 0.5,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    void Commit()
    {
        if (TextureChain)
        {
            ovr_CommitTextureSwapChain(Session, TextureChain);
        }
    }
};

//-------------------------------------------------------------------------------------------
struct OGL
{
    static const bool       UseDebugContext = false;

    HWND                    Window;
    HDC                     hDC;
    HGLRC                   WglContext;
	HGLRC                   WglContext_VideoThread;
    OVR::GLEContext         GLEContext;
    bool                    Running;
    bool                    Key[256];
    int                     WinSizeW;
    int                     WinSizeH;
    GLuint                  fboId;
    HINSTANCE               hInstance;


    static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        OGL *p = reinterpret_cast<OGL *>(GetWindowLongPtr(hWnd, 0));
        switch (Msg)
        {
        case WM_KEYDOWN:
            p->Key[wParam] = true;
            break;
        case WM_KEYUP:
            p->Key[wParam] = false;
            break;
        case WM_DESTROY:
            p->Running = false;
            break;
		case WM_SIZE:
			p->WinSizeW = LOWORD(lParam);
			p->WinSizeH = HIWORD(lParam);
			break;
        default:
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
        }
        if ((p->Key['Q'] && p->Key[VK_CONTROL]) || p->Key[VK_ESCAPE])
        {
            p->Running = false;
        }
        return 0;
    }

    OGL() :
        Window(nullptr),
        hDC(nullptr),
        WglContext(nullptr),
		WglContext_VideoThread(nullptr),
        GLEContext(),
        Running(false),
        WinSizeW(0),
        WinSizeH(0),
        fboId(0),
        hInstance(nullptr)
    {
		// Clear input
		for (int i = 0; i < sizeof(Key) / sizeof(Key[0]); ++i)
            Key[i] = false;
    }

    ~OGL()
    {
        ReleaseDevice();
        CloseWindow();
    }

    bool InitWindow(HINSTANCE hInst, LPCWSTR title)
    {
        hInstance = hInst;
        Running = true;

        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbWndExtra = sizeof(struct OGL *);
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"ORT";
        RegisterClassW(&wc);

        // adjust the window size and show at InitDevice time
        Window = CreateWindowW(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, 0);
        if (!Window) return false;

        SetWindowLongPtr(Window, 0, LONG_PTR(this));

        hDC = GetDC(Window);

        return true;
    }

    void CloseWindow()
    {
        if (Window)
        {
            if (hDC)
            {
                ReleaseDC(Window, hDC);
                hDC = nullptr;
            }
            DestroyWindow(Window);
            Window = nullptr;
            UnregisterClassW(L"OGL", hInstance);
        }
    }


    // Note: currently there is no way to get GL to use the passed pLuid
    bool InitDevice(int vpW, int vpH, const LUID* /*pLuid*/, bool windowed = true)
    {
        UNREFERENCED_PARAMETER(windowed);

        WinSizeW = vpW;
        WinSizeH = vpH;

        RECT size = { 0, 0, vpW, vpH };
        AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
        const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
        if (!SetWindowPos(Window, nullptr, 0, 0, size.right - size.left, size.bottom - size.top, flags))
            return false;

        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBFunc = nullptr;
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARBFunc = nullptr;
		//Nevermind this part, context destroyed at the end, only necessary for getting access to  wglChoosePixelFormatARB / wglCreateContextAttribsARB.
        {
            // First create a context for the purpose of getting access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd, 0, sizeof(pfd));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 16;
            int pf = ChoosePixelFormat(hDC, &pfd);
            VALIDATE(pf, "Failed to choose pixel format.");

            VALIDATE(SetPixelFormat(hDC, pf, &pfd), "Failed to set pixel format.");

            HGLRC context = wglCreateContext(hDC);
            VALIDATE(context, "wglCreateContextfailed.");
            VALIDATE(wglMakeCurrent(hDC, context), "wglMakeCurrent failed.");

            wglChoosePixelFormatARBFunc = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARBFunc = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
            assert(wglChoosePixelFormatARBFunc && wglCreateContextAttribsARBFunc);

            wglDeleteContext(context);
        }

		//CREATE CONTEXT
        // Now create the real context that we will be using.
        int iAttributes[] =
        {
            // WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 16,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0, 0
        };

        float fAttributes[] = { 0, 0 };
        int   pf = 0;
        UINT  numFormats = 0;

        VALIDATE(wglChoosePixelFormatARBFunc(hDC, iAttributes, fAttributes, 1, &pf, &numFormats),
            "wglChoosePixelFormatARBFunc failed.");

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        VALIDATE(SetPixelFormat(hDC, pf, &pfd), "SetPixelFormat failed.");

        GLint attribs[16];
        int   attribCount = 0;
        if (UseDebugContext)
        {
            attribs[attribCount++] = WGL_CONTEXT_FLAGS_ARB;
            attribs[attribCount++] = WGL_CONTEXT_DEBUG_BIT_ARB;
        }

        attribs[attribCount] = 0;

        WglContext = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
		/////////////////////////////////////////////////////////////////////
		//Create second context
		WglContext_VideoThread = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
		BOOL error_share = wglShareLists(WglContext, WglContext_VideoThread);
		if (error_share == FALSE)
		{
			DWORD errorCode = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
			MessageBox(NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);
			LocalFree(lpMsgBuf);
			//Destroy the GL context and just use 1 GL context
			wglDeleteContext(WglContext_VideoThread);
		}
		/////////////////////////////////////////////////////////////////////
        VALIDATE(wglMakeCurrent(hDC, WglContext), "wglMakeCurrent failed.");

        OVR::GLEContext::SetCurrentContext(&GLEContext);
        GLEContext.Init();

        glGenFramebuffers(1, &fboId);

        glEnable(GL_DEPTH_TEST);
        glFrontFace(GL_CW);
        glEnable(GL_CULL_FACE);

        if (UseDebugContext && GLE_ARB_debug_output)
        {
            glDebugMessageCallbackARB(DebugGLCallback, NULL);
            if (glGetError())
            {
                OVR_DEBUG_LOG(("glDebugMessageCallbackARB failed."));
            }

            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

            // Explicitly disable notification severity output.
            glDebugMessageControlARB(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
        }

		PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXTFunc = nullptr;
		wglSwapIntervalEXTFunc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if (wglSwapIntervalEXTFunc)
		{
			wglSwapIntervalEXTFunc(0);
		}


        return true;
    }

    bool HandleMessages(void)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return Running;
    }

    void Run(bool (*MainLoop)(bool retryCreate))
    {
        while (HandleMessages())
        {
            // true => we'll attempt to retry for ovrError_DisplayLost
            if (!MainLoop(true))
                break;
            // Sleep a bit before retrying to reduce CPU load while the HMD is disconnected
            Sleep(10);
        }
    }

    void ReleaseDevice()
    {
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
        if (WglContext)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(WglContext);
            WglContext = nullptr;
        }
        GLEContext.Shutdown();
    }

    static void GLAPIENTRY DebugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        UNREFERENCED_PARAMETER(source);
        UNREFERENCED_PARAMETER(type);
        UNREFERENCED_PARAMETER(id);
        UNREFERENCED_PARAMETER(severity);
        UNREFERENCED_PARAMETER(length);
        UNREFERENCED_PARAMETER(message);
        UNREFERENCED_PARAMETER(userParam);
        OVR_DEBUG_LOG(("Message from OpenGL: %s\n", message));
    }
};

// Global OpenGL state
static OGL Platform;

//------------------------------------------------------------------------------

struct VertexBuffer
{
    GLuint    buffer;

    VertexBuffer(void* vertices, size_t size)
    {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }
    ~VertexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

//----------------------------------------------------------------
struct IndexBuffer
{
    GLuint    buffer;

    IndexBuffer(void* indices, size_t size)
    {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }
    ~IndexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

//---------------------------------------------------------------------------
struct Model
{
    struct Vertex
    {
        Vector3f  Pos;
        DWORD     C;
        float     U, V;
    };

    Vector3f        Pos;
    Quatf           Rot;
    Matrix4f        Mat;
    int             numVertices, numIndices;
    Vertex          Vertices[50000000]; // NOTE fixed maximum!!
    GLuint        Indices[50000000];
    VertexBuffer  * vertexBuffer;
    IndexBuffer   * indexBuffer;

    Model(Vector3f pos) :
        numVertices(0),
        numIndices(0),
        Pos(pos),
        Rot(),
        Mat(),
        vertexBuffer(nullptr),
        indexBuffer(nullptr)
    {}

    ~Model()
    {
        FreeBuffers();
    }

    Matrix4f& GetMatrix()
    {
        Mat = Matrix4f(Rot);
        Mat = Matrix4f::Translation(Pos) * Mat;
        return Mat;
    }

    void AddVertex(const Vertex& v) { Vertices[numVertices++] = v; }
    void AddIndex(GLuint a) { Indices[numIndices++] = a; }

    void AllocateBuffers()
    {
        vertexBuffer = new VertexBuffer(&Vertices[0], numVertices * sizeof(Vertices[0]));
        indexBuffer = new IndexBuffer(&Indices[0], numIndices * sizeof(Indices[0]));
    }

    void FreeBuffers()
    {
        delete vertexBuffer; vertexBuffer = nullptr;
        delete indexBuffer; indexBuffer = nullptr;
    }

	void AddSphere(float radius, int rings, int slices) {
		 
		//Generate sphere
		float x, y, z, r, s;
		float const R = 1.f / (float)(rings - 1);
		float const S = 1.f / (float)(slices - 1);
		Vertex vertex;
		for (r = 0; r < rings; ++r) {
			for (s = 0; s < slices; ++s) {
				x = cosf(2 * M_PI * s * S) * sinf(M_PI * r * R);
				z = sinf(2 * M_PI * s * S) * sinf(M_PI * r * R);
				y = sinf(-M_PI_2 + (M_PI * r * R));
				vertex.Pos.x = x * radius;
				vertex.Pos.y = y * radius;
				vertex.Pos.z = z * radius;
				vertex.U = s*S;
				vertex.V = r*R;
				vertex.C = 0xffffffff;
				AddVertex(vertex);

			}
		}
		unsigned int ringStart, nextRingStart, nextslice;
		for (r = 0; r < rings - 1; r++) {
			ringStart = r * slices;
			nextRingStart = (r + 1) * slices;
			for (s = 0; s < slices -1; s++) {
				nextslice = s + 1;
				// The quad
				AddIndex(ringStart + s);	
				AddIndex(ringStart + nextslice);
				AddIndex(nextRingStart + nextslice);
				AddIndex(nextRingStart + s);											
				AddIndex(ringStart + s);				
				AddIndex(nextRingStart + nextslice);				
				
			}
		}

		
	}
		

	void RenderBlack(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, double ablack, float radius, bool isblack, ARGS args, Shader * Shaders[10])
	{
		Matrix4f combined = proj * view * GetMatrix();
		Matrix4f mtw = GetMatrix();

		if (poly_mesh == true)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glDisable(GL_DEPTH_TEST);
		GLuint shader_black = Shaders[4]->program;
		glUseProgram(shader_black);

		float black = isblack ? 1.0f : 0.0f;
		glUniform1f(glGetUniformLocation(shader_black, "black"), (float)black);
		glUniform1f(glGetUniformLocation(shader_black, "radius"), (float)radius);
		glUniform1f(glGetUniformLocation(shader_black, "alpha"), (float)ablack);
		glUniformMatrix4fv(glGetUniformLocation(shader_black, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, *args.black_text);
		glUniform1i(glGetUniformLocation(shader_black, "bgtext"), 4);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
		glUniform1i(glGetUniformLocation(shader_black, "depthbg"), 5);


		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

		GLuint posLoc = glGetAttribLocation(shader_black, "Position");
		GLuint colorLoc = glGetAttribLocation(shader_black, "Color");
		GLuint uvLoc = glGetAttribLocation(shader_black, "TexCoord");

		glEnableVertexAttribArray(posLoc);
		glEnableVertexAttribArray(colorLoc);
		glEnableVertexAttribArray(uvLoc);

		glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
		glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
		glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));


		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);
		//glDrawElements(GL_POINTS, numIndices, GL_UNSIGNED_SHORT, NULL);

		glDisableVertexAttribArray(posLoc);
		glDisableVertexAttribArray(colorLoc);
		glDisableVertexAttribArray(uvLoc);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glUseProgram(0);


	};

	void RenderSimple(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, float desat, ARGS args, Shader * Shaders[10])
	{
		Matrix4f combined = proj * view * GetMatrix();
		Matrix4f mtw = GetMatrix();

		if (poly_mesh == true)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		//glDisable(GL_DEPTH_TEST);
		GLuint shader_simple = Shaders[3]->program;
		glUseProgram(shader_simple);

		glUniformMatrix4fv(glGetUniformLocation(shader_simple, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);
		glUniform1f(glGetUniformLocation(shader_simple, "desat"), (float)desat);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
		glUniform1i(glGetUniformLocation(shader_simple, "depthbg"), 5);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_left);
		glUniform1i(glGetUniformLocation(shader_simple, "bgtext"), 4);


		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

		GLuint posLoc = glGetAttribLocation(shader_simple, "Position");
		GLuint colorLoc = glGetAttribLocation(shader_simple, "Color");
		GLuint uvLoc = glGetAttribLocation(shader_simple, "TexCoord");

		glEnableVertexAttribArray(posLoc);
		glEnableVertexAttribArray(colorLoc);
		glEnableVertexAttribArray(uvLoc);

		glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
		glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
		glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));


		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);
		//glDrawElements(GL_POINTS, numIndices, GL_UNSIGNED_SHORT, NULL);

		glDisableVertexAttribArray(posLoc);
		glDisableVertexAttribArray(colorLoc);
		glDisableVertexAttribArray(uvLoc);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glUseProgram(0);

	};

    void Render(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, float desat, ARGS args, Shader * Shaders[10])
    {
        Matrix4f combined = proj * view * GetMatrix();
		Matrix4f mtw = GetMatrix();

		if (poly_mesh == true)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);	
		//glDisable(GL_DEPTH_TEST);
		GLuint posLoc;
		GLuint colorLoc;
		GLuint uvLoc;
		/////////////////////////////////////////////////////////////////////////////////////
		
		GLuint shader_bg = Shaders[0]->program;
		glUseProgram(shader_bg);
		glUniformMatrix4fv(glGetUniformLocation(shader_bg, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);
		float vOut = colored ? 1.0f : 0.0f;
		glUniform1fv(glGetUniformLocation(shader_bg, "colored"), 1, (FLOAT*)&vOut);
		glUniform1f(glGetUniformLocation(shader_bg, "desat"), (float)desat);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_bbgd);
		glUniform1i(glGetUniformLocation(shader_bg, "depthbg"), 1);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_bbg);
		glUniform1i(glGetUniformLocation(shader_bg, "bgtext"), 3);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
		glUniform1i(glGetUniformLocation(shader_bg, "depthfront"), 5);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, *args.mFront_bgd);
		glUniform1i(glGetUniformLocation(shader_bg, "depthfg"), 1);

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

		posLoc = glGetAttribLocation(shader_bg, "Position");
		colorLoc = glGetAttribLocation(shader_bg, "Color");
		uvLoc = glGetAttribLocation(shader_bg, "TexCoord");

		glEnableVertexAttribArray(posLoc);
		glEnableVertexAttribArray(colorLoc);
		glEnableVertexAttribArray(uvLoc);

		glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
		glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
		glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));

		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);

		glDisableVertexAttribArray(posLoc);
		glDisableVertexAttribArray(colorLoc);
		glDisableVertexAttribArray(uvLoc);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glUseProgram(0);
		
		////////////////////////////////////////////////////////////////////////////////////
		if (layers >= 2.0f) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			GLuint shader_fg = Shaders[1]->program;
			glUseProgram(shader_fg);

			glUniformMatrix4fv(glGetUniformLocation(shader_fg, "matWVP2"), 1, GL_TRUE, (FLOAT*)&combined);
			float vOut = colored ? 1.0f : 0.0f;
			glUniform1fv(glGetUniformLocation(shader_fg, "colored"), 1, (FLOAT*)&vOut);
			glUniform1f(glGetUniformLocation(shader_fg, "desat"), (float)desat);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_bgd);
			glUniform1i(glGetUniformLocation(shader_fg, "fgdepth"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_bgd);
			glUniform1i(glGetUniformLocation(shader_fg, "Fragfgdepth"), 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_bg);
			glUniform1i(glGetUniformLocation(shader_fg, "fgtext"), 0);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, *args.bga_text);
			glUniform1i(glGetUniformLocation(shader_fg, "alphamask"), 7);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
			glUniform1i(glGetUniformLocation(shader_fg, "frontdepth"), 5);

			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

			posLoc = glGetAttribLocation(shader_fg, "Position2");
			colorLoc = glGetAttribLocation(shader_fg, "Color2");
			uvLoc = glGetAttribLocation(shader_fg, "TexCoord2");

			glEnableVertexAttribArray(posLoc);
			glEnableVertexAttribArray(colorLoc);
			glEnableVertexAttribArray(uvLoc);

			glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
			glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
			glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));


			glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);

			glDisableVertexAttribArray(posLoc);
			glDisableVertexAttribArray(colorLoc);
			glDisableVertexAttribArray(uvLoc);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glUseProgram(0);
		}
		
		/////////////////////////////////////////////////////////////////////////////////////
		if (layers >= 3.0f) {
			GLuint shader_mov = Shaders[2]->program;
			glUseProgram(shader_mov);

			glUniformMatrix4fv(glGetUniformLocation(shader_mov, "matWVP2"), 1, GL_TRUE, (FLOAT*)&combined);
			glUniformMatrix4fv(glGetUniformLocation(shader_mov, "matW2"), 1, GL_TRUE, (FLOAT*)&mtw);
			glUniformMatrix4fv(glGetUniformLocation(shader_mov, "ViewDir2"), 1, GL_TRUE, (FLOAT*)&view);
			glUniform3fv(glGetUniformLocation(shader_mov, "SphCenter2"), 1, (FLOAT*)&spherecenter);
			glUniform3fv(glGetUniformLocation(shader_mov, "spherecenter"), 1, (FLOAT*)&spherecenter);
			glUniform3fv(glGetUniformLocation(shader_mov, "eyepos"), 1, (FLOAT*)&EyePos);
			glUniform3fv(glGetUniformLocation(shader_mov, "headposition"), 1, (FLOAT*)&HeadPos);
			float vOut = colored ? 1.0f : 0.0f;
			glUniform1fv(glGetUniformLocation(shader_mov, "colored"), 1, (FLOAT*)&vOut);
			glUniform1f(glGetUniformLocation(shader_mov, "desat"), (float)desat);


			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
			glUniform1i(glGetUniformLocation(shader_mov, "fgdepth"), 5);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_left);
			glUniform1i(glGetUniformLocation(shader_mov, "fgtext"), 4);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_aleft);
			glUniform1i(glGetUniformLocation(shader_mov, "alphamask"), 7);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, *args.mFront_dleft);
			glUniform1i(glGetUniformLocation(shader_mov, "Fragfgdepth"), 6);



			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

			posLoc = glGetAttribLocation(shader_mov, "Position2");
			colorLoc = glGetAttribLocation(shader_mov, "Color2");
			uvLoc = glGetAttribLocation(shader_mov, "TexCoord2");

			glEnableVertexAttribArray(posLoc);
			glEnableVertexAttribArray(colorLoc);
			glEnableVertexAttribArray(uvLoc);

			glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
			glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
			glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));


			glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, NULL);
			//glDrawElements(GL_POINTS, numIndices, GL_UNSIGNED_SHORT, NULL);

			glDisableVertexAttribArray(posLoc);
			glDisableVertexAttribArray(colorLoc);
			glDisableVertexAttribArray(uvLoc);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glUseProgram(0);
		}

		///////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////

		//glDeleteTextures(1, &texture);
		//glDeleteTextures(1, &depth_texture);

    }
};

//------------------------------------------------------------------------- 
struct Scene
{
    int     numModels;
    Model * Models[10];
	int numShaders;
	Shader * Shaders[10];

    void    Add(Model * n)
    {
        Models[numModels++] = n;
    }

	void    AddShader(Shader * n)
	{
		Shaders[numShaders++] = n;
	}

    void Render(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, float desat, ARGS args)
    {
        for (int i = 0; i < numModels; ++i)
            Models[i]->Render(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, desat, args, Shaders);
    }
	void RenderBlack(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, double ablack, float radius, bool isblack, ARGS args)
	{
		for (int i = 0; i < numModels; ++i)
			Models[i]->RenderBlack(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, ablack, radius, isblack, args, Shaders);
	}
	void RenderSimple(Vector2f ScreenSize, Vector3f spherecenter, Vector3f EyePos, Vector3f HeadPos, Matrix4f view, Matrix4f proj, int eye, bool poly_mesh, bool stereo, bool render_depth, bool colored, double layers, float desat,ARGS args)
	{
		for (int i = 0; i < numModels; ++i)
			Models[i]->RenderSimple(ScreenSize, spherecenter, EyePos, HeadPos, view, proj, eye, poly_mesh, stereo, render_depth, colored, layers, desat, args, Shaders);
	}

    void Init(int includeIntensiveGPUobject, Vector3f HeadPos, Vector2i SphereSize)
    {

		const char* vertexsrc = "Resources/VertexShader-bg_simple.vs";
		const char* fragsrc = "Resources/FragmentShader-bg_simple.fs";
		Shader * s = new Shader(vertexsrc, fragsrc);
		AddShader(s);

		vertexsrc = "Resources/VertexShader-fg_simple.vs";
		fragsrc = "Resources/FragmentShader-fg_simple.fs";
		s = new Shader(vertexsrc, fragsrc);
		AddShader(s);

		vertexsrc = "Resources/VertexShader-mov_simple.vs";
		fragsrc = "Resources/FragmentShader-mov_simple.fs";
		s = new Shader(vertexsrc, fragsrc);
		AddShader(s);

		vertexsrc = "Resources/VertexShader-simple-simple.vs";
		fragsrc = "Resources/FragmentShader-bg_simple.fs";
		s = new Shader(vertexsrc, fragsrc);
		AddShader(s);
		
		vertexsrc = "Resources/VertexShader-black.vs";
		fragsrc = "Resources/FragmentShader-black.fs";
		s = new Shader(vertexsrc, fragsrc);
		AddShader(s);

        // Construct geometry		
		Model * m = new Model(HeadPos);
		//Change number of rings and slices to desired
		m->AddSphere(1.0, SphereSize.x, SphereSize.y); //radius, rings, slices
		m->AllocateBuffers();
		Add(m);
    }

	Scene() :  numModels(0) {
		numShaders = 0;
	};
	Scene(bool includeIntensiveGPUobject, Vector3f HeadPos, Vector2i SphereSize) :	numModels(0)
    {
		numShaders = 0;
		numModels = 0;
        Init(includeIntensiveGPUobject, HeadPos, SphereSize);
    }
    void Release()
    {
        while (numModels-- > 0)
            delete Models[numModels];
    }
	void cleanprograms()
	{
		while (numShaders-- > 0)
			glDeleteProgram(Shaders[numShaders]->program);
			
	}
    ~Scene()
    {
        Release();
		cleanprograms();
    }
};


#endif // OVR_Win32_GLAppUtil_h
