/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2023 Tencent. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "core/codecs/jpeg/JpegCodec.h"
#include <csetjmp>
#include "core/utils/OrientationHelper.h"
#include "skcms.h"
#include "tgfx/core/Buffer.h"
#include "tgfx/core/Data.h"
#include "tgfx/core/Pixmap.h"

extern "C" {
#include "jerror.h"
#include "jpeglib.h"
}

namespace tgfx {

bool JpegCodec::IsJpeg(const std::shared_ptr<Data>& data) {
  constexpr uint8_t jpegSig[] = {0xFF, 0xD8, 0xFF};
  return data->size() >= 3 && !memcmp(data->bytes(), jpegSig, sizeof(jpegSig));
}

const uint32_t kExifHeaderSize = 14;
const uint32_t kExifMarker = JPEG_APP0 + 1;

static bool is_orientation_marker(jpeg_marker_struct* marker, Orientation* orientation) {
  if (kExifMarker != marker->marker || marker->data_length < kExifHeaderSize) {
    return false;
  }

  constexpr uint8_t kExifSig[]{'E', 'x', 'i', 'f', '\0'};
  if (memcmp(marker->data, kExifSig, sizeof(kExifSig)) != 0) {
    return false;
  }

  // Account for 'E', 'x', 'i', 'f', '\0', '<fill byte>'.
  constexpr size_t kOffset = 6;
  return is_orientation_marker(marker->data + kOffset, marker->data_length - kOffset, orientation);
}

static Orientation get_exif_orientation(jpeg_decompress_struct* dinfo) {
  Orientation orientation;
  for (jpeg_marker_struct* marker = dinfo->marker_list; marker; marker = marker->next) {
    if (is_orientation_marker(marker, &orientation)) {
      return orientation;
    }
  }
  return Orientation::TopLeft;
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

std::shared_ptr<ImageCodec> JpegCodec::MakeFrom(const std::string& filePath) {
  return MakeFromData(filePath, nullptr);
}

std::shared_ptr<ImageCodec> JpegCodec::MakeFrom(std::shared_ptr<Data> imageBytes) {
  return MakeFromData("", std::move(imageBytes));
}

std::shared_ptr<ImageCodec> JpegCodec::MakeFromData(const std::string& filePath,
                                                    std::shared_ptr<Data> byteData) {
  FILE* infile = nullptr;
  if (byteData == nullptr && (infile = fopen(filePath.c_str(), "rb")) == nullptr) {
    return nullptr;
  }
  jpeg_decompress_struct cinfo = {};
  my_error_mgr jerr = {};
  cinfo.err = jpeg_std_error(&jerr.pub);
  Orientation orientation = Orientation::TopLeft;
  do {
    if (setjmp(jerr.setjmp_buffer)) break;
    jpeg_create_decompress(&cinfo);
    if (infile) {
      jpeg_stdio_src(&cinfo, infile);
    } else {
      jpeg_mem_src(&cinfo, byteData->bytes(), byteData->size());
    }
    jpeg_save_markers(&cinfo, kExifMarker, 0xFFFF);
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) break;
    orientation = get_exif_orientation(&cinfo);
  } while (false);
  jpeg_destroy_decompress(&cinfo);
  if (infile) fclose(infile);
  if (cinfo.image_width == 0 || cinfo.image_height == 0) {
    return nullptr;
  }
  return std::shared_ptr<ImageCodec>(new JpegCodec(static_cast<int>(cinfo.image_width),
                                                   static_cast<int>(cinfo.image_height),
                                                   orientation, filePath, std::move(byteData)));
}

bool ExtractICCProfile(jpeg_decompress_struct* cinfo, std::vector<uint8_t>& iccProfileData) {
  jpeg_saved_marker_ptr marker = cinfo->marker_list;
  while (marker) {
    if (marker->marker == JPEG_APP0 + 2 && marker->data_length > 14 &&
        std::memcmp(marker->data, "ICC_PROFILE", 12) == 0) {
      iccProfileData.insert(iccProfileData.end(), marker->data + 14,
                            marker->data + marker->data_length);
    }
    marker = marker->next;
  }
  return !iccProfileData.empty();
}

bool ParseICCProfile(const std::vector<uint8_t>& iccProfileData, gfx::skcms_ICCProfile* profile) {
  return skcms_Parse(iccProfileData.data(), iccProfileData.size(), profile);
}

bool ConvertCMYKPixels(void* dst, const gfx::skcms_ICCProfile cmykProfile,
                       const ImageInfo& dstInfo) {
  gfx::skcms_PixelFormat dstPixelFormat;
  switch (dstInfo.colorType()) {
    case ColorType::BGRA_8888:
      dstPixelFormat = gfx::skcms_PixelFormat_BGRA_8888;
      break;
    case ColorType::RGBA_8888:
      dstPixelFormat = gfx::skcms_PixelFormat_RGBA_8888;
      break;
    default:
      return false;
  }
  const gfx::skcms_ICCProfile* dstProfile = gfx::skcms_sRGB_profile();
  auto width = dstInfo.width();
  auto height = dstInfo.height();
  auto src = dst;
  for (int i = 0; i < height; i++) {
    bool status = gfx::skcms_Transform(
        src, gfx::skcms_PixelFormat_RGBA_8888, gfx::skcms_AlphaFormat_Unpremul, &cmykProfile, dst,
        dstPixelFormat, gfx::skcms_AlphaFormat_Unpremul, dstProfile, static_cast<size_t>(width));
    if (!status) {
      return false;
    }
    dst = reinterpret_cast<uint8_t*>(dst) + dstInfo.rowBytes();
    src = reinterpret_cast<uint8_t*>(src) + dstInfo.rowBytes();
  }
  return true;
}

bool JpegCodec::readPixels(const ImageInfo& dstInfo, void* dstPixels) const {
  if (dstPixels == nullptr || dstInfo.isEmpty()) {
    return false;
  }
  if (dstInfo.colorType() == ColorType::ALPHA_8) {
    memset(dstPixels, 255, dstInfo.rowBytes() * static_cast<size_t>(height()));
    return true;
  }
  Bitmap bitmap = {};
  auto outPixels = dstPixels;
  auto outRowBytes = dstInfo.rowBytes();
  J_COLOR_SPACE out_color_space;
  switch (dstInfo.colorType()) {
    case ColorType::RGBA_8888:
      out_color_space = JCS_EXT_RGBA;
      break;
    case ColorType::BGRA_8888:
      out_color_space = JCS_EXT_BGRA;
      break;
    case ColorType::Gray_8:
      out_color_space = JCS_GRAYSCALE;
      break;
    case ColorType::RGB_565:
      out_color_space = JCS_RGB565;
      break;
    default:
      auto success = bitmap.allocPixels(dstInfo.width(), dstInfo.height(), false, false);
      if (!success) {
        return false;
      }
      out_color_space = JCS_EXT_RGBA;
      break;
  }
  Pixmap pixmap(bitmap);
  if (!pixmap.isEmpty()) {
    outPixels = pixmap.writablePixels();
    outRowBytes = pixmap.rowBytes();
  }
  FILE* infile = nullptr;
  if (fileData == nullptr && (infile = fopen(filePath.c_str(), "rb")) == nullptr) {
    return false;
  }
  jpeg_decompress_struct cinfo = {};
  my_error_mgr jerr = {};
  cinfo.err = jpeg_std_error(&jerr.pub);
  bool result = false;
  do {
    if (setjmp(jerr.setjmp_buffer)) break;
    jpeg_create_decompress(&cinfo);
    if (infile) {
      jpeg_stdio_src(&cinfo, infile);
    } else {
      jpeg_mem_src(&cinfo, fileData->bytes(), fileData->size());
    }
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xFFFF);
    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xFFFF);
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
      break;
    }
    cinfo.out_color_space = out_color_space;
    if (cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK) {
      cinfo.out_color_space = JCS_CMYK;
    }
    if (!jpeg_start_decompress(&cinfo)) {
      break;
    }
    JSAMPROW pRow[1];
    int line = 0;
    JDIMENSION h = static_cast<JDIMENSION>(height());
    while (cinfo.output_scanline < h) {
      pRow[0] = (JSAMPROW)(static_cast<unsigned char*>(outPixels) +
                           outRowBytes * static_cast<size_t>(line));
      jpeg_read_scanlines(&cinfo, pRow, 1);
      line++;
    }
    if (cinfo.out_color_space == JCS_CMYK) {
      std::vector<uint8_t> iccProfileData;
      if (ExtractICCProfile(&cinfo, iccProfileData)) {
        gfx::skcms_ICCProfile cmykProfile;
        if (ParseICCProfile(iccProfileData, &cmykProfile)) {
          if (!ConvertCMYKPixels(outPixels, cmykProfile, dstInfo)) {
            result = false;
            jpeg_finish_decompress(&cinfo);
            break;
          }
        }
      }
    }
    result = jpeg_finish_decompress(&cinfo);
  } while (false);
  jpeg_destroy_decompress(&cinfo);
  if (infile) {
    fclose(infile);
  }
  if (result && !pixmap.isEmpty()) {
    pixmap.readPixels(dstInfo, dstPixels);
  }
  return result;
}

std::shared_ptr<Data> JpegCodec::getEncodedData() const {
  if (fileData) {
    return fileData;
  }
  if (!filePath.empty()) {
    return Data::MakeFromFile(filePath);
  }
  return nullptr;
}

#ifdef TGFX_USE_JPEG_ENCODE
std::shared_ptr<Data> JpegCodec::Encode(const Pixmap& pixmap, int quality) {
  auto srcPixels = static_cast<uint8_t*>(const_cast<void*>(pixmap.pixels()));
  int srcRowBytes = static_cast<int>(pixmap.rowBytes());
  jpeg_compress_struct cinfo = {};
  jpeg_error_mgr jerr = {};
  JSAMPROW row_pointer[1];
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  uint8_t* dstBuffer = nullptr;
  unsigned long dstBufferSize = 0;  // NOLINT
  jpeg_mem_dest(&cinfo, &dstBuffer, &dstBufferSize);
  cinfo.image_width = static_cast<JDIMENSION>(pixmap.width());
  cinfo.image_height = static_cast<JDIMENSION>(pixmap.height());
  Buffer buffer = {};
  switch (pixmap.colorType()) {
    case ColorType::RGBA_8888:
      cinfo.in_color_space = JCS_EXT_RGBA;
      cinfo.input_components = 4;
      break;
    case ColorType::BGRA_8888:
      cinfo.in_color_space = JCS_EXT_BGRA;
      cinfo.input_components = 4;
      break;
    case ColorType::Gray_8:
      cinfo.in_color_space = JCS_GRAYSCALE;
      cinfo.input_components = 1;
      break;
    default:
      auto info = ImageInfo::Make(pixmap.width(), pixmap.height(), ColorType::RGBA_8888);
      buffer.alloc(info.byteSize());
      if (buffer.isEmpty()) {
        return nullptr;
      }
      srcPixels = buffer.bytes();
      srcRowBytes = static_cast<int>(info.rowBytes());
      Pixmap(info, srcPixels).writePixels(pixmap.info(), pixmap.pixels());
      cinfo.in_color_space = JCS_EXT_RGBA;
      cinfo.input_components = 4;
      break;
  }
  jpeg_set_defaults(&cinfo);
  cinfo.optimize_coding = TRUE;
  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &(srcPixels)[cinfo.next_scanline * static_cast<JDIMENSION>(srcRowBytes)];
    (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* similar to read file, clean up after we're done compressing */
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  return Data::MakeAdopted(dstBuffer, dstBufferSize, Data::FreeProc);
}
#endif

}  // namespace tgfx