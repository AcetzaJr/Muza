#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

const char *mzspncstr = "%s:%d In function '%s'";

#define MZSPNC(ARG) mzpnc(ARG, 1, mzspncstr, __FILE__, __LINE__, __func__)

void mzpnc(bool cnd, int code, const char *msg, ...) {
  if (!cnd) {
    return;
  }
  printf("> Panic: ");
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf("\n");
  fflush(stdout);
  exit(code);
}

typedef struct {
  const char *midi_dev;
  const char *pcm_dev;
  snd_rawmidi_t *midi_hnd;
  snd_pcm_t *pcm_hnd;
  pthread_mutex_t mtx;
  pthread_t midithrd;
  pthread_t pcmthrd;
  int channels;
  int frate;
  bool work;
} mzg_t;

mzg_t mzg = {.pcm_dev = "default",
             .midi_dev = "hw:CARD=USBMIDI",
             .work = true,
             .channels = 2,
             .frate = 48'000};

void *mzpcm(void *) { MZSPNC(snd_pcm_drain(msg.pcm_hnd) < 0); }

void *mzmidi(void *) {
  struct timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 1'000'000;
  unsigned char buf[3];
  ssize_t read;
  while (true) {
    while ((read = snd_rawmidi_read(mzg.midi_hnd, buf, sizeof(buf))) > 0) {
      printf("> Message read with %li bytes\n", read);
      for (ssize_t i = 0; i < read; i++) {
        printf("[%02x]", buf[i]);
      }
      printf("\n");
    }
    MZSPNC(read != -EAGAIN);
    MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
    if (!mzg.work) {
      MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
      break;
    }
    MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
    MZSPNC(nanosleep(&time, NULL) == -1);
  }
  return NULL;
}

void mzmidiinit() {
  MZSPNC(snd_rawmidi_open(&mzg.midi_hnd, NULL, mzg.midi_dev, 0) < 0);
  MZSPNC(snd_rawmidi_nonblock(mzg.midi_hnd, 1) < 0);
  MZSPNC(pthread_create(&mzg.midithrd, NULL, mzmidi, NULL) != 0);
}

void mzmidiend() {
  MZSPNC(pthread_join(mzg.midithrd, NULL) != 0);
  MZSPNC(snd_rawmidi_close(mzg.midi_hnd) < 0);
}

void mzinit() { pthread_mutex_init(&mzg.mtx, NULL); }

void mzpcminit() {
  MZSPNC(snd_pcm_open(&mzg.pcm_hnd, mzg.pcm_dev, SND_PCM_STREAM_PLAYBACK, 0) <
         0);
  MZSPNC(snd_pcm_set_params(mzg.pcm_hnd, SND_PCM_FORMAT_FLOAT64,
                            SND_PCM_ACCESS_RW_INTERLEAVED, mzg.channels,
                            mzg.frate, 1, 25'000) < 0);
  MZSPNC(pthread_create(&mzg.pcmthrd, NULL, mzpcm, NULL) != 0);
}

void mzwaitenter() {
  printf("> Press enter to exit\n");
  while (getchar() != '\n') {
  }
  MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
  mzg.work = false;
  MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
}

int main(void) {
  mzinit();
  mzwaitenter();
  return 0;
}