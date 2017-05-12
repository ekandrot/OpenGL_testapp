#include "Texture.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdarg.h>

#include "jpeglib.h"
#include <png.h>


#ifdef _WIN32
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
    IWICBitmapFlipRotator* flipper;

    if (gFactory == NULL) {
        CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&gFactory));
    }
    gFactory->CreateDecoderFromFilename(file, NULL, GENERIC_READ | GENERIC_WRITE, WICDecodeMetadataCacheOnDemand, &decoder);	
    decoder->GetFrame(0, &frame);

    gFactory->CreateFormatConverter(&converter);
    converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);

    gFactory->CreateBitmapFlipRotator(&flipper);
    flipper->Initialize(converter, WICBitmapTransformFlipVertical);

    gFactory->CreateBitmapFromSource(flipper, WICBitmapNoCache, bitmap);

    WICPixelFormatGUID  fmt;
    converter->GetPixelFormat(&fmt);

    //SafeRelease(&factory);
    SafeRelease(&flipper);
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
    } else if (fmt == GUID_WICPixelFormat32bppBGRA) {
        textureFormat = GL_BGRA;
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
        GL_RGBA8,
        r.Width, r.Height,
        textureFormat,
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

#else


void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}


int x, y;

int width, height, stride;
uint8_t *image;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void read_png_file(const char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        size_t elementsRead = fread(header, 1, 8, fp);
        if (elementsRead != 8) {
            abort_("[read_png_file] File %s only read %ld bytes", file_name, elementsRead);
        }
        if (png_sig_cmp((const unsigned char*)header, 0, 8))
            abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        stride = png_get_rowbytes(png_ptr,info_ptr);
        image = (uint8_t*)malloc(stride * height);

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++) {
            row_pointers[y] = image + y * stride;
        }
         //       row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        free(row_pointers);

        fclose(fp);
}


////////////////////////////////////////////////////////////////////////////

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


int read_JPEG_file (const char * filename) {
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.txt for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
    height = cinfo.output_height;
    width = cinfo.output_width;
    stride = row_stride;
    image = (uint8_t*)malloc(height * stride);
    while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
        * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        memcpy(image+stride*(cinfo.output_scanline-1), buffer[0], row_stride);
    }

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return 1;
}


////////////////////////////////////////////////////////////////////////////

static GLenum GetImageFromFile(__attribute__((unused)) const char *fileName, __attribute__((unused)) uint8_t **bitmap) {
    std::string t(fileName);
    if (t.substr(t.find_last_of(".")+1) == "png") {
        read_png_file(fileName);
        *bitmap = image;
        if (color_type == PNG_COLOR_TYPE_RGB)
            return GL_RGB;
    } else if (t.substr(t.find_last_of(".")+1) == "jpg") {
        read_JPEG_file(fileName);
        *bitmap = image;
         return GL_RGB;
    } else {
        std::cout << fileName << std::endl;
    }
    return GL_BGRA;
}

GLuint Create(__attribute__((unused)) const std::string& fileName) {
    const char *file = fileName.c_str();
    if (!glIsEnabled(GL_TEXTURE_2D)) {
        glEnable(GL_TEXTURE_2D);
    }

    uint8_t* bitmap = NULL;

    GLenum textureFormat = GetImageFromFile(file, &bitmap);

    if (!bitmap) {
        GLuint glid = 0;
        glGenTextures(1, &glid);
        glBindTexture(GL_TEXTURE_2D, glid);

        uint32_t buf[16*16];
        for (int x=0;x<16*16;) {
          buf[x++] = 0xFF000000;        //ARGB
          buf[x++] = 0xFFFF0000;
        }
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8 , 16, 16, 0, textureFormat, GL_UNSIGNED_BYTE, (void*)buf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        return glid;
    }

    GLuint glid = 0;
    glGenTextures(1, &glid);
    glBindTexture(GL_TEXTURE_2D, glid);

#if 0
    gluBuild2DMipmaps(GL_TEXTURE_2D,
        GL_RGBA8,
        width, height,
        textureFormat,
        GL_UNSIGNED_BYTE,
        image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, r.Width, r.Height, 0, textureFormat, GL_UNSIGNED_BYTE, data);
#else
    printf("w: %d, h:%d, d:%d, c:%d\n", width, height, bit_depth, color_type);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8 , width, height, 0, textureFormat, GL_UNSIGNED_BYTE, (void*)bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    free(bitmap);
    image = NULL;
#endif
    //printf("Loading texture:  %s\n", ((GL_NO_ERROR == glGetError())?"success":"failed"));

    return glid;
}

////////////////////////////////////////////////////////////////////////////

Texture::Texture(const std::string& fileName) {
    texID = Create(fileName);
}

#endif

Texture::~Texture(void) {
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
