#ifndef _HEADER_H_INCLUDED_
#define _HEADER_H_INCLUDED_

typedef struct amb_hal_header_s {
	char magic[16] ;
	unsigned int major_version ;
	unsigned int minor_version ;
	unsigned char chip_name[8] ;
	unsigned char chip_stepping[8] ;
	unsigned char build_id[32] ;
	unsigned char build_date[32] ;
} amb_hal_header_t;

typedef struct amb_hal_function_info_s {
	unsigned int (*function)(unsigned int, unsigned int,
				 unsigned int, unsigned int) ;
	const char* name ;
} amb_hal_function_info_t ;

#endif // ifndef _HEADER_H_INCLUDED_
