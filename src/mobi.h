/*==========================================================================
kobo
mobi.h
Copyright (c)2017 Kevin Boone, GPLv3.0
*==========================================================================*/

#pragma once

BOOL mobi_get_metadata (const char *filename, 
        char **title, char **creator, char **year, char **genre, 
        char **comment, char **error);


char *mobi_get_book_summary_line (const char *path);
char *mobi_get_author (const char *path);


