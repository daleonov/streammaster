/*
 * example1.c
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
#include <time.h>
#include "bs1770_ctx.h"

// assume 2 channel interleaved/48 kHz/32 bit float raw pcm.
#define CHANNELS        2
#define RATE            48000

#define NUM_SAMPLES     4096
#define BUF_SIZE        (CHANNELS*NUM_SAMPLES)

int main(int argc, char **argv)
{
  /*
   * Suffix <sfx>:
   *   * i16: int16_t, i.e. 16 bit integer
   *   * i32: int32_t, i.e. 32 bit integer
   *   * f32: float, i.e. 32 bit float
   *   * f64: double, i.e. 64 bit float
   *
   * Change
   *   * bs1770_<sfx>_t,
   *   * bs1770_sample_<sfx>_t, and
   *   * bs1770_ctx_add_samples_i_<sfx>()
   * accordingly.
   */
  static bs1770_f32_t buf[BUF_SIZE];

  bs1770_ctx_t *ctx;
  bs1770_sample_f32_t sample;
  double lufs,lra;
  int i;
  FILE *f;
  size_t n;
  clock_t t=clock();

  // print out library version.
  fprintf(stderr,"lib1770 v%s\n",bs1770_version());

  // open a 1770 context.
  if (NULL==(ctx=bs1770_ctx_open_default(1)))
    goto cleanup;

  // for each track ...
  for (i=1;i<argc;++i) {
    // open the raw pcm stream.
    if (NULL==(f=fopen(argv[i],"rb")))
      continue;

    // display the name of the current track.
    fprintf(stderr, "%s: ", argv[i]);
    fflush(stderr);

    // while there are samples ...
    while (0<(n=fread(buf,sizeof buf[0],BUF_SIZE,f))) {
      // consume the samples read.
      bs1770_ctx_add_samples_i_f32(ctx,0,RATE,CHANNELS,buf,NUM_SAMPLES);
    }

    // print out track statistics (implicitely clears).
    lufs=bs1770_ctx_track_lufs_r128(ctx,0);
    lra=bs1770_ctx_track_lra_default(ctx,0);
    fprintf(stderr, "%.1f LUFS, %.1f LU.\n",lufs,lra);

    // close the raw stream.
    fclose(f);
  }

  // print out album statistics (implicitely clears).
  lufs=bs1770_ctx_album_lufs_r128(ctx);
  lra=bs1770_ctx_album_lra_default(ctx);
  fprintf(stderr, "ALBUM: %.1f LUFS, %.1f LU.\n",lufs,lra);
cleanup:
  // close the 1770 context.
  if (NULL!=ctx)
    bs1770_ctx_close(ctx);

  fprintf(stderr,"Duration: %.0f ms.\n",(double)(clock()-t));

  return 0;
}
