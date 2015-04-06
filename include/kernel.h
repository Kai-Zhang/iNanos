#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "common.h"
#include "lib/string.h"
#include "x86/x86.h"
#include "memory.h"
#include "process.h"

size_t dev_read(const char *dev_name, pid_t reqst_pid, void *buf, off_t offset, size_t len);
size_t dev_write(const char *dev_name, pid_t reqst_pid, void *buf, off_t offset, size_t len);

#endif
