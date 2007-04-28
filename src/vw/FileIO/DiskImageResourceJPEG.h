// __BEGIN_LICENSE__
// 
// Copyright (C) 2006 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration
// (NASA).  All Rights Reserved.
// 
// Copyright 2006 Carnegie Mellon University. All rights reserved.
// 
// This software is distributed under the NASA Open Source Agreement
// (NOSA), version 1.3.  The NOSA has been approved by the Open Source
// Initiative.  See the file COPYING at the top of the distribution
// directory tree for the complete NOSA document.
// 
// THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
// KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
// LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
// SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
// THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
// DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.
// 
// __END_LICENSE__

/// \file DiskImageResourceJPEG.h
/// 
/// Provides support for file formats via libJPEG.
///
#ifndef __VW_FILEIO_DISK_IMAGE_RESOUCE_JPEG_H__
#define __VW_FILEIO_DISK_IMAGE_RESOUCE_JPEG_H__

#include <string>

#include <vw/Image/PixelTypes.h>
#include <vw/FileIO/DiskImageResource.h>

namespace vw {

  class DiskImageResourceJPEG : public DiskImageResource {
  public:

    DiskImageResourceJPEG( std::string const& filename )
      : DiskImageResource( filename )
    {
      m_subsample_factor = default_subsampilng_factor;
      m_quality = default_quality;
      m_file_ptr = NULL;
      m_jpg_compress_header = NULL;
      m_jpg_decompress_header = NULL;
      open( filename );
    }
    
    DiskImageResourceJPEG( std::string const& filename, 
                           ImageFormat const& format )
      : DiskImageResource( filename )
    {
      m_subsample_factor = default_subsampilng_factor;
      m_quality = default_quality;
      m_file_ptr = NULL;
      m_jpg_compress_header = NULL;
      m_jpg_decompress_header = NULL;
      create( filename, format );
    }
    
    virtual ~DiskImageResourceJPEG();
    
    /// Returns the type of disk image resource.
    static std::string type_static() { return "JPEG"; }

    /// Returns the type of disk image resource.
    virtual std::string type() { return type_static(); }
    
    virtual void read( ImageBuffer const& dest, BBox2i const& bbox ) const;

    virtual void write( ImageBuffer const& dest, BBox2i const& bbox );

    virtual void flush();

    /// Set the compression quality of the jpeg image.  The quality is
    /// a value between 0.0 and 1.0.  The lower the quality, the more
    /// lossy the compression.
    void set_quality(float quality) { m_quality = quality; }

    /// Set the default compression quality of jpeg images.
    static void set_default_quality(float quality) { default_quality = quality; }

    /// Set the subsample factor.  The default is no scaling.  Valid
    /// values are 1, 2, 4, and 8.  Smaller scaling ratios permit
    /// significantly faster decoding since fewer pixels need to be
    /// processed and a simpler IDCT method can be used.
    void set_subsample_factor(int subsample_factor) { 
      // Cloes and reopen the file with the new subsampling factor
      flush();
      open(m_filename, subsample_factor);
    }

    void open( std::string const& filename );
    void open( std::string const& filename, int subsample_factor ) {
      if (subsample_factor == 1 || subsample_factor == 2 ||
          subsample_factor == 4 || subsample_factor == 8) {
        m_subsample_factor = subsample_factor; 
      } else {
        vw_throw( ArgumentErr() << "DiskImageResourceJPEG: subsample_factor must be 1, 2, 4, or 8" );
      }
      open(filename);
    }

    void create( std::string const& filename,
                 ImageFormat const& format );

    static DiskImageResource* construct_open( std::string const& filename );

    static DiskImageResource* construct_create( std::string const& filename,
                                                ImageFormat const& format );

  private:
    
    std::string m_filename;
    float m_quality;
    int m_subsample_factor;
    void* m_jpg_decompress_header;
    void* m_jpg_compress_header;
    void* m_file_ptr;

    static int default_subsampilng_factor;
    static float default_quality;

  };

} // namespace vw

#endif // __VW_FILEIO_DISK_IMAGE_RESOUCE_JPEG_H__
