/*************************************************************************
	> File Name: dict_e2c.c
	> Author: 
	> Mail: 
	> Created Time: 2021年10月13日 星期三 20时20分16秒
 ************************************************************************/

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLEN 1024

typedef struct Node {
    char *wd;
    char *tran;
    struct Node *next;
} Node;

Node *loadDict() {
    char buff[MAXLEN];
    FILE *fp = fopen("./dict_e2c.txt", "r");
    if (!fp) { //读取失败
        perror("open dict");
        exit(1);
    }

    while (fgets(buff, MAXLEN, fp)) { //一行一行循环读取
        Node *nn = (Node *) malloc(sizeof(Node));
        nn -> wd = (char *) malloc(sizeof(char) * (strlen(buff) + 1));
        strcpy(nn -> wd, buff);
    }
}

int main() {


    return 0;
}
