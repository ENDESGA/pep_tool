////////////////////////////////////////////////////////////////
//
//  pep_tool
//
//  author(s):
//  ENDESGA - https://x.com/ENDESGA | https://bsky.app/profile/endesga.bsky.social
//
//  https://github.com/ENDESGA/pep_tool
//  2025 - CC0 - FOSS forever
//

////////////////////////////////
/// include(s)

#define PEP_IMPLEMENTATION
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
#include <H_tool.h>

////////////////////////////////
/// version

#define PEP_TOOL_VERSION_MAJOR 0
#define PEP_TOOL_VERSION_MINOR 1
#define PEP_TOOL_VERSION_PATCH 0
#define PEP_TOOL_VERSION AS_BYTES( PEP_TOOL_VERSION_MAJOR ) "." AS_BYTES( PEP_TOOL_VERSION_MINOR ) "." AS_BYTES( PEP_TOOL_VERSION_PATCH )

////////////////////////////////////////////////////////////////

#define EXIT( TYPE )\
	print_reset_format();\
	exit( TYPE )

#define error_exit( MESSAGE )\
	print( format_magenta "error: " format_yellow MESSAGE newline );\
	EXIT( failure )

start
{
	if( start_parameters_count is 2 and ( bytes_compare( start_parameters[ 1 ], "version", 8 ) is 0 or bytes_compare( start_parameters[ 1 ], "-v", 3 ) is 0 ) )
	{
		print( "pep_tool v" PEP_TOOL_VERSION newline );
		EXIT( success );
	}

	perm pep_channel_bits channel_bits = pep_8bit;

	H_tool_start();

	print( format_yellow "pep_tool " format_cyan "[" format_white "channel_bits:1/2/4/8" format_cyan "] <" format_white "input" format_cyan "> [<" format_white "output" format_cyan ">], " format_white "help" newline format_cyan "> " format_white );
	print_show();

	get_inputs();

	if( bytes_compare( inputs[ current_input ], "help", 5 ) is 0 )
	{
		print_clear();
		print( format_yellow "usage: " format_white "pep_tool " format_cyan "[" format_white "channel_bits:1/2/4/8" format_cyan "] <" format_white "input" format_cyan "> [<" format_white "output" format_cyan ">]" newline );
		print( format_yellow "converts: " format_white "image " format_cyan "<-> " format_white "pep " format_cyan "| " format_yellow "default: " format_white ".image" format_cyan "->" format_white ".pep" format_cyan ", " format_white ".pep" format_cyan "->" format_white ".png" newline );
		print( format_yellow "input formats: " newline tab format_white "pep" format_cyan ", " format_white "jpg" format_cyan ", " format_white "bmp" format_cyan ", " format_white "tga" format_cyan ", " format_white "png" format_cyan ", " format_white "gif " format_magenta "(first frame)" format_cyan ", " format_white "pnm" format_cyan ", " format_white "pic" newline );
		print( format_yellow "output formats: " newline tab format_white "pep" format_cyan ", " format_white "jpg" format_cyan ", " format_white "bmp" format_cyan ", " format_white "tga" format_cyan ", " format_white "png" newline );
		print( format_yellow "examples:" newline );
		print( tab format_white "pep_tool channel_bits:2 image.png " format_magenta "(makes image.pep)" newline );
		print( tab format_white "pep_tool image.pep output.jpg " format_magenta "(makes output.jpg)" newline );
		print( tab format_white "pep_tool image.bmp " format_magenta "(makes image.pep with 8bits/channel)" newline );
		print( format_magenta "Press " format_cyan "Enter" format_magenta " to continue..." );
		getchar();
		inputs_next();
	}

	if( bytes_compare( inputs[ current_input ], "channel_bits:", 13 ) is 0 and inputs[ current_input ][ 14 ] is eof_byte )
	{
		with( inputs[ current_input ][ 13 ] )
		{
			when( '1' )
			{
				channel_bits = pep_1bit;
				skip;
			}
			when( '2' )
			{
				channel_bits = pep_2bit;
				skip;
			}
			when( '4' )
			{
				channel_bits = pep_4bit;
				skip;
			}
			when( '8' )
			{
				channel_bits = pep_8bit;
				skip;
			}
			other
			{
				error_exit( "invalid channel_bits" );
			}
		}
		inputs_next();
	}

	if( inputs[ current_input ][ 0 ] is eof_byte or not file_exists( inputs[ current_input ] ) )
	{
		error_exit( "invalid input" );
	}

	temp const byte const_ref input_ext = path_get_extension( inputs[ current_input ] );
	byte output[ max_path_size ];
	temp byte ref output_ref = output;

	if( inputs[ current_input + 1 ][ 0 ] isnt eof_byte )
	{
		bytes_paste_move( output_ref, inputs[ current_input + 1 ] );
	}
	else
	{
		bytes_paste_move( output_ref, inputs[ current_input ] );
		output_ref = path_get_extension( output );
		bytes_paste_move( output_ref, pick( bytes_compare( input_ext, "pep", 4 ) is 0, "png", "pep" ) );
	}
	bytes_end( output_ref );

	if( bytes_compare( input_ext, "pep", 4 ) is 0 )
	{
		pep p = pep_load( inputs[ current_input ] );
		temp const byte const_ref data = to( const byte const_ref, pep_decompress( ref_of( p ), pep_rgba, 0 ) );
		temp const byte const_ref out_ext = path_get_extension( output );

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
			error_exit( "save failed" );
		}

		print( inputs[ current_input ] );
		print( " -> " );
		print( output );
		print_newline();
	}
	else
	{
		i4 w;
		i4 h;
		stbi_uc ref img = stbi_load( inputs[ current_input ], ref_of( w ), ref_of( h ), nothing, 4 );

		if( img is nothing )
		{
			error_exit( "load failed" );
		}

		pep p = pep_compress( to( n4 ref, img ), w, h, pep_rgba, channel_bits );
		pep_save( ref_of( p ), output );
		stbi_image_free( img );

		print( inputs[ current_input ] );
		print( " -> " );
		print( output );
		print_newline();
	}

	EXIT( success );
}

////////////////////////////////////////////////////////////////

