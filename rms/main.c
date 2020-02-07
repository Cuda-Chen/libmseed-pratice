#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libmseed.h"

#define VERSION "[libmseed " LIBMSEED_VERSION "]"
#define PROGRAMNAME "libmseed_hello_world"

static int8_t verbose  = 0;
static int8_t ppackets = 0;
static int8_t basicsum = 0;
static int printdata   = 0;
static char *inputfile = 0;

static int parse_parameter (int argcount, char **argvec);
static void show_usage (void);

int
main (int argc, char **argv)
{
  MS3Record *msr = 0;
  uint32_t flags = 0;

  int64_t totalrecs  = 0;
  int64_t totalsamps = 0;
  int retcode;

  int64_t temp = 1; /* Currently only choose the first record */
  //double rms = 0;

  /* Process given parameters (command line and parameter file) */
  if (parse_parameter (argc, argv) < 0)
    return -1;

  /* Set flag to validate CRCs when reading */
  flags |= MSF_VALIDATECRC;

  /* Set flag to unpack data if printing samples */
  if (printdata)
    flags |= MSF_UNPACKDATA;

  /* Loop over the input file record by record */
  while ((retcode = ms3_readmsr (&msr, inputfile, NULL, NULL,
                                 flags, verbose)) == MS_NOERROR)
  {
    totalrecs++;
    totalsamps += msr->samplecnt;

    msr3_print (msr, ppackets);

    /* Print each data samples and its corresponding RMS value */
    if (printdata && msr->numsamples > 0)
    {
      int line, col, cnt, samplesize;
      int lines = (msr->numsamples / 6) + 1;
      void *sptr;

      if ((samplesize = ms_samplesize (msr->sampletype)) == 0)
      {
        ms_log (2, "Unrecognized sample type: '%c'\n", msr->sampletype);
      }

      for (cnt = 0, line = 0; line < lines; line++)
      {
        for (col = 0; col < 6; col++)
        {
          if (cnt < msr->numsamples)
          {
            sptr = (char *)msr->datasamples + (cnt * samplesize);

            if (msr->sampletype == 'i')
            {
              ms_log (0, "%10d  ", *(int32_t *)sptr);
            }
            else if (msr->sampletype == 'f')
            {
              ms_log (0, "%10.8g  ", *(float *)sptr);
            }
            else if (msr->sampletype == 'd')
            {
              ms_log (0, "%10.10g  ", *(double *)sptr);
            }

            cnt++;
          }
        }
        ms_log (0, "\n");
      }
    }

    /* <--- */
    if (totalrecs >= temp)
    {
      break;
    }
  }

  /* Make sure everything is cleaned up */
  ms3_readmsr (&msr, NULL, NULL, NULL, 0, 0);

  if (basicsum)
  {
    ms_log (1, "Records: %" PRId64 ", Samples: %" PRId64 "\n",
            totalrecs, totalsamps);
  }

  return 0;
}

/***************************************************************************
 * parse_parameter():
 * Parse the command line parameters.
 *
 * Return 0 on success, and -1 on failure.
 ***************************************************************************/
static int
parse_parameter (int argcount, char **argvec)
{
  int counter;

  /* Try to parse all command line arguments. */
  for (counter = 1; counter < argcount; counter++)
  {
    if (strcmp (argvec[counter], "-h") == 0)
    {
      show_usage ();
      exit (0);
    }
    else if (strncmp (argvec[counter], "-D", 2) == 0)
    {
      printdata = 1;
    }
    else if (strcmp (argvec[counter], "-s") == 0)
    {
      basicsum = 1;
    }
    else if (strncmp (argvec[counter], "-", 1) == 0 &&
             strlen (argvec[counter]) > 1)
    {
      ms_log (2, "Unknown option: %s\n", argvec[counter]);
      exit (1);
    }
    else if (inputfile == 0)
    {
      inputfile = argvec[counter];
    }
    else
    {
      ms_log (2, "Unknown option: %s\n", argvec[counter]);
      exit (1);
    }
  }

  /* Make sure an inputfile was specified */
  if (!inputfile)
  {
    ms_log (2, "No input file was specified\n\n");
    ms_log (1, "%s version %s\n\n", PROGRAMNAME, VERSION);
    ms_log (1, "Try %s -h for usage\n", PROGRAMNAME);
    exit (1);
  }

  return 0;
} /* End of parse_parameter() */

/***************************************************************************
 * show_usage():
 * Print the usage message and exit.
 ***************************************************************************/
static void
show_usage (void)
{
  fprintf (stderr, "%s version: %s\n\n", PROGRAMNAME, VERSION);
  fprintf (stderr, "Usage: %s [options] file\n\n", PROGRAMNAME);
  fprintf (stderr,
           " ## Options ##\n"
           " -h             Show this usage message\n"
           " -D             Print all sample values\n"
           " -s             Print a basic summary after processing a file\n"
           "\n"
           " file           File of miniSEED records\n"
           "\n");
} /* End of show_usage() */
