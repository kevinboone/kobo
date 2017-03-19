/*===========================================================================
kobo
mobi.c
Copyright (c)2017 Kevin Boone, GPL v3.0
===========================================================================*/
#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <fcntl.h>
#include "kmslogging.h"

/*===========================================================================
do_mobi_record
===========================================================================*/
void do_mobi_record (int f, int offset, int length, char **title, 
       char **creator, char **year, char **genre, 
       char **comment) 
  {
  //printf ("offset=%d\n", offset);
  //printf ("length=%d\n", length);
  int old_offset = lseek (f, 0, SEEK_CUR);
  lseek (f, offset, SEEK_SET);

  unsigned char buff[128];
  read (f, buff, sizeof (buff));

  if (strncmp ((const char *)buff+16, "MOBI", 4) == 0)
    {
    int mobi_len = 
          256 * 256 * 256 * (int)buff[20] +
          256 * 256 * (int)buff[21] +
          256 * (int)buff[22] +
          (int)buff[23];

    lseek (f, offset + mobi_len + 16, SEEK_SET);
    // We should now be four bytes int the EXTH header, if there is one
    read (f, buff, 12); // Just read up to the start of the EXTH records 
    //printf ("%c %c %c %c\n", buff[0], buff[1], buff[2], buff[3]);
    if (strncmp ((const char *)buff, "EXTH", 4) == 0)
      {
      //int ext_len = 
      //    256 * 256 * 256 * (int)buff[4] +
      //    256 * 256 * (int)buff[5] +
      //    256 * (int)buff[6] +
      //    (int)buff[7];
      
      int ext_count = 
          256 * 256 * 256 * (int)buff[8] +
          256 * 256 * (int)buff[9] +
          256 * (int)buff[10] +
          (int)buff[11];
      
      //printf ("GOT EXTH %d %d\n", ext_len, ext_count);

      int j;
      for (j = 0; j < ext_count; j++)
        {
        read (f, buff, 4); 
        
        int record_type = 
          256 * 256 * 256 * (int)buff[0] +
          256 * 256 * (int)buff[1] +
          256 * (int)buff[2] +
          (int)buff[3];
      
        read (f, buff, 4); 
        
        int record_len = 
          256 * 256 * 256 * (int)buff[0] +
          256 * 256 * (int)buff[1] +
          256 * (int)buff[2] +
          (int)buff[3];

        char *exth = malloc (record_len - 8 + 1);
        memset (exth, 0, record_len - 8 + 1);
        read (f, exth, record_len - 8); 
        //printf ("type=%d, len=%d, data=%s\n", record_type, record_len, exth);
        
        if (record_type == 100)
          *creator = strdup (exth);
        else if (record_type == 503)
          *title = strdup (exth);
        else if (record_type == 106)
          {
          *year = strdup (exth);
          if (strlen (*year) > 4) (*year)[4] = 0;
          }
        else if (record_type == 105)
          {
          if (!(*genre))
          *genre = strdup (exth);
          }
        else if (record_type == 103)
          *comment = strdup (exth);

        free (exth);
        }
      }
    }

  lseek (f, old_offset, SEEK_SET);
  }

/*===========================================================================
mobi_get_metadata
===========================================================================*/
BOOL mobi_get_metadata (const char *filename, 
        char **title, char **creator, char **year, char **genre, 
        char **comment, char **error)
  {
  BOOL ret = FALSE;
  int f = open (filename, O_RDONLY);
  if (!f)
    {
    kmslog_error ("Can't open file for reading: %s", filename);
    return ret;
    }

  unsigned char buff[78];
  int n;
  n = read (f, buff, sizeof (buff));
  if (n == sizeof (buff))
    {
    if (strncmp ((const char *)buff + 64, "MOBI", 4) == 0)
      {
      // If we get this far, we are almost certainly looking at a MOBI
      //  file, whether it turns out to have any meta-data or not.
      // So this is a successful read.
      ret = TRUE;
      int num_records = 256 * (int)buff[76] + (int)buff[77];
      read (f, buff, 8);
      int last_offset = 
          256 * 256 * 256 * (int)buff[0] +
          256 * 256 * (int)buff[1] +
          256 * (int)buff[2] +
          (int)buff[3];
      for (n = 1; n < num_records; n++)
        {
        read (f, buff, 8);
        int offset = 
          256 * 256 * 256 * (int)buff[0] +
          256 * 256 * (int)buff[1] +
          256 * (int)buff[2] +
          (int)buff[3];
        do_mobi_record (f, last_offset, offset-last_offset, title,
          creator, year, genre, comment);
        last_offset = offset;
        }
      }
    else
      {
      kmslog_debug ("Did not find MOBI identifier in file: %s", filename);
      }
    }
  else
      kmslog_error ("Can't read file header: %s", filename);

  close (f);
  return ret;
  }


/*===========================================================================
mobi_get_book_summary_line
===========================================================================*/
char *mobi_get_book_summary_line (const char *path)
  {
  char *title = NULL;
  char *creator = NULL;
  char *year = NULL;
  char *genre = NULL;
  char *comment = NULL;
  char *error = NULL;
  char *ret = NULL;
  if (mobi_get_metadata (path, 
     &title, &creator, &year, &genre, &comment,
     &error))
    {
    if (title && creator)
      asprintf (&ret, "%s/%s", creator, title); 
    }
  else
    {
    if (error)
      {
      kmslog_warning ("Can't read MOBI metadata: %s", error);
      free (error); 
      }
    }
  if (title) free (title);
  if (genre) free (genre);
  if (creator) free (creator);
  if (year) free (year);
  if (comment) free (comment);
  return ret;
  }


/*===========================================================================
mobi_get_author
===========================================================================*/
char *mobi_get_author (const char *path)
  {
  char *title = NULL;
  char *creator = NULL;
  char *year = NULL;
  char *genre = NULL;
  char *comment = NULL;
  char *error = NULL;
  char *ret = NULL;
  if (mobi_get_metadata (path, 
     &title, &creator, &year, &genre, &comment,
     &error))
    {
    if (creator)
      asprintf (&ret, "%s", creator); 
    }
  else
    {
    if (error)
      {
      kmslog_warning ("Can't read EPUB metadata: %s", error);
      free (error); 
      }
    }
  if (title) free (title);
  if (genre) free (genre);
  if (creator) free (creator);
  if (year) free (year);
  if (comment) free (comment);
  return ret;
  }



