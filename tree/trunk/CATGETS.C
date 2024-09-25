/* Functions that emulate UNIX catgets */

/* Copyright (C) 1999,2000 Jim Hall <jhall1@isd.net> */

/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <stdio.h>			/* sprintf */
#include <stdlib.h>			/* getenv  */
#include <string.h>			/* strtok, strchr */

/* #include <dir.h>			/* fnmerge */

#include "db.h"
#include "catgets.h"


/* External functions */

char *get_line (FILE *pfile, int continue_ch);


/* Local prototypes */

int _catread (char *catfile);		/* Reads a catfile into the hash */


/* Globals */

nl_catd catalog = 0;			/* Catalog descriptor, either 0 or 1 */


/* Functions */

char *
catgets(nl_catd  cat,  int set_number, int message_number,
	 char *message)
{
  /* get message from a message catalog */

  /* 'message' should really be const, but not when it is returned */

  /* On success, catgets() returns a pointer to an internal buffer
     area containing the null-terminated message string.  On failure,
     catgets() returns the value 'message'.  */

  char key[10];
  db_t *ptr;

  /* Is this the same catalog we have in memory? */

  if (cat != catalog)
    {
      return (message);
    }

  /* fetch the message that goes with the set/message number */

  sprintf (key, "%d.%d", set_number, message_number);
  ptr = db_fetch (key);

  if (ptr)
    {
      return (ptr->value);
    }

  /* else */

  return (message);
}

nl_catd
catopen(char *name, int flag)
{
  /* catopen() returns a message catalog descriptor of type nl_catd on
     success.  On failure, it returns -1. */

  /* 'flag' is completely ignored. */

  /* I'm not sure 128 is a good value to use here.  Would MAXPATH be
     better, from dir.h? */

  char catfile[128];			/* full path to the msg catalog */
  char nlspath[128];                    /* value of %NLSPATH% */
  char nlspath_lang[128];               /* value of %NLSPATH%\%LANG% */
  char *nlsptr;				/* ptr to NLSPATH */
  char *lang;                           /* ptr to LANG */
  char lang_2[3];                       /* 2-char version of %LANG% */
  char *tok;                            /* pointer when using strtok */

  /* Open the catalog file */

  /* The value of `catalog' will be set based on _catread */

  if (catalog)
    {
      /* Already one open */

      return (-1);
    }

  /* If the message catalog file name contains a directory separator,
     assume that this is a real path to the catalog file.  Note that
     _catread will return a true or false value based on its ability
     to read the catfile. */

  if (strchr (name, '\\'))
    {
      /* first approximation: 'name' is a filename */

      strcpy (catfile, name);
      catalog = _catread (catfile);
      return (catalog);
    }

  /* If the message catalog file name does not contain a directory
     separator, then we need to try to locate the message catalog on
     our own.  We will use several methods to find it. */

  /* We will need the value of LANG, and may need a 2-letter abbrev of
     LANG later on, so get it now. */

  lang = getenv ("LANG");

  if (lang == NULL)
    {
      /* Return failure - we won't be able to locate the cat file */
      return (-1);
    }

  strncpy (lang_2, lang, 2);
  lang_2[2] = '\0';

  /* step through NLSPATH */

  nlsptr = getenv ("NLSPATH");

  if (nlsptr == NULL)
    {
      /* Return failure - we won't be able to locate the cat file */
      return (-1);
    }

  strcpy (nlspath, nlsptr);

  tok = strtok (nlspath, ";");
  while (tok != NULL)
    {
      /* Try to find the catalog file in each path from NLSPATH */

      /* Rule #1: %NLSPATH%\%LANG%\cat */

      strcpy (nlspath_lang, nlspath);
      strcat (nlspath_lang, "\\");
      strcat (nlspath_lang, lang);

      _makepath (catfile, NULL, nlspath_lang, name, NULL);
      catalog = _catread (catfile);
      if (catalog)
	{
	  return (catalog);
	}

      /* Rule #2: %NLSPATH%\cat.%LANG% */

      _makepath (catfile, NULL, tok, name, lang);
      catalog = _catread (catfile);
      if (catalog)
	{
	  return (catalog);
	}

      /* Rule #3: if LANG looks to be in format "en-uk" then
         %NLSPATH%\cat.EN */

      if (lang[2] == '-')
        {
          _makepath (catfile, NULL, tok, name, lang_2);
	  catalog = _catread (catfile);
	  if (catalog)
	    {
	      return (catalog);
	    }
        }

      /* Grab next tok for the next while iteration */

      tok = strtok (NULL, ";");
    } /* while tok */

  /* We could not find it.  Return failure. */

  return (0);
}

int
_catread (char *catfile)
{
  FILE *pfile;				/* pointer to the catfile */
  char *key;				/* part of key-value for hash */
  char *value;				/* part of key-value for hash */
  char *str;                            /* the string read from the file */

  /* Open the catfile for reading */

  pfile = fopen (catfile, "r");
  if (!pfile)
    {
      /* Cannot open the file.  Return failure */
      return (0);
    }

  /* Read the file into memory */

  while ((str = get_line (pfile, 0)) != NULL)
    {
      /* Break into parts.  Entries should be of the form:
	 "1.2:This is a message" */

      /* A line that starts with '#' is considered a comment, and will
         be thrown away without reading it. */

      /* Assumes no blank lines */

      if (str[0] != '#')
	{
	  key = strtok (str, ":");
	  value = strtok (NULL, "\n");

	  db_insert (key, value);
	} /* if comment */

      free (str);
    } /* while */

  fclose (pfile);

  /* Return success */

  return (1);
}

void
catclose (nl_catd cat)
{
  /* close a message catalog */

  catalog = 0;
}
