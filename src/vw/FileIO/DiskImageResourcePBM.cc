// __BEGIN_LICENSE__
// Copyright (C) 2006, 2007 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


/// \file DiskImageResourcePBM.cc
///
/// Provides support for the PBM file format.
///

#ifdef _MSC_VER
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#endif

#include <vector>
#include <fstream>
#include <stdio.h>

#include <boost/algorithm/string.hpp>
using namespace boost;

#include <vw/Core/Exception.h>
#include <vw/Core/Debugging.h>
#include <vw/FileIO/DiskImageResourcePBM.h>

using namespace vw;

// Used to skip comment lines found in file
void skip_any_comments( FILE * stream ) {
  char temp;
  temp = fgetc(stream);
  while ( temp == '#' ) {
    do {
      temp = fgetc(stream);
    } while ( temp != '\n' );
    temp = fgetc(stream);
  }
  fseek(stream,-1,SEEK_CUR);
}
// Used to normalize an array of uint8s
void normalize( uint8* data, uint32 count, uint8 max_value ) {
  uint8* pointer = data;
  for ( uint32 i = 0; i < count; i++ ) {
    if ( *pointer > max_value )
      *pointer = 255;
    else {
      uint8 temp = *pointer;
      *pointer = uint8(float(temp)*255/float(max_value));
    }
    pointer++;
  }
}

// Constructors
DiskImageResourcePBM::DiskImageResourcePBM( std::string const& filename ) : DiskImageResource( filename ) {
  open( filename );
}

DiskImageResourcePBM::DiskImageResourcePBM( std::string const& filename, ImageFormat const& format ) : DiskImageResource( filename ) {
  create( filename, format );
}

// Bind the resource to a file for reading. Confirm that we can open
// the file and that it has a sane pixel format.
void DiskImageResourcePBM::open( std::string const& filename ) { 

  FILE* input_file = fopen(filename.c_str(), "r");
  if (!input_file) vw_throw( vw::IOErr() << "Failed to open \"" << filename << "\"." );

  char c_line[2048];

  // Reading version info
  skip_any_comments(input_file);
  fscanf(input_file,"%s",c_line);
  m_magic = std::string(c_line);
  if ( !(m_magic == "P6" || m_magic == "P5" || m_magic == "P4" ||
	 m_magic == "P3" || m_magic == "P2" || m_magic == "P1" ) )
    vw_throw( IOErr() << "DiskImageResourcePBM: unsupported / or incorrect magic number identifer \"" << m_magic << "\"." );

  // Getting image width, height, and max gray value.
  int32 iwidth, iheight;
  skip_any_comments(input_file);
  fscanf(input_file,"%d",&iwidth);
  skip_any_comments(input_file);
  fscanf(input_file,"%d",&iheight);
  if ( m_magic != "P1" && m_magic != "P4" ) {
    skip_any_comments(input_file);
    fscanf(input_file,"%d",&m_max_value);
  } else
    m_max_value = 1; // Binary image
  fgetpos(input_file, &m_image_data_position);
  fclose(input_file);

  // Checking bit sanity
  if (imax <= 0 || imax >= 255 )
    vw_throw( IOErr() << "DiskImageResourcePBM: invalid bit type, Netpbm support 8 bit channel types and lower." );

  m_format.cols = iwidth;
  m_format.rows = iheight;
  m_format.planes = 1;

  switch( m_magic ) {
  case "P1":
  case "P4":
    // Boolean File Type
    m_format.channel_type = VW_CHANNEL_BOOL;
    m_format.pixel_format = VW_PIXEL_GRAY;
    break;
  case "P2":
  case "P5":
    // Grayscale image
    m_format.channel_type = VW_CHANNEL_UINT8;
    m_format.pixel_format = VW_PIXEL_GRAY;
    break;
  case "P3":
  case "P6":
    // RGB image
    m_format.channel_type = VW_CHANNEL_UINT8;
    m_format.pixel_format = VW_PIXEL_RGB;
  default:
    vw_throw( IOErr() << "DiskImageResourcePBM: how'd you get here? Invalid magic number." );
  }

}

// Read the disk image into the given buffer.
void DiskImageResourcePBM::read( ImageBuffer const& dest, BBox2i const& bbox )  const {

  VW_ASSERT( bbox.width()==int(cols()) && bbox.height()==int(rows()),
	     NoImplErr() << "DiskImageResourcePBM does not support partial reads." );
  VW_ASSERT( dest.format.cols==cols() && dest.format.rows==rows(),
	     IOErr() << "Buffer has wrong dimensions in PBM read." );
    
  FILE* input_file = fopen(m_filename.c_str(), "r");
  if (!input_file) vw_throw( IOErr() << "Failed to open \"" << m_filename << "\"." );
  fsetpos(input_file,&m_image_data_position);

  // Reading image data
  ImageBuffer src;
  int32 num_pixels = m_format.cols*m_format.rows;
  src.format = m_format;
  src.cstride = 1;
  src.rstride = m_format.cols;
  src.pstride = num_pixels;

  switch ( m_magic ) {
  case "P1": // Bool ASCII
    bool* image_data = new bool[num_pixels];
    bool* point = image_data;
    char buffer;
    for ( int32 i = 0; i < num_pixels; i++ ) {
      fscanf( input_file, "%c", &buffer );
      if ( buffer == '1' )
	*point = true;
      else
	*point = false;
      point++;
    }
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  case "P2": // Gray uint8 ASCII
    uint8* image_data = new uint8[num_pixels];
    uint8* point = image_data;
    for ( int32 i = 0; i < num_pixels; i++ ) {
      fscanf( input_file, "%hu", point );
      point++;
    }
    normalize( image_data, num_pixels, m_max_value );
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  case "P3": // RGB uint8 ASCII
    uint8* image_data = new uint8[num_pixels*3];
    uint8* point = image_data;
    for ( int32 i = 0; i < num_pixels; i++ ) {
      fscanf( input_file, "%hu", point );
      point++;
      fscanf( input_file, "%hu", point );
      point++;
      fscanf( input_file, "%hu", point );
      point++;
    }
    normalize( image_data, num_pixels*3, m_max_value );
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  case "P4": // Bool Binary
    bool* image_data = new bool[num_pixels];
    fread( image_data, sizeof(bool), num_pixels, input_file );
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  case "P5": // Gray uint8 Binary
    uint8* image_data = new uint8[num_pixels];
    fread( image_data, sizeof(uint8), num_pixels, input_file );
    normalize( image_data, num_pixels, m_max_value );
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  case "P6": // RGB uint8 Binary
    uint8* image_data = new uint8[num_pixels*3];
    fread( image_data, sizeof(uint8), num_pixels*3, input_file );
    normalize( image_data, num_pixels*3, m_max_value );
    src.data = image_data;
    convert( dest, src, m_rescale );
    delete[] image_data;
    break;
  default:
    vw_throw( NoImplErr() << "Unknown input channel type." );
  }

  fclose(input_file);
}

// Bind the resource to a file for writing.
void DiskImageResourcePBM::create( std::string const& filename,
				   ImageFormat const& format ) {

  if ( format.planes != 1 )
    vw_throw( NoImplErr() << "DiskImageResourcePBM doesn't support multi-plane images.");

  m_filename = filename;
  m_format = format;

  FILE* output_file = fopen(filename.c_str(), "w");
  fprintf( output_file, "P5\n" );
  fprintf( output_file, "%d\n", m_format.cols );
  fprintf( output_file, "%d\n", m_format.rows );

  // Converting to a PPM

  switch ( m_format.channel_type ) {
  case VW_CHANNEL_BOOL:
    fprintf( output_file, "1\n" );
    break;
  case VW_CHANNEL_UINT8:
    fprintf( output_file, "255\n" );
    break;
  case VW_CHANNEL_UINT16:
    fprintf( output_file, "65535\n" );
    break;
  default:
    vw_throw( NoImplErr() << "Incorrect channel type. PBM supports only BOOL, UINT8, UINT16. Got: " << );
    break;
  }

  fgetpos( output_file, &m_image_data_position );
  fclose( output_file );
}

// Write the given buffer into the disk image.
void DiskImageResourcePBM::write( ImageBuffer const& src,
				  BBox2i const& bbox ) {
  VW_ASSERT( bbox.width()==int(cols()) && bbox.height()==int(rows()),
	     NoImplErr() << "DiskImageResourcePBM does not support partial writes." );
  VW_ASSERT( src.format.cols==cols() && src.format.rows==rows(),
	     IOErr() << "Buffer has wrong dimensions in PBM write." );

  FILE* output_file = fopen(m_filename.c_str(), "a");
  fsetpos(output_file,&m_image_data_position);
  
  ImageBuffer dst;
  int32 num_pixels = m_format.cols*m_format.rows;
  dst.format = m_format;
  dst.cstride = 1;
  dst.rstride = cols();
  dst.pstride = num_pixels;

  if ( m_format.channel_type == VW_CHANNEL_UINT8 ) {
    uint8* image_data = new uint8[num_pixels];
    dst.data = image_data;
    convert( dst, src, m_rescale );
    fwrite( image_data, sizeof(uint8), num_pixels, output_file );
    delete[] image_data;
  } else if ( m_format.channel_type == VW_CHANNEL_UINT16 ) {
    uint16* image_data = new uint16[num_pixels];
    dst.data = image_data;
    convert( dst, src, m_rescale );
    fwrite( image_data, sizeof(uint16), num_pixels, output_file );
    delete[] image_data;
  } else if ( m_format.channel_type == VW_CHANNEL_BOOL ) {
    bool* image_data = new bool[num_pixels];
    dst.data = image_data;
    convert( dst, src, m_rescale );
    fwrite( image_data, sizeof(bool), num_pixels, output_file );
    delete[] image_data;
  } else {
    vw_throw( NoImplErr() << "Unknown input channel type." );
  }

  fclose( output_file );
}

// A FileIO hook to open a file for reading
DiskImageResource* DiskImageResourcePBM::construct_open( std::string const& filename ) {
  return new DiskImageResourcePBM( filename );
}

// A FileIO hook to open a file for writing
DiskImageResource* DiskImageResourcePBM::construct_create( std::string const& filename,
							   ImageFormat const& format ) {
  return new DiskImageResourcePBM( filename, format );
}