# 进程间通信概述

每个进程各自有不同的用户地址空间，任何一个进程的全局变量对另一个进程来讲都是不可见的，所以进程间交换数据必须通过内核

1. 在内核中开辟一个缓冲区
2. 进程1把数据从用户空间拷贝到内核缓冲区中
3. 进程2从内核缓冲区中提取相应数据

内核提供的这种机制称为**进程间通信（IPC，Inter Process Communication）**

![image-20211024112113540](C:\Users\gaoxiang7\AppData\Roaming\Typora\typora-user-images\image-20211024112113540.png)





# 管道

管道是一种最基本的IPC机制，由pipe函数创建

```c
#include <unistd.h>
int pipe(int filedes[2]);

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
int pipe2(int filedes[2], int flags);
```

调用pipe函数时在内核中开辟一块缓冲区（俗称管道）用于通信，它有一个读取端和一个写入端，然后通过filedes参数传出给用户程序连个文件描述符

- filedes[0]：指向管道的读端
- filedes[1]：指向管道的写端



## 如何通过管道实现IPC

### 父子进程间通信

1. 父进程调用pipe函数在内核中生成管道，父进程的文件描述符表中会记录管道的读端和写端
2. 父进程调用fork生成子进程，子进程会copy父进程的文件描述符表，此时父进程和子进程同时记录了管道的读端和写端
3. 父进程关闭读端，子进程关闭写端，就实现了信息从父进程到子进程的单项传递

![image-20211024112525803](C:\Users\gaoxiang7\AppData\Roaming\Typora\typora-user-images\image-20211024112525803.png)

![image-20211024113751560](C:\Users\gaoxiang7\AppData\Roaming\Typora\typora-user-images\image-20211024113751560.png)

![image-20211024113833321](C:\Users\gaoxiang7\AppData\Roaming\Typora\typora-user-images\image-20211024113833321.png)

```c
#include<stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    //1. 父进程创建管道
    int fd[2], n;
    char buff[25];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(1);
    }

    //2. 父进程创建子进程
    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    //3. 父进程和子进程分别关闭管道的一端
    if (pid > 0) { //父进程空间
        close(fd[0]); //父进程关闭管道读端
        write(fd[1], "hello pipe\n", 11); //父进程向管道写入信息
        wait(NULL); //等待子进程终止
    } else { //子进程
        close(fd[1]); //子进程关闭写端
        n = read(fd[0], buff, 20); //子进程从管道读取信息
        sleep(1);
        write(1, buff, n); //子进程输出读到的信息
    }

    return 0;
}
```

### 两个子进程间通信

1. 父进程构建好管道
2. 父进程连续调用两次fork，创建两个子进程，两个子进程都继承了父进程的文件描述符表，即都记录了管道的读写端口文件描述符
3. 子进程1关闭读端，子进程2关闭写端，即可实现从子进程1到子进程2的单向IPC

总之需要通过fork传递文件描述符使两个进程都能访问同一管道，他们才能通信



## 使用管道注意事项

使用管道需要注意一下4种特殊情况（假设都是阻塞I/O操作，即没有设置O_NONBLOCK标志）

1. 如果所有指向管道写端的文件描述符都关闭了，而仍然有进程从管道的读端读取数据，那么管道种剩余的数据都被读取后，再次read会返回0，就像读到文件末尾一样
2. 如果有指向管道写端的文件描述符没关闭，而持有管道写端的进程也没有向管道中写数据，此时有进程从管道读端读取数据，那么管道中剩余数据被读取后，再次read会阻塞，直到管道中有数据可读了才读取数据并返回
3. 如果所有指向管道读端的文件描述符都关闭了，这是有进程向管道的写端write，那么该进程会收到信号SIGPIPE，通常会导致进程异常终止
4. 如果有指向管道读端的文件描述符没关闭，而持有管道读端的进程也没有从管道中读取数据，这是有进程向管道写端写入数据，那么管道被写满时再次write会被阻塞，直到管道中有空位置才写入数据并返回

```c
#include<stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    //1. 父进程创建管道
    int fd[2], n;
    char buff[25];
    char test[1024];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(1);
    }

    //2. 父进程创建子进程
    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    //3. 父进程和子进程分别关闭管道的一端
    if (pid > 0) { //父进程空间
        close(fd[0]); //父进程关闭管道读端
        int i;
        for (i = 0; i < 100; ++i) {
            write(fd[1], test, 1024); //父进程向管道写入信息
            printf("i = %d\n", i);
        }
        //close(fd[1]); //特殊情况1
        perror("write");
        wait(NULL); //等待子进程终止
    } else { //子进程
        close(fd[1]); //子进程关闭写端
        sleep(10);
        /*
        n = read(fd[0], buff, 20); //子进程从管道读取信息
        printf("n = %d\n", n);
        n = read(fd[0], buff, 20); //特殊情况2子进程阻塞读管道内容
        printf("n = %d\n", n);
        write(1, buff, n); //子进程输出读到的信息
        */
        exit(3);
    }

    return 0;
}
```



## 管道操作封装

常用的管道操作封装函数：

- popen
- pclose

这两个函数实现的操作是：创建一个管道，fork一个子进程，关闭管道的不使用端，exec一个cmd命令，等待命令终止

```c
#include <stdlio.h>
FILE *popen(const char *command, const char *type);
//Return value: 如果成功，则返回文件指针，如果出错返回NULL
int pclose(FILE *stream);
//Return value: command的终止状态，如果出错则为-1
```

### 执行过程

1. 函数popen先执行fork，然后调用exec以执行command，并返回一个标准I/O文件指针（即管道的读写端）
2. 如果type是”r"，则文件指针连接到cmd的标准输出上（即将管道的读端连接到cmd的输出上）
3. 如果type是”w“，则文件指针链接到cmd的标准输入上（即将管道的写端连接到cmd的输入上）
4. pclose函数关闭标准I/O流，等待命令执行结束，然后返回cmd的终止状态，如果cmd不能被执行，则pclose返回的终止状态与shell执行exit一样

```c
#include<stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main() {
    //FILE *fp = popen("cat ./pipeOpt.c", "r"); //设置管道读端到*fp
    FILE *fp = popen("./upper", "w"); //设置管道写端到*fp
    if (!fp) {
        perror("popen");
        exit(1);
    }

    /*
    //从fp中输出内容
    int c;
    while (~(c = fgetc(fp))) {
        putchar(toupper(c));
    }
    */

    //向fp中写入内容
    fprintf(fp, "hello world 3\n ttt good dnisaoc\n");

    pclose(fp);
    return 0;
}
```





# 共享内存

共享存储允许两个或多个进程共享一个给定的存储区，因为数据不需要在服务器和客户终端之间复制，所以这是最快的一种IPC

```c
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

key_t ftok(const char *pathname, int proj_id); //获取共享内存的键

int shmget(key_t key, size_t size, int shmflg); //得到共享内存结构

void *shmat(int shmid, const void *shmaddr, int shmflg); //获取内核中共享内存的地址，挂载到用户空间

int shmdt(const void *shmaddr); //取消用户空间挂载的内核中共享内存

int shmctl(int shmid, int cmd, struct shmid_ds *buf); //对共享内存区域的操作
```

![image-20211024163030808](C:\Users\gaoxiang7\AppData\Roaming\Typora\typora-user-images\image-20211024163030808.png)



## 各步骤详解

### ftok获取内核中共享内存key

```c
key_t ftok(const char *pathname, int proj_id);
```

ftok函数利用给定的路径找到指定的文件，并和一个小端8位proj_id（必须是非0值）生成一个key_t类型的System V IPC key，后续可以通过这个key指向内核中的这块共享内存区域

### shmget获取共享内存标识符

```c
int shmget(key_t key, size_t size, int shmflg);
```

- 返回值：获取的共享内存标识符id，出错返回-1
- key：通过ftok获得的指示内核区间的一个键
- size：用于指定需要的共享内存大小
  - 创建新段：一般在服务器中，则必须指定size
  - 访问已有段：一般在客户机中，则将size指定为0
- shmflg：
  - IPC_CREAT：如果不存在则创建，创建时需指明该段内存的访问权限，与open mode类似，将访问权限和该宏进行按位与
  - IPC_EXCL：如果将该宏和IPC_CREAT按位与后不可重复创建

### shmat映射共享内存空间

用户空间的进程可以利用shmat方法将shmget获取的共享内存标识符传入，对内核空间的共享内存映射到用户空间

```c
void *shmat(int shmid, const void *addr, int shmflg);
```

- 返回值：如果成功则返回指向共享内存段的指针，如果出错则返回-1
- shmid：由shmget获取得到的共享内存标识符
- addr：
  - 如果为NULL，则此段连接到由内核选择的第一个可用地址上
  - 如果为非NULL，并且没有指定SHM_RND，则此段连接到addr所指向的地址上
  - 如果addr非0，并且制定了SHM_RND，则此段连接到（addr - (addr mod SHMLBA））所表示的地址上，即向下取整到最近1个SHM_LBA的倍数
    - SHM_RND：取整
    - SHM_LBA：低边界地址倍数，它总是2的乘方
  - 一般应指定addr为0，以便由内核选择地址



## 代码演示

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>

int main() {

    key_t key = ftok("./pipeOpt.c", 99); //利用文件和项目号获取key值
    printf("key = 0x%x\n", key);

    int shm_id = shmget(key, 20, IPC_CREAT | 0666); //利用key申请一个共享内存空间
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }
    printf("shmid = %d\n", shm_id);

    char *shm_p = shmat(shm_id, NULL, 0); //将共享内存空间映射到用户空间
    if (shm_p < 0) {
        perror("shmat");
        exit(1);
    }
    printf("shmp = %p\n", shm_p);
    //snprintf(shm_p, 20, "hello\n");
    //printf("%s", shm_p);

    bzero(shm_p, 20); //清空共享内存空间20个字节

    //利用共享内存空间实现父子进程间的通信
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0) {
        //parent process
        while (1) {
            //父进程循环写入，直到quit终止
            scanf("%s", shm_p);
            if (!strcmp(shm_p, "quit")) break;
        }
        wait(NULL);
    } else {
        //child process
        while (1) {
            //子进程循环读，直到读到quit终止
            if (!strcmp(shm_p, "quit")) break;
            if (*shm_p) {
                printf("child read: %s\n", shm_p);
                bzero(shm_p, 20);
            }
            sleep(1);
        }
    }

    shmdt(shm_p); //取消共享内存映射关系

    return 0;
}
```





# 消息队列

系统内核维护了一个存放消息的队列，不同用户可以向队列中发送消息，或者从队列中接收消息

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```

![image-20211024221453145](C:\Users\m1874\AppData\Roaming\Typora\typora-user-images\image-20211024221453145.png)



## 各部分详解

### msgget创建消息队列

系统创建或获取消息队列，如果不存在则创建，如果存在则获取

```c
int msgget(key_t key, int mode);
```

- 返回值：创建好的消息队列id
- key：由ftok生成的内核空间key
- mode：由宏定义组成的不同选项
  - IPC_CREAT | 0666：在创建的时候要赋予访问权限

### msgsnd向队列发送消息

往消息队列中发送一条消息，此操作被中断后不会被重启（信号处理中SA_RESTAT)

```c
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```

- 返回值：成功写入返回0，失败返回-1

- msgid：由msgget获得的消息队列id

- msgp：消息结构体指针，该结构体如下

  ```c
  struct msgbuf {
      long mtype; //消息类型，必须大于0
      char mtext[100]; //消息数据，可定义类型和大小
  }；
  ```

- msgsz：消息的长度，指消息数据的长度
- msgflg：宏组合，用户不同的发送模式
  - IPC_NOWAIT：非阻塞写入
  - MSG_EXCPT：接收不检测mtype
  - MSG_NOERROR：消息数据过长时会截断数据

### msgrcv读取队列中的消息

从队列中读取对应type最靠前的那条消息

**注意：使用msgrcv读取消息会将此条消息从队列中移除**

```c
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```

- 返回值：成功返回实际读取到mtext中的字节数，失败返回-1
- msgtyp：指定需要从队列中读取的消息类别
- 其余参数同msgsnd



## 代码演示

### 向队列中发送消息

```c
#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/msg.h>

#define MSGLEN 20

typedef struct msgbuf {
    long mtype;
    char mtext[MSGLEN];
} MSG;

int main() {

    //利用ftok获取内核地址段的key
    key_t key = ftok("./proj_id.txt", 9);
    printf("key = 0x%x\n", key);

    //利用msgget创建消息队列，并赋予它读写权限
    int mq_id = msgget(key, IPC_CREAT | 0666);
    printf("mqid = %d\n", mq_id);

    //定义消息结构体并调用msgsnd向消息队列中输出消息
    MSG msg;
    msg.mtype = 1; //消息1
    strncpy(msg.mtext, "hi, how are you?\n", MSGLEN);
    msgsnd(mq_id, &msg, MSGLEN, 0);
    msg.mtype = 2; //消息2
    strncpy(msg.mtext, "xiaokai: online\n", MSGLEN);
    msgsnd(mq_id, &msg, MSGLEN, 0);

    return 0;
}
```

### 从队列中读取消息

```c
#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/msg.h>

#define MSGLEN 20

typedef struct msgbuf {
    long mtype;
    char mtext[MSGLEN];
} MSG;

int main() {

    //利用ftok获取内核地址段的key
    key_t key = ftok("./proj_id.txt", 9);
    printf("key = 0x%x\n", key);

    //利用msgget创建消息队列，并赋予它读写权限
    int mq_id = msgget(key, IPC_CREAT | 0666);
    printf("mqid = %d\n", mq_id);

    //定义消息结构体并调用msgsnd向消息队列中输出消息
    MSG msg;
    msgrcv(mq_id, &msg, MSGLEN, 2, 0);
    printf("msg,type = %ld\nmsg.text = %s\n", msg.mtype, msg.mtext);
    msgrcv(mq_id, &msg, MSGLEN, 1, 0);
    printf("msg,type = %ld\nmsg.text = %s\n", msg.mtype, msg.mtext);

    return 0;
}
```

