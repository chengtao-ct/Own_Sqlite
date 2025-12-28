#define _GNU_SOURCE  // 关键！建议加上这一行，防止编译器找不到 getline
#include <stdio.h>//I/O (输入输出)
#include <stdlib.h>//内存分配，进程控制，转换等
#include <string.h>//字符串处理
#include <stdbool.h>//布尔类型支持
typedef struct 
{
    char* Buffer;
    size_t Buffer_Length;
    ssize_t Input_Length;
} InputBuffer;//创建一个新的输入缓冲区，包含缓冲区指针，缓冲区长度和输入长度
typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGINIZED_COMMND
} MetaCommandResult;//用于处理非元命令的指令的Status
typedef enum{
    PREPARE_SUCCESS,
    PREAPRE_UNRECOGINZED_COMMAD            
} PrepareCommandResult;//用于处理元命令的指令的Status
typedef  enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;//用于选择指令模式
typedef struct
{
    StatementType type;
} Statement;//可以以后添加更多的指令分区
/*
    进行对输入缓冲区的清理工作，释放缓存区指针和输入缓冲区本身
*/
InputBuffer* new_input_buffer()
{
    InputBuffer* inputbuffer = malloc(sizeof(InputBuffer));
    inputbuffer->Buffer = NULL;
    inputbuffer->Buffer_Length = 0;
    inputbuffer->Input_Length = 0;
    return inputbuffer;
}
/*
    提示信息函数
*/
void print_prompt(){
    printf("CTsay:>");
}
/*
    读取输入信息函数
    参数一:输入内容
*/
void read_input(InputBuffer* inputbuffer)
{
    /*
    getline函数用于从输入流中读取一行文本
    第一个参数：指针的地址，用于存储读取的行
    第二个参数：指向缓冲区长度的指针
    第三个参数：输入流，这里使用标准输入stdin
    ssize_t getline(char **lineptr, size_t *n, FILE *stream)
    当然现在的标准sqlite3用的是readline (GNU Readline) 或 linenoise库来处理输入行
    */
    ssize_t bytes_read = getline(&(inputbuffer->Buffer),&(inputbuffer->Buffer_Length),stdin);
    if (bytes_read <= 0)//对于错误或者未输入的反馈
    {
        printf("Error reading input\n");
        exit (EXIT_FAILURE);//退出程序
    }
    //忽略输入行的换行符
    inputbuffer->Buffer[bytes_read -1] = 0;//将换行符替换为字符串结束符
    inputbuffer->Input_Length = bytes_read -1;//更新输入长度
}
/*
    清理输入堆内存
    参数一:输入内容
*/
void close_input_buffer(InputBuffer* inputbuffer)
{
    free (inputbuffer->Buffer);//释放缓冲区指针
    free (inputbuffer);//释放输入缓冲区本身
    /*以后都要记得先释放值的指针，再释放内存*/
}
/*
    对于非元指令处理的函数
    参数一:输入内容
    返回:非元指令控制结果
*/
MetaCommandResult do_meta_command(InputBuffer* inputbuffer)
{
        //strcmp函数用于比较两个字符串是否相等,相等返回0
      if(strcmp(inputbuffer->Buffer,".exit") == 0)//输入退出的情况
        {
            close_input_buffer(inputbuffer);
            exit(EXIT_SUCCESS);//成功退出程序
        }
    else{
        return META_COMMAND_UNRECOGINIZED_COMMND;//后续可以扩张更多对非元指令的操纵
    }
}
/*
    对于元指令处理函数
    参数一:输入内容
    参数二:元指令模式
    返回:元指令控制结果
*/
PrepareCommandResult do_prepare_command(InputBuffer * inputbuffer,Statement * statement)
{
    if(strncmp(inputbuffer->Buffer,"insert",6) == 0){
        statement->type = STATEMENT_INSERT;
        return  PREPARE_SUCCESS;
    }//这边是实现假如输入符合insert指令模式,则初始化他,并且返回成功操作指令
    if(strcmp(inputbuffer ->Buffer,"select") == 0)
    {
        statement ->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    else
    {
        return PREAPRE_UNRECOGINZED_COMMAD;
    }
}
/*
    执行元指令
    参数一:元指令模式
*/
 void execute_statement(Statement* statement)
 {
    switch(statement ->type)
    {
        case(STATEMENT_INSERT):
        printf("This is where we would do an insert.\n");
      break;
        case(STATEMENT_SELECT):
        printf("This is where we would do an select.\n");
      break;
    }
 }
int  main(int argc,char* argv[])//这是程序启动的那一瞬间接收的数据，argc是参数个数，argv是参数数组
{
    InputBuffer* inputbuffer = new_input_buffer();//调用创建输入缓冲区函数
    while(true)//无限循环
    {
        print_prompt();//提示符
        read_input(inputbuffer);//读取输入
        if(inputbuffer->Buffer[0] == '.' )
        {
            switch(do_meta_command(inputbuffer))
            {
                case(META_COMMAND_SUCCESS):
                continue;
                case(META_COMMAND_UNRECOGINIZED_COMMND):
                printf("ERROR Command'%s'\n",inputbuffer->Buffer);
                continue;
            }
        }
        Statement statement;
        switch (do_prepare_command(inputbuffer,&statement))
        {
            case(PREPARE_SUCCESS):
            break;
            case(PREAPRE_UNRECOGINZED_COMMAD):
            printf("Unrecognized keyword at start of '%s'.\n",inputbuffer->Buffer);
            continue;
        }
        execute_statement(&statement);
        printf("Executed.\n");
    }
}
