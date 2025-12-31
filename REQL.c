#define _GNU_SOURCE  // 关键！建议加上这一行，防止编译器找不到 getline
#define USERNAME_SIZE 32
#define EMAIL_SIZE 255
#include <stdio.h>//I/O (输入输出)
#include <stdlib.h>//内存分配，进程控制，转换等
#include <string.h>//字符串处理
#include <stdbool.h>//布尔类型支持
#include <stdint.h>//标准整数类型
//定义行的紧凑表示：
typedef struct
{
    uint32_t ID;
    char username[USERNAME_SIZE];
    char email[EMAIL_SIZE];

} Row;//用于存储用户信息,相当于行，这涉及到sqlite的数据结构
#define size_of_attributte(Struct,Attribute) sizeof(((Struct*)0)->Attribute)
//重点在这个宏定义，他的作用是在不创建结构体实例的情况下，获取结构体中某个成员变量的数据类型大小
const uint32_t ID_SIZE = size_of_attributte(Row,ID);
const uint32_t USERNAME_SIZE_VAL = size_of_attributte(Row,username);
const uint32_t EMAIL_SIZE_VAL = size_of_attributte(Row,email);
//这边是对于行的紧凑表示的偏移量（相对于行首offset），用于计算行的位置
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET +ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET +USERNAME_SIZE_VAL;
const uint32_t ROW_SIZE = ID_OFFSET + ID_SIZE + USERNAME_SIZE_VAL + EMAIL_SIZE_VAL;
//这边只是对于行的紧凑表示（减少内存占用），实际上sqlite还有更多的数据结构和算法来优化性能
//下面是页的紧凑表示：
const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;//每页最多可以存储的行数
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;//表最大可以存储的行数
typedef struct 
 {
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;//表结构体
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
    PREAPRE_UNRECOGINZED_COMMAD,     
    PREPARE_SYNTAX_ERROR,//语法错误
} PrepareCommandResult;//用于处理元命令的指令的Status
typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;
typedef  enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;//用于选择指令模式
typedef struct
{
    StatementType type;//用于存储指令模式
    Row row_to_insert;//用于存储插入的用户信息,相当于行，这涉及到sqlite的数据结构
} Statement;//可以以后添加更多的指令分区

// 提前声明函数，防止编译警告/错误
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void* row_slot(Table* table, uint32_t row_num);

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
    输出行
*/
void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->ID, row->username, row->email);
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
        /*
        sscanf函数用于从字符串中读取格式化数据
        第一个参数：读取的字符串
        第二个参数：格式化字符串
        第三个参数：存储读取的数据
        返回:成功读取的参数个数
        */
        int args_assigned = sscanf(inputbuffer->Buffer,"insert %d %s %s",&statement->row_to_insert.ID,
            statement->row_to_insert.username,statement->row_to_insert.email);
        if(args_assigned < 3)
        {
            return PREPARE_SYNTAX_ERROR;
        }
        
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
    将数据加入到内存
    参数一:元指令模式
    参数二：表内容
    返回：实验结果
*/
 ExecuteResult execute_insert(Statement* statement,Table* table)
 {
    if(table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }
    Row* row_to_insert  = &(statement->row_to_insert);
    serialize_row(row_to_insert,row_slot(table, table->num_rows));//将数据储存到所在row
    table->num_rows += 1;//
    return EXECUTE_SUCCESS;
 }
 /*
    选择从内存中读取数据
    参数一:元指令模式
    参数二：表内容
    返回：实验结果

 */
 ExecuteResult execute_select(Statement* statement, Table* table)
 {
    Row row;
    for(uint32_t i=0; i < table->num_rows;i++ )
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
 }
 /*
    执行指令文件
    参数一：元指令模式
    参数二：表内容
    返回：实验结果
 */
 ExecuteResult execute_statement(Statement* statement, Table* table)
 {
    switch(statement->type){
        case(STATEMENT_INSERT):
        return execute_insert(statement, table);
        case(STATEMENT_SELECT):
        return execute_select(statement,table);
        default:
        return EXECUTE_SUCCESS;
    }
 }
 /*
    将行数据序列化到内存中
    参数一:源行数据指针
    参数二:目标内存地址指针
 */
 void serialize_row(Row* source,void* destination)
 {
    char* dest = (char*)destination;
    memcpy(dest + ID_OFFSET,&(source->ID),ID_SIZE);
    memcpy(dest + USERNAME_OFFSET,&(source->username),USERNAME_SIZE_VAL);
    memcpy(dest + EMAIL_OFFSET,&(source->email),EMAIL_SIZE_VAL);
 }
 /*
    将行数据反序列化到内存中
    参数一:源内存地址指针
    参数二:目标行数据指针
 */
 void deserialize_row(void* source,Row* destination)
 {
    /*
    memcpy函数用于将数据从源地址复制到目标地址
    第一个参数：目标地址
    第二个参数：源地址
    第三个参数：复制的数据大小
    */
    char* src = (char*)source;
    memcpy(&(destination->ID),src + ID_OFFSET,ID_SIZE);
    memcpy(&(destination->username),src + USERNAME_OFFSET,USERNAME_SIZE_VAL);
    memcpy(&(destination->email),src + EMAIL_OFFSET,EMAIL_SIZE_VAL);
 }
 /*
    获取行数据指针
    参数一:表指针
    参数二:行数
    返回:行数据指针
    这样设计可以高效管理大量行的内存，同时只在访问时分配实际需要的页，而不是一次性分配所有页的内存
 */
 void* row_slot(Table*table,uint32_t row_num)
 {
    uint32_t page_num = row_num / ROWS_PER_PAGE;//计算所在的页数
    void* page = table->pages[page_num];//获取所在的页数据指针
    if(page == NULL)
    {
        //如果页数据指针为空，则需要分配内存
        page = malloc(PAGE_SIZE);
        if(page == NULL)
        {
            printf("Error: malloc failed\n");
            exit(EXIT_FAILURE);
        }
        table->pages[page_num] = page;//将页数据指针存储到表中
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;//计算所在的行偏移量
    char* page_ptr = (char*)page;
    return page_ptr + row_offset * ROW_SIZE;//返回所在的行数据指针
 }
 Table * new_table()
 {
    Table* table = (Table*)malloc(sizeof(Table));
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
        {
            table->pages[i] = NULL;
        }
        return table;
 }
 
 void free_table(Table* table)
 {
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        free(table->pages[i]);
    }
    free(table);
 }
int  main(int argc,char* argv[])//这是程序启动的那一瞬间接收的数据，argc是参数个数，argv是参数数组
{
    InputBuffer* inputbuffer = new_input_buffer();//调用创建输入缓冲区函数
    Table* table = new_table();
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
            case (PREPARE_SYNTAX_ERROR):
            printf("Syntax error. Could not parse statement.\n");
            continue;
        }
        switch (execute_statement(&statement, table)) 
                {
                case (EXECUTE_SUCCESS):
                    printf("Executed.\n");
                    break;
                case (EXECUTE_TABLE_FULL):
                    printf("Error: Table full.\n");
                   break;
               }
    }
}
