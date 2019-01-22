#define	FIX_MAX_ELEMENTS_NUMBER	32
#define	FIX_MAX_LINE_LENGTH	256

#include <libtrading/proto/fix_message.h>

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct felem {
	struct fix_message msg;
	char buf[FIX_MAX_LINE_LENGTH];
};

struct fcontainer {
	unsigned long nr;
	unsigned long cur;
	struct felem felems[FIX_MAX_ELEMENTS_NUMBER];
};

void fcontainer_free(struct fcontainer *container);
struct felem *next_elem(struct fcontainer *self);
struct felem *cur_elem(struct fcontainer *self);
struct felem *add_elem(struct fcontainer *self);
int init_elem(struct felem *elem, char *line);
struct fcontainer *fcontainer_new(void);

int script_read(FILE *stream, struct fcontainer *server, struct fcontainer *client);
int fmsgcmp(struct fix_message *expected, struct fix_message *actual);
void fprintmsg_iov(FILE *stream, struct fix_message *msg);
void fprintmsg(FILE *stream, struct fix_message *msg);
void msg_string(char *buf, struct fix_message *msg);
