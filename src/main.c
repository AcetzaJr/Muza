#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define MZKEYS 128
#define MZBLOCKS 3
#define MZFRATE 48'000
#define MZCHAN 2
#define MZLATENCYMILLI 25
#define MZLATENCY (MZLATENCYMILLI) * 1'000
#define MZFRAMES ((MZFRATE) / 1'000) * (MZLATENCYMILLI)
#define MZBUFSIZE (MZCHAN) * (MZFRAMES)
#define MZSPNC(ARG) mzpnc (ARG, 1, mzspncstr, __FILE__, __LINE__, __func__)

const char *mzspncstr = "%s:%d In function '%s'";

void
mzpnc (bool cnd, int code, const char *msg, ...)
{
  if (!cnd)
    {
      return;
    }
  printf ("> Panic: ");
  va_list args;
  va_start (args, msg);
  vprintf (msg, args);
  va_end (args);
  printf ("\n");
  fflush (stdout);
  exit (code);
}

typedef enum
{
  MZIDLE,
  MZATTACK,
  MZHOLD,
  MZDECAY,
  MZSUSTAIN,
  MZRELEASE
} mzphase_t;

typedef struct
{
  size_t frame;
  mzphase_t phase;
} mzstate_t;

typedef struct
{
  mzstate_t states[MZKEYS];
} mzsynth_t;

typedef struct
{
  float buffer[MZBUFSIZE];
  bool ready;
} mzblock_t;

typedef struct
{
  mzblock_t blocks[MZBLOCKS];
  int current;
  int first;
  int last;
  const char *mididev;
  const char *pcmdev;
  snd_rawmidi_t *midihnd;
  snd_pcm_t *pcmhnd;
  pthread_mutex_t mtx;
  pthread_t midithrd;
  pthread_t pcmthrd;
  int channels;
  int frate;
  bool work;
} mzg_t;

mzg_t mzg = { .pcmdev = "default", .mididev = "hw:CARD=USBMIDI", .work = true, .channels = MZCHAN, .frate = MZFRATE };

bool
mzworking ()
{
  bool working;
  MZSPNC (pthread_mutex_lock (&mzg.mtx) != 0);
  working = mzg.work;
  MZSPNC (pthread_mutex_unlock (&mzg.mtx) != 0);
  return working;
}

bool
mzswap ()
{
  if (!mzg.blocks[mzg.last].ready)
    {
      return false;
    }
  mzg.blocks[mzg.current].ready = false;
  mzg.current = (mzg.current + 1) % MZBLOCKS;
  mzg.last = (mzg.last + 1) % MZBLOCKS;
  mzg.first = (mzg.first + 1) % MZBLOCKS;
  return true;
}

float *
mznext ()
{
  MZSPNC (!mzswap ());
  return mzg.blocks[mzg.current].buffer;
}

void *
mzpcm (void *)
{
  while (mzworking ())
    {
      printf ("pcm\n");
      snd_pcm_sframes_t frames = snd_pcm_writei (mzg.pcmhnd, mznext (), MZFRAMES);
      if (frames < 0)
        frames = snd_pcm_recover (mzg.pcmhnd, frames, 0);
      mzpnc (frames < 0, 1, "snd_pcm_writei failed: %s\n", snd_strerror (frames));
      mzpnc (frames != MZFRAMES, 1, "wrong write (expected %i, wrote %li)\n", MZFRAMES, frames);
    }
  MZSPNC (snd_pcm_drain (mzg.pcmhnd) < 0);
  return NULL;
}

void *
mzmidi (void *)
{
  unsigned char buf[3];
  ssize_t read;
  while (mzworking ())
    {
      while ((read = snd_rawmidi_read (mzg.midihnd, buf, sizeof (buf))) > 0)
        {
          printf ("> Message read with %li bytes\n", read);
          for (ssize_t i = 0; i < read; i++)
            {
              printf ("[%02x]", buf[i]);
            }
          printf ("\n");
        }
      MZSPNC (read != -EAGAIN);
    MZSPNC(usleep(1'000) == -1);
    }
  return NULL;
}

void
mzmidiinit ()
{
  MZSPNC (snd_rawmidi_open (&mzg.midihnd, NULL, mzg.mididev, 0) < 0);
  MZSPNC (snd_rawmidi_nonblock (mzg.midihnd, 1) < 0);
  MZSPNC (pthread_create (&mzg.midithrd, NULL, mzmidi, NULL) != 0);
}

void
mzmidiend ()
{
  MZSPNC (pthread_join (mzg.midithrd, NULL) != 0);
  MZSPNC (snd_rawmidi_close (mzg.midihnd) < 0);
}

void
mzinit ()
{
  pthread_mutex_init (&mzg.mtx, NULL);
  for (int b = 0; b < MZBLOCKS; b++)
    {
      for (int s = 0; s < MZBUFSIZE; s++)
        {
          mzg.blocks[b].buffer[s] = 0;
        }
      mzg.blocks[b].ready = true;
    }
}

void
mzpcminit ()
{
  MZSPNC (snd_pcm_open (&mzg.pcmhnd, mzg.pcmdev, SND_PCM_STREAM_PLAYBACK, 0) < 0);
  MZSPNC(snd_pcm_set_params(mzg.pcmhnd, SND_PCM_FORMAT_FLOAT,
                            SND_PCM_ACCESS_RW_INTERLEAVED, mzg.channels,
                            mzg.frate, 1, 25'000) < 0);
  MZSPNC(pthread_create(&mzg.pcmthrd, NULL, mzpcm, NULL) != 0);
}

void
mzpcmend ()
{
  MZSPNC (pthread_join (mzg.pcmthrd, NULL) != 0);
  MZSPNC (snd_pcm_close (mzg.pcmhnd) < 0);
}

void
mzwaitenter ()
{
  printf ("> Press enter to exit\n");
  while (getchar () != '\n')
    {
    }
  MZSPNC (pthread_mutex_lock (&mzg.mtx) != 0);
  mzg.work = false;
  MZSPNC (pthread_mutex_unlock (&mzg.mtx) != 0);
}

int
main (void)
{
  mzinit ();
  mzpcminit ();
  mzwaitenter ();
  mzpcmend ();
  return 0;
}
