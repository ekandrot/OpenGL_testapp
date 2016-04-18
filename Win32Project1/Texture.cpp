#include "Texture.h"
#include <stdio.h>
#include <Wincodec.h>



////////////////////////////////////////////////////////////////////////////

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////

static WICPixelFormatGUID GetImageFromFile(LPCWSTR file, IWICBitmap** bitmap) {
    static IWICImagingFactory* gFactory = NULL;
    IWICBitmapDecoder* decoder = NULL;
    IWICBitmapFrameDecode* frame = NULL;
    IWICFormatConverter* converter = NULL;

    if (gFactory == NULL) {
        CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&gFactory));
    }
    gFactory->CreateDecoderFromFilename(file, NULL, GENERIC_READ | GENERIC_WRITE, WICDecodeMetadataCacheOnDemand, &decoder);	
    decoder->GetFrame(0, &frame);
    gFactory->CreateFormatConverter(&converter);
    converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    gFactory->CreateBitmapFromSource(frame, WICBitmapNoCache, bitmap);

    WICPixelFormatGUID  fmt;
    frame->GetPixelFormat(&fmt);

    //SafeRelease(&factory);
    SafeRelease(&converter);
    SafeRelease(&frame);
    SafeRelease(&decoder);
    return fmt;
}

////////////////////////////////////////////////////////////////////////////

GLuint Create(const std::wstring& fileName) {
    LPCWSTR file = fileName.c_str();
    if (!glIsEnabled(GL_TEXTURE_2D))
    glEnable(GL_TEXTURE_2D);

    IWICBitmap* bitmap = NULL;
    IWICBitmapLock* lock = NULL;

    WICPixelFormatGUID fmt = GetImageFromFile(file, &bitmap);
    if (!bitmap) return FALSE;

    GLenum  textureFormat;
    if (fmt == GUID_WICPixelFormat24bppBGR) {
        textureFormat = GL_BGR;
    } else {
        textureFormat = GL_RED;
        printf("*** unknown texture format for %s ***", file);
    }

    WICRect r = {0};
    bitmap->GetSize((UINT*)&r.Width, (UINT*)&r.Height);

    BYTE* data = NULL;
    UINT len = 0;

    bitmap->Lock(&r, WICBitmapLockRead, &lock);
    lock->GetDataPointer(&len, &data);

    GLuint glid = 0;
    glGenTextures(1, &glid);
    glBindTexture(GL_TEXTURE_2D, glid);
    gluBuild2DMipmaps(GL_TEXTURE_2D,
                        GL_RGB,
                        r.Width, r.Height,
                        GL_RGB,
                        GL_UNSIGNED_BYTE,
                        data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

#if 0
    DWORD32 buf[16*16];
    DWORD32 x;
    for (x=0;x<16*16;) {
      buf[x++] = 0xFF000000;        //ARGB
      buf[x++] = 0xFFFF0000;
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8 , 16, 16, 0, GL_BGRA, GL_UNSIGNED_BYTE, (void*)buf);
    }
#endif

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, r.Width, r.Height, 0, textureFormat, GL_UNSIGNED_BYTE, data);

    lock->Release();
    bitmap->Release();

//    printf("Loading texture:  %s\n", ((GL_NO_ERROR == glGetError())?"success":"failed"));

    return glid;
}

////////////////////////////////////////////////////////////////////////////

Texture::Texture(const std::wstring& fileName) {
    texID = Create(fileName);
}

Texture::~Texture(void) {
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
