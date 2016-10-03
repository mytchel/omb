/*
 *
 * Copyright (c) <2016> Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <fs.h>
#include <stdarg.h>

int
ppipe0(int fd);

int
ppipe1(int fd);

int
filetest(void);

int
commount(char *path);

int
tmpmount(char *path);

int
initmmcs(void);

int stdin, stdout, stderr;

int
main(void)
{
  int f, fd, fds[2];

  fd = open("/dev", O_WRONLY|O_CREATE,
	    ATTR_wr|ATTR_rd|ATTR_dir);
  if (fd < 0) {
    return -1;
  }

  f = commount("/dev/com");
  if (f < 0) {
    return -1;
  }

  stdin = open("/dev/com", O_RDONLY);
  stdout = open("/dev/com", O_WRONLY);
  stderr = open("/dev/com", O_WRONLY);

  if (stdin < 0) return -2;
  if (stdout < 0) return -3;
  if (stderr < 0) return -3;

  printf("/dev/com mounted pid %i\n", f);

  f = tmpmount("/tmp");
  if (f < 0) {
    return -1;
  }
  
  printf("/tmp mounted on pid %i\n", f);

  if (pipe(fds) == ERR) {
    return -3;
  }

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -1;
  } else if (!f) {
    close(fds[1]);
    return ppipe0(fds[0]);
  }
	
  f = fork(FORK_sngroup);
  if (f < 0) {
    return -1;
  } else if (!f) {
    close(fds[0]);
    return ppipe1(fds[1]);
  }

  close(fds[0]);
  close(fds[1]);

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -1;
  } else if (!f) {
    return initmmcs();
  }

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -1;
  } else if (!f) {
    return filetest();
  }

  printf("Init completed. Exiting...\n");
  
  return 0;
}
