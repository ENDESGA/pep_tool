////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  pep_tool
//
//  author(s):
//  ENDESGA - https://twitter.com/ENDESGA | https://bsky.app/profile/endesga.bsky.social
//
//  https://github.com/ENDESGA/pep_tool
//  2026 - CC0 - FOSS forever
//

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - DEPENDENCIES
//

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#define PEP_IMPLEMENTATION
#define PEP_DEBUG
#include <pep.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_PSD
#ifdef __TINYC__
	#define STBI_NO_SIMD
#endif
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <H.h>

#pragma endregion dependencies

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - CONSTANTS
//

#define PEP_TOOL_NAME "pep_tool"

////////////////////////////////////////////////////////////////
#pragma region - version

#define PEP_TOOL_VERSION_MAJOR 0
#define PEP_TOOL_VERSION_MINOR 6
#define PEP_TOOL_VERSION_PATCH 0
#define PEP_TOOL_VERSION_COMMIT 0
#define PEP_TOOL_VERSION AS_BYTES( PEP_TOOL_VERSION_MAJOR ) "." AS_BYTES( PEP_TOOL_VERSION_MINOR ) "." AS_BYTES( PEP_TOOL_VERSION_PATCH ) "-" AS_BYTES( PEP_TOOL_VERSION_COMMIT )

#pragma endregion

#pragma endregion defaults

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - START
//

start
{
	if( start_inputs_count is 2 and ( bytes_match( start_inputs[ 1 ], "version" ) or bytes_match( start_inputs[ 1 ], "-v" ) ) )
	{
		print( "pep_tool v" PEP_TOOL_VERSION newline );
		exit( success );
	}

	pep_channel_bits channel_bits = pep_8bit;
	n1 input_index = 1;

	if( bytes_match( start_inputs[ input_index ], "help" ) )
	{
		print_help:
		{
			print_clear();
			print( "usage: pep_tool [channel_bits:1/2/4/8] <input> [<output>]" newline );
			print( "converts: image <-> pep | default: .image->.pep, .pep->.png" newline );
			print( "input formats: " newline tab "pep, jpg, bmp, tga, png, gif (first frame), pnm, pic" newline );
			print( "output formats: " newline tab "pep, h, jpg, bmp, tga, png" newline );
			print( "examples:" newline );
			print( tab "pep_tool channel_bits:2 image.png (makes image.pep)" newline );
			print( tab "pep_tool image.pep output.jpg (makes output.jpg)" newline );
			print( tab "pep_tool image.bmp (makes image.pep with 8bits/channel)" newline );
			print( tab "pep_tool image.pep output.h (makes output.h source)" newline );
			exit( success );
		}
	}

	if( bytes_match( start_inputs[ input_index ], "channel_bits:" ) and start_inputs[ input_index ][ 14 ] is eof_byte )
	{
		with( start_inputs[ input_index ][ 13 ] )
		{
			when( '1' ) channel_bits = pep_1bit;
			when( '2' ) channel_bits = pep_2bit;
			when( '4' ) channel_bits = pep_4bit;
			when( '8' ) channel_bits = pep_8bit;
			other jump print_help;
		}
		input_index += 1;
	}

	if( start_inputs[ input_index ][ 0 ] is eof_byte or not os_file_exists( start_inputs[ input_index ] ) )
	{
		jump print_help;
	}

	byte const ref const input_ext = path_get_extension( to( byte ref, start_inputs[ input_index ] ) );
	byte output[ path_max_size ];
	byte ref output_ref = output;

	if( start_inputs_count > input_index + 1 and start_inputs[ input_index + 1 ][ 0 ] isnt eof_byte )
	{
		bytes_paste_move( output_ref, start_inputs[ input_index + 1 ] );
	}
	else
	{
		bytes_paste_move( output_ref, start_inputs[ input_index ] );
		output_ref = path_get_extension( output );
		bytes_paste_move( output_ref, pick( bytes_compare( input_ext, "pep", 4 ) is 0, "png", "pep" ) );
	}
	bytes_end( output_ref );

	if( bytes_compare( input_ext, "pep", 4 ) is 0 )
	{
		byte const ref const out_ext = path_get_extension( output );

		if( bytes_compare( out_ext, "h", 2 ) is 0 )
		{
			os_file const in_file = os_map_file( start_inputs[ input_index ] );

			static byte pep_h[ MB( 1 ) ];
			byte ref pep_h_ref = pep_h;

			byte var_name[ 64 ] = { 0 };
			byte ref var_name_ref = var_name;
			byte const ref const base_name = path_get_name( output );

			for( n4 i = 0; base_name[ i ] isnt eof_byte and base_name[ i ] isnt '.' and i < 63; ++i )
			{
				bytes_set_move( var_name_ref, base_name[ i ] );
			}

			bytes_paste_move( pep_h_ref, "static char const " );
			if( var_name[ 0 ] is eof_byte ) bytes_paste_move( pep_h_ref, "pep_data" );
			else bytes_paste_move( pep_h_ref, var_name );
			bytes_paste_move( pep_h_ref, "[] = " );

			bytes_to_h( in_file.mapped_bytes, in_file.size, ref_of( pep_h_ref ) );
			bytes_paste_move( pep_h_ref, ";\n" );

			os_file f = os_create_file( output );
			os_file_ref_save( ref_of( f ), pep_h, to( n4, pep_h_ref - pep_h ) );

			print( start_inputs[ input_index ] );
			print( " -> " );
			print( output );
			print_newline();
		}
		else
		{
			pep p = pep_load( start_inputs[ input_index ] );
			byte const ref const data = to( byte const ref const, pep_decompress( ref_of( p ), pep_rgba, 0, no ) );

			perm flag ok = no;
			if( bytes_compare( out_ext, "png", 4 ) is 0 )
			{
				ok = stbi_write_png( output, p.width, p.height, 4, data, p.width << 2 );
			}
			else if( bytes_compare( out_ext, "jpg", 4 ) is 0 or bytes_compare( out_ext, "jpeg", 5 ) is 0 )
			{
				ok = stbi_write_jpg( output, p.width, p.height, 4, data, 90 );
			}
			else if( bytes_compare( out_ext, "bmp", 4 ) is 0 )
			{
				ok = stbi_write_bmp( output, p.width, p.height, 4, data );
			}
			else if( bytes_compare( out_ext, "tga", 4 ) is 0 )
			{
				ok = stbi_write_tga( output, p.width, p.height, 4, data );
			}

			pep_free( ref_of( p ) );

			if( not ok )
			{
				exit( failure );
			}

			print( start_inputs[ input_index ] );
			print( " -> " );
			print( output );
			print_newline();
		}
	}
	else
	{
		i4 w;
		i4 h;
		stbi_uc ref img = stbi_load( start_inputs[ input_index ], ref_of( w ), ref_of( h ), nothing, 4 );

		if( img is nothing )
		{
			exit( failure );
		}

		pep p = pep_compress( to( n4 ref, img ), w, h, pep_rgba, channel_bits );
		pep_save( ref_of( p ), output );
		stbi_image_free( img );

		print( start_inputs[ input_index ] );
		print( " -> " );
		print( output );
		print_newline();
	}

	exit( success );
}

#pragma endregion start

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
