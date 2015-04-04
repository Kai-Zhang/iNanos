#ifndef __MESSAGE_H__
#define __MESSAGE_H__
#include "common/types.h"
#include "lib/list.h"

#define ANY -1

typedef struct Message {
	pid_t src, dst;
	union {
		int type;
		int ret;
	};
	union {
		int i[5];
		struct {
			pid_t req_pid;
			int dev_id;
			void *buf;
			off_t offset;
			size_t len;
		};
	};
	struct list_head list;
} Message;

void send(pid_t dst, Message *msg);
void receive(pid_t src,Message *msg);

#endif
