#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "kmsconstants.h" 
#include "epub.h" 
#include "kobo.h" 

/*==========================================================================
 Find the device (e.g., /dev/sdb) which represents the Kobo. If no Kobo
 device found, return NULL
==========================================================================*/
char *find_kobo_dev (void)
  {
  char *ret = NULL;

  FILE *f = popen ("udisksctl status", "r");
  if (f)
    {
    char s[256];
    BOOL done = FALSE;
    while (!done)
      {
      if (fgets (s, sizeof (s), f))
        {
        s[sizeof(s) - 1] = 0;
        if (s[strlen(s) - 1] == 10)
          s[strlen(s) -1] = 0;

        if (strcasestr (s, "kobo"))
          {
          char *t = strtok (s, " \t");
          while (t)
            {
            if (strlen (t) >= 3)
              {
              if (strncmp (t, "sd", 2) == 0)
                {
                ret = malloc (strlen (t) + 10);
                strcpy (ret, "/dev/");
                strcat (ret, t);
                done = TRUE;
                }
              }
            t = strtok (NULL, " \t");
            }
          }
        //printf ("%s\n", s);
        }
      else
        done = TRUE;
      }
    pclose (f);
    }
  return ret; 
  }


/*==========================================================================
 Find the directory on which the Kobo is mounted, if any. If no Kobo is
 mounted,  return NULL
==========================================================================*/
char *find_kobo_mount (void)
  {
  char *ret = NULL;
  char *kdev = find_kobo_dev ();
  
  if (kdev)
    {
    char cmd[256];
    snprintf (cmd, sizeof (cmd), "udisksctl info -b %s", kdev);
    FILE *f = popen (cmd, "r");
    if (f)
      {
      char s[256];
      BOOL done = FALSE;
      while (!done)
        {
        if (fgets (s, sizeof (s), f))
          {
          s[sizeof(s) - 1] = 0;
          if (s[strlen(s) - 1] == 10)
            s[strlen(s) -1] = 0;

          if (strcasestr (s, "mountpoints"))
            {
            char *p = strchr (s, '/');
            if (p)
              {
              ret = strdup (p);
              //printf ("%s\n", s);
              done = TRUE;
              }
            }
          }
        else
          done = TRUE;
        }
      pclose (f);
      }
    }

  return ret;
  }


/*==========================================================================
  returns TRUE if the file is a book that Kobo can handle
==========================================================================*/
BOOL kobo_is_book (const char *path)
  {
  BOOL ret = FALSE;
  const char *p = strrchr (path, '.');
  if (p)
    {
    const char *ext = p+1; 
    if (strcasecmp (ext, "epub") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "epub3") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "mobi") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "pdf") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "txt") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "html") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "rtf") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "cbr") == 0)
      ret = TRUE; 
    if (strcasecmp (ext, "cbz") == 0)
      ret = TRUE; 
    }
  return ret;
  }


/*==========================================================================
  kobo_mount
==========================================================================*/
void kobo_mount (const char *kdev)
  {
  char cmd[256];
  snprintf (cmd, sizeof (cmd), "udisksctl mount -b %s > /dev/null", kdev);
  system (cmd);
  }


/*==========================================================================
  kobo_unmount
==========================================================================*/
void kobo_unmount (const char *kdev)
  {
  char cmd[256];
  snprintf (cmd, sizeof (cmd), "udisksctl unmount -b %s > /dev/null", kdev);
  system (cmd);
  }




