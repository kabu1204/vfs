//
// Created by yuchengye on 2021/10/22.
//
#include "client.h"
#include "shell.h"

int main()
{
    char tty[64]="./ipc_msg";
    shell sh(tty, 123);
}