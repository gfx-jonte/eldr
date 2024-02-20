/**
 * Bitmap implementation adapted from the Mitsuba project
 */
#include <eldr/core/bitmap.hpp>
#include <eldr/core/fstream.hpp>
#include <eldr/core/logger.hpp>

#include <jerror.h>
#include <jpeglib.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace eldr {

Bitmap::Bitmap(PixelFormat px_format, Struct::Type component_format,
               const Vec2u& size, size_t channel_count,
               const std::vector<std::string>& channel_names, uint8_t* data)
  : pixel_format_(px_format), component_format_(component_format), size_(size),
    data_(data)

{
  if (component_format_ == Struct::Type::UInt8)
    srgb_gamma_ = true;
  else
    srgb_gamma_ = false;

  if (!data_) {
    data_ = std::make_unique<uint8_t[]>(bufferSize());

    owns_data_ = true;
  }
}

Bitmap::Bitmap(const Bitmap& bitmap)
  : pixel_format_(bitmap.pixel_format_),
    component_format_(bitmap.component_format_), size_(bitmap.size_),
    struct_(std::make_unique<Struct>(*bitmap.struct_.get())),
    srgb_gamma_(bitmap.srgb_gamma_),
    premultiplied_alpha_(bitmap.premultiplied_alpha_), owns_data_(true)
{
  size_t size = bufferSize();
  data_       = std::make_unique<uint8_t[]>(size);
  memcpy(data_.get(), bitmap.data_.get(), size);
}

Bitmap::Bitmap(Bitmap&& bitmap)
  : pixel_format_(bitmap.pixel_format_),
    component_format_(bitmap.component_format_), size_(bitmap.size_),
    struct_(std::move(bitmap.struct_)), srgb_gamma_(bitmap.srgb_gamma_),
    premultiplied_alpha_(bitmap.premultiplied_alpha_),
    data_(std::move(bitmap.data_)), owns_data_(bitmap.owns_data_)
{
}

Bitmap::Bitmap(Stream* stream, FileFormat format) { read(stream, format); }

Bitmap::Bitmap(const std::filesystem::path& path, FileFormat format)
{
  auto fs = std::make_unique<FileStream>(path);
  read(fs.get(), format);
}

Bitmap::~Bitmap()
{
  if (!owns_data_)
    data_.release();
}

void Bitmap::rebuildStruct(size_t                          channel_count,
                           const std::vector<std::string>& channel_names)
{
  std::vector<std::string> channels;

  switch (pixel_format_) {
    case PixelFormat::Y:
      channels = { "Y" };
      break;
    // case PixelFormat::YA:    channels = { "Y", "A" };               break;
    case PixelFormat::RGB:
      channels = { "R", "G", "B" };
      break;
    case PixelFormat::RGBA:
      channels = { "R", "G", "B", "A" };
      break;
      // case PixelFormat::RGBW:  channels = { "R", "G", "B", "W"};      break;
      // case PixelFormat::RGBAW: channels = { "R", "G", "B", "A", "W"}; break;
      // case PixelFormat::XYZ:   channels = { "X", "Y", "Z"};           break;
      // case PixelFormat::XYZA:  channels = { "X", "Y", "Z", "A"};      break;
      // case PixelFormat::MultiChannel:
      if (channel_names.size() == 0) {
        for (size_t i = 0; i < channel_count; ++i)
          channels.push_back(fmt::format("ch%i", i));
      }
      else {
        std::vector<std::string> channels_sorted = channel_names;
        std::sort(channels_sorted.begin(), channels_sorted.end());
        for (size_t i = 1; i < channels_sorted.size(); ++i) {
          if (channels_sorted[i] == channels_sorted[i - 1])
            Throw("Bitmap::rebuildStruct(): duplicate channel name \"%s\"",
                  channels_sorted[i]);
        }
        for (size_t i = 0; i < channel_count; ++i)
          channels.push_back(channel_names[i]);
      }
      break;
    default:
      Throw("Unknown pixel format!");
  }

  if (channel_count != 0 && channel_count != channels.size())
    Throw("Bitmap::rebuild_struct(): channel count (%i) does not match pixel "
          "format (%s)!",
          channel_count, (uint32_t) pixel_format_);

  struct_ = std::make_unique<Struct>();
  for (auto ch : channels) {
    bool     is_alpha = ch == "A" && pixel_format_ != PixelFormat::MultiChannel;
    uint32_t flags    = (uint32_t) Struct::Flags::Empty;
    if (!is_alpha && ch != "W" && srgb_gamma_)
      flags |= (uint32_t) Struct::Flags::Gamma;
    if (!is_alpha && ch != "W" && premultiplied_alpha_)
      flags |= +(uint32_t) Struct::Flags::PremultipliedAlpha;
    if (is_alpha)
      flags |= +(uint32_t) Struct::Flags::Alpha;
    if (ch == "W")
      flags |= (uint32_t) Struct::Flags::Weight;
    if (Struct::isInteger(component_format_))
      flags |= (uint32_t) Struct::Flags::Normalized;
    struct_->append(ch, component_format_, flags);
  }
}

size_t Bitmap::bufferSize() const { return pixelCount() * bytesPerPixel(); }

size_t Bitmap::bytesPerPixel() const
{
  size_t result;
  switch (component_format_) {
    case Struct::Type::Int8:
    case Struct::Type::UInt8:
      result = 1;
      break;
    case Struct::Type::Int16:
    case Struct::Type::UInt16:
      result = 2;
      break;
    case Struct::Type::Int32:
    case Struct::Type::UInt32:
      result = 4;
      break;
    case Struct::Type::Int64:
    case Struct::Type::UInt64:
      result = 8;
      break;
    case Struct::Type::Float16:
      result = 2;
      break;
    case Struct::Type::Float32:
      result = 4;
      break;
    case Struct::Type::Float64:
      result = 8;
      break;
    default:
      throw std::runtime_error("Unknown component format: " +
                               std::to_string((uint32_t) component_format_));
  }
  return result * channelCount();
}

void Bitmap::read(Stream* stream, FileFormat format)
{
  if (format == FileFormat::Auto)
    format = detectFileFormat(stream);

  switch (format) {
    // case FileFormat::BMP:
    //   read_bmp(stream);
    //   break;
    case FileFormat::JPEG:
      readJPEG(stream);
      break;
    // case FileFormat::OpenEXR:
    //   read_exr(stream);
    //   break;
    // case FileFormat::RGBE:
    //   read_rgbe(stream);
    //   break;
    // case FileFormat::PFM:
    //   read_pfm(stream);
    //   break;
    // case FileFormat::PPM:
    //   read_ppm(stream);
    //   break;
    // case FileFormat::TGA:
    //   read_tga(stream);
    //   break;
    // case FileFormat::PNG:
    //   read_png(stream);
    //   break;
    default:
      Throw("Bitmap: Unknown file format!");
  }
}

Bitmap::FileFormat Bitmap::detectFileFormat(Stream* stream)
{
  FileFormat format = FileFormat::Unknown;

  // Try to automatically detect the file format
  size_t  pos = stream->tell();
  uint8_t start[8];
  stream->read(start, 8);

  //  if (start[0] == 'B' && start[1] == 'M') {
  //    format = FileFormat::BMP;
  //  }
  //  else if (start[0] == '#' && start[1] == '?') {
  //    format = FileFormat::RGBE;
  //  }
  //  else if (start[0] == 'P' && (start[1] == 'F' || start[1] == 'f')) {
  //    format = FileFormat::PFM;
  //  }
  //  else if (start[0] == 'P' && start[1] == '6') {
  //    format = FileFormat::PPM;
  //  }
  if (start[0] == 0xFF && start[1] == 0xD8) {
    format = FileFormat::JPEG;
  }
  //  else if (png_sig_cmp(start, 0, 8) == 0) {
  //    format = FileFormat::PNG;
  //  }
  //  else if (Imf::isImfMagic((const char*) start)) {
  //    format = FileFormat::OpenEXR;
  //  }
  //  else {
  //    // Check for a TGAv2 file
  //    char footer[18];
  //    stream->seek(stream->size() - 18);
  //    stream->read(footer, 18);
  //    if (footer[17] == 0 && strncmp(footer, "TRUEVISION-XFILE.", 17) == 0)
  //      format = FileFormat::TGA;
  //  }
  stream->seek(pos);
  return format;
}

extern "C" {
static const size_t jpeg_buffer_size = 0x8000;

typedef struct {
  struct jpeg_source_mgr mgr;
  JOCTET*                buffer;
  Stream*                stream;
} jbuf_in_t;

typedef struct {
  struct jpeg_destination_mgr mgr;
  JOCTET*                     buffer;
  Stream*                     stream;
} jbuf_out_t;

METHODDEF(void) jpeg_init_source(j_decompress_ptr cinfo)
{
  jbuf_in_t* p = (jbuf_in_t*) cinfo->src;
  p->buffer    = new JOCTET[jpeg_buffer_size];
}

METHODDEF(boolean) jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
  jbuf_in_t* p          = (jbuf_in_t*) cinfo->src;
  size_t     bytes_read = 0;

  try {
    p->stream->read(p->buffer, jpeg_buffer_size);
    bytes_read = jpeg_buffer_size;
  }
  catch (const EOFException& e) {
    bytes_read = e.gcount();
    if (bytes_read == 0) {
      // Insert a fake EOI marker
      p->buffer[0] = (JOCTET) 0xFF;
      p->buffer[1] = (JOCTET) JPEG_EOI;
      bytes_read   = 2;
    }
  }

  cinfo->src->bytes_in_buffer = bytes_read;
  cinfo->src->next_input_byte = p->buffer;
  return TRUE;
}

METHODDEF(void) jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes > 0) {
    while (num_bytes > (long) cinfo->src->bytes_in_buffer) {
      num_bytes -= (long) cinfo->src->bytes_in_buffer;
      jpeg_fill_input_buffer(cinfo);
    }
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

METHODDEF(void) jpeg_term_source(j_decompress_ptr cinfo)
{
  jbuf_in_t* p = (jbuf_in_t*) cinfo->src;
  delete[] p->buffer;
}

METHODDEF(void) jpeg_init_destination(j_compress_ptr cinfo)
{
  jbuf_out_t* p = (jbuf_out_t*) cinfo->dest;

  p->buffer               = new JOCTET[jpeg_buffer_size];
  p->mgr.next_output_byte = p->buffer;
  p->mgr.free_in_buffer   = jpeg_buffer_size;
}

METHODDEF(boolean) jpeg_empty_output_buffer(j_compress_ptr cinfo)
{
  jbuf_out_t* p = (jbuf_out_t*) cinfo->dest;
  p->stream->write(p->buffer, jpeg_buffer_size);
  p->mgr.next_output_byte = p->buffer;
  p->mgr.free_in_buffer   = jpeg_buffer_size;
  return TRUE;
}

METHODDEF(void) jpeg_term_destination(j_compress_ptr cinfo)
{
  jbuf_out_t* p = (jbuf_out_t*) cinfo->dest;
  p->stream->write(p->buffer, jpeg_buffer_size - p->mgr.free_in_buffer);
  delete[] p->buffer;
  p->mgr.free_in_buffer = 0;
}

METHODDEF(void) jpeg_error_exit(j_common_ptr cinfo)
{
  char msg[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message)(cinfo, msg);
  Throw("Critical libjpeg error: %s", msg);
}
};

void Bitmap::readJPEG(Stream* stream)
{
  // ScopedPhase                   phase(ProfilerPhase::BitmapRead);
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;
  jbuf_in_t                     jbuf;

  memset(&jbuf, 0, sizeof(jbuf_in_t));

  cinfo.err       = jpeg_std_error(&jerr);
  jerr.error_exit = jpeg_error_exit;
  jpeg_create_decompress(&cinfo);
  cinfo.src                  = (struct jpeg_source_mgr*) &jbuf;
  jbuf.mgr.init_source       = jpeg_init_source;
  jbuf.mgr.fill_input_buffer = jpeg_fill_input_buffer;
  jbuf.mgr.skip_input_data   = jpeg_skip_input_data;
  jbuf.mgr.term_source       = jpeg_term_source;
  jbuf.mgr.resync_to_restart = jpeg_resync_to_restart;
  jbuf.stream                = stream;

  jpeg_read_header(&cinfo, TRUE);
  cinfo.out_color_space = JCS_EXT_RGBA; // Force additional opaque alpha channel
  jpeg_start_decompress(&cinfo);

  size_                = Vec2u(cinfo.output_width, cinfo.output_height);
  component_format_    = Struct::Type::UInt8;
  srgb_gamma_          = true;
  premultiplied_alpha_ = false;

  if (cinfo.output_components == 4)
    // Add component to JPEGs so that the data can be given to Vulkan
    pixel_format_ = PixelFormat::RGBA;
  else
    Throw("readJPEG(): Unexpected number of components!");

  rebuildStruct();

  auto fs = dynamic_cast<FileStream*>(stream);
  spdlog::debug("Loading JPEG file \"{}\" ({}x{}, {}, {}) ..",
                fs ? fs->path().string() : "<stream>", size_.x, size_.y,
                toString(pixel_format_), toString(component_format_));

  size_t row_stride =
    (size_t) cinfo.output_width * (size_t) cinfo.output_components;

  data_      = std::make_unique<uint8_t[]>(bufferSize());
  owns_data_ = true;

  auto scanlines = std::make_unique<uint8_t*[]>(size_.y);

  for (size_t i = 0; i < size_.y; ++i)
    scanlines[i] = uint8Data() + row_stride * i;

  // Process scanline by scanline
  int counter = 0;
  while (cinfo.output_scanline < cinfo.output_height)
    counter +=
      jpeg_read_scanlines(&cinfo, scanlines.get() + counter,
                          (JDIMENSION) (size_.y - cinfo.output_scanline));

  // Release the libjpeg data structures
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}
/*
void Bitmap::write_jpeg(Stream* stream, int quality) const
{
  ScopedPhase                 phase(ProfilerPhase::BitmapWrite);
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  jbuf_out_t                  jbuf;

  int components = 0;
  if (m_pixel_format == PixelFormat::Y)
    components = 1;
  else if (m_pixel_format == PixelFormat::RGB ||
           m_pixel_format == PixelFormat::XYZ)
    components = 3;
  else
    Throw("write_jpeg(): Unsupported pixel format!");

  if (m_component_format != Struct::Type::UInt8)
    Throw("write_jpeg(): Unsupported component format %s, expected %s",
          m_component_format, Struct::Type::UInt8);

  memset(&jbuf, 0, sizeof(jbuf_out_t));
  cinfo.err       = jpeg_std_error(&jerr);
  jerr.error_exit = jpeg_error_exit;
  jpeg_create_compress(&cinfo);

  cinfo.dest                   = (struct jpeg_destination_mgr*) &jbuf;
  jbuf.mgr.init_destination    = jpeg_init_destination;
  jbuf.mgr.empty_output_buffer = jpeg_empty_output_buffer;
  jbuf.mgr.term_destination    = jpeg_term_destination;
  jbuf.stream                  = stream;

  cinfo.image_width      = (JDIMENSION) m_size.x();
  cinfo.image_height     = (JDIMENSION) m_size.y();
  cinfo.input_components = components;
  cinfo.in_color_space   = components == 1 ? JCS_GRAYSCALE : JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  if (quality == 100) {
    // Disable chroma subsampling
    cinfo.comp_info[0].v_samp_factor = 1;
    cinfo.comp_info[0].h_samp_factor = 1;
  }

  jpeg_start_compress(&cinfo, TRUE);

  // Write scanline by scanline
  for (size_t i = 0; i < m_size.y(); ++i) {
    uint8_t* source = m_data.get() + i * m_size.x() * cinfo.input_components;
    jpeg_write_scanlines(&cinfo, &source, 1);
  }

  // Release the libjpeg data structures
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
}
*/
}; // namespace eldr
