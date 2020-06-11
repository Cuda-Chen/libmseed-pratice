#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include <libmseed.h>

int
main (int argc, char **argv)
{
  MS3TraceList *mstl        = NULL;
  MS3TraceID *tid           = NULL;
  MS3TraceSeg *seg          = NULL;

  char *mseedfile = NULL;
  const char *output = "temp.mseed";
  uint32_t flags = 0;
  int8_t verbose = 0;
  size_t idx;
  int rv;
  int reclen = 1024; /* Desired maximum record length */

  int64_t unpacked;
  uint8_t samplesize;
  char sampletype;
  size_t lineidx;
  size_t lines;
  int col;
  void *sptr;

  int value = 0;

  if (argc != 2)
  {
    ms_log (2, "Usage: %s <mseedfile>\n", argv[0]);
    return -1;
  }

  /* Simplistic argument parsing */
  mseedfile     = argv[1];

  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  /* Read all miniSEED into a trace list, limiting to selections */
  rv = ms3_readtracelist (&mstl, mseedfile, NULL, 0, flags, verbose);
  if (rv != MS_NOERROR)
  {
    ms_log (2, "Cannot read miniSEED from file: %s\n", ms_errorstr (rv));
    return -1;
  }

  /* Traverse trace list structures and print summary information */
  tid = mstl->traces;
  while (tid)
  {
    ms_log (0, "TraceID for %s (%d), segments: %u\n",
            tid->sid, tid->pubversion, tid->numsegments);

    seg = tid->first;
    while (seg)
    {
      /* Unpack and print samples for this trace segment */
      if (seg->recordlist && seg->recordlist->first)
      {
        /* Determine sample size and type based on encoding of first record */
        ms_encoding_sizetype (seg->recordlist->first->msr->encoding, &samplesize, &sampletype);

        /* Unpack data samples using record list.
         * No data buffer is supplied, so it will be allocated and assigned to the segment.
         * Alternatively, a user-specified data buffer can be provided here. */
        unpacked = mstl3_unpack_recordlist (tid, seg, NULL, 0, verbose);

        if (unpacked != seg->samplecnt)
        {
          ms_log (2, "Cannot unpack samples for %s\n", tid->sid);
        }
        else
        {
          //ms_log (0, "DATA (%" PRId64 " samples) of type '%c':\n", seg->numsamples, seg->sampletype);

          if (sampletype == 'a')
          {
            printf ("%*s",
                    (seg->numsamples > INT_MAX) ? INT_MAX : (int)seg->numsamples,
                    (char *)seg->datasamples);
          }
          else
          {
            lines = (unpacked / 6) + 1;

            for (idx = 0, lineidx = 0; lineidx < lines; lineidx++)
            {
              for (col = 0; col < 6 && idx < seg->numsamples; col++)
              {
                sptr = (char *)seg->datasamples + (idx * samplesize);

                if (sampletype == 'i')
                {
                  //data[idx] = (double)(*(int32_t *)sptr);
                  (*(int32_t *)sptr) = value;
                }
                else if (sampletype == 'f')
                {
                  //data[idx] = (double)(*(float *)sptr);
                  (*(float *)sptr) = value;
                }
                else if (sampletype == 'd')
                {
                  //data[idx] = (double)(*(double *)sptr);
                  (*(double *)sptr) = value;
                }
                idx++;
              }
            }
          }
        }
      }

      seg = seg->next;
    }

    tid = tid->next;
  }

  /* Write to output miniSeed file */
  int64_t packedrecords = mstl3_writemseed(mstl, output, 1, reclen,
                                           DE_INT32, MSF_FLUSHDATA, verbose); 

  /* Make sure everything is cleaned up */
  if (mstl)
    mstl3_free (&mstl, 0);

  return 0;
}
