/*==========================================================================
kobo
kobo.h
Copyright (c)2017 Kevin Boone, GPLv3.0
*==========================================================================*/

#pragma once
char     *find_kobo_dev (void);
char     *find_kobo_mount (void);
BOOL      kobo_is_book (const char *path);
void      kobo_mount (const char *kdev);
void      kobo_unmount (const char *kdev);

