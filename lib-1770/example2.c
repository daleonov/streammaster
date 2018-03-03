/*
 * example2.c
 * Copyright (C) 2011, 2012 Peter Belkner <pbelkner@snafu.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
/*
 * ctx=bs1770_ctx_open()
 *
 * for each album {
 *   for each track {
 *     while there are samples
 *       bs1770_ctx_add_samples(ctx, rate, channels, samples)
 *
 *     bs1770_ctx_track_lufs(ctx)
 *   }
 *
 *   bs1770_ctx_album_lufs(ctx)
 * }
 *
 * bs1770_ctx_close(ctx)
 */
#include <stdio.h>

#if defined (WIN32)
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "bs1770_ctx.h"

// assume 2 channel interleaved/48 kHz/32 bit float raw pcm.
#define CHANNELS    2
#define RATE        48000

// number of parallel processing threads.
#define NSLOTS      6

// information that has to be available for a processing thread.
typedef struct slot {
  struct slot *next;

  int i;
  bs1770_ctx_t *ctx;
  HANDLE hEvent;
  CRITICAL_SECTION *cs;
  struct slot **done_list;

  char *path;
  HANDLE hThread;

  size_t size;
  size_t n;
  bs1770_f32_t *buf;
} slot_t;

// read a file from disk and store it's context in a memory buffer.
int read_file(slot_t *slot)
{
  int error=1;
  struct stat buf;
  FILE *f=NULL;
  size_t size;

  if (0!=stat(slot->path,&buf))
    goto cleanup;

  slot->n=buf.st_size/sizeof(bs1770_f32_t);

  if (NULL==(f=fopen(slot->path,"rb")))
    goto cleanup;

  if (slot->size<buf.st_size) {
    bs1770_f32_t *tmp;

    if (NULL==(tmp=malloc(buf.st_size)))
      goto cleanup;

    if (NULL!=slot->buf)
      free(slot->buf);

    slot->buf=tmp;
    slot->size=buf.st_size;
  }

  if (slot->n!=(size=fread((char *)slot->buf,sizeof(bs1770_f32_t),slot->n,f)))
    goto cleanup;

  error=0;
cleanup:
  if (NULL!=f)
    fclose(f);

  return error;
}

static DWORD WINAPI thread(LPVOID p)
{
  slot_t *slot=p;
  bs1770_sample_f32_t sample;
  int channel=0;  // offset in sample vector.
  int j;

  for (j=0;j<slot->n;++j) {
    // build up a sample vector accross the channels.
    // NOTE: client code may apply a "channel map" right here.
    //   lib1770 assumes the order "left", "right", "centre",
    //   "left sorround", "right sorround".
    if (channel<BS1770_MAX_CHANNELS)
      sample[channel]=slot->buf[j];

      // if the sample vector is done add it to the 1770 statistics.
      if (++channel==CHANNELS) {
        bs1770_ctx_add_sample_f32(slot->ctx,slot->i,RATE,CHANNELS,sample);
        channel=0;
      }
  }

  // link this slot into the list of processed slots.
  EnterCriticalSection(slot->cs);
  slot->next=*slot->done_list;
  *slot->done_list=slot;
  LeaveCriticalSection(slot->cs);

  // signal the main loop that we're done.
  SetEvent(slot->hEvent);

  return 0;
}

int main(int argc, char **argv)
{
  bs1770_ctx_t *ctx;
  HANDLE hEvent=NULL;
  CRITICAL_SECTION cs;
  slot_t slot[NSLOTS];
  slot_t *cur_slot;
  slot_t *free_list=NULL;
  slot_t *done_list=NULL;
  int i,n;
  clock_t t=clock();

  // print out library version.
  fprintf(stderr, "lib1770 v%s\n", bs1770_version());

  // open a 1770 context.
  if (NULL==(ctx=bs1770_ctx_open_default(NSLOTS)))
    goto cleanup;

  InitializeCriticalSection(&cs);

  hEvent=CreateEvent(
    NULL,           // lpEventAttributes
    FALSE,          // bManualReset
    FALSE,          // bInitialState
    NULL            // lpName
  );

  if (NULL==hEvent)
    goto cleanup;

  // initialize the array of slots.
  for (i=0;i<NSLOTS;++i) {
    cur_slot=slot+i;

    // link into free list.
    cur_slot->next=free_list;
    free_list=cur_slot;

    // initialize
    cur_slot->i=i;
    cur_slot->ctx=ctx;
    cur_slot->hEvent=hEvent;
    cur_slot->cs=&cs;
    cur_slot->done_list=&done_list;

    cur_slot->path=NULL;
    cur_slot->size=0;
    cur_slot->buf=NULL;
  }

  // for each track ...
  i=1; // index of next argument to be processed.
  n=0; // number of running processing threads.

  while (i<argc||0<n) {
    // get a slot from the free list, read a file, and start a
    // processing thread.
    // hopefully no lock is needed when testing whether there are
    // results because the test should be an atomic operation.
    while (i<argc&&NULL!=free_list&&NULL==done_list) {
      // get a slot from the free list.
      cur_slot=free_list;
      free_list=cur_slot->next;

      cur_slot->next=NULL;
      cur_slot->path=argv[i];

      // read a file ...
      if (0==read_file(cur_slot)) {
        // in case of success start a processing thread.
        cur_slot->hThread=CreateThread(
          NULL,       // lpThreadAttributes (0 / CREATE_SUSPENDED)
          0,          // dwStackSize
          thread,     // lpStartAddress
          cur_slot,   // lpParameter
          0,          // dwCreationFlags
          NULL        // lpThreadId
        );

        // increase the number of running processing threads.
        ++n;
      }
      else {
        // in case of an error put the slot back into the free list.
        cur_slot->next=free_list;
        free_list=cur_slot;
      }

      // increase the index of processed arguments.
      ++i;
    }

    // wait until a processing thread is finished.
    EnterCriticalSection(&cs);

    while (NULL==done_list) {
      LeaveCriticalSection(&cs);
      WaitForSingleObject(hEvent,INFINITE);
      EnterCriticalSection(&cs);
    }

    // get a slot out of the processed list.
    cur_slot=done_list;
    done_list=cur_slot->next;
    LeaveCriticalSection(&cs);

    // wait for the thread to be finished.
    WaitForSingleObject(cur_slot->hThread,INFINITE);
    CloseHandle(cur_slot->hThread);

    // print out track statistics (implicitely clears).
    fprintf(stderr, "%s: %.1f LUFS, %.1f LU.\n",
        cur_slot->path,
        bs1770_ctx_track_lufs_r128(ctx,cur_slot->i),
        bs1770_ctx_track_lra_default(ctx,cur_slot->i));

    // put the slot back into the free list.
    cur_slot->next=free_list;
    free_list=cur_slot;

    // decrease the number of running processing threads.
    --n;
  }

  // print out album statistics (implicitely clears).
  fprintf(stderr, "ALBUM: %.1f LUFS, %.1f LU.\n",
      bs1770_ctx_album_lufs_r128(ctx),
      bs1770_ctx_album_lra_default(ctx));
cleanup:
  // cleanup the array of slots.
  for (i=NSLOTS-1;0<=i;--i) {
    cur_slot=slot+i;

    if (NULL!=cur_slot->buf)
      free(cur_slot->buf);
  }

  // close the event object.
  if (NULL!=hEvent)
    CloseHandle(hEvent);

  // close the 1770 context.
  if (NULL!=ctx)
    bs1770_ctx_close(ctx);

  fprintf(stderr, "Duration: %.0f ms.\n", (double)(clock()-t));

  return 0;
}
#else
int main(int argc, char **argv)
{
  fprintf(stderr, "Sorry, not implemented yet.\n");
  exit(1);
}
#endif
