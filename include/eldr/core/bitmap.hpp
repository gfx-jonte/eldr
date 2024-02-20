/**
 * Bitmap class adapted from Mitsuba3
 */
#pragma once

#include <eldr/core/fwd.hpp>
#include <eldr/core/stream.hpp>
#include <eldr/core/struct.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace eldr {

class Bitmap {
  ELDR_IMPORT_CORE_TYPES();

public:
  enum class PixelFormat {
    Y,
    // YA,
    RGB,
    RGBA,
    // RGBW,
    // RGBAW,
    // XYZ,
    // XYZA,
    MultiChannel
  };

  enum class FileFormat { JPEG, Auto, Unknown };

  Bitmap(PixelFormat px_format, Struct::Type component_format,
         const Vec2u& size, size_t channel_count,
         const std::vector<std::string>& channel_names = {},
         uint8_t*                        data          = nullptr);

  Bitmap(const std::filesystem::path& path, FileFormat = FileFormat::Auto);

  /// Copy constructor (copies the image contents)
  Bitmap(const Bitmap& bitmap);

  /// Move constructor
  Bitmap(Bitmap&& bitmap);

  Bitmap(Stream* stream, FileFormat format);

  virtual ~Bitmap();

  /// Return the pixel format of this bitmap
  PixelFormat pixelFormat() const { return pixel_format_; }

  /// Return the component format of this bitmap
  Struct::Type componentFormat() const { return component_format_; }

  /// Return a pointer to the underlying bitmap storage
  void* data() { return data_.get(); }

  /// Return a pointer to the underlying bitmap storage
  const void* data() const { return data_.get(); }

  /// Return a pointer to the underlying data
  uint8_t* uint8Data() { return data_.get(); }

  /// Return a pointer to the underlying data (const)
  const uint8_t* uint8Data() const { return data_.get(); }

  /// Return the bitmap dimensions in pixels
  const Vec2u& size() const { return size_; }

  /// Return the bitmap's width in pixels
  uint32_t width() const { return size_.x; }

  /// Return the bitmap's height in pixels
  uint32_t          height() const { return size_.y; }
  size_t            channelCount() const { return struct_->fieldCount(); }
  size_t            pixelCount() const { return (size_t) size_.x * size_.y; }
  size_t            bytesPerPixel() const;
  size_t            bufferSize() const;
  static FileFormat detectFileFormat(Stream* stream);

protected:
  void rebuildStruct(size_t                          channel_count = 0,
                     const std::vector<std::string>& channel_names = {});
  /// Read a file from a stream
  void read(Stream* stream, FileFormat format);

  /// Read a file encoded using the OpenEXR file format
  // void read_exr(Stream* stream);

  ///// Write a file using the OpenEXR file format
  // void write_exr(Stream* stream, int compression = -1) const;

  ///// Read a file encoded using the JPEG file format
  void readJPEG(Stream* stream);

  ///// Save a file using the JPEG file format
  // void write_jpeg(Stream* stream, int quality) const;

  ///// Read a file encoded using the PNG file format
  // void read_png(Stream* stream);

  ///// Save a file using the PNG file format
  // void write_png(Stream* stream, int quality) const;

  ///// Read a file encoded using the PPM file format
  // void read_ppm(Stream* stream);

  ///// Save a file using the PPM file format
  // void write_ppm(Stream* stream) const;

  ///// Read a file encoded using the BMP file format
  // void read_bmp(Stream* stream);

  ///// Read a file encoded using the TGA file format
  // void read_tga(Stream* stream);

  ///// Read a file encoded using the RGBE file format
  // void read_rgbe(Stream* stream);

  ///// Save a file using the RGBE file format
  // void write_rgbe(Stream* stream) const;

  ///// Read a file encoded using the PFM file format
  // void read_pfm(Stream* stream);

  ///// Save a file using the PFM file format
  // void write_pfm(Stream* stream) const;

private:
  PixelFormat                pixel_format_;
  Struct::Type               component_format_;
  Vec2u                      size_;
  std::unique_ptr<Struct>    struct_;
  bool                       srgb_gamma_;
  bool                       premultiplied_alpha_;
  std::unique_ptr<uint8_t[]> data_;
  bool                       owns_data_;
};

constexpr const char* toString(Bitmap::PixelFormat px_format)
{
  switch (px_format) {
    case Bitmap::PixelFormat::Y:
      return "Y";
      // YA,
    case Bitmap::PixelFormat::RGB:
      return "RGB";
    case Bitmap::PixelFormat::RGBA:
      return "RGBA";
    // RGBW,
    // RGBAW,
    // XYZ,
    // XYZA,
    case Bitmap::PixelFormat::MultiChannel:
      return "MultiChannel";
    default:
      throw std::invalid_argument("Pixel format not implemented!");
  }
}

} // namespace eldr
