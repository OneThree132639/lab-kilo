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

### 波浪号(`Tildes`, `~`)

像 `vim` 一样, 我们使用位于行首的波浪号 `~` 表示在文件末尾之后的行, 也就是说, 这些行是不被包括在文件中的. 

```c
/*** output ***/

void editorDrawRows() {
	int y; 
	for (y = 0; y < 24; y++) {
		write(STDOUT_FILENO, "~\r\n", 3); 
	}
}

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); 
	write(STDOUT_FILENO, "\x1b[H", 3); 

	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3); 
}
```

`editorDrawRows()` 函数将用于绘制文本的每一行. 当前它的功能是在前 `24` 行的行首绘制一个波浪号, 表明对应行不是文件的一部分. 由于当前并不知道终端的具体大小, 因此我们简单将行数设置为 `24`. 在绘制完成后利用转义字符 `\x1b[H` 将光标移动回屏幕的左上角. 

### 全局状态

接下来我们希望获取终端的行数等信息, 但在此之前, 我们先定义一个结构体 `editorConfig` 来保存全局信息, 并将之前用于保存终端原始设置的全局变量 `orig_termios` 放入其中: 

```c
/*** data ***/

struct editorConfig {
	struct termios orig_termios; 
}; 

struct editorConfig E; 

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
		die("tcsetattr"); 
	}
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
		die("tcgetattr"); 
	}
	atexit(disableRawMode);  

	struct termios raw = E.orig_termios; 
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
```

并将之前使用的 `orig_termios` 替换为 `E.orig_termios`. 

补充: 在现代 `C` 标准中, 无参数函数的规范写法是在括号中加入 `void` 关键字, 而不是保留空括号(表明未指定参数). 为了避免编译器对该问题的警告, 之后的代码将采用使用 `void` 关键字的写法. 

### 窗口大小, 简单的方式

在大部分系统中, 可以通过调用 `ioctl()` 函数并使用 `TIOCGWINSZ` 参数获取终端的大小. `TIOCGWINSZ` 指 `Terminal IOCtl Get WINdow SiZe`, 而 `IOCtl` 指 `Input/Output Control`. 

```c
#include <sys/ioctl.h>

int getWindowSize(int *rows, int *cols) {
	struct winsize ws; 

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1; 
	} 
	*cols = ws.ws_col; 
	*rows = ws.ws_row; 
	return 0; 
}
```

`ioctl()` 函数, `TIOCGWINSZ` 宏和 `struct winsize` 结构体均来自 `<sys/ioctl.h>`. 

我们使用返回值来判断获取窗口大小是否成功, 并使用 `*rows` 和 `*cols` 引用来获得具体的行数和列数的值. 这是一种在 `C` 中常见的返回多个值的方式. 

接下来, 向全局状态结构体 `editorConfig` 中添加成员变量 `screenrows` 和 `screencols`, 并调用 `getWindowSize()` 函数获取行数和列数的值. 

```c
/*** data ***/

struct editorConfig {
	int screenrows; 
	int screencols; 
	struct termios orig_termios; 
}; 

/*** init ***/

void initEditor(void) {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}

int main(void) {
	enableRawMode(); 
	initEditor(); 

	while (1) {
		editorRefreshScreen(); 
		editorProcessKeypress(); 
	}

	return 0; 
}
```

`initEditor()` 函数将用于初始化 `E` 结构体中的所有字段. 

接下来我们可以显示正确行数的波浪号, 而不是固定为 `24` 行: 

```c
/*** output ***/

void editorDrawRows(void) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		write(STDOUT_FILENO, "~\r\n", 3); 
	}
}
```

现在我们可以打印出正确行数的波浪号了. 

### 窗口大小, 困难的方式

并不是所有的系统都可以使用 `ioctl()` 函数获取窗口大小, 因此我们需要一个回落方案解决这些系统的获取窗口大小问题. 

该方案的策略是将光标放置屏幕的右下方, 然后使用转义序列获取光标的坐标, 这样就可以获得窗口的行数的列数. 

首先, 将光标移动至屏幕的右下方: 

```c
/*** terminal ***/

int getWindowSize(int *rows, int *cols) {
	struct winsize ws; 

	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; 
		editorReadKey(); 
		return -1; 
	} 
	*cols = ws.ws_col; 
	*rows = ws.ws_row; 
	return 0; 
}
```

由于没有直接的“将光标移动至屏幕的右下方”的命令, 我们使用连续的两个转义序列 `\x1b[999C` 和 `\x1b[999B`, 分别表示“将光标右移 `999` 个单位”和“将光标下移 `999` 个单位”. 参数 `999` 表示移动的单位数, 将其设置为一个足够大的数以便确保光标可以移动至屏幕的右下方. 当光标移动至屏幕边缘的时候, `C` 和 `B` 转义序列不会使其进一步移动, 也就是说, 不用担心光标会移动到屏幕之外. 由于 `H` 命令(将光标移动到固定的坐标)并没有说明当参数超出屏幕时的行为, 为了防止未定义行为的发生, 不使用 `\x1b[999;999H` 转义序列. 

注意到我们在使用 `ioctl()` 函数之前添加了 `1 ||` 使得该条件判断式始终为真, 以便调试这一回落分支. 

由于在该分支中总是返回 `-1`, 在成功使用转义序列之后, 调用一次 `editorReadKey()` 函数以便停顿观察程序状态. 运行程序, 在正常情况下, 可以看到光标位于屏幕的右下方. 按任意按键, 可以看到清除屏幕之后显示的调用 `die()` 函数打印的错误信息. 

接下来是获取光标的坐标, 使用 `n` 转义序列可以获取终端的状态信息, 其中参数 `6` 可以获取光标位置有关信息. 

```c
/*** terminal ***/

int getCursorPosition(int *rows, int *cols) {
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; 

	printf("\r\n"); 
	char c; 
	while (read(STDIN_FILENO, &c, 1) == 1) {
		if (iscntrl(c)) {
			printf("%d\r\n", c); 
		} else {
			printf("%d (\'%c\')\r\n", c, c); 
		}
	}

	editorReadKey(); 
	return -1; 
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws; 

	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; 
		return getCursorPosition(rows, cols); 
	} 
	*cols = ws.ws_col; 
	*rows = ws.ws_row; 
	return 0; 
}
```

尝试使用上述程序查看使用 `\x1b[6n` 转义序列得到的返回值, 得到的结果为: 

```text
27
91 ('[')
51 ('3')
53 ('5')
59 (';')
49 ('1')
51 ('3')
53 ('5')
82 ('R')
```

可以看到返回的结果是一个转义序列, 其格式为 `\x1b[rows;colsR]`, 其中 `rows` 和 `cols` 分别为行数和列数. 

和之前一样, 我们调用了一个 `editorReadKey()` 函数以停顿程序并观察输出. 

为了从这串但会转义序列中提取行数和列数, 首先需要将其读取至缓冲区中, 从标准输入中逐个读取字符直至遇到 `'R'`: 

```c
int getCursorPosition(int *rows, int *cols) {
	char buf[32]; 
	unsigned int i = 0; 

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; 

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break; 
		if (buf[i] == 'R') break; 
		i++; 
	}
	buf[i] = '\0'; 

	printf("\r\n&buf[1]: \'%s\'\r\n", &buf[1]); 

	editorReadKey(); 
	return -1; 
}
```

由于 `buf` 读取到的第一个字节是 `\x1b`, 即转义字符, 如果将其通过 `printf()` 函数会进行转义而无法打印出具体的字符, 因此向 `printf()` 函数中传入 `&buf[1]` 以打印后续的字符, 输出应当类似: 

```text
&buf[1]: '[35;135'
```

此外, `printf()` 函数要求字符数组以 `\0` 标志结束, 因此, 我们在 `buf` 的最后一个字节处添加 `\0`. 

接下来, 使用 `sscanf()` 函数提取字符串中的行数和列数: 

```c
/*** terminal ***/

int getCursorPosition(int *rows, int *cols) {
	char buf[32]; 
	unsigned int i = 0; 

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; 

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break; 
		if (buf[i] == 'R') break; 
		i++; 
	}
	buf[i] = '\0'; 

	if (buf[0] != '\x1b' || buf[1] != '[') return -1; 
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; 
 
	return 0; 
}
```

`sscanf()` 函数来自 `<stdio.h>`. 修改之后运行程序可以看到程序输出了正确行数的波浪号. 

在确认运行正常之后, 我们可以将之前用于调试的 `1 ||` 判断条件删除: 

```c
/*** terminal ***/

int getWindowSize(int *rows, int *cols) {
	struct winsize ws; 

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; 
		return getCursorPosition(rows, cols); 
	} 
	*cols = ws.ws_col; 
	*rows = ws.ws_row; 
	return 0; 
}
```

### 最后一行

观察我们现在的程序运行, 你可以发现最后一行并没有波浪号, 实际上, 当前的光标指向的是第 `2` 行而不是第 `1` 行, 这是因为我们在输出最后一行的波浪号之后继续输出了 `\r\n`, 导致回车和换行的发生, 使第 `1` 行上移至屏幕之外, 光标回到左上角的时候自然位于第 `2` 行. 因此, 我们需要避免在最后一行的波浪号输出完成之后继续输出 `\r\n`: 

```c
/*** output ***/

void editorDrawRows(void) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		write(STDOUT_FILENO, "~", 1);
		
		if (y < E.screenrows - 1) {
			write(STDOUT_FILENO, "\r\n", 2); 
		}
	}
}
```

### 添加缓冲

当前我们的程序中存在着大量调用 `write()` 函数的行为. 这对屏幕刷新而言不是一个很好的方法, 容易导致闪屏. 更加好的方法是将需要输出的字符串拼接起来, 然后一次性输出. 因此, 我们需要一个支持添加操作的字符串缓冲. 

```c
/*** append buffer ***/

struct abuf {
	char *b; 
	int len; 
}; 

#define ABUF_INIT {NULL, 0}
```

添加缓冲结构体包括一个指向缓冲内存的指针 `b` 和长度 `len`. 常量 `ABUF_INIT` 表示一个空添加缓冲, 它用于构建 `abuf` 类变量. 

接下来, 我们定义 `abAppend()` 操作函数和 `abFree()` 析构函数: 

```c
#include <string.h>

/*** append buffer ***/

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len); 

	if (new == NULL) return; 
	memcpy(&new[ab->len], s, len); 
	ab->b = new; 
	ab->len += len; 
}

void abFree(struct abuf *ab) {
	free(ab->b); 
}
```

其中, `realloc()` 和 `free()` 函数均来自 `stdlib.h`, `memcpy()` 函数来自 `string.h`. 

 - `void *realloc(void *ptr, size_t size)`: 改变 `ptr` 指向的内存的大小至 `size`, 原大小和 `size` 中的较小值以内的内容将会被复制, 如果发生了内存的整体移动, 原来的内存将会被释放, 如果 `ptr` 为 `NULL`, 则效果等同于分配内存, 如果 `size` 为 `0`, 则效果等同于释放内存. 返回值为指向新分配的地址的指针. 
 - `void free(void *ptr)`: 释放 `ptr` 指向的内存, 当 `ptr` 为 `NULL` 时不进行任何操作. 
 - `void *memcpy(void *s1, const void *s2, size_t n)`: 将 `s2` 指向的内存中的前 `n` 个字节复制至 `s1` 的前 `n` 个字节中. 返回值为 `s1`. 

在完成上面两个功能函数之后, 我们的 `abuf` 可以正式投入使用了: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		abAppend(ab, "~", 1);
		
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}

void editorRefreshScreen(void) {
	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[2J", 4); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	abAppend(&ab, "\x1b[H", 3); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

### 在重绘屏幕的时候隐藏光标

当光标位于屏幕中间时, 刷新屏幕会导致光标四处跳动, 造成不好的操作体验. 因此我们希望在屏幕刷新之前隐藏光标, 并在刷新完毕之后重新显示. 

```c
/*** output ***/

void editorRefreshScreen(void) {
	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[2J", 4); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	abAppend(&ab, "\x1b[H", 3); 
	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

其中, 我们使用到了 `l` 和 `h` 转义字符, 它们分别用于复位终端模式和设置终端模式, `?25` 控制光标的可见与不可见. 

部分终端可能不支持隐藏和显示光标的功能, 但是这些代码不会对其它功能造成影响, 因此可以放心保留. 

### 逐行清除

比起每次清除整个屏幕, 在重绘的时候对对应行进行清除显然是一个更好的方法. 因此, 我们移除代码中的 `\x1b[2J` 转义序列, 转而在每一行末尾用 `\x1b[K` 转义序列. 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		abAppend(ab, "~", 1);
		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}

void editorRefreshScreen(void) {
	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	abAppend(&ab, "\x1b[H", 3); 
	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

`K` 支持的参数与 `J` 类似: 
| 参数 | 意义 |
| --- | --- |
| `0` | 清空光标当前位置到行末尾的所有内容. |
| `1` | 清空行开头到光标当前位置的所有内容. |
| `2` | 清空整个行的内容. |

我们使用默认的模式, 即 `0`. 

### 欢迎信息

现在让我们显示一些欢迎信息, 比如让编辑器的名字和版本号显示在屏幕上三分之一处的位置. 

```c
/*** defines ***/

#define KILO_VERSION "0.0.1"

/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		if (y == E.screenrows / 3) {
			char welcome[80]; 
			int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
			if (welcomelen > E.screencols) welcomelen = E.screencols; 
			abAppend(ab, welcome, welcomelen); 
		} else {
			abAppend(ab, "~", 1);
		}
		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

其中, `int snprintf(char *s, size_t n, const char *format, ...)` 来自 `<stdio.h>`, 将格式化字符串输出至缓冲 `s` 中, `n` 指示缓冲 `s` 的大小. 

此处对过窄的终端进行了适应处理, 将超出终端宽度的部分截去. 

接下来将欢迎信息放到行中央: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		if (y == E.screenrows / 3) {
			char welcome[80]; 
			int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
			if (welcomelen > E.screencols) welcomelen = E.screencols; 
			int padding = (E.screencols - welcomelen) / 2; 
			if (padding) {
				abAppend(ab, "~", 1); 
				padding--; 
			}
			while (padding--) abAppend(ab, " ", 1); 
			abAppend(ab, welcome, welcomelen); 
		} else {
			abAppend(ab, "~", 1);
		}
		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

现在可以看到欢迎信息位于行正中间, 同时对应行的行首有一个波浪号(在终端足够宽的条件下). 

### 移动光标

现在让我们转向输入, 我们希望用户可以控制光标的移动, 首先需要跟踪光标的坐标. 利用全局状态 `editorConfig` 中的变量记录光标当前的坐标: 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int screenrows; 
	int screencols; 
	struct termios orig_termios; 
}; 

struct editorConfig E; 

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

其中, `E.cx` 指光标的所在列, `E.cy` 指光标的所在行. 注意这两个变量的索引基于 `0`, 而转义序列 `H` 的索引基于 `1`. 

接下来通过转义序列在每次刷新的时候将光标移动到 `E.cx` 和 `E.cy` 指向的位置: 

```c
/*** output ***/

void editorRefreshScreen(void) {
	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); 
	abAppend(&ab, buf, strlen(buf)); 

	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

其中, `strlen()` 函数来自 `<string.h>`. 其返回以 `\0` 标志结尾的字符串的长度, 不包括 `\0`. 

我们使用一个指定了坐标的 `H` 转义序列代替了原来的默认 `H` 转义序列. 

接下来我们可以尝试通过按键控制光标的移动, 首先从 `W` `A` `S` `D` 这四个按键开始. 按照这几个按键在键盘上的位置, 我们规定: 
| 按键 | 移动方向 |
| --- | --- |
| `W` | 上 |
| `A` | 左 | 
| `S` | 下 |
| `D` | 右 |

```c
/*** input ***/

void editorMoveCursor(char key) {
	switch (key) {
		case 'a': 
			E.cx--; 
			break;
		case 'd': 
			E.cx++; 
			break;
		case 'w': 
			E.cy--; 
			break;
		case 's': 
			E.cy++; 
			break;
	}
}

void editorProcessKeypress(void) {
	char c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case 'w': 
		case 'a': 
		case 's': 
		case 'd': 
			editorMoveCursor(c); 
			break; 
	}
}
```

现在程序可以成功使用这些按键控制光标. 

### 方向键

接下来让我们使用方向键替换先前的 `W` `A` `S` `D`. 在先前的步骤中我们知道了方向键信号实际上是一类转义序列: 

| 方向键 | 对应转义序列 |
| --- | --- |
| 上 | `\x1b[A` |
| 下 | `\x1b[B` |
| 右 | `\x1b[C` |
| 左 | `\x1b[D` |

首先修改 `editorReadKey` 使其可以处理转义字符: 

```c
/*** terminal ***/

char editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			switch (seq[1]) {
				case 'A': return 'w'; 
				case 'B': return 's'; 
				case 'C': return 'd'; 
				case 'D': return 'a'; 
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}
```

设置 `buf` 的长度为 `3` 是因为后续需要处理更加长的转义序列. 

现在程序可以处理方向键的信号了. 

当前我们将方向键信号与 `'w'`, `'a'`, `'s'`, `'d'` 字符进行绑定, 考虑到后续这些键应当作为正常输入使用, 我们创建常量来指示方向键: 

```c
/*** defines ***/

enum editorKey {
	ARROW_LEFT = 'a', 
	ARROW_RIGHT = 'd', 
	ARROW_UP = 'w', 
	ARROW_DOWN = 's'
}; 

/*** terminal ***/

char editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			switch (seq[1]) {
				case 'A': return ARROW_UP; 
				case 'B': return ARROW_DOWN; 
				case 'C': return ARROW_RIGHT; 
				case 'D': return ARROW_LEFT; 
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}

/*** input ***/

void editorMoveCursor(char key) {
	switch (key) {
		case ARROW_LEFT: 
			E.cx--; 
			break;
		case ARROW_RIGHT: 
			E.cx++; 
			break;
		case ARROW_UP: 
			E.cy--; 
			break;
		case ARROW_DOWN: 
			E.cy++; 
			break;
	}
}

void editorProcessKeypress(void) {
	char c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

在此基础上, 我们可以为 `enum editorKey` 选择不会与已知信号发生冲突的值, 比如一个足够大的超出 `ASCII` 编码范围的数: 

```c
/*** defines ***/

enum editorKey {
	ARROW_LEFT = 1000, 
	ARROW_RIGHT, 
	ARROW_UP, 
	ARROW_DOWN
}; 

/*** terminal ***/

int editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			switch (seq[1]) {
				case 'A': return ARROW_UP; 
				case 'B': return ARROW_DOWN; 
				case 'C': return ARROW_RIGHT; 
				case 'D': return ARROW_LEFT; 
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}

/*** input ***/

void editorMoveCursor(int key) {
	switch (key) {
		case ARROW_LEFT: 
			E.cx--; 
			break;
		case ARROW_RIGHT: 
			E.cx++; 
			break;
		case ARROW_UP: 
			E.cy--; 
			break;
		case ARROW_DOWN: 
			E.cy++; 
			break;
	}
}

void editorProcessKeypress(void) {
	int c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

由于 `enum` 会自动按序将未显式赋值的枚举量复制, 因此将 `ARROW_LEFT` 赋值为 `1000` 之后, `ARROW_RIGHT`, `ARROW_UP`, `ARROW_DOWN` 会被依次赋值为 `1001`, `1002`, `1003`. 随着这个改动, 原本部分类型为 `char` 的变量或函数返回值应当修改为 `int`. 

现在在程序中只能通过方向键控制光标的移动. 但是, 如果你的手速很快, 或者你可以尝试调整 `void enableRawMode(void)` 函数中 `raw.c_cc[VTIME] = 1;` 语句的赋值, 
你可以在程序中通过连续按 `Escape` + `[` + `Shift+A` 的方式实现光标的上移, 这相当于手动输入上箭头的转义序列, 其它方向键同理. 

### 防止光标移动出屏幕

当前程序允许 `E.cx` 和 `E.cy` 变为负数或者超出 `E.screencols` 和 `E.screenrows`, 为了解决这个问题, 加入边界检查: 

```c
/*** input ***/

void editorMoveCursor(int key) {
	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			}
			break;
		case ARROW_RIGHT: 
			if (E.cx != E.screencols - 1) {
				E.cx++; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy != E.screenrows - 1) {
				E.cy++; 
			}
			break;
	}
}
```

### 页面上滚 `Page Up` 与页面下滚 `Page Down` 键

我们需要实现更多类似方向键的底层按键. 让我们从页面上滚 `Page Up` 和页面下滚 `Page Down` 开始, 它们对应的转义序列分别为 `\x1b[5~` 和 `\x1b[6~`. 

```c
/*** defines ***/

enum editorKey {
	ARROW_LEFT = 1000, 
	ARROW_RIGHT, 
	ARROW_UP, 
	ARROW_DOWN, 
	PAGE_UP, 
	PAGE_DOWN
}; 

/*** terminal ***/

int editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			if (seq[0] >= '0' && seq[0] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b'; 
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '5': return PAGE_UP; 
						case '6': return PAGE_DOWN; 
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP; 
					case 'B': return ARROW_DOWN; 
					case 'C': return ARROW_RIGHT; 
					case 'D': return ARROW_LEFT; 
				}
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}
```

让我们利用 `Page Up` 和 `Page Down` 实现一些功能. 比如, 按下 `Page Up` 时, 让光标移动到屏幕最上方, 按下 `Page Down` 时, 让光标移动到屏幕最下方: 

```c
/*** input ***/

void editorProcessKeypress(void) {
	int c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case PAGE_UP: 
		case PAGE_DOWN: {
			int times = E.screenrows; 
			while (times--) {
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN); 
			}
			break;
		}
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

在带有 `fn` 按键的键盘上, 使用 `fn+↑` 和 `fn+↓` 可能可以实现 `Page Up` 和 `Page Down` 的效果. (暂时未在 MacOS 上复现, 推测可能是终端的问题. 在 WSL 上使用 `Page Up` 和 `Page Down` 也未成功, 需要进一步实验等)

### `Home` 和 `End` 键

接下来让我们实现 `Home` 键和 `End` 键的功能. 根据不同的操作系统和终端模拟器, `Home` 键对应的转义序列可能是 `\x1b[1~`, `\x1b[7~`, `\x1b[H` 或者 `\x1bOH`, `End` 键对应的转义序列可能是 `\x1b[4~`, `\x1b[8~`, `\x1b[F` 或者 `\x1bOF`. 我们需要处理这些所有的可能情况: 

```c
/*** defines ***/

enum editorKey {
	ARROW_LEFT = 1000, 
	ARROW_RIGHT, 
	ARROW_UP, 
	ARROW_DOWN, 
	HOME_KEY, 
	END_KEY, 
	PAGE_UP, 
	PAGE_DOWN
}; 

/*** terminal ***/

int editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			if (seq[0] >= '0' && seq[0] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b'; 
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY; 
						case '4': return END_KEY; 
						case '5': return PAGE_UP; 
						case '6': return PAGE_DOWN; 
						case '7': return HOME_KEY; 
						case '8': return END_KEY; 
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP; 
					case 'B': return ARROW_DOWN; 
					case 'C': return ARROW_RIGHT; 
					case 'D': return ARROW_LEFT; 
					case 'F': return END_KEY; 
					case 'H': return HOME_KEY; 
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'F': return END_KEY; 
				case 'H': return HOME_KEY; 
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}
```

然后我们可以为 `Home` 和 `End` 键添加功能, 比如, 当按下 `Home` 键时, 让光标移动到屏幕的最左端, 当按下 `End` 键时, 让光标移动到屏幕的最右端: 

```c
/*** input ***/

void editorProcessKeypress(void) {
	int c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case HOME_KEY: 
			E.cx = 0;
			break;
		case END_KEY: 
			E.cx = E.screencols - 1; 
			break; 
		case PAGE_UP: 
		case PAGE_DOWN: {
			int times = E.screenrows; 
			while (times--) {
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN); 
			}
			break;
		}
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

在带有 `fn` 按键的键盘上, 使用 `fn+←` 和 `fn+→` 可能可以实现 `Page Up` 和 `Page Down` 的效果. (在 MacOS Terminal 上同样未成功)

### 删除键 `Delete`

删除键对应的转义序列为 `\x1b[3~`, 我们将其简单添加至程序中, 暂时不为其添加功能: 

```c
/*** defines ***/

enum editorKey {
	ARROW_LEFT = 1000, 
	ARROW_RIGHT, 
	ARROW_UP, 
	ARROW_DOWN, 
	DEL_KEY, 
	HOME_KEY, 
	END_KEY, 
	PAGE_UP, 
	PAGE_DOWN
}; 

/*** terminal ***/

int editorReadKey(void) {
	int nread; 
	char c; 
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read"); 
		}
	}

	if (c == '\x1b') {
		char seq[3]; 

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b'; 
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b'; 

		if (seq[0] == '[') {
			if (seq[0] >= '0' && seq[0] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b'; 
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY; 
						case '3': return DEL_KEY; 
						case '4': return END_KEY; 
						case '5': return PAGE_UP; 
						case '6': return PAGE_DOWN; 
						case '7': return HOME_KEY; 
						case '8': return END_KEY; 
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP; 
					case 'B': return ARROW_DOWN; 
					case 'C': return ARROW_RIGHT; 
					case 'D': return ARROW_LEFT; 
					case 'F': return END_KEY; 
					case 'H': return HOME_KEY; 
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'F': return END_KEY; 
				case 'H': return HOME_KEY; 
			}
		}

		return '\x1b'; 
	} else {
		return c; 
	}
}
```

在带有 `fn` 按键的键盘上, 使用 `fn+Backspace` 可能可以实现 `Delete` 的效果. (在 MacOS Terminal 上捕捉到该信号. )

## 文本查看器

### 行查看器

首先创建一个数据类型存储编辑器中一行的文本: 

```c
/*** data ***/

typedef struct erow {
	int size; 
	char *chars; 
} erow; 

struct editorConfig {
	int cx, cy; 
	int screenrows; 
	int screencols; 
	int numrows; 
	erow row; 
	struct termios orig_termios; 
}; 

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.numrows = 0; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

`erow` 是 `editor row` 的意思, 存储一个指向动态字符数组的指针. 使用 `typedef` 让我们可以指代这个数据类型为 `erow` 而不必使用 `struct erow`. 

在全局状态 `editorConfig` 中, 我们添加了 `erow` 类型的 `row` 字段和 `int` 类型的 `numrows` 字段. 由于我们当前只希望显示 `1` 行的文本, 因此 `numrows` 的取值为 `0` 或 `1`, 在 `initEditor()` 函数中将其初始化为 `0`. 

接下来让我们往 `erow` 中填入一些文本, 暂时不考虑读取其它文件的内容, 而是硬编码 `"Hello, world!"`. 

```c
/*** includes ***/

#include <sys/types.h>

/*** file i/o ***/

void editorOpen(void) {
	char *line = "Hello, world!";
	ssize_t linelen = 13; 
	
	E.row.size = linelen; 
	E.row.chars = malloc(linelen + 1); 
	memcpy(E.row.chars, line, linelen); 
	E.row.chars[linelen] = '\0'; 
	E.numrows = 1; 
}

/*** init ***/

int main(void) {
	enableRawMode(); 
	initEditor(); 
	editorOpen(); 

	while (1) {
		editorRefreshScreen(); 
		editorProcessKeypress(); 
	}

	return 0; 
}
```

其中, `malloc()` 函数来自 `<stdlib.h>`, `ssize_t` 来自 `<sys/types.h>`. 

 - `void *malloc(size_t size)`: 分配 `size` 个字节的内存并返回一个指向该内存头部的指针. 
 - `ssize_t`: 有符号整数类型, 允许 `-1` 等值. 

`editorOpen()` 函数将用于从磁盘中打开并读取文件, 因此我们将其放入新的 `/*** file i/o ***/` 区域中. 

接下来显示这段文本: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		if (y >= E.numrows) {
			if (y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row.size; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, E.row.chars, len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

我们将之前显示波浪号 `~` 和项目名称与版本号的代码逻辑打包进一个判断分支中, 另一个分支用于打印正常的文本内容. 同样的, 如果文本内容超出了终端的宽度, 它会受到截断. 

接下来, 让我们真正读取一个文件: 

```c
/*** file i/o ***/

void editorOpen(char *filename) {
	FILE *fp = fopen(filename, "r"); 
	if (!fp) die("fopen"); 

	char *line = NULL; 
	size_t linecap = 0; 
	ssize_t linelen; 
	linelen = getline(&line, &linecap, fp); 
	if (linelen != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--; 
		}
		E.row.size = linelen; 
		E.row.chars = malloc(linelen + 1); 
		memcpy(E.row.chars, line, linelen); 
		E.row.chars[linelen] = '\0'; 
		E.numrows = 1; 
	}

	free(line); 
	fclose(fp); 
}

/*** init ***/

int main(int argc, char *argv[]) {
	enableRawMode(); 
	initEditor(); 
	if (argc >= 2) {
		editorOpen(argv[1]); 
	}

	while (1) {
		editorRefreshScreen(); 
		editorProcessKeypress(); 
	}

	return 0; 
}
```

 - `FILE`: 来自 `<stdio.h>`. 包含了管理文件流的所有信息(文件缓冲区指针, 当前读写位置, 错误状态等). 
 - `FILE *fopen(const char *filename, const char *mode)`: 来自 `<stdio.h>`. 打开 `filename` 字符串对应路径的文件, 并返回该文件的文件流指针. `mode` 的值为 `r` 或 `rb` 时表示读取文件. 
 - `ssize_t getline(char **lineptr, size_t *n, FILE *stream)`: 来自 `<stdio.h>`. 其中 `lineptr` 是指向字符指针的指针, 如果为 `NULL`, 则 `getline() ` 函数会为其分配内存, `n` 为指向存储分配缓冲区大小的指针, `stream` 为要读取的文件流指针. 返回值为成功读取的字符数(不包括末尾的 `\0`), 如果读取失败(如已经到达文件末尾)则返回 `-1`. 

`argc` 是命令行参数的个数, `argv` 是一个指向参数的指针数组, `argv[0]` 为可执行文件名, 在本项目下, `argv[0]` 一般为 `kilo` . `argv[1]` 为第一个参数, 我们期望是文件名. 

在当前, 我们只读取 `1` 行文本, 在之后读取多行文本时将会重复使用 `getline()` 函数, 它自动管理内存的功能非常有效. 

我们去除了读取到的行文本末尾的 `\r` 和 `\n` 字符, 因为我们通过 `erow` 手动管理一行的文本, 因此在此处我们没有必要将它们打印出来. 

由于 `getline()` 函数属于 `<stdio.h>` 的 GNU 扩展函数, 对于部分编译器, 其声明可能不会被激活, 因此需要在头文件之前加上功能测试宏 `feature test macro`: 

```c
/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
```

最后, 我们修改一个小地方: 只有当没有输入参数的时候显示程序名称与版本号: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		if (y >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row.size; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, E.row.chars, len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

### 多行

为了存储多行的文本, 我们需要让 `E.row` 变为存储 `erow` 结构体的数组, 由于它是动态变化的, 因此 `E.row` 应当为 `erow` 类型的指针, 并且其初始值为 `NULL`. 需要注意的是, 这一改动会影响很大一部分当前的代码, 因此接下来的一部分步骤完成之后的代码是无法通过编译的. 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int screenrows; 
	int screencols; 
	int numrows; 
	erow *row; 
	struct termios orig_termios; 
}; 

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.numrows = 0; 
	E.row = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

接下来, 将在 `editorOpen()` 中初始化 `E.row` 的代码移动至新的函数 `editorAppendRow()` 中, 并将其放入新的区域 `/*** row operations ***/` 中. 

```c
/*** raw operations ***/

void editorAppendRow(char *s, size_t len) {
	E.row.size = len; 
	E.row.chars = malloc(len + 1); 
	memcpy(E.row.chars, s, len); 
	E.row.chars[len] = '\0'; 
	E.numrows = 1; 
}

/*** file i/o ***/

void editorOpen(char *filename) {
	FILE *fp = fopen(filename, "r"); 
	if (!fp) die("fopen"); 

	char *line = NULL; 
	size_t linecap = 0; 
	ssize_t linelen; 
	linelen = getline(&line, &linecap, fp); 
	if (linelen != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--; 
		}
		editorAppendRow(line, linelen); 
	}

	free(line); 
	fclose(fp); 
}
```

我们希望 `editorAppendRow()` 支持添加多行, 这要求每次处理时为新行分配内存并将新行复制到 `E.row` 中的最后一行: 

```c
/*** raw operations ***/

void editorAppendRow(char *s, size_t len) {
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1)); 

	int at = E.numrows; 
	E.row[at].size = len; 
	E.row[at].chars = malloc(len + 1); 
	memcpy(E.row[at].chars, s, len); 
	E.row[at].chars[len] = '\0'; 
	E.numrows++; 
}
```

接下来修改 `editorDrawRows()` 函数以支持多行打印: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		if (y >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row[y].size; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, E.row[y].chars, len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

现在程序可以正常编译了, 但是当前的程序依旧只读取 `1` 行文本. 我们需要修改 `editorOpen()` 以支持多行文本的读取: 

```c
/*** file i/o ***/

void editorOpen(char *filename) {
	FILE *fp = fopen(filename, "r"); 
	if (!fp) die("fopen"); 

	char *line = NULL; 
	size_t linecap = 0; 
	ssize_t linelen; 
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--; 
		}
		editorAppendRow(line, linelen); 
	}

	free(line); 
	fclose(fp); 
}
```

现在程序可以正确读取多行文本了. 

### 竖直滚动

现在我们希望用户可以滚动浏览整个文件, 而不是只能浏览文件的前几行. 为了实现这一功能, 我们需要一个新的全局状态变量 `rowoff` 记录当前用户滚向的行. 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int rowoff; 
	int screenrows; 
	int screencols; 
	int numrows; 
	erow *row; 
	struct termios orig_termios; 
}; 

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.rowoff = 0; 
	E.numrows = 0; 
	E.row = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

将其初始化为 `0`, 表明我们默认滚动到文件的顶部. 

接下来我们根据 `E.rowoff` 的值来显示对应行的文本: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff; 
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row[filerow].size; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, E.row[filerow].chars, len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

接下来的问题是在哪里改变 `E.rowoff` 的值. 我们采取如下策略: 在刷新屏幕之前检查光标是否超出了屏幕的边界, 如果超出了则调整 `E.rowoff` 的值以保持光标始终在屏幕中. 我们将检查并调整的逻辑放在新的函数 `editorScroll()` 中, 并在刷新屏幕之前调用它: 

```c
/*** output ***/

void editorScroll(void) {
	if (E.cy < E.rowoff) {
		E.rowoff = E.cy; 
	} 
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1; 
	}
}

void editorRefreshScreen(void) {
	editorScroll();

	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); 
	abAppend(&ab, buf, strlen(buf)); 

	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

接下来, 我们允许光标的坐标超出屏幕的范围, 但是不超出文件的范围: 

```c
/*** input ***/

void editorMoveCursor(int key) {
	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			}
			break;
		case ARROW_RIGHT: 
			if (E.cx != E.screencols - 1) {
				E.cx++; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}
}
```

现在在程序中可以通过按下方向键浏览文件的剩余部分了. (如果你打开了一份含有 `Tab` 字符的文件, 你可能会发现这些字符可能没有被正确消除, 我们将在之后解决这一问题. 你可以尝试打开一份没有 `Tab` 字符的文件进行测试. )

如果你尝试向上回滚, 你会发现光标并没有按照预想的情况移动, 这是因为当前的 `E.cy` 不再指向屏幕上的坐标, 而是在文件中的坐标, 因此, 我们在设置坐标位置的时候需要减去 `E.rowoff` 偏移值: 

```c
/*** output ***/

void editorRefreshScreen(void) {
	editorScroll();

	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, E.cx + 1); 
	abAppend(&ab, buf, strlen(buf)); 

	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

现在程序可以正常滚动了. 

### 水平滚动

实现水平滚动的思路与竖直滚动类似, 首先添加一个全局状态变量 `coloff`: 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int rowoff; 
	int coloff;
	int screenrows; 
	int screencols; 
	int numrows; 
	erow *row; 
	struct termios orig_termios; 
}; 

*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.rowoff = 0; 
	E.coloff = 0; 
	E.numrows = 0; 
	E.row = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

对于每一行而言, 我们应当表示该行从 `E.coloff` 开始的文本, 一直到该行或者屏幕结束. 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff; 
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row[filerow].size - E.coloff; 
			if (len < 0) len = 0; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, &E.row[filerow].chars[E.coloff], len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

接着更新 `editorScroll()` 函数以处理水平滚动: 

```c
/*** output ***/

void editorScroll(void) {
	if (E.cy < E.rowoff) {
		E.rowoff = E.cy; 
	} 
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1; 
	}
	if (E.cx < E.coloff) {
		E.coloff = E.cx; 
	}
	if (E.cx >= E.coloff + E.screencols) {
		E.coloff = E.cx - E.screencols + 1; 
	}
}
```

然后是允许用户滚动出屏幕的右边缘: 

```c
/*** input ***/

void editorMoveCursor(int key) {
	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			}
			break;
		case ARROW_RIGHT: 
			E.cx++; 
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}
}
```

最后, 修复光标的定位问题: 

```c
/*** output ***/

void editorRefreshScreen(void) {
	editorScroll();

	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.cx - E.coloff) + 1); 
	abAppend(&ab, buf, strlen(buf)); 

	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

这样, 水平滚动也完成了. 

### 限制向右滚动

当前的 `E.cx` 和 `E.cy` 指的是光标在文件中的位置, 而不是在屏幕上的位置. 我们希望将 `E.cx` 和 `E.cy` 约束在一定的有效值内以避免用户控制光标移动超出文本的范围, 唯一的例外是光标可以移动至该行文本的最后一个字符之后的一格以便新文本的插入. 

首先, 让我们限制光标不超出当前行的末尾: 

```c
/*** input ***/

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 

	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			}
			break;
		case ARROW_RIGHT: 
			if (row && E.cx < row->size) {
				E.cx++; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}
}
```

由于 `E.cy` 允许指向文件行数多 `1` 行的位置(为了后续插入新行), 在 `editorMoveCursor()` 函数的开头需要检查是否对应行是否存在有效的指针 `erow` . 

### 移动光标至行尾

在运行当前程序的时候, 光标依然可以移动至超出文本的范围, 通过将光标移动至长行的末尾, 然后按上方向键或下方向键至短行即可做到这一点. 由于 `E.cx` 坐标没有改变, 因此光标超出了文本的范围. 

因此, 我们需要在发生此类情况的时候对 `E.cx` 进行修改: 

```c
/*** input ***/

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 

	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			}
			break;
		case ARROW_RIGHT: 
			if (row && E.cx < row->size) {
				E.cx++; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 
	int rowlen = row ? row->size : 0; 
	if (E.cx > rowlen) {
		E.cx = rowlen; 
	}
}
```

我们认为 `NULL` 行的长度为 `0`. 

### 在行首左移

让我们实现在一行的行首按下左方向键时移动至上一行的行尾的功能. 

```c
/*** input ***/

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 

	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			} else if (E.cy > 0) {
				E.cy--; 
				E.cx = E.row[E.cy].size; 
			}
			break;
		case ARROW_RIGHT: 
			if (row && E.cx < row->size) {
				E.cx++; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 
	int rowlen = row ? row->size : 0; 
	if (E.cx > rowlen) {
		E.cx = rowlen; 
	}
}
```

当然, 当光标位于第 `1` 行的时候没有向上移动的空间, 因此需要排除该情况. 

### 在行尾右移

同样地, 我们希望实现在一行的行尾按下右方向键时移动至下一行行首的功能. 

```c
/*** input ***/

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 

	switch (key) {
		case ARROW_LEFT: 
			if (E.cx != 0) {
				E.cx--; 
			} else if (E.cy > 0) {
				E.cy--; 
				E.cx = E.row[E.cy].size; 
			}
			break;
		case ARROW_RIGHT: 
			if (row && E.cx < row->size) {
				E.cx++; 
			} else if (row && E.cx == row->size) {
				E.cy++; 
				E.cx = 0; 
			}
			break;
		case ARROW_UP: 
			if (E.cy != 0) {
				E.cy--; 
			}
			break;
		case ARROW_DOWN: 
			if (E.cy < E.numrows) {
				E.cy++; 
			}
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; 
	int rowlen = row ? row->size : 0; 
	if (E.cx > rowlen) {
		E.cx = rowlen; 
	}
}
```

在此处我们要避免光标不在最后一行(即 `row` 指针为 `NULL`)的情况. 

### 渲染 `Tab`

运行 `./kilo Makefile`, 可以发现第 `2` 行的 `Tab` 占据了 `8` 列. `Tab` 占据的具体列数取决于终端及其设置. 

为了解决这个问题, 我们在 `erow` 结构体中添加一个新的字符串字段 `render` 表示对应行实际打印到屏幕上的字符. 当前我们暂时只使用 `render` 渲染 `Tab` 字符, 在之后我们会使用其打印更多不可打印的字符如将 `Ctrl+A` 打印为 `^A`, 这也是在终端中表示控制字符的通用方法. 

此外, `Tab` 的作用是将光标移动至下一个 `Tab` 停止点, 而不会对 `Tab` 中间的字符进行操作, 这是导致渲染出现问题的主要原因, 也是我们希望将 `Tab` 渲染为多个空格(`Space`)的原因. 

那么, 首先添加 `render` 字段和 `rsize` 字段, `rsize` 字段用于记录 `render` 字符串的长度. 并完成它们的初始化. 

```c
/*** data ***/

typedef struct erow {
	int size; 
	int rsize; 
	char *chars; 
	char *render; 
} erow; 

/*** row operations ***/

void editorAppendRow(char *s, size_t len) {
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1)); 

	int at = E.numrows; 
	E.row[at].size = len; 
	E.row[at].chars = malloc(len + 1); 
	memcpy(E.row[at].chars, s, len); 
	E.row[at].chars[len] = '\0'; 

	E.row[at].rsize = 0; 
	E.row[at].render = NULL; 

	E.numrows++; 
}
```

接下来, 创建 `editorUpdateRow()` 函数用于将 `erow.chars` 转换为 `erow.render`, 首先从简单的全部复制开始: 

```c
/*** row operations ***/

void editorUpdateRow(erow *row) {
	free(row->render); 
	row->render = malloc(row->size + 1); 

	int j; 
	int idx = 0; 
	for (j = 0; j < row->size; j++) {
		row->render[idx++] = row->chars[j]; 
	}
	row->render[idx] = '\0'; 
	row->rsize = idx; 
}

void editorAppendRow(char *s, size_t len) {
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1)); 

	int at = E.numrows; 
	E.row[at].size = len; 
	E.row[at].chars = malloc(len + 1); 
	memcpy(E.row[at].chars, s, len); 
	E.row[at].chars[len] = '\0'; 

	E.row[at].rsize = 0; 
	E.row[at].render = NULL; 
	editorUpdateRow(&E.row[at]); 

	E.numrows++; 
}
```

接下来, 使用 `erow.render` 和 `erow.rsize` 替换原本在显示 `erow` 中使用的 `erow.chars` 和 `erow.size`: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff; 
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row[filerow].rsize - E.coloff; 
			if (len < 0) len = 0; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, &E.row[filerow].render[E.coloff], len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
```

接下来修改代码使 `editorUpdateRow()` 函数支持 `Tab` 向多个空格 `Space` 的转换: 

```c
/*** row operations ***/

void editorUpdateRow(erow *row) {
	int tabs = 0; 
	int j; 
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') tabs++; 
	}

	free(row->render); 
	row->render = malloc(row->size + tabs * 7 + 1); 
 
	int idx = 0; 
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' '; 
			while (idx % 8 != 0) row->render[idx++] = ' '; 
		} else {
			row->render[idx++] = row->chars[j]; 
		}
	}
	row->render[idx] = '\0'; 
	row->rsize = idx; 
}
```

首先, 我们需要知道文本中一共存在的 `Tab` 个数以确定分配给 `erow.render` 的内存的大小. 每个 `Tab` 最多需要 `8` 个空格键 `Space`. 为了模拟 `Tab` 的效果, 当碰到 `Tab` 的时候, 填充空格直至移动至 `8` 的倍数列, 即 `Tab` 停止点. 

我们可以将 `Tab` 步长设置为常量: 

```c
/*** defines ***/

#define KILO_TAB_STOP 8

/*** row operations ***/

void editorUpdateRow(erow *row) {
	int tabs = 0; 
	int j; 
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') tabs++; 
	}

	free(row->render); 
	row->render = malloc(row->size + tabs * (KILO_TAB_STOP - 1) + 1); 
 
	int idx = 0; 
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' '; 
			while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' '; 
		} else {
			row->render[idx++] = row->chars[j]; 
		}
	}
	row->render[idx] = '\0'; 
	row->rsize = idx; 
}
```

这使得代码更易读的同时方便我们设置 `Tab` 步长. 

### `Tab` 与光标

光标在 `Tab` 附近移动的时候, 应当跳至 `Tab` 的停止点, 而不应当出现在 `Tab` 中间. 这也是当前的程序在 `Tab` 存在的行中光标无法移动至行尾的原因. 为了解决这个问题, 我们需要引入新的变量 `E.rx` 用于记录 `erow.render` 的索引, 而原来的 `E.cx` 用于记录 `erow.chars` 的索引. `E.rx` 比 `E.cx` 大一个与对应行中经过的 `Tab` 的个数正相关的数. 

在初始化的时候将 `E.rx` 设置为 `0`. 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int rx; 
	int rowoff; 
	int coloff;
	int screenrows; 
	int screencols; 
	int numrows; 
	erow *row; 
	struct termios orig_termios; 
}; 

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.rx = 0; 
	E.rowoff = 0; 
	E.coloff = 0; 
	E.numrows = 0; 
	E.row = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
}
```

我们先在 `editorScroll()` 函数中简单将 `E.cx` 替换为 `E.rx`. 

```c
/*** output ***/

void editorScroll(void) {
	E.rx = E.cx; 

	if (E.cy < E.rowoff) {
		E.rowoff = E.cy; 
	} 
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1; 
	}
	if (E.rx < E.coloff) {
		E.coloff = E.rx; 
	}
	if (E.rx >= E.coloff + E.screencols) {
		E.coloff = E.rx - E.screencols + 1; 
	}
}
```

接下来, 在 `editorRefreshScreen()` 函数中也将 `E.cx` 修改为 `E.rx`: 

```c
/*** output ***/

void editorRefreshScreen(void) {
	editorScroll();

	struct abuf ab = ABUF_INIT; 

	abAppend(&ab, "\x1b[?25l", 6); 
	abAppend(&ab, "\x1b[H", 3); 

	editorDrawRows(&ab);

	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1); 
	abAppend(&ab, buf, strlen(buf)); 

	abAppend(&ab, "\x1b[?25h", 6); 
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
```

接下来就是在 `editorScroll()`函数中将 `E.cx` 转换为 `E.rx`. 我们将转换的过程放在 `editorRowCxToRx()` 函数中. 转换的逻辑很简单: 计数本行 `E.cx` 左侧一共有多少个 `Tab`, 计算这些 `Tab` 占据的列数: 

```c
/*** row operations ***/

int editorRowCxToRx(erow *row, int cx) {
	int rx = 0; 
	int j; 
	for (j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') {
			rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP); 
		}
		rx++; 
	}
	return rx; 
}
```

同样地, 在遇到 `Tab` 的时候 `rx` 增加至下一个 `Tab` 停止点处. 

在 `editorScroll()` 函数的顶部调用 `editorRowCxToRx()` 以正确设置 `E.rx`: 

```c
/*** output ***/

void editorScroll(void) {
	E.rx = 0; 
	if (E.cy < E.numrows) {
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx); 
	}

	if (E.cy < E.rowoff) {
		E.rowoff = E.cy; 
	} 
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1; 
	}
	if (E.rx < E.coloff) {
		E.coloff = E.rx; 
	}
	if (E.rx >= E.coloff + E.screencols) {
		E.coloff = E.rx - E.screencols + 1; 
	}
}
```

现在光标应该可以正常运作了. 

### 使用页面上滚 `Page Up` 和页面下滚 `Page Down` 滚动屏幕

使用页面上滚 `Page Up` 和页面下滚 `Page Down` 实现整个页面的上滚和下滚: 

```c
/** input ***/

void editorProcessKeypress(void) {
	int c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case HOME_KEY: 
			E.cx = 0;
			break;
		case END_KEY: 
			E.cx = E.screencols - 1; 
			break; 
		case PAGE_UP: 
		case PAGE_DOWN: {
			if (c == PAGE_UP) {
				E.cy = E.rowoff; 
			} else if (c == PAGE_DOWN) {
				E.cy = E.rowoff + E.screenrows - 1; 
				if (E.cy > E.numrows) E.cy = E.numrows; 
			}
			int times = E.screenrows; 
			while (times--) {
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN); 
			}
			break;
		}
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

首先根据页面滚动的方向将光标移动至屏幕顶部或屏幕底部, 然后根据页面滚动的方向滚动一整个页面的大小, 实现页面滚动的效果. 

### 使用 `End` 键移动至当前行的末尾

```c
/*** input ***/

void editorProcessKeypress(void) {
	int c = editorReadKey(); 

	switch (c) {
		case CTRL_KEY('q'): 
			write(STDOUT_FILENO, "\x1b[2J", 4); 
			write(STDOUT_FILENO, "\x1b[H", 3); 
			exit(0); 
			break; 
		case HOME_KEY: 
			E.cx = 0;
			break;
		case END_KEY: 
			if (E.cy < E.numrows) {
				E.cx = E.row[E.cy].size; 
			}
			break; 
		case PAGE_UP: 
		case PAGE_DOWN: {
			if (c == PAGE_UP) {
				E.cy = E.rowoff; 
			} else if (c == PAGE_DOWN) {
				E.cy = E.rowoff + E.screenrows - 1; 
				if (E.cy > E.numrows) E.cy = E.numrows; 
			}
			int times = E.screenrows; 
			while (times--) {
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN); 
			}
			break;
		}
		case ARROW_UP:
		case ARROW_DOWN: 
		case ARROW_LEFT: 
		case ARROW_RIGHT: 
			editorMoveCursor(c); 
			break; 
	}
}
```

另外, 按 `Home` 键移动至行首的功能已经实现, 因为在此之前我们已经令 `E.cx` 与文件行相关而不是与屏幕行相关. 

### 状态条

在进入实现文本编辑之前, 让我们先实现状态条, 在状态条中会显示文件名称、文件总行数、当前所在行等信息. 在之后我们还会添加标识以表示文件在上一次保存之后是否存在修改, 以及在实现语法高亮的时候显示文件类型. 

首先, 为状态条在屏幕底部留出 `1` 行的空间: 

```c
/*** output ***/

void editorDrawRows(struct abuf *ab) {
	int y; 
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff; 
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80]; 
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION); 
				if (welcomelen > E.screencols) welcomelen = E.screencols; 
				int padding = (E.screencols - welcomelen) / 2; 
				if (padding) {
					abAppend(ab, "~", 1); 
					padding--; 
				}
				while (padding--) abAppend(ab, " ", 1); 
				abAppend(ab, welcome, welcomelen); 
			} else {
				abAppend(ab, "~", 1);
			}
		} else {
			int len = E.row[filerow].rsize - E.coloff; 
			if (len < 0) len = 0; 
			if (len > E.screencols) len = E.screencols; 
			abAppend(ab, &E.row[filerow].render[E.coloff], len); 
		}

		
		abAppend(ab, "\x1b[K", 3); 
		abAppend(ab, "\r\n", 2);
	}
}

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.rx = 0; 
	E.rowoff = 0; 
	E.coloff = 0; 
	E.numrows = 0; 
	E.row = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
	E.screenrows -= 1; 
}
```

在 `initEditor()` 中将 `E.screenrows` 减少了 `1`, 为状态条提供了空间. 同时, 移除了`editorDrawRows()` 中为了避免在最后一行加上换行符而使用的条件分支. 

值得注意的是, 在经过这些改动之后, 除了空出最后一行之外, 程序依旧可以正常执行浏览功能. 

为了让状态条更醒目, 我们希望它以反转色的形式显示, 也就是说, 如果终端原本是在白色背景上的黑色字, 那么状态条部分就是在黑色背景上的白色字. 使用转义序列 `\x1b[7m` 切换到反转色模式, 使用 `\x1b[m` 可以切换回来. 首先让我们绘制一个全为空格的状态条: 

```c
/*** output ***/

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4); 
	int len = 0; 
	while (len < E.screencols) {
		abAppend(ab, " ", 1); 
		len++; 
	}
	abAppend(ab, "\x1b[m", 3); 
}
```

转义序列 `m` 允许在其之后打印的文本获得不同的属性, 如 `1` 表示粗体, `4` 表示下划线, `5` 表示闪烁, `7` 表示反转色, `0` 表示清除属性. 一次性启用多个属性的情况, 属性之间通过 `;` 分隔. 

接下来让我们在状态条中加入文件名. 在全局状态 `editorConfig` 中新建一个字段 `filename` 用于存储文件名. 

```c
/*** data ***/

struct editorConfig {
	int cx, cy; 
	int rx; 
	int rowoff; 
	int coloff;
	int screenrows; 
	int screencols; 
	int numrows; 
	erow *row; 
	char *filename; 
	struct termios orig_termios; 
}; 

/*** file i/o ***/

void editorOpen(char *filename) {
	free(E.filename); 
	E.filename = strdup(filename); 

	FILE *fp = fopen(filename, "r"); 
	if (!fp) die("fopen"); 

	char *line = NULL; 
	size_t linecap = 0; 
	ssize_t linelen; 
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
			linelen--; 
		}
		editorAppendRow(line, linelen); 
	}

	free(line); 
	fclose(fp); 
}

/*** init ***/

void initEditor(void) {
	E.cx = 0; 
	E.cy = 0; 
	E.rx = 0; 
	E.rowoff = 0; 
	E.coloff = 0; 
	E.numrows = 0; 
	E.row = NULL; 
	E.filename = NULL; 

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize"); 
	}
	E.screenrows -= 1; 
}
```

 - `char *strdup(const char *s1)`: 来自 `<string.h>`. 返回一个可以被 `free()` 函数释放的指向 `s1` 的拷贝的指针, 如果字符串创建失败, 则返回空指针. 

接下来, 将文件名显示在状态条中. 如果没有传入文件名参数, 显示 `[No Name]`. 

```c
/*** output ***/

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4); 
	char status[80]; 
	int len = snprintf(status, sizeof(status), "%.20s - %d lines", E.filename ? E.filename : "[No Name]", E.numrows); 
	if (len > E.screencols) len = E.screencols; 
	abAppend(ab, status, len); 
	while (len < E.screencols) {
		abAppend(ab, " ", 1); 
		len++; 
	}
	abAppend(ab, "\x1b[m", 3); 
}
```

当窗口宽度不足的时候, 对状态条输出信息进行裁剪. 对于过剩的部分, 绘制空格以保证反转色部分填充一整行. 

接下来, 显示当前所在行的编号在状态条的右边缘. 

```c
/*** output ***/

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4); 
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s - %d lines", E.filename ? E.filename : "[No Name]", E.numrows); 
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.numrows); 
	if (len > E.screencols) len = E.screencols; 
	abAppend(ab, status, len); 
	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen); 
			break; 
		} else {
			abAppend(ab, " ", 1); 
			len++; 
		}
	}
	abAppend(ab, "\x1b[m", 3); 
}
```

由于 `E.cy` 的索引基于 `0`, 显示行号的时候需要加 `1`. 

当终端宽度不足的时候, 右侧的行号信息不会显示. 

