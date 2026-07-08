# 实验笔记

## 读取按键

```c
#include <unistd.h>

int main() {
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1);
	return 0;
}
```

 - `ssize_t read(int fildes, void *buf, size_t nbyte)`: 来自 `<unistd.h>`. 其中, `int fildes` 是文件描述符(文件对应的非负整数索引, `Linux` 中规定, 系统刚刚启动时, `0` 指向标准输入 `stdin`, `1` 指向标准输出 `stdout`, `2` 指向标准错误 `stderr`), 在当前使用的值为 `STDIN_FILENO`, 即标准输入; `void *buf` 为缓冲区指针, 用于存放读取到的数据, 此处指向创建的字符变量 `c`; `size_t nbyte` 为本次操作读取的字节数, 当结果为 `-1` 时, 表示发生错误并设置错误代码 `errno`. 
 - 一般情况下, `终端(terminal)` 在 `规范模式(canonical mode/cooked mode)` 下运行, 在这种模式下, 用户按键产生的字符会存入内部缓冲区, 在此期间, 可以使用 `退格(Backspace)` 删除位于光标前面的字符, 使用 `Ctrl+C` 终止进程并释放内存等, 直至用户按下 `回车(Enter)`, 此时缓冲区的字符被写入标准输入 `STDIN_FILENO`. 即按行管理. 

## 按 `q` 退出

```c
#include <unistd.h>
int main() {
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
} 
```

 - 读取到字符 `q` 时, 退出循环, 程序结束. 如果 `q` 之后还有未被读取的字符, 这些字符可能会在程序退出之后作为 `shell` 的输入. 

## 关闭 `ECHO`

```c
#include <termios.h>
#include <unistd.h>

void enableRawMode() {
	struct termios raw;
	tcgetattr(STDIN_FILENO, &raw);
	raw.c_lflag &= ~(ECHO);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
	enableRawMode();
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
}
```

关闭 `ECHO` 属性, 使用户在按键输入的时候不会在终端中打印对应的字符. 需要注意的是在当前程序中依然需要按 `回车(Enter)` 才能将输入字符写入标准输入 `STDIN_FILENO`. 
如果发现退出程序之后依然不能在终端中显示输入的字符, 可以通过 `Ctrl+C(Control+C)` 新起一行并输入 `reset` 命令并按 `回车(Enter)` 恢复终端状态, 或者重新开启一个终端. 

 - `struct termios`: 来自 `<termios.h>`. 标准终端和串口接口, 可以通过其对终端的属性、控制输入输出行为进行配置. 
 - `tcflag_t c_lflag`: `struct termios` 中的属性, 控制终端的 `本地模式(Local Mode)`. `ECHO` 定义为 `0x0008`, 对应比特位控制 `ECHO` 属性的开关. 
 - `tc/TC`: 即 `Terminal Control` 终端控制. 
 - `int tcgetattr(int fildes, struct termios *termios_p)`: 来自 `<termios.h>`. 获取 `fildes` 文件描述符指向的终端的属性并保存至结构体 `termios_p` 中. `0` 返回值表示成功操作, `-1` 返回值表示发生错误并设置错误代码 `errno`. 
 - `int tcsetattr(int fildes, int optional_actions, const struct termios *termios_p)`: 通过结构体 `termios_p` 设置 `fildes` 打开文件描述符指向的终端的属性. 当前 `optional_actions` 参数值为 `TCSAFLUSH`(`SA` 指 `Set Attributes` 设置属性), 指属性的改动会在所有当前位于输出缓冲区的字符输出之后生效, 同时清空输出缓冲区, 以保证获得一个清洁的终端状态. 
  
## 退出程序时恢复终端状态

```c
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios; 

void disableRawMode() {
	tcsetsttr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
}

void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_lflag &= ~(ECHO); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

在程序退出时, 恢复终端状态. 

 - `int atexit(void (*func)(void))`: 来自 `<stdlib.h>`. 用于登记 `disableRawMode()` 函数, 使其在程序正常退出时(`main()` 返回或者调用 `exit()` 函数时)自动执行. 

运行程序之后可以发现此时位于 `q` 之后的输入字符不会在程序结束之后作为终端的输入, 这是因为 `TCSAFLUSH` 切换模式时清除了输入缓冲区. 

## 关闭规范模式 `canonical mode`

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_lflag &= ~(ECHO | ICANON); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `ICANON`: 来自 `<termios.h>`. 定义为 `0x0002`. 虽然以 `I` 开头但是它不是输入标志 `input flag` 而是本地标志 `local flag`. 

可以发现修改后的程序在输入字符 `q` 之后立即退出. 

## 打印按键

```c
#include <ctype.h>
#include <stdio.h>

int main() {
	enableRawMode(); 

	char c; 
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if (iscntrl(c)) {
			printf("%d\n", c); 
		} else {
			printf("%d (\'%c\')\n", c, c); 
		}
	}
	return 0; 
}
```

 - `int iscntrl(int c)`: 来自 `<ctype.h>`. 判断字符 `c` 是否为控制字符(`ASCII` 中编码为 `0-31` 和 `127` 的字符), 如果是控制字符则返回非零值, 否则返回零值. 
 - `int printf(const char *format, ...)`: 来自 `<stdio.h>`. 将格式化字符串发送至标准输出 `stdout`. `%d` 表示以十进制形式输出带符号整数, `%c` 表示输出单个字符. 

当前程序会打印出按键的编码, 如果输入的字符不是控制字符, 则还会打印出字符本身. 值得注意的是方向键 `Arrow Keys` 等由多个字节构成, 它们的第一个字节编码为 `27`, 即 `Escape`. 这些信号被称为转义序列 `escape sequence`. 

## 关闭 `Ctrl+C` 和 `Ctrl+Z` 信号

在终端中, 默认情况下使用 `Ctrl+C` 会向当前进程发送一个 `SIGINT`(`Interrupt`) 信号以终止进程(停止执行并释放资源), 使用 `Ctrl+Z` 会向当前进程发送一个 `SIGTSTP`(`Terminal Stop`) 信号以挂起进程(暂定进程的执行, 将其放入后台, 并将控制权还给命令行提示符). 使用 `jobs` 可以查看当前 `shell`(命令行解释器进程, 如一个 MacOS Terminal 窗口)会话中的后台任务. 使用 `fg` 可以将后台任务切换到前台继续执行, 使用 `bg` 可以将挂起的任务在后台继续执行. 

需要注意的是, `read()` 函数在读取数据前被信号中断, 会讲返回值设置为 `-1` 并设置错误代码 `errno` 为 `EINTR`(即 `4`). 因此当前代码在使用 `Ctrl+Z` 挂起之后再使用 `fg` 或 `bg` 恢复进程时, `read()` 函数会返回 `-1` 导致退出循环并结束进程. `bg` 还会因为作为后台进程希望向终端进行输出而触发其它信号. 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_lflag &= ~(ECHO | ICANON | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `ISIG`: 来自 `<termios.h>`. 与 `ICANON` 类似, 以 `I` 开头但是它不是输入标志 `input flag` 而是本地标志 `local flag`. 

现在, 在进程中使用 `Ctrl+C` 和 `Ctrl+Z` 键将分别显示 `3` 和 `26`, 而不是退出或者挂起进程. 
在 MacOS 上, 这一步同时关闭了 `Ctrl+Y` 的功能. 

## 关闭 `Ctrl+S` 和 `Ctrl+Q` 信号

默认情况下, `Ctrl+S` 和 `Ctrl+Q` 用于 `软件流控制(software flow control)`. 其中, `Ctrl+S` 用于停止向终端写入数据, `Ctrl+Q` 用于恢复向终端写入数据. 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(IXON); 
	raw.c_lflag &= ~(ECHO | ICANON | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `IXON`: 来自 `<termios.h>`. 用于控制是否启用软件流控制(即 `XON/XOFF` 协议). 

现在, 在进程中使用 `Ctrl+S` 和 `Ctrl+Q` 键将分别显示 `19` 和 `17`. 

## 关闭 `Ctrl+V` 信号

在部分系统上, 使用 `Ctrl+V` 允许我们再其之后输入字面字符(比如 `Ctrl+V`, `Ctrl+C` 组合不会停止进程, 而是输入 `3` 个字节). 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(IXON); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `IEXTEN`: 来自 `<termios.h>`. 与 `ICANON` 类似, 以 `I` 开头但是它不是输入标志 `input flag` 而是本地标志 `local flag`. 控制扩展输入处理功能. 

现在, 在进程中使用 `Ctrl+V` 键将显示 `22`. 
在 MacOS 上, 这一步同时关闭了 `Ctrl+O` 的功能. 

## 修复 `Ctrl+M`

在当前的程序中, 输入 `Ctrl+M` 将会得到 `10` 而不是期望的 `13`. 这是因为操作系统自动将回车 `carriage return`(`\r`, `13`) 转换为换行 `newline`(`\n`, `10`). 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(ICRNL | IXON); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `ICRNL`: 来自 `<termios.h>`. 控制回车转换. 

现在, 在进程中使用 `Ctrl+M` 键将显示 `13`, 使用 `Enter` 键也将显示 `13`. 