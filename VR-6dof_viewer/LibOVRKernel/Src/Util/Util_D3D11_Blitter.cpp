/************************************************************************************

Filename    :   Util_D3D11_Blitter.cpp
Content     :   D3D11 implementation for blitting, supporting scaling & rotation
Created     :   February 24, 2015
Authors     :   Reza Nourai

Copyright   :   Copyright 2014-2016 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.3 (the "License");
you may not use the Oculus VR Rift SDK except in compliance with the License,
which is provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.3

Unless required by applicable law or agreed to in writing, the Oculus VR SDK
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#include "Util_D3D11_Blitter.h"

#ifdef OVR_OS_MS

#include "Util/Util_Direct3D.h"
#include "Shaders/Blt_vs.h"
#include "Shaders/Blt_ps.h"
#include "Shaders/Blt_ps_ms2.h"
#include "Shaders/Blt_ps_ms4.h"

namespace OVR {
namespace D3DUtil {

static ovrlog::Channel Log("Blitter");

//-------------------------------------------------------------------------------------
// ***** CAPI::Blitter

Blitter::Blitter(const Ptr<ID3D11Device>& device)
    : Device(),
      Context1(),
      BltState(),
      IL(),
      VB(),
      VS(),
      PS(),
      PS_MS2(),
      PS_MS4(),
      Sampler(),
      AlreadyInitialized(false) {
  device->QueryInterface(IID_PPV_ARGS(&Device.GetRawRef()));
  OVR_ASSERT(Device);

  Device->GetImmediateContext1(&Context1.GetRawRef());
  OVR_ASSERT(Context1);
}

Blitter::~Blitter() {}

bool Blitter::Initialize() {
  if (!Device) {
    OVR_ASSERT(false);
    return false;
  }

  OVR_ASSERT(!AlreadyInitialized);
  if (AlreadyInitialized) {
    return false;
  }

  UINT deviceFlags = Device->GetCreationFlags();
  D3D_FEATURE_LEVEL featureLevel = Device->GetFeatureLevel();

  // If the device is single threaded, the context state must be too
  UINT stateFlags = 0;
  if (deviceFlags & D3D11_CREATE_DEVICE_SINGLETHREADED) {
    stateFlags |= D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED;
  }

  // TODO: Clean this up with OVR_D3D_CREATE() when we move OVRError to kernel.

  OVR_ASSERT(!BltState); // Expected to be null on the way in.
  BltState = nullptr; // Prevents a potential leak on the next line.
  HRESULT hr = Device->CreateDeviceContextState(
      stateFlags,
      &featureLevel,
      1,
      D3D11_SDK_VERSION,
      __uuidof(ID3D11Device1),
      nullptr,
      &BltState.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(BltState);

  OVR_ASSERT(!VS); // Expected to be null on the way in.
  VS = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreateVertexShader(Blt_vs, sizeof(Blt_vs), nullptr, &VS.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(VS);

  OVR_ASSERT(!PS); // Expected to be null on the way in.
  PS = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreatePixelShader(Blt_ps, sizeof(Blt_ps), nullptr, &PS.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(PS);

  OVR_ASSERT(!PS_MS2); // Expected to be null on the way in.
  PS_MS2 = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreatePixelShader(Blt_ps_ms2, sizeof(Blt_ps_ms2), nullptr, &PS_MS2.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(PS_MS2);

  OVR_ASSERT(!PS_MS4); // Expected to be null on the way in.
  PS_MS4 = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreatePixelShader(Blt_ps_ms4, sizeof(Blt_ps_ms4), nullptr, &PS_MS4.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(PS_MS4);

  D3D11_INPUT_ELEMENT_DESC elems[2] = {};
  elems[0].Format = DXGI_FORMAT_R32G32_FLOAT;
  elems[0].SemanticName = "POSITION";
  elems[1].AlignedByteOffset = sizeof(float) * 2;
  elems[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  elems[1].SemanticName = "TEXCOORD";

  OVR_ASSERT(!IL); // Expected to be null on the way in.
  IL = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreateInputLayout(elems, _countof(elems), Blt_vs, sizeof(Blt_vs), &IL.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(IL);

  // Quad with texcoords designed to rotate the source 90deg clockwise
  BltVertex vertices[] = {
      {-1, 1, 0, 0}, {1, 1, 1, 0}, {1, -1, 1, 1}, {-1, 1, 0, 0}, {1, -1, 1, 1}, {-1, -1, 0, 1}};

  D3D11_BUFFER_DESC bd = {};
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.ByteWidth = sizeof(vertices);
  bd.StructureByteStride = sizeof(BltVertex);
  bd.Usage = D3D11_USAGE_DEFAULT;

  D3D11_SUBRESOURCE_DATA init = {};
  init.pSysMem = vertices;
  init.SysMemPitch = sizeof(vertices);
  init.SysMemSlicePitch = init.SysMemPitch;

  OVR_ASSERT(!VB); // Expected to be null on the way in.
  VB = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreateBuffer(&bd, &init, &VB.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  OVR_D3D_TAG_OBJECT(VB);

  D3D11_SAMPLER_DESC ss = {};
  ss.AddressU = ss.AddressV = ss.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  ss.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  ss.MaxLOD = 15;

  OVR_ASSERT(!Sampler); // Expected to be null on the way in.
  Sampler = nullptr; // Prevents a potential leak on the next line.
  hr = Device->CreateSamplerState(&ss, &Sampler.GetRawRef());
  OVR_D3D_CHECK_RET_FALSE(hr);
  // OVR_D3D_TAG_OBJECT();  Seems to already have a name.

  // Swap to our blt state to set it up
  Ptr<ID3DDeviceContextState> existingState;
  Context1->SwapDeviceContextState(BltState, &existingState.GetRawRef());

  Context1->IASetInputLayout(IL);
  Context1->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context1->VSSetShader(VS, nullptr, 0);
  Context1->PSSetSamplers(0, 1, &Sampler.GetRawRef());

  // Swap back
  Context1->SwapDeviceContextState(existingState, nullptr);

  AlreadyInitialized = true;
  return true;
}

bool Blitter::Blt(ID3D11RenderTargetView* dest, ID3D11ShaderResourceView* source) {
  Ptr<ID3D11Resource> resource;
  dest->GetResource(&resource.GetRawRef());
  Ptr<ID3D11Texture2D> texture;
  HRESULT hr = resource->QueryInterface(IID_PPV_ARGS(&texture.GetRawRef()));
  OVR_D3D_CHECK_RET_FALSE(hr);

  D3D11_TEXTURE2D_DESC desc = {};
  texture->GetDesc(&desc);

  return Blt(dest, source, 0, 0, desc.Width, desc.Height);
}

bool Blitter::Blt(
    ID3D11RenderTargetView* dest,
    ID3D11ShaderResourceView* source,
    uint32_t topLeftX,
    uint32_t topLeftY,
    uint32_t width,
    uint32_t height) {
  OVR_ASSERT(AlreadyInitialized);
  if (!AlreadyInitialized) {
    return false;
  }

  // Switch to our state
  Ptr<ID3DDeviceContextState> existingState;
  Context1->SwapDeviceContextState(BltState, &existingState.GetRawRef());

  ID3D11RenderTargetView* nullRTVs[] = {nullptr, nullptr, nullptr, nullptr};
  ID3D11ShaderResourceView* nullSRVs[] = {nullptr, nullptr, nullptr, nullptr};

  Context1->OMSetRenderTargets(_countof(nullRTVs), nullRTVs, nullptr);
  Context1->PSSetShaderResources(0, _countof(nullSRVs), nullSRVs);

  // Set the mirror as the render target
  Context1->OMSetRenderTargets(1, &dest, nullptr);

  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = (float)topLeftX;
  vp.TopLeftY = (float)topLeftY;
  vp.Width = (float)width;
  vp.Height = (float)height;
  vp.MaxDepth = 1.0f;
  Context1->RSSetViewports(1, &vp);

  Context1->PSSetShaderResources(0, 1, &source);

  Ptr<ID3D11Resource> resource;
  source->GetResource(&resource.GetRawRef());

  Ptr<ID3D11Texture2D> tmpTexture;
  HRESULT hr = resource->QueryInterface(IID_PPV_ARGS(&tmpTexture.GetRawRef()));
  if (FAILED(hr)) {
    OVR_ASSERT(false);
    return false;
  }

  D3D11_TEXTURE2D_DESC texDesc;
  tmpTexture->GetDesc(&texDesc);

  switch (texDesc.SampleDesc.Count) {
    case 1:
      Context1->PSSetShader(PS, nullptr, 0);
      break;
    case 2:
      Context1->PSSetShader(PS_MS2, nullptr, 0);
      break;
    case 4:
      Context1->PSSetShader(PS_MS4, nullptr, 0);
      break;
    default:
      // Need to include additional sample levels
      OVR_ASSERT(false);
  }

  static const uint32_t stride = sizeof(BltVertex);
  static const uint32_t offset = 0;
  Context1->IASetVertexBuffers(0, 1, &VB.GetRawRef(), &stride, &offset);

  Context1->Draw(6, 0);

  Context1->OMSetRenderTargets(_countof(nullRTVs), nullRTVs, nullptr);
  Context1->PSSetShaderResources(0, _countof(nullSRVs), nullSRVs);

  // Switch back to app state
  Context1->SwapDeviceContextState(existingState, nullptr);

  return true;
}

static bool operator!=(const DXGI_SAMPLE_DESC& s1, const DXGI_SAMPLE_DESC& s2) {
  return (s1.Count != s2.Count) || (s1.Quality != s2.Quality);
}

static bool operator!=(const D3D11_TEXTURE2D_DESC& d1, const D3D11_TEXTURE2D_DESC& d2) {
  return (d1.Width != d2.Width) || (d1.Height != d2.Height) || (d1.MipLevels != d2.MipLevels) ||
      (d1.ArraySize != d2.ArraySize) || (d1.Format != d2.Format) ||
      (d1.SampleDesc != d2.SampleDesc) || (d1.Usage != d2.Usage) ||
      (d1.BindFlags != d2.BindFlags) || (d1.CPUAccessFlags != d2.CPUAccessFlags) ||
      (d1.MiscFlags != d2.MiscFlags);
}

D3DTextureWriter::D3DTextureWriter(ID3D11Device* deviceNew)
    : device(deviceNew), textureCopy(), pixels() {
  memset(&textureCopyDesc, 0, sizeof(textureCopyDesc)); // We use memset instead of ={} because the
  // former zeroes filler memory between
  // variables, allowing comparison via
  // memcmp.
}

void D3DTextureWriter::Shutdown() {
  device.Clear();
  textureCopy.Clear();
  textureCopyDesc = {};
  pixels.clear();
  pixels.shrink_to_fit(); // Frees any memory associated with the container.
}

void D3DTextureWriter::SetDevice(ID3D11Device* deviceNew) {
  if (device != deviceNew) {
    textureCopy.Clear();
    textureCopyDesc = {};
    // No need to clear the pixels.

    device = deviceNew;
  }
}

D3DTextureWriter::Result D3DTextureWriter::SaveTexture(
    ID3D11Texture2D* texture,
    UINT subresource,
    bool copyTexture,
    const wchar_t* path,
    const ovrTimewarpProjectionDesc* depthProj,
    const float* linearDepthScale) {
  if (texture == nullptr)
    return Result::NULL_SURFACE;

  if (device == nullptr)
    return Result::NULL_DEVICE;

  Ptr<ID3D11DeviceContext> deviceContext;
  device->GetImmediateContext(&deviceContext.GetRawRef()); // Always succeeds.

  Ptr<ID3D11Texture2D>
      textureSource; // This will point to either the input texture or to our textureCopy.

  // Create textureCopy surface to copy back to CPU.
  D3D11_TEXTURE2D_DESC textureDesc{};
  texture->GetDesc(&textureDesc);

  if (copyTexture) {
    textureDesc.BindFlags = 0;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textureDesc.MiscFlags = 0;

    // We try to use our existing cached textureCopy as an intermediate texture, which will often
    // be possible because a typical usage of this class is to keep copying the same texture.
    if (textureDesc != textureCopyDesc) // If not equal...
    {
      textureCopyDesc = textureDesc;
      textureCopy.Clear();

      HRESULT hr = device->CreateTexture2D(&textureCopyDesc, nullptr, &textureCopy.GetRawRef());

      if (FAILED(hr)) {
        textureCopy.Clear();
        textureCopyDesc = {};
        return Result::TEXTURE_CREATION_FAILURE;
      }
    }

    // Copy texture to textureCopy.
    deviceContext->CopyResource(textureCopy, texture); // Always succeeds.

    textureSource = textureCopy;
  } else {
    textureSource = texture;
  }

  // At this point we have a valid D3D device, source texture, and intermediate texture.
  // We will write the source texture to the intermediate texture and then copy the intermediate
  // texture to a memory buffer while converting to BGRA, then write the memory buffer to disk.
  // We don't copy the source texture directly to the memory buffer because that will block for some
  // time.

  // Map textureSource so we can read its pixels.
  D3D11_MAPPED_SUBRESOURCE mapped{};
  HRESULT hr = deviceContext->Map(textureSource, subresource, D3D11_MAP_READ, 0, &mapped);

  if (FAILED(hr)) {
    return Result::TEXTURE_MAP_FAILURE;
  }

  // Now copy textureSource to pixels, converting textureSource's format as-needed to make pixels be
  // BGRA.
  pixels.resize(textureDesc.Width * textureDesc.Height);

  if (textureDesc.Format == DXGI_FORMAT_R11G11B10_FLOAT) {
    // Convert from R11G11B10_FLOAT to R8G8B8
    uint32_t inputPitchInPixels = (mapped.RowPitch / 4);

    for (uint32_t y = 0; y < textureDesc.Height; ++y) {
      for (uint32_t x = 0; x < textureDesc.Width; ++x) {
#pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
        union XMFLOAT3PK {
          union {
            uint32_t v;
            struct {
              uint32_t xm : 6;
              uint32_t xe : 5;
              uint32_t ym : 6;
              uint32_t ye : 5;
              uint32_t zm : 5;
              uint32_t ze : 5;
            };
          };
        };

        XMFLOAT3PK packedFloat{((uint32_t*)mapped.pData)[y * inputPitchInPixels + x]};
        float rFloat, gFloat, bFloat;

        uint32_t* floatRef = (uint32_t*)&rFloat;
        *floatRef = ((packedFloat.xe - 15 + 127) << 23U) | (packedFloat.xm << 17);

        floatRef = (uint32_t*)&gFloat;
        *floatRef = ((packedFloat.ye - 15 + 127) << 23U) | (packedFloat.ym << 17);

        floatRef = (uint32_t*)&bFloat;
        *floatRef = ((packedFloat.ze - 15 + 127) << 23U) | (packedFloat.zm << 18);

        // This is back-asswards but we're converting out of linear so all
        // the other images stored directly match in brightness for comparison.
        uint32_t r = (uint32_t)(powf(rFloat, 1.0f / 2.2f) * 255.0f);
        uint32_t g = (uint32_t)(powf(gFloat, 1.0f / 2.2f) * 255.0f);
        uint32_t b = (uint32_t)(powf(bFloat, 1.0f / 2.2f) * 255.0f);

        uint32_t bgra = (255 << 24) | (r << 16) | (g << 8) | b;

        pixels[(y * textureDesc.Width) + x] = bgra;
      }
    }
  } else if (textureDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) {
    // Convert from R10G10B10 to R8G8B8
    uint32_t inputPitchInPixels = mapped.RowPitch / 4;

    for (uint32_t y = 0; y < textureDesc.Height; ++y) {
      for (uint32_t x = 0; x < textureDesc.Width; ++x) {
        uint32_t wideRGBA = ((uint32_t*)mapped.pData)[(y * inputPitchInPixels) + x];
        uint32_t r = (wideRGBA >> 00) & 0x3ff;
        uint32_t g = (wideRGBA >> 10) & 0x3ff;
        uint32_t b = (wideRGBA >> 20) & 0x3ff;
        uint32_t a = (wideRGBA >> 30) & 0x003;

        // This is back-asswards but we're converting out of linear so all
        // the other images stored directly match in brightness for comparison.
        r = (uint32_t)(powf((float)r / 1024.0f, 1.0f / 2.2f) * 255.0f);
        g = (uint32_t)(powf((float)g / 1024.0f, 1.0f / 2.2f) * 255.0f);
        b = (uint32_t)(powf((float)b / 1024.0f, 1.0f / 2.2f) * 255.0f);

        uint32_t bgra = (a << 24) | (r << 16) | (g << 8) | b;

        pixels[(y * textureDesc.Width) + x] = bgra;
      }
    }
  } else if (
      textureDesc.Format == DXGI_FORMAT_R32_FLOAT ||
      textureDesc.Format == DXGI_FORMAT_R32_TYPELESS ||
      textureDesc.Format == DXGI_FORMAT_D32_FLOAT) {
    if (depthProj == nullptr && linearDepthScale == nullptr) {
      OVR_ASSERT(false);
      Log.LogError("Tried to save depth image but depth projection is null or invalid");
    }

    uint32_t inputPitchInPixels = mapped.RowPitch / 4;

    for (uint32_t y = 0; y < textureDesc.Height; ++y) {
      for (uint32_t x = 0; x < textureDesc.Width; ++x) {
        float rValue = ((float*)mapped.pData)[y * inputPitchInPixels + x];

        // linearDepth = -(Proj.M[2][3]) / ( Proj.M[2][2] - Proj.M[3][2] * nonLinearDepth))
        float linearDepth = 0.0f;
        if (linearDepthScale) {
          linearDepth = rValue * *linearDepthScale;
        } else {
          linearDepth = -(depthProj->Projection23) /
              (depthProj->Projection22 - depthProj->Projection32 * rValue);
        }

        linearDepth *= -10.0f;

        // This is back-asswards but we're converting out of linear so
        // all the other images stored directly match in brightness
        // for comparision
        uint32_t r = (uint32_t)(255.0f - linearDepth);

        r = std::min(r, 255u);
        r = std::max(r, 0u);

        uint32_t bgra = 0xff << 24 | r << 16 | r << 8 | r;

        pixels[y * textureDesc.Width + x] = bgra;
      }
    }
  } else if (
      (textureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) &&
      (textureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)) {
    // Convert from RGBA to BGRA.
    for (uint32_t y = 0; y < textureDesc.Height; ++y) {
      for (uint32_t x = 0; x < textureDesc.Width; ++x) {
        const uint8_t* rgba = (uint8_t*)mapped.pData + (y * mapped.RowPitch) + (x * 4);
        uint32_t bgra = (rgba[3] << 24) | (rgba[0] << 16) | (rgba[1] << 8) | rgba[2];

        pixels[(y * textureDesc.Width) + x] = bgra;
      }
    }
  } else {
    // DXGI_FORMAT_NV12, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UINT, possibly others.
    uint32_t inputPitchInPixels = mapped.RowPitch;

    for (uint32_t y = 0; y < textureDesc.Height; ++y) {
      for (uint32_t x = 0; x < textureDesc.Width; ++x) {
        uint8_t c = ((uint8_t*)mapped.pData)[(y * inputPitchInPixels) + x];
        uint32_t bgra = (c << 24) | (c << 16) | (c << 8) | c; // Write as an RGB-based gray.

        pixels[(y * textureDesc.Width) + x] = bgra;
      }
    }
  }

  deviceContext->Unmap(textureSource, 0); // Always succeeds.

  // Create & write the file
  ScopedFileHANDLE bmpFile(CreateFileW(
      path, FILE_GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));

  if (!bmpFile.IsValid()) {
    return Result::FILE_CREATION_FAILURE;
  }

  const int BytesPerPixel = 4;

  BITMAPFILEHEADER bfh{};
  bfh.bfType = 0x4d42;
  bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
      (textureDesc.Width * textureDesc.Height * BytesPerPixel);
  bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  BITMAPINFOHEADER bih{};
  bih.biSize = sizeof(bih);
  bih.biBitCount = 8 * (WORD)BytesPerPixel;
  bih.biPlanes = 1;
  bih.biWidth = textureDesc.Width;
  bih.biHeight = textureDesc.Height;
  bih.biSizeImage = textureDesc.Width * textureDesc.Height * BytesPerPixel;

  DWORD bytesWritten = 0;
  WriteFile(bmpFile.Get(), &bfh, sizeof(bfh), &bytesWritten, nullptr);
  WriteFile(bmpFile.Get(), &bih, sizeof(bih), &bytesWritten, nullptr);

  size_t offset = textureDesc.Width * (textureDesc.Height - 1);
  for (uint32_t y = 0; y < textureDesc.Height; ++y) {
    WriteFile(
        bmpFile.Get(),
        pixels.data() + offset,
        textureDesc.Width * BytesPerPixel,
        &bytesWritten,
        nullptr);
    offset -= textureDesc.Width;
  }

  return Result::SUCCESS;
}
}
} // namespace OVR::D3DUtil

#endif // OVR_OS_MS
