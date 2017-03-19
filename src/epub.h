/*==========================================================================
kobo
epub.h
Copyright (c)2017 Kevin Boone, GPLv3.0
*==========================================================================*/

#pragma once

BOOL epub_get_metadata (const char *filename, 
        char **title, char **creator, char **year, char **genre, 
        char **comment, char **error);


char *epub_get_book_summary_line (const char *path);
char *epub_get_author (const char *path);

