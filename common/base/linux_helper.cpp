#include "base/linux_helper.h"
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <fcntl.h> 
#include <sys/file.h>
#include "base/string_helper.h"
//#include <process.h>

namespace Astra {

void LinuxHelper::Deamonize() {
    if(fork() != 0) {
		exit(0);
	}

	setsid();
	signal(SIGINT,  SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	struct sigaction sig;
	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE, &sig, NULL);

	if (fork() != 0) {
		exit(0);
	}

	umask(0);
}

bool LinuxHelper::ChangeWorkingDirectory(const char* dir) {
    return chdir(dir) == 0;
}

const char* LinuxHelper::GetBinDirectory() {
    static bool inited = false;
	static char bin_dir[1024] = {0};

	if(false == inited) {
		char link_file[1024] = {0};
		snprintf(link_file, sizeof(link_file), "/proc/%d/exe", getpid());

		char buff[1024];
		int ret = readlink(link_file, buff, sizeof(buff));
		if(ret <= 0 || ret >= (int)sizeof(buff)) {
			return "";
		}

		buff[ret] = 0;
		int idx = StringHelper::FindLastChar(buff, ret, '/');
		if(idx < 0) {
			snprintf(bin_dir,sizeof(bin_dir), "/");
		} else {
			memcpy(bin_dir, buff, idx + 1);
            bin_dir[idx + 1] = 0;
		}

		inited = true;
	}

	return bin_dir;
}

bool LinuxHelper::LockFile(const char* file) {
    int fd = open(file, O_RDWR|O_CREAT, 0640);
	if(fd < 0) {
		return false;
	}

	if(flock(fd, LOCK_EX | LOCK_NB) < 0) {
		close(fd);
		return false;
	}

	return true;
}

}