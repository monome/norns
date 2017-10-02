
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "device_input.h"
#include "events.h"

#define TEST_NULL_AND_FREE(p) if( (p) != NULL ) { free(p); } \
  else { printf("error: double free in device_input.c \n"); }

static void add_types(struct dev_input *d) {
  struct libevdev *dev = d->dev;
  d->types = calloc( EV_MAX, sizeof(int) );
  d->num_types = 0;
  for (int i = 0; i < EV_MAX; i++) {
    if ( libevdev_has_event_type(dev, i) ) {
      d->types[d->num_types++] = i;
    }
  }
  d->types = realloc( d->types, d->num_types * sizeof(int) );
}

static void add_codes(struct dev_input *d) {
  struct libevdev *dev = d->dev;
  d->num_codes = calloc( d->num_types, sizeof(int) );
  d->codes = calloc( d->num_types, sizeof(dev_code_t *) );
  for(int i = 0; i < d->num_types; i++) {
    int max_codes, num_codes = 0;
    uint16_t *codes;
    int type = d->types[i];
    switch(type) {
    case EV_KEY:
      max_codes = KEY_MAX;
      break;
    case EV_REL:
      max_codes = REL_MAX;
      break;
    case EV_ABS:
      max_codes = ABS_MAX;
      break;
    case EV_LED:
      max_codes = LED_MAX;
      break;
    default:
      max_codes = 0;
    }
    codes = calloc( max_codes, sizeof(dev_code_t) );

    for(int code = 0; code < max_codes; code++) {
      if( libevdev_has_event_code(dev, type, code) ) {
        codes[num_codes++] = code;
      }
    }
    codes = realloc( codes, num_codes * sizeof(dev_code_t) );
    d->num_codes[i] = num_codes;
    d->codes[i] = codes;
  }
}

static void dev_input_print(struct dev_input *d ) {
  printf("%s\n", d->base.name);
  for(int i = 0; i < d->num_types; i++) {
    printf( "  %d : %d (%s) : \n",
            i,
            d->types[i],
            libevdev_event_type_get_name(d->types[i]) );
    for(int j = 0; j < d->num_codes[i]; j++) {
      printf( "      %d : %d (%s)\n",
              j, d->codes[i][j],
              libevdev_event_code_get_name(d->types[i], d->codes[i][j]) );
    }
  }
  fflush(stdout);
}

int dev_input_init(void *self, bool print) {
  struct dev_input *d = (struct dev_input *)self;
  struct dev_common *base = (struct dev_common *)self;
  struct libevdev *dev = NULL;
  int ret = 1;
  int fd = open(d->base.path, O_RDONLY);
  const char *name;

  if (fd < 0) {
    printf("failed to open input device: %s\n", d->base.path);
    fflush(stdout);
    return -1;
  }

  ret = libevdev_new_from_fd(fd, &dev);
  if (ret < 0) {
    printf( "failed to init libevdev (%s)\n", strerror(-ret) );
    fflush(stdout);
    return ret;
  }

  d->dev = dev;

  add_types(d);
  add_codes(d);

  d->vid = libevdev_get_id_vendor(dev);
  d->pid = libevdev_get_id_product(dev);

  name = libevdev_get_name(dev);
  base->name = calloc( strlen(name) + 1, sizeof(char) );
  strcpy(base->name, name);
  // FIXME: probably not the ideal way to create serial string
  base->serial = calloc( 12, sizeof(char) );
  sprintf(base->serial, "%04X%04X", d->vid, d->pid);

  if(print ) { dev_input_print(d); }

  base->start =  &dev_input_start;
  base->deinit = &dev_input_deinit;
  return 0;
}

static void handle_event(struct dev_input *dev, struct input_event *inev) {
  union event_data *ev = event_data_new(EVENT_INPUT_EVENT);
  ev->input_event.id = dev->base.id;
  ev->input_event.type = inev->type;
  ev->input_event.code = inev->code;
  ev->input_event.value = inev->value;
  event_post(ev);
}

void *dev_input_start(void *self) {
  struct dev_input *di = (struct dev_input *)self;
  int rc = 1;
  do {
    struct input_event ev;
    rc = libevdev_next_event(di->dev,
                             LIBEVDEV_READ_FLAG_NORMAL
                             | LIBEVDEV_READ_FLAG_BLOCKING,
                             &ev);

    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      // dropped...
      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        rc = libevdev_next_event(di->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
      }
      // re-synced...
    } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
      // filter out sync and msc events
      if( !( ( ev.type == EV_SYN) || ( ev.type == EV_MSC) ) ) {
        handle_event(di, &ev);
      }
    }
  } while ( rc == LIBEVDEV_READ_STATUS_SYNC
            || rc == LIBEVDEV_READ_STATUS_SUCCESS
            || rc == -EAGAIN );
  return NULL;
}

void dev_input_deinit(void *self) {
  struct dev_input *di = (struct dev_input *)self;
  for(int i = 0; i < di->num_types; i++) {
    TEST_NULL_AND_FREE(di->codes[i]);
  }
  TEST_NULL_AND_FREE(di->codes);
  TEST_NULL_AND_FREE(di->types);
}
