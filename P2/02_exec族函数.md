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

  - 和命令行参数argv类似，环境变量表也是一个字符串数组name=value

  - 可以用getenv函数，查询对应name的value

    ```c
    #include <stdlib.h>
    char *getenv(const char *name);
    ```

  - 可以用setenv函数，将环境变量name的值设置为value

    ```c
    #include <stdlib.h>
    int setenv(const char *name, const char *value, int rewrite);
    void unsetenv(const char *name);
    ```

- **带有字母l**：（表示list）的exec函数要求将新程序的每个命令行参数都当作一个参数传给它，命令行参数的个数是可变的（变参列表），最后一个可变参数应该是NULL，起到哨兵的作用
- **带有字母v**：（表示vector）的函数，则应该先构造一个指向参数的指针数组，然后将该数组的首地址当作参数传给它，数组中的最后一个地址应该是NULL，类似于main函数的argv参数或环境变量表一样

- **不带字母p**：（表示path）的exec函数第一个参数必须是程序的相对路径或绝对路径，例如“/bin/ls"或”./a.out"
- **带有字母p**：如果参数中包含/，则将其视为路径名。否则视为不带路径的程序名，在PATH环境变量的目录列表中搜索这个程序
- **对于以e（表示environment）结尾的exec函数**：可以把一份新的环境变量表传给它，其他exec函数仍使用当前的环境变量表执行新程序

## 代码示例

```c
#include<stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    execlp("ls", "ls", "-a", "-l", NULL); //截取进程，开始执行ls -al
    perror("exec");
    exit(1);
    return 0;
}
```

## 利用exec函数实现流的重定向

```c
#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {

    if (argc < 3) {
        printf("usage: cmd + input file + output file\n");
        return 1;
    }

    //实现输入流的重定向
    int fd = open(argv[1], O_RDONLY);
    dup2(fd, 0); //重定向标准输入流0为fd
    close(fd);

    //实现输出流的重定向
    fd = open(argv[2], O_WRONLY | O_CREAT, 0644); //只写形式，如果文件不存在创建它
    dup2(fd, 1); //重定向标准输出流1为fd
    close(fd);

    //截断进程，调用upper程序运行
    execl("./upper", "./upper", NULL);
    perror("exec"); //upper程序调度失败
    exit(1);
}
```

