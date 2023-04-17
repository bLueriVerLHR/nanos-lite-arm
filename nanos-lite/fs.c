#include "fs.h"

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();

size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

static size_t disk_sz = 0;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_END};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

size_t valid_read(void *buf, size_t offset, size_t len) {
  return ramdisk_read(buf, offset, len);
}

size_t valid_write(const void *buf, size_t offset, size_t len) {
  return ramdisk_write(buf, offset, len);
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]    = {"stdin",           0, 0, invalid_read,   invalid_write },
  [FD_STDOUT]   = {"stdout",          0, 0, invalid_read,   serial_write  },
  [FD_STDERR]   = {"stderr",          0, 0, invalid_read,   serial_write  },
  [FD_FB]       = {"/dev/fb",         0, 0, invalid_read,   fb_write      },
  [FD_EVENTS]   = {"/dev/events",     0, 0, events_read,    invalid_write },
  [FD_DISPINFO] = {"/proc/dispinfo",  0, 0, dispinfo_read,  invalid_write },
// #include "files.h"
};

#define FT_FIX_SZ sizeof(file_table) / sizeof(*file_table)

static size_t file_offset_table[FT_FIX_SZ] = {0};

void init_fs() {
  disk_sz = get_ramdisk_size();

  file_table[FD_FB].size = 400 * 300 * 4;
}


int fs_open(const char *pathname, int flags, int mode) {
  // Log("Opening file: %s", pathname);
  int i = 0;
  for (i = 0; i < FD_END; ++i) {
    if (!strcmp(pathname, file_table[i].name)) {
      return i;
    }
  }
  for (i = FD_END; i < FT_FIX_SZ; ++i) {
    if (!strcmp(pathname, file_table[i].name)) {
      if (flags == O_RDONLY) {
        file_table[i].read = valid_read;
        file_table[i].write = invalid_write;
      } else if (flags == O_WRONLY) {
        file_table[i].read = invalid_read;
        file_table[i].write = valid_write;
      } else if (flags == O_RDWR) {
        file_table[i].read = valid_read;
        file_table[i].write = valid_write;
      } else {
        file_table[i].read = invalid_read;
        file_table[i].write = invalid_write;
      }
      file_offset_table[i] = 0;
      return i;
    }
  }
  panic("can not file the file");
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd > -1 && fd < FT_FIX_SZ);
  if (fd < FD_END) {
    return file_table[fd].read(buf, file_table[fd].disk_offset + file_offset_table[fd], len);
  }
  if (len + file_offset_table[fd] > file_table[fd].size) {
    len = file_table[fd].size - file_offset_table[fd];
  }
  size_t offset = file_table[fd].read(buf, file_table[fd].disk_offset + file_offset_table[fd], len);
  file_offset_table[fd] += offset;
  return offset;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd > -1 && fd < FT_FIX_SZ);
  if (fd < FD_END) {
    return file_table[fd].write(buf, file_table[fd].disk_offset + file_offset_table[fd], len);
  }
  if (len + file_offset_table[fd] > file_table[fd].size) {
    len = file_table[fd].size - file_offset_table[fd];
  }
  size_t offset = file_table[fd].write(buf, file_table[fd].disk_offset + file_offset_table[fd], len);
  file_offset_table[fd] += offset;
  return offset;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd > -1 && fd < FT_FIX_SZ);
  size_t new_pos = 0;
  switch (whence) {
    case SEEK_SET:
      new_pos = offset;
      break;
    case SEEK_CUR:
      new_pos = file_offset_table[fd] + offset;
      break;
    case SEEK_END:
      new_pos = file_table[fd].size + offset;
      break;
    default:
      return -1;
  }
  if (new_pos > file_table[fd].size) {
    return -1;
  }
  file_offset_table[fd] = new_pos;
  return new_pos;
}

int fs_close(int fd) {
  assert(fd > -1 && fd < FT_FIX_SZ);
  file_table[fd].read  = invalid_read;
  file_table[fd].write = invalid_write;
  file_offset_table[fd] = 0;
  return 0;
}