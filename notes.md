# 实验笔记

## 引言

## 进入原始模式(`raw mode`)

### 读取按键

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

### 按 `q` 退出

```c
#include <unistd.h>
int main() {
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	return 0;
} 
```

 - 读取到字符 `q` 时, 退出循环, 程序结束. 如果 `q` 之后还有未被读取的字符, 这些字符可能会在程序退出之后作为 `shell` 的输入. 

### 关闭 `ECHO`

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
  
### 退出程序时恢复终端状态

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

### 关闭规范模式 `canonical mode`

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

### 打印按键

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

### 关闭 `Ctrl+C` 和 `Ctrl+Z` 信号

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

### 关闭 `Ctrl+S` 和 `Ctrl+Q` 信号

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

### 关闭 `Ctrl+V` 信号

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

### 修复 `Ctrl+M`

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

### 关闭所有输出处理

回车 `\r` 的作用是将光标移动到行首, 换行 `\n` 的作用是将光标向下移动一行. 因此, 终端想要新起一行需要这两个字符的协同作用 `\r\n`. 终端在输出的时候会自动将 `\n` 替换为 `\r\n`. 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(ICRNL | IXON); 
	raw.c_oflag &= ~(OPOST); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

 - `OPOST`: 来自 `<termios.h>`. `POST` 指 `post-process output`. 

运行程序, 可以发现输出程阶梯状排列. 每次输出只会让光标下移而不会令其回到行首, 为了修复这个问题, 我们手动增加回车 `\r`: 

```c
int main() {
	enableRawMode(); 

	char c; 
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if (iscntrl(c)) {
			printf("%d\r\n", c); 
		} else {
			printf("%d (\'%c\')\r\n", c, c); 
		}
	}
	return 0; 
}
```

现在, 光标可以像之前一样正常回到行首. 需要注意在之后需要进行“移动至下一行行首”的操作的时候, 都需要同时使用 `\r\n`. 

### 更多的标志位

为了实现文本编辑器的目标, 我们需要关闭更多的标志位: 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
	raw.c_oflag &= ~(OPOST); 
	raw.c_cflag |= (CS8); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}
```

这些宏均来自 `<termios.h>`. 部分可能与系统的默认设置一致, 具体不作深入. 

### 给 `read()` 设置超时

当前的 `read()` 函数会一直等待键盘输入再返回. 在文本编辑器中, 我们需要画面一直刷新, 这就要求 `read()` 函数允许在一段时间内没有接收到输入的时候返回. 

```c
void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
	raw.c_oflag &= ~(OPOST); 
	raw.c_cflag |= (CS8); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
	raw.c_cc[VMIN] = 0; 
	raw.c_cc[VTIME] = 1; 

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
}

int main() {
	enableRawMode(); 

	while (1) {
		char c = '\0'; 
		read(STDIN_FILENO, &c, 1); 
		if (iscntrl(c)) {
			printf("%d\r\n", c); 
		} else {
			prinf("%d (\'%c\')\r\n", c, c); 
		}
		if (c == 'q') break; 
	}
	
	return 0; 
}
```

 - `VMIN` 和 `VTIME`: 均来自 `<termios.h>`. `VMIN` 设置 `read()` 函数返回所需要读取的最少字节数, `VTIME` 设置 `read()` 函数返回之前等待的最大时间, 其单位为十分之一秒, 即 100 毫秒. 

运行程序, 可以看到在不输入的时候, 程序自动打印 `0`, 因为这是我们设置的默认值且 `read()` 函数并没有对其进行修改. 如果输入字符, 它可以向先前一样打印字符的 `ASCII` 编码, 当字符本身可打印时, 字符本身也会一并打出. 

### 错误处理

`die()` 函数用于打印错误信息并退出程序. 错误码为 `1`. 

```c
void die(const char *s) {
	perror(s); 
	exit(1); 
}
```

 - `void perror(const char *s)`: 来自 `<stdio.h>`. 按照下面的顺序将错误信息输出到标准错误 `stderr`: 字符指针 `s` (不为空或不指向空字节) 指向的字符串, 一个冒号和一个空格 `: `, 由 `errno` 的值决定的错误信息, 换行符 `\n`. 
 - `void exit(int status)`: 来自 `<stdlib.h>`, 按照登记顺序的逆序运行所有被 `atexit()` 函数登记过的函数. 

```c
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios; 

void die(const char *s) {
	perror(s); 
	exit(1); 
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		die("tcsetattr"); 
	}
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		die("tcgetattr"); 
	}
	atexit(disableRawMode);  

	struct termios raw = orig_termios; 
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
	raw.c_oflag &= ~(OPOST); 
	raw.c_cflag |= (CS8); 
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
	raw.c_cc[VMIN] = 0; 
	raw.c_cc[VTIME] = 1; 

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		die("tcsetattr"); 
	}
}

int main() {
	enableRawMode(); 

	while (1) {
		char c = '\0'; 
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
			die("read"); 
		}
		if (iscntrl(c)) {
			printf("%d\r\n", c); 
		} else {
			printf("%d (\'%c\')\r\n", c, c); 
		}
		if (c == 'q') break; 
	}

	return 0; 
}
```

 - `errno`: 来自 `<errno.h>`. 当 `tcgetattr()`, `tcsetattr()`, `read()` 发生错误时, 除了返回 `-1` 值之外还会设置 `errno` 的值以说明错误发生的原因. 
 - `EAGAIN`: 来自 `<errno.h>`. 在 `Cygwin` 上, 当 `read()` 超时时, 它返回 `-1` 并将 `erron` 设为 `EAGAIN`, 而不是返回 `0`. 为了兼容在 `Cygwin` 上的运行, 此处不将 `errno == EAGAIN` 认为是错误. 

使用 `./kilo < <kilo.c` 将文件作为标准输入, 或者使用 `echo test | ./kilo` 将管道作为标准输入, 均可以使 `tcgetattr()` 函数发生错误, 对应错误信息为: `Inappropriate ioctl for device`. 

### 分区

使用注释对各个部分进行区分以便管理. 

```c
/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

void die(const char *s) { … }

void disableRawMode() { … }

void enableRawMode() { … }

/*** init ***/

int main() { … }
```

## 原始输入和输出

### 按键 `Ctrl+Q` 退出程序

```c
/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** init ***/

int main() {
	enableRawMode(); 

	while (1) {
		char c = '\0'; 
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
			die("read"); 
		}
		if (iscntrl(c)) {
			printf("%d\r\n", c); 
		} else {
			printf("%d (\'%c\')\r\n", c, c); 
		}
		if (c == CTRL_KEY('q')) break; 
	}

	return 0; 
}
```

`CTRL_KEY` 宏用于获得传入字符参数低 `5` 位的值, 从效果来看, 这与 `Ctrl` + 字母按键的效果是一样的. 

### 重构键盘输入

```c
/*** terminal ***/

char editorReadKey() {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}
	return c; 
}

/*** input ***/
void editorProcessKeypress() {
	char c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			exit(0); 
			break; 
	}
}

/*** init ***/

int main() {
	enableRawMode(); 

	while (1) {
		editorProcessKeyPress(); 
	}

	return 0; 
}
```

 - `editorReadKey()` 函数用于读取按键信号. 之后会扩展其功能以便处理转义序列(`escape sequence`)如方向键. 它隶属于 `/*** terminal ***/` 部分, 处理终端的输入. 
 - `editorProcessKeypress` 函数用于等待按键信号并执行. 之后会扩展其功能以便处理各种 `Ctrl` + 字母按键组合或者其它特殊按键以实现不同的功能. 它隶属于 `/*** input ***/` 部分, 位于更高的抽象层次. 

此外, 我们去除了打印按键输入的部分, 以便防止这些输出影响后续的步骤. 

### 清空屏幕

为了实现文本编辑器的功能, 我们希望在用户每次按键之后在渲染(`render`)界面(`interface`). 一般的终端都是一行行运行过的命令与其输出, 文本编辑器的首要任务就是清空这些文字. 

```c
/*** output ***/

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); 
}

/*** init ***/

int main() {
	enableRawMode(); 

	while (1) {
		editorRefreshScreen(); 
		editorProcessKeypress(); 
	}

	return 0; 
}
```

 - `ssize_t write(int fildes, const void *buf, size_t nbyte)`: 来自 `<unistd.h>`. 将 `buf` 指针指向的缓存中的 `nbyte` 字节写入打开文件描述符 `fildes` 指向的文件. 返回值为成功写入的字节数, 返回值为 `-1` 表明发生了错误并设置 `errno` 的值. 
 - `STDOUT_FILENO`: 来自 `<unistd.h>`. 标准输出文件, 值为 `1`. 

在 `editorRefreshScreen` 中, 向标准输出文件中写入 `4` 个字节. 其中, 第一个字节使用十六进制表示(`\x1b`), 即 `27`, 是转义字符(`escape character`), 我们将在之后多次使用到它. 这 `4` 个字节组成一个转义序列(`escape sequence`), 转义序列总是以 `\x16[` 开头. 
`J` 字符指示的命令会根据选择的参数清空屏幕中的部分字符, 默认参数为 `0`: 
| 参数 | 意义 |
| --- | --- |
| `0` | 清空光标当前位置到屏幕末尾的所有内容. |
| `1` | 清空屏幕开头到光标当前位置的所有内容. |
| `2` | 清空整个屏幕的内容. |

在此处我们使用 `VT100` 转义字符, 它是广受现代终端模拟器支持的标准. 如果希望支持更多的终端, 可以通过使用 `ncurses` 库获得特定终端使用的转义字符. 

### 重定位光标(`cursor`)

运行程序之后发现光标位于屏幕左下方. 而文本编辑器希望光标的初始位置位于屏幕的左上角. 

```c
/*** output ***/

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); 
	write(STDOUT_FILENO, "\x1b[H", 3); 
}
```

`\x1b[a;bH` 用于将光标移动至 `a` 行 `b` 列, 其中 `a` 和 `b` 的默认值均为 `1`, 因此 `\x1b[H` 对应的就是屏幕的左上角. 

### 在退出的时候清空屏幕

如果在运行的时候发生错误, 错误信息会在光标所在位置开始打印, 这会导致屏幕上的信息杂乱(很有可能的情况是, 错误信息与之前编辑的文本混杂在一起). 为了避免这个问题, 我们需要在程序正常或因错误退出时清空屏幕并对光标进行重定位. 

```c
/*** terminal ***/

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); 
	write(STDOUT_FILENO, "\x1b[H", 3); 

	perror(s); 
	exit(1); 
}

/*** input ***/

void editorProcessKeypress() {
	char c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
	}
}
```

我们在 `die()` 函数和按键 `Ctrl+Q` 响应中添加了清空屏幕和光标重定位的功能. 

另一种方式是在 `atexit()` 函数中添加清空屏幕和光标重定位的功能函数, 但是这种方式会导致 `die()` 函数打印出的错误信息在打印之后被立刻清楚. 