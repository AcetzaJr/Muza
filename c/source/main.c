#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define mznull NULL

struct mzg {
  double scale[12];
  double *samp;
  int fram;
  int chan;
};

struct mzg mzgvar;

struct mzcmd {
  double freq;
  double time;
  double dur;
  double amp;
};

double mzequaltempered(double note, double basefreq) {
  return basefreq * pow(2, note / 12);
}

void mzinit() {
  mzgvar.scale[0] = (double)1;
  mzgvar.scale[1] = (double)256 / 243;
  mzgvar.scale[2] = (double)9 / 8;
  mzgvar.scale[3] = (double)32 / 27;
  mzgvar.scale[4] = (double)81 / 64;
  mzgvar.scale[5] = (double)4 / 3;
  mzgvar.scale[6] = mzequaltempered(6, 1);
  mzgvar.scale[7] = (double)3 / 2;
  mzgvar.scale[8] = (double)128 / 81;
  mzgvar.scale[9] = (double)27 / 16;
  mzgvar.scale[10] = (double)16 / 9;
  mzgvar.scale[11] = (double)243 / 128;
  mzgvar.samp = mznull;
  mzgvar.fram = 0;
  mzgvar.chan = 2;
}

double mznote(int ration, int octave) {
  return mzgvar.scale[ration] * pow(2, octave);
}

void mzend(const char *path) {}

void mzadd(struct mzcmd cmd) {}

int main() {
  mzinit();
  struct mzcmd cmd;
  cmd.freq = mznote(0, 4);
  cmd.time = 0;
  cmd.dur = 1;
  cmd.amp = 1;
  mzadd(cmd);
  mzend("out/song.wav");
}
