# GDB调试多进程

GDB在调试的时候只能制定其中一个进程

例如制定监控父进程，如下所示

```shell
(gdb) set follow-fork-mode parent
```

此时子进程是没有被打断点的，可以继续向下运行，而父进程会被断点卡住，从而对父进程进行调试



# exec函数族

当进程调用一种exec函数时，该进程的用户空间代码和数据完全被新程序替换，从新程序的启动例程开始执行

```c
#include <unistd.h>
extern char **environ;
int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ..., char *const envp[]);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);

#include <unistd.h>
int execve(const char *filename, char *const argv[], char *const envp[]);
```

- 返回值
  - 这些函数如果调用成功则加载新的程序从启动代码开始执行，**不再返回**
  - 如果调用出错则返回-1，所以exec函数只有出错的返回值而没有成功的返回值

- extern char **environ：环境变量表，exec系统调用执行新程序时会把命令行参数和环境变量表传递给main函数

