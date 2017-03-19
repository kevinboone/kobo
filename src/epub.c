/*===========================================================================
kobo
epub.c
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
#include "sxmlc.h" 

/*===========================================================================
parse_content
===========================================================================*/
static BOOL parse_content (const char *filename, 
    char **title, char **creator, char **year, 
    char **genre, char **comment, char **error)
  {
  BOOL ok = TRUE;

  // Note that all hrefs in this file are relative to this file's
  // location, not to the location of the manifest.
  char *filename2 = strdup (filename);
  
  FILE *f = fopen (filename, "r");
  if (f)
    {
    XMLDoc *xmldoc = malloc (sizeof (XMLDoc));
    XMLDoc_init (xmldoc);
    if (XMLDoc_parse_file_DOM (filename, xmldoc))
      {
      XMLNode *root = XMLDoc_root (xmldoc);
      int i, l = root->n_children;
      for (i = 0; i < l; i++)
        {
        XMLNode *r1 = root->children[i];
        if (strcasecmp (r1->tag, "metadata") == 0)
          {
          int i, l = r1->n_children;
          for (i = 0; i < l; i++)
            {
            XMLNode *m = r1->children[i];
            if (strcasestr (m->tag, "creator"))
              {
              if (m->text && creator) *creator = strdup (m->text);
              }
            else if (strcasestr (m->tag, "description"))
              {
              if (m->text && comment) *comment = strdup (m->text);
              }
            else if (strcasestr (m->tag, "title"))
              {
              if (m->text && title) *title = strdup (m->text);
              }
            else if (m->text && strcasestr (m->tag, "date"))
              {
              char *y = strdup (m->text);
              char *p = strchr (y, '-');
              if (p)
                {
                *p = 0; 
                if (year) *year = strdup (y);
                }
              free (y);
              }
            else if (strcasestr (m->tag, "subject"))
              {
              if (m->text && genre) *genre = strdup (m->text);
              }
            }
          }
        else if (strcasecmp (r1->tag, "manifest") == 0)
          {
          }
        }
      }
    else
      {
      asprintf (error, "parsing EPUB: Can't parse content file %s\n", filename);
      }
    fclose (f);
    }
  else
    {
    asprintf (error, "parseing EPUB: Can't open content file %s\n", filename);
    }
  free (filename2);
  return ok;
  }

 
/*===========================================================================
get_epub_metadata
===========================================================================*/
BOOL epub_get_metadata (const char *filename, 
     char **title, char **creator, char **year, char **genre, char **comment,
     char **error) 
  {
  BOOL ok = TRUE;

  const char *tempbase = getenv("TMP");
  if (!tempbase) tempbase = "/tmp";
  char *tempdir;
  asprintf (&tempdir, "%s/epub2txt%d", tempbase, getpid());

  char *cmd;
  asprintf (&cmd, "mkdir -p \"%s\"", tempdir);
  system (cmd);
  free (cmd);

  asprintf (&cmd, "unzip -o -qq \"%s\" -d \"%s\"", filename, tempdir);
  system (cmd);
  free (cmd);

  asprintf (&cmd, "chmod -R 744 \"%s\"", tempdir);
  system (cmd);
  free (cmd);
  
  char *opf;
  asprintf (&opf, "%s/META-INF/container.xml", tempdir);
  FILE *f = fopen (opf, "r");
  if (f)
    {
    XMLDoc *xmldoc = malloc (sizeof (XMLDoc));
    XMLDoc_init (xmldoc);
    if (XMLDoc_parse_file_DOM (opf, xmldoc))
      {
      XMLNode *root = XMLDoc_root (xmldoc);
      int i, l = root->n_children;
      for (i = 0; i < l; i++)
        {
        XMLNode *r1 = root->children[i];
        if (strcmp (r1->tag, "rootfiles") == 0)
          {
          XMLNode *rootfiles = r1;
          int i, l = rootfiles->n_children;
          for (i = 0; i < l; i++)
            {
            XMLNode *r1 = rootfiles->children[i];
            if (strcmp (r1->tag, "rootfile") == 0)
              {
              int k, nattrs = r1->n_attributes;
              for (k = 0; k < nattrs; k++)
                {
                char *name = r1->attributes[k].name;
                char *value = r1->attributes[k].value;
                if (strcmp (name, "full-path") == 0)
                  {
                  char *c;
                  asprintf (&c, "%s/%s", tempdir, value);
                  ok = parse_content (c, 
                    title, creator, year, genre, comment, error);
                  free (c);
                  }
                }
              }
            }
          }
        }
      XMLDoc_free (xmldoc);
      }
    else
      {
      asprintf (error, "parsing EPUB: can't parse container.xml\n");
      ok = FALSE;
      }

    fclose (f);
    }
  else
    {
    asprintf (error, "parsing EPUB: can't open file %s\n", opf);
    ok = FALSE;
    }

  free (opf);

  asprintf (&cmd, "rm -rf \"%s\"", tempdir);
  system (cmd);
  free (cmd);

  free (tempdir);
  return ok;
  }


/*===========================================================================
epub_get_book_summary_line
===========================================================================*/
char *epub_get_book_summary_line (const char *path)
  {
  char *title = NULL;
  char *creator = NULL;
  char *year = NULL;
  char *genre = NULL;
  char *comment = NULL;
  char *error = NULL;
  char *ret = NULL;
  if (epub_get_metadata (path, 
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


/*===========================================================================
epub_get_author
===========================================================================*/
char *epub_get_author (const char *path)
  {
  char *title = NULL;
  char *creator = NULL;
  char *year = NULL;
  char *genre = NULL;
  char *comment = NULL;
  char *error = NULL;
  char *ret = NULL;
  if (epub_get_metadata (path, 
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



