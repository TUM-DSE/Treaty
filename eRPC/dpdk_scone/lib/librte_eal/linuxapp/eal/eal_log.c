/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_spinlock.h>
#include <rte_log.h>

#include "eal_private.h"

/*
 * default log function
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static ssize_t
console_log_write(__attribute__((unused)) void *c, const char *buf, size_t size)
{
	char copybuf[BUFSIZ + 1];
	ssize_t ret;
	uint32_t loglevel;

	/* write on stdout */
	ret = fwrite(buf, 1, size, stdout);
	fflush(stdout);

	/* truncate message if too big (should not happen) */
	if (size > BUFSIZ)
		size = BUFSIZ;

	/* Syslog error levels are from 0 to 7, so subtract 1 to convert */
	loglevel = rte_log_cur_msg_loglevel() - 1;
	memcpy(copybuf, buf, size);
	copybuf[size] = '\0';

	/* write on syslog too */
	syslog(loglevel, "%s", copybuf);

	return ret;
}
#pragma GCC diagnostic pop

/*
 * set the log to default function, called during eal init process,
 * once memzones are available.
 */
int
rte_eal_log_init(const char *id, int facility)
{
	openlog(id, LOG_NDELAY | LOG_PID, facility);

	eal_log_set_default(stdout);

	return 0;
}
