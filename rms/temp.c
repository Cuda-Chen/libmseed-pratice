#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <math.h>

#include <libmseed.h>

int main(int argc, char **argv)
{
  MS3TraceList *mstl = NULL;
  MS3TraceID *tid = NULL;
  MS3TraceSeg *seg = NULL;
  //MS3RecordPtr *recptr = NULL;

  char *mseedfile = NULL;
  char starttimestr[30];
  char endtimestr[30];
  uint32_t flags = 0;
  int8_t verbose = 0;
  size_t idx;
  int rv;

  char printdata = 'd';
  int64_t unpacked;
  uint8_t samplesize;
  char sampletype;
  size_t lineidx;
  size_t lines;
  int col;
  void *sptr;

  int32_t *data = (int32_t *)malloc(sizeof(int32_t) * 6);
  double rms = 0.0f;

  if (argc < 2)
  {
    ms_log (2, "Usage: %s <mseedfile> [-v] [-d] [-D]\n",argv[0]);
    return -1;
  }

  /* Simplistic argument parsing */
  mseedfile = argv[1];
  for (idx = 2; idx < argc; idx++)
  {
    if (strncmp (argv[idx], "-v", 2) == 0)
      verbose += strspn (&argv[idx][1], "v");
    else if (strncmp (argv[idx], "-d", 2) == 0)
      printdata = 'd';
    else if (strncmp (argv[idx], "-D", 2) == 0)
      printdata = 'D';
  }

#if 0
  if (argc < 2)
  {
    ms_log (2, "Usage: %s <mseedfile> <selectionfile>\n",argv[0]);
    return -1;
  }

  /* Simplistic argument parsing */
  mseedfile = argv[1];
  selectionfile = argv[2];
  /* Read data selections from specified file */
  if (ms3_readselectionsfile (&selections, selectionfile) < 0)
  {
    ms_log (2, "Cannot read data selection file\n");
    return -1;
  }
#endif

  /* Set bit flag to validate CRC */
  flags |= MSF_VALIDATECRC;

  /* Set bit flag to build a record list */
  flags |= MSF_RECORDLIST;

  /* Read all miniSEED into a trace list, limiting to selections */
  rv = ms3_readtracelist (&mstl, mseedfile, NULL, 0, flags, verbose);
#if 0
  rv = ms3_readtracelist_selection (&mstl, mseedfile, NULL,
                                    selections, 0, flags, verbose);
#endif

  if (rv != MS_NOERROR)
  {
    ms_log (2, "Cannot read miniSEED from file: %s\n", ms_errorstr(rv));
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
      if (!ms_nstime2timestr (seg->starttime, starttimestr, ISOMONTHDAY, NANO) ||
          !ms_nstime2timestr (seg->endtime, endtimestr, ISOMONTHDAY, NANO))
      {
        ms_log (2, "Cannot create time strings\n");
        starttimestr[0] = endtimestr[0] = '\0';
      }

      ms_log (0, "  Segment %s - %s, samples: %" PRId64 ", sample rate: %g\n",
              starttimestr, endtimestr, seg->samplecnt, seg->samprate);

#if 0
      if (seg->recordlist)
      {
        ms_log (0, "  Record list:\n");

        /* Traverse record list print summary information */
        recptr = seg->recordlist->first;
        while (recptr)
        {
          ms_log (0, "    RECORD: bufferptr: %p, fileptr: %p, filename: %s, fileoffset: %"PRId64"\n",
                  recptr->bufferptr, recptr->fileptr, recptr->filename, recptr->fileoffset);
          ms_nstime2timestrz (recptr->msr->starttime, starttimestr, ISOMONTHDAY, NANO);
          ms_nstime2timestrz (recptr->endtime, endtimestr, ISOMONTHDAY, NANO);
          ms_log (0, "    Start: %s, End: %s\n", starttimestr, endtimestr);

          recptr = recptr->next;
        }
      }
#endif

      /* Unpack and print samples for this trace segment */
      if (printdata && seg->recordlist && seg->recordlist->first)
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
          ms_log (0, "DATA (%" PRId64 " samples) of type '%c':\n", seg->numsamples, seg->sampletype);

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
                  ms_log (0, "%10d  ", *(int32_t *)sptr);

                else if (sampletype == 'f')
                  ms_log (0, "%10.8g  ", *(float *)sptr);

                else if (sampletype == 'd')
                  ms_log (0, "%10.10g  ", *(double *)sptr);

                data[idx] = *(int32_t *)sptr;

                idx++;
              }
              ms_log (0, "\n");

              if (printdata == 'd')
                break;
            }
          }
        }
      }

      seg = seg->next;
    }

    /* Calculate the RMS */
    puts("=====");
    double mean = 0.0f;
    double sum = 0.0f;
    for(int i = 0; i < 6; i++)
    {
      mean += data[i];
      printf("%10d  ", data[i]);
    }
    printf("\n");
    mean /= 6;
    for(int i = 0; i < 6; i++)
    {
      data[i] = data[i] - mean;
      sum += pow(data[i], 2);
    }
    rms = pow(sum/6, 0.5);
    printf("mean %10.10g, sum %10.10g, RMS: %10.10g\n", mean, sum, rms);

    tid = tid->next;
  }

  /* Make sure everything is cleaned up */
  if (mstl)
    mstl3_free (&mstl, 0);

  free(data);

  return 0;
}
