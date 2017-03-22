#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include "kmsconstants.h" 
#include "epub.h" 
#include "mobi.h" 
#include "kobo.h" 
#include "kmslogging.h" 

#define FLAG_SHOW_METADATA 0x0001

typedef BOOL (*FileFunc) (const char *mount, const char *path, 
  int numfiles, void *data);

/*==========================================================================
  enumerate_mount 
==========================================================================*/
BOOL enumerate_mount (const char *mount, const char *path, 
      int *numfiles, FileFunc fn, 
      void *data)
  {
  kmslog_debug ("recursive scan '%s'", path);
  BOOL ret = TRUE;
  if (TRUE)
    {
    BOOL stop = FALSE;
    char fullpath [1024];
    snprintf (fullpath, sizeof (fullpath) - 1, "%s/%s", mount, path);
    DIR *dir = opendir (fullpath);
    if (dir)
      {
      struct dirent *d = readdir (dir);
      while (d && !stop) 
	{
	if (d->d_name[0] != '.')
	  {
	  char *newpath = malloc (strlen(path) + 5 + strlen(d->d_name));
	  strcpy (newpath, path);
	  if (newpath[strlen(newpath) - 1] != '/')
	    strcat (newpath, "/");
	  strcat (newpath, d->d_name);
	  if (d->d_type == DT_DIR)
	    {
	    ret = enumerate_mount (mount, newpath, numfiles, fn, data);
            if (!ret) stop = TRUE;
	    }
	  else if (d->d_type == DT_REG)
	    {
            if (kobo_is_book (newpath))
	      {
              if (fn (mount, newpath, *numfiles, data) == FALSE)
                {
                stop = TRUE;
                ret = FALSE;
                }
              (*numfiles)++;
              }
	    }
	  free (newpath);
	  }
	d = readdir (dir);
	} 
      closedir (dir);
      }
    else
      {
      kmslog_warning ("Can't open directory '%s'", path);
      }
    }
  return ret;
  }


/*==========================================================================
  get_book_author
==========================================================================*/
char *get_book_author (const char *path)
  {
  char *ret = NULL; 

  char *p = strrchr (path, '.');
  if (p)
    {
    char *ext = p+1;
    if (strcasecmp (ext, "epub") == 0)
        ret = epub_get_author (path);
    else if (strcasecmp (ext, "mobi") == 0)
        ret = mobi_get_author (path);
     }

  return ret;
  }


/*==========================================================================
  get_book_summary 
==========================================================================*/
char *get_book_summary (const char *mount, const char *path, int flags)
  {
  char *ret = NULL; 

  if (flags & FLAG_SHOW_METADATA)
    {
    char fullpath[1000];
    snprintf (fullpath, sizeof (fullpath) - 1, "%s%s", mount, path);

    char *p = strrchr (fullpath, '.');
    if (p)
      {
      char *ext = p+1;
      if (strcasecmp (ext, "epub") == 0)
        ret = epub_get_book_summary_line (fullpath);
      else if (strcasecmp (ext, "mobi") == 0)
        ret = mobi_get_book_summary_line (fullpath);
      }
    }

  if (ret == NULL)
    {
    ret = strdup (path);
    }
  return ret;
  }


/*==========================================================================
  func_show_book 
==========================================================================*/
BOOL func_show_book (const char *mount, const char *path, int num, void *data) 
  {
  int *pflags = (int *) data;
  int flags = *pflags;
  char *summary = get_book_summary (mount, path, flags);
  printf ("%03d %s\n", num, summary);
  free (summary);
  return TRUE;
  }


/*==========================================================================
  func_remove
==========================================================================*/
BOOL func_remove (const char *mount, const char *path, int num, void *data) 
  {
  int *pnum_to_delete = (int *) data;
  int num_to_delete = *pnum_to_delete;
  if (num_to_delete == num)
    {
    char *fullpath;
    asprintf (&fullpath, "%s/%s", mount, path);
    kmslog_info ("Deleting %s", fullpath);
    unlink (fullpath);
    free (fullpath);
    return FALSE; // Don't continue enumeration once matched
    }
  return TRUE; 
  }




/*==========================================================================
  list_books 
==========================================================================*/
void list_books (const char *mount, int flags)
  {
  int numfiles = 0;
  enumerate_mount (mount, "", &numfiles, func_show_book, &flags);
  }


/*==========================================================================
  remove_empty_dirs
==========================================================================*/
void remove_empty_dirs (const char *mount)
  {
  kmslog_info ("Cleaning up empty directories");
  char *cmd;
  asprintf (&cmd, "find \"%s\" -type d -exec rmdir {} \\; 2> /dev/null", mount);
  system (cmd); 
  free (cmd);
  }


/*==========================================================================
  remove_file 
==========================================================================*/
void remove_file (const char *mount, int number)
  {
  int numfiles = 0;
  enumerate_mount (mount, "", &numfiles, func_remove, &number);
  remove_empty_dirs (mount);
  }


/*==========================================================================
  install
==========================================================================*/
void install (const char *mount, const char *author, const char *file)
  {
  if (access (file, R_OK) == 0)
    {
    if (kobo_is_book (file))
      {
      char *using_author = NULL;
      if (author)
        using_author = strdup (author);
      else
        {
        char *detected_author = get_book_author (file); 
        if (detected_author)
          {
          kmslog_info ("Detected author: %s", detected_author);
          using_author = detected_author;
          }
        }  

      if (using_author)
        {
        char *newdir;
        asprintf (&newdir, "%s/%s", mount, using_author);
        char *filename = basename (file);
        kmslog_info ("Installing file %s in directory %s", filename, newdir);
	mkdir (newdir, 0777);
        char *cmd = NULL;
        asprintf (&cmd, "cp \"%s\" \"%s/%s\"", file, newdir, filename);
        system (cmd);
        free (newdir);
        free (cmd);
        free (using_author);
        }
      else
        {
        kmslog_error ("Author cannot be detected, and has not been supplied");
        kmslog_error ("Use --author to force an author name");
        }
      }
    else
      kmslog_error ("This does not appear to be a compatible book: %s", 
        file);
    }
  else
    kmslog_error ("Can't open file for reading: %s", file);
  }


/*==========================================================================
  main
==========================================================================*/
int main (int argc, char **argv)
  {
  static BOOL show_version = FALSE;
  static BOOL show_usage = FALSE;
  static BOOL list = FALSE;
  static int loglevel = ERROR;
  static BOOL nometa = FALSE;
  char *kobo_dir = NULL;
  char *install_file = NULL;
  char *author = NULL;
  char *sremove = NULL;

  static struct option long_options[] = 
   {
     {"version", no_argument, &show_version, 'v'},
     {"help", no_argument, &show_usage, '?'},
     {"nometa", no_argument, &nometa, 'n'},
     {"list", no_argument, &list, 'l'},
     {"dir", required_argument, NULL, 'd'},
     {"install", required_argument, NULL, 'i'},
     {"author", required_argument, NULL, 'a'},
     {"remove", required_argument, NULL, 'r'},
     {"loglevel", required_argument, NULL, 0},
     {0, 0, 0, 0}
   };

  int opt;
  while (1)
   {
   int option_index = 0;
   opt = getopt_long (argc, argv, "?vd:lni:r:a:",
     long_options, &option_index);

   if (opt == -1) break;

   switch (opt)
     {
     case 0:
        if (strcmp (long_options[option_index].name, "version") == 0)
          show_version = TRUE;
        else if (strcmp (long_options[option_index].name, "help") == 0)
          show_usage = TRUE;
        else if (strcmp (long_options[option_index].name, "list") == 0)
          list = TRUE;
        else if (strcmp (long_options[option_index].name, "nometa") == 0)
          nometa = TRUE;
        else if (strcmp (long_options[option_index].name, "dir") == 0)
          kobo_dir = strdup (optarg);
        else if (strcmp (long_options[option_index].name, "install") == 0)
          install_file = strdup (optarg);
        else if (strcmp (long_options[option_index].name, "author") == 0)
          author = strdup (optarg);
        else if (strcmp (long_options[option_index].name, "loglevel") == 0)
          loglevel = atoi (optarg);
        else if (strcmp (long_options[option_index].name, "remove") == 0)
          sremove = strdup (optarg);
        else
          exit (-1);
        break;

     case 'v': show_version = TRUE; break;
     case 'd': kobo_dir = strdup (optarg); break; 
     case 'i': install_file = strdup (optarg); break; 
     case 'r': sremove = strdup (optarg); break; 
     case 'a': author = strdup (optarg); break; 
     case '?': show_usage = TRUE; break;
     case 'n': nometa = TRUE; break;
     case 'l': list = TRUE; break;
     default:  exit(-1);
     }
   }

  if (show_usage)
    {
    printf ("Usage %s [options]\n", argv[0]);
    printf ("  -a, --author [name]   force author name (if not guessed)\n");
    printf ("  -d, --dir             device directory (if not guessed)\n");
    printf ("  -i, --intstall [file] install book file on device\n");
    printf ("  -l, --list            show books on device\n");
    printf ("      --loglevel N      log verbosity, 0 (default) - 3\n");
    printf ("  -n, --nometa          don't try to show book metadata\n");
    printf ("  -r, --remove N        remove file N (from list)\n");
    printf ("  -v, --version         show version information\n");
    printf ("  -?                    show this message\n");
    exit (0);
    }
 
  if (show_version)
    {
    printf ("kobo " VERSION "\n");
    printf ("Copyright (c)2017 Kevin Boone\n");
    printf ("Distributed according to the terms of the GPL, v3.0\n");
    exit (0);
    }
 
  kmslogging_set_level (loglevel); 

  BOOL do_something = FALSE;
  if (list) do_something = TRUE;
  if (install_file) do_something = TRUE;
  if (sremove) do_something = TRUE;

  if (do_something)
    {
    char *kdev = NULL; 
    char *kmount = NULL; 
    BOOL unmount = FALSE;

    if (TRUE)
      {
      if (kobo_dir)
        {
        kmslog_info ("Over-riding device auto-detection");
        kmount = strdup (kobo_dir);
        }
      else
        {
        kdev = find_kobo_dev();
        kmslog_info ("Found device at %s", kdev);
        kmount = find_kobo_mount();
        if (kmount)
          kmslog_info ("Device is mounted at %s", kmount);
        if (kdev && !kmount)
          {
          // We have a device, but it is (?) not mounted. Try to 
          //   mount it
          kmslog_info ("Trying to auto-mount device at %s", kdev);
          kobo_mount (kdev);
          kmount = find_kobo_mount();
          if (!kmount)
            {
            kmslog_warning ("Can't auto-mount device at %s", kdev);
            }
          else
            {
            kmslog_info ("Device auto-mounted at %s", kmount);
            unmount = TRUE; // If we mount it, we should unmount it
            }
          }
        }
      }

    if (kmount)
      {
      if (list)
        {
        int flag = 0;
        if (!nometa) flag |= FLAG_SHOW_METADATA;
        list_books (kmount, flag);
        }
      if (install_file)
        {
        install (kmount, author, install_file);
        }
      if (sremove)
        {
        int remove;
        if (sscanf (sremove, "%d", &remove) == 1)
          {
          remove_file (kmount, remove);
          }
        else
          {
          kmslog_error ("Not a number: %s", sremove);
          }
        }
      }
    else
      {
      fprintf (stderr, "%s: Kobo mount point unknown (is it connected?)\n"
         "Use --dir if it is connected, but cannot be mounted automatically\n", argv[0]);
      }

    if (unmount && kdev)
      {
      kmslog_info ("Unmounting device from %s", kmount);
      kobo_unmount (kdev);
      }
    if (kdev) free (kdev);
    if (kmount) free (kmount);
    }
  else
    {
    fprintf (stderr, "Please select one of --install, --remove, or --list\n");
    fprintf (stderr, "'%s --help' for more information\n", argv[0]);
    }

  if (install_file) free (install_file);
  if (author) free (author);
  if (kobo_dir) free (kobo_dir);
  if (sremove) free (sremove);
  return 0;
  }



