#include "../config.h"
#include "LogXferlog.h"
//for xclose
#include <errno.h>
//snprintf
#include <stdio.h>
//malloc
#include <stdlib.h>
#include <time.h>
//fchown
#include <unistd.h>
//open
#include <fcntl.h>
//xclose
#include "../security.h"
//strlen
#include <string.h>
//getuid
#include <pwd.h>
//user data, gl_var
#include "Sftp.h"
#include "../hash.h"
#include "Log.h"

tGlobal	*gl_var;
int global_xferlog_fullpath;


char * string_replace(char * str, char * to_replace, char * replacement){
 char * chr;
 if (!(chr = strstr(str, to_replace))){
  return str;
 }
 static char temp[128];
 strncpy(temp, str, chr - str);
 temp[chr - str] = 0;
 sprintf(temp + (chr - str), "%s%s", replacement, chr + strlen(to_replace));
 return temp;
}

typedef struct s_logx
{
 const char * file_path;
 int filedes;
 unsigned char reopen;
} t_logx;

static t_logx * xferlog_intern = NULL;

void xferlog_open(const char * file_path){
	int filedes;
	if( (filedes = open( file_path, O_CREAT | O_APPEND | O_WRONLY, 0644)) > -1){
	 fchown(filedes, 0, 0);
		if(xferlog_intern == NULL){
 	 xferlog_intern = calloc(1, sizeof(*xferlog_intern));
 	}
 	xferlog_intern->file_path = file_path;
		xferlog_intern->filedes = filedes;
	}
}
void xferlog_close(){
	if(xferlog_intern != NULL){
		xclose(xferlog_intern->filedes);
 }
}
void xferlog_reopen(){
	if(xferlog_intern != NULL){
		xferlog_intern->reopen = 1;
	}
}
void xferlog_close_and_free(){
 xferlog_close();
 free(xferlog_intern);
	xferlog_intern = NULL;
}

//writes a line into the logfile
void xferlog_write(
	const char * remote_host, u_int32_t transfer_time,
	u_int64_t filesize,	const char * file_path,
	const char * direction,	const char * username,
	const char * completion_status
	){
 /*
  transfer-mode  always binary
  special-action  this sftp server never does special actions like compression after upload
 */
 if(xferlog_intern == NULL){
  return;
 }

 time_t cur_time;
 cur_time = time(NULL);

 //reopen logfile if wanted
 if(xferlog_intern->reopen == 1){
  xferlog_close();
  xferlog_open(xferlog_intern->file_path);
  xferlog_intern->reopen = 0;
 }
 
 struct tm * tm_obj = gmtime(&cur_time);
 char * res_asctime = asctime(tm_obj);
 res_asctime[strlen(res_asctime) - 1] = '\0';
 res_asctime = string_replace(res_asctime, "  ", " ");

 unsigned short int max_line_size = 1024;
 char line[max_line_size];

 char * access_mode;
 if(strcmp(username, "anonymous") == 0){
  access_mode = "a";
 }else if(strcmp(username, "guest") == 0){
		access_mode = "g";
 }else{
  access_mode = "r";
 }

	//xferlog patch
 	//if we have a changeroot and want to log the full path in xferlog:
	char * path_home = gl_var->who->home;
	int p_len = strlen(path_home) + strlen(file_path) + 2;
	char path_log[p_len];
	//empty path_log
	strcpy(path_log, "\0");
	//check if chroot
	if (HAS_BIT(gl_var->flagsGlobals, SFTPWHO_VIRTUAL_CHROOT)) {
		//check config for loggin full path to xferlog
		//we dont use the weird bitmasks here, no clue how that should work so we have a global value here
		if (global_xferlog_fullpath > 0) {
			//add home path to string
			strcat(path_log, path_home);
		}
	}
	//memo: this ftp-"server" runs as a shell for ssh logins.
	//for some weird reasons it wont close after disconnect and caches connection and settings.
	//this may lead to an unwanted behaviour when changing config settings on the fly
	//to make new settings work, a reconnect of the client is not enough, even its just a shell which obviously should close after disconnect.
	//the shell wont close after disconnect and runs as a daemon or whatever in background until timeout or so?
	//very weird behaviour, costs hours of testing. use killall -9 MySecureShell or use sftp-stat to be sure the shell loads its new config!
	
	//add path/file to string
	strcat(path_log, file_path);
	//log to xferlog
	int line_size= snprintf(line, max_line_size, "%s %i %s %i %s b _ %s %s %s sftp 0 %i %s\n", res_asctime,
		transfer_time, remote_host, (int)filesize, path_log, direction, access_mode, username, getuid(), 
		completion_status);
	//write out	
	write(xferlog_intern->filedes, line, line_size);
	//xferlog patch end
}
