#ifndef __CR_H__
#define __CR_H__

#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* No need for platform-specific defines since Cosmopolitan handles it */
#define CR_PLUGIN(name) name ".so"

/* Export/import defines are simplified */
#define CR_EXPORT __attribute__((visibility("default")))
#define CR_IMPORT

/* Operation modes */
typedef enum {
  CR_SAFEST = 0, /* Validate address and size */
  CR_SAFE = 1,   /* Validate only size */
  CR_UNSAFE = 2, /* Only validate size fits */
  CR_DISABLE = 3 /* Disable state transfer */
} cr_mode;

/* Plugin operations */
typedef enum { CR_LOAD = 0, CR_STEP = 1, CR_UNLOAD = 2, CR_CLOSE = 3 } cr_op;

/* Failure types */
typedef enum {
  CR_NONE = 0,          /* No error */
  CR_SEGFAULT,          /* Segmentation fault */
  CR_ILLEGAL,           /* Illegal instruction */
  CR_ABORT,             /* Abort */
  CR_MISALIGN,          /* Bus error/misalignment */
  CR_BOUNDS,            /* Array bounds exceeded */
  CR_STACKOVERFLOW,     /* Stack overflow */
  CR_STATE_INVALIDATED, /* State validation failed */
  CR_BAD_IMAGE,         /* Invalid binary */
  CR_INITIAL_FAILURE,   /* Initial plugin crash */
  CR_OTHER,             /* Unknown signal */
  CR_USER = 0x100       /* User error */
} cr_failure;

struct cr_plugin;
typedef int (*cr_plugin_main_func)(struct cr_plugin *ctx, cr_op operation);

/* Plugin context */
typedef struct cr_plugin {
  void *p;
  void *userdata;
  unsigned int version;
  cr_failure failure;
  unsigned int next_version;
  unsigned int last_working_version;
} cr_plugin;

/* Section data */
typedef struct {
  char *ptr;
  intptr_t base;
  int64_t size;
  void *data;
} cr_section;

/* Segment info */
typedef struct {
  char *ptr;
  int64_t size;
} cr_segment;

/* Internal state */
typedef struct cr_internal {
  char *fullname;    /* Full path to plugin */
  char *temppath;    /* Temporary path for versions */
  int64_t timestamp; /* Last modification time */
  void *handle;      /* Dynamic library handle */
  cr_plugin_main_func main;
  cr_segment seg;
  cr_section data[2][2]; /* [type][version] - simplified from original */
  cr_mode mode;
} cr_internal;

/* Core functions */
CR_EXPORT int cr_plugin_update(cr_plugin *ctx, int reloadCheck);
CR_EXPORT int cr_plugin_open(cr_plugin *ctx, const char *fullpath);
CR_EXPORT void cr_plugin_close(cr_plugin *ctx);
CR_EXPORT void cr_set_temporary_path(cr_plugin *ctx, const char *path);

/* Implementation */

static int64_t cr_last_write_time(const char *path) {
  struct stat st;
  if (stat(path, &st) == -1)
    return -1;
  return st.st_mtime;
}

static int cr_exists(const char *path) { return access(path, F_OK) != -1; }

static void *cr_dlopen(const char *path) {
  void *handle = dlopen(path, RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "dlopen error: %s\n", dlerror());
  }
  return handle;
}

static void cr_dlclose(void *handle) {
  if (handle)
    dlclose(handle);
}

static void *cr_dlsym(void *handle, const char *name) {
  return dlsym(handle, name);
}

static int cr_copy_file(const char *src, const char *dst) {
  int fd_src = open(src, O_RDONLY);
  if (fd_src == -1)
    return 0;

  int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_dst == -1) {
    close(fd_src);
    return 0;
  }

  char buf[4096];
  ssize_t bytes_read;
  while ((bytes_read = read(fd_src, buf, sizeof(buf))) > 0) {
    if (write(fd_dst, buf, bytes_read) != bytes_read) {
      close(fd_src);
      close(fd_dst);
      return 0;
    }
  }

  close(fd_src);
  close(fd_dst);
  return 1;
}

/* Version path generation */
static char *cr_version_path(const char *basepath, unsigned int version,
                             const char *temppath, char *buffer) {
  char folder[1024], fname[256], ext[32];
  const char *last_slash = strrchr(basepath, '/');
  const char *last_dot = strrchr(basepath, '.');

  if (!last_slash) {
    strncpy(folder, "", sizeof(folder));
    last_slash = basepath - 1;
  } else {
    size_t len = last_slash - basepath + 1;
    strncpy(folder, basepath, len);
    folder[len] = '\0';
  }

  if (!last_dot || last_dot < last_slash) {
    strncpy(ext, "", sizeof(ext));
    strncpy(fname, last_slash + 1, sizeof(fname));
  } else {
    size_t len = last_dot - (last_slash + 1);
    strncpy(fname, last_slash + 1, len);
    fname[len] = '\0';
    strncpy(ext, last_dot, sizeof(ext));
  }

  if (temppath && *temppath) {
    strncpy(folder, temppath, sizeof(folder));
  }

  snprintf(buffer, 1024, "%s%s%d%s", folder, fname, version, ext);
  return buffer;
}

/* Section validation */
static int cr_plugin_section_validate(cr_plugin *ctx, int type, intptr_t vaddr,
                                      intptr_t base, int64_t size) {
  cr_internal *p = (cr_internal *)ctx->p;
  switch (p->mode) {
  case CR_SAFE:
    return p->data[type][0].size == size;
  case CR_UNSAFE:
    return p->data[type][0].size <= size;
  case CR_DISABLE:
    return 1;
  default: /* CR_SAFEST */
    return p->data[type][0].base == base && p->data[type][0].size == size;
  }
}

/* Section management */
static void cr_plugin_sections_store(cr_plugin *ctx) {
  cr_internal *p = (cr_internal *)ctx->p;
  if (p->mode == CR_DISABLE)
    return;

  for (int i = 0; i < 2; i++) { /* 2 section types */
    if (p->data[i][0].ptr && p->data[i][0].data) {
      memcpy(p->data[i][0].data, p->data[i][0].ptr, p->data[i][0].size);
    }
  }

  /* Backup current data */
  for (int i = 0; i < 2; i++) {
    if (p->data[i][0].ptr) {
      if (p->data[i][1].data)
        free(p->data[i][1].data);
      p->data[i][1].data = malloc(p->data[i][0].size);
      if (p->data[i][1].data) {
        memcpy(p->data[i][1].data, p->data[i][0].data, p->data[i][0].size);
        p->data[i][1].size = p->data[i][0].size;
        p->data[i][1].ptr = p->data[i][0].ptr;
        p->data[i][1].base = p->data[i][0].base;
      }
    }
  }
}

static void cr_plugin_sections_reload(cr_plugin *ctx, int version) {
  cr_internal *p = (cr_internal *)ctx->p;
  if (p->mode == CR_DISABLE)
    return;

  for (int i = 0; i < 2; i++) {
    if (p->data[i][version].data) {
      void *dest = p->data[i][0].ptr;
      if (dest) {
        memcpy(dest, p->data[i][version].data, p->data[i][version].size);
      }
    }
  }
}

/* Signal handling setup */
static sigjmp_buf cr_jmp_buf;
static cr_plugin *cr_current_ctx;

static void cr_signal_handler(int sig, siginfo_t *si, void *unused) {
  (void)si;
  (void)unused;

  if (!cr_current_ctx)
    return;

  switch (sig) {
  case SIGSEGV:
    cr_current_ctx->failure = CR_SEGFAULT;
    break;
  case SIGILL:
    cr_current_ctx->failure = CR_ILLEGAL;
    break;
  case SIGABRT:
    cr_current_ctx->failure = CR_ABORT;
    break;
  case SIGBUS:
    cr_current_ctx->failure = CR_MISALIGN;
    break;
  default:
    cr_current_ctx->failure = CR_OTHER;
    break;
  }

  siglongjmp(cr_jmp_buf, sig);
}

/* Plugin loading/unloading */
static int cr_plugin_unload(cr_plugin *ctx, int rollback, int close) {
  cr_internal *p = (cr_internal *)ctx->p;
  int r = 0;

  if (p->handle) {
    if (!rollback) {
      r = ((cr_plugin_main_func)p->main)(ctx, close ? CR_CLOSE : CR_UNLOAD);
      if (r >= 0) {
        cr_plugin_sections_store(ctx);
      }
    }
    cr_dlclose(p->handle);
    p->handle = NULL;
    p->main = NULL;
  }
  return r;
}

static int cr_plugin_load_internal(cr_plugin *ctx, int rollback) {
  cr_internal *p = (cr_internal *)ctx->p;
  char path_buf[1024];
  const char *file = p->fullname;

  if (cr_exists(file) || rollback) {
    const unsigned int new_version =
        rollback ? ctx->version : ctx->next_version;
    cr_version_path(file, new_version, p->temppath, path_buf);

    int r = cr_plugin_unload(ctx, rollback, 0);
    if (r < 0)
      return 0;

    if (rollback) {
      if (ctx->version == 0) {
        ctx->failure = CR_INITIAL_FAILURE;
        return 0;
      }
      ctx->last_working_version = ctx->version > 0 ? ctx->version - 1 : 0;
    } else {
      ctx->last_working_version = ctx->version;
      if (!cr_copy_file(file, path_buf))
        return 0;
      ctx->next_version = new_version + 1;
    }

    void *handle = cr_dlopen(path_buf);
    if (!handle) {
      ctx->failure = CR_BAD_IMAGE;
      return 0;
    }

    cr_plugin_main_func new_main =
        (cr_plugin_main_func)cr_dlsym(handle, "cr_main");
    if (!new_main)
      return 0;

    p->handle = handle;
    p->main = new_main;
    p->timestamp = cr_last_write_time(file);
    ctx->version = new_version;

    return 1;
  }
  return 0;
}

/* Core API Implementation */
CR_EXPORT int cr_plugin_update(cr_plugin *ctx, int reloadCheck) {
  if (ctx->failure) {
    if (!cr_plugin_load_internal(ctx, 1)) { /* Try rollback */
      return -2;
    }
    if (((cr_plugin_main_func)((cr_internal *)ctx->p)->main)(ctx, CR_LOAD) <
        0) {
      return -2;
    }
    ctx->failure = CR_NONE;
  } else if (reloadCheck) {
    cr_internal *p = (cr_internal *)ctx->p;
    if (cr_last_write_time(p->fullname) > p->timestamp) {
      if (!cr_plugin_load_internal(ctx, 0)) {
        return -2;
      }
      if (((cr_plugin_main_func)p->main)(ctx, CR_LOAD) < 0) {
        ctx->failure = CR_USER;
        return -2;
      }
    }
  }

  /* Setup signal handler for crash protection */
  struct sigaction sa = {0};
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = cr_signal_handler;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);

  cr_current_ctx = ctx;
  int sig = sigsetjmp(cr_jmp_buf, 1);
  if (sig) {
    return -1;
  }

  int r = ((cr_plugin_main_func)((cr_internal *)ctx->p)->main)(ctx, CR_STEP);
  if (r < 0 && !ctx->failure) {
    ctx->failure = CR_USER;
  }
  return r;
}

CR_EXPORT int cr_plugin_open(cr_plugin *ctx, const char *fullpath) {
  if (!fullpath || !cr_exists(fullpath))
    return 0;

  cr_internal *p = malloc(sizeof(cr_internal));
  if (!p)
    return 0;

  memset(p, 0, sizeof(cr_internal));
  p->mode = CR_UNSAFE; /* Default mode */
  p->fullname = strdup(fullpath);

  ctx->p = p;
  ctx->next_version = 1;
  ctx->last_working_version = 0;
  ctx->version = 0;
  ctx->failure = CR_NONE;

  return 1;
}

CR_EXPORT void cr_plugin_close(cr_plugin *ctx) {
  if (!ctx || !ctx->p)
    return;

  cr_plugin_unload(ctx, 0, 1);
  cr_internal *p = (cr_internal *)ctx->p;

  /* Free sections data */
  for (int i = 0; i < 2; i++) {
    for (int v = 0; v < 2; v++) {
      free(p->data[i][v].data);
    }
  }

  /* Delete old versions */
  char path_buf[1024];
  for (unsigned int i = 0; i < ctx->version; i++) {
    cr_version_path(p->fullname, i, p->temppath, path_buf);
    unlink(path_buf);
  }

  free(p->fullname);
  free(p->temppath);
  free(p);
  ctx->p = NULL;
  ctx->version = 0;
}

CR_EXPORT void cr_set_temporary_path(cr_plugin *ctx, const char *path) {
  cr_internal *p = (cr_internal *)ctx->p;
  if (p->temppath)
    free(p->temppath);
  p->temppath = path ? strdup(path) : NULL;
}

#endif /* __CR_H__ */
