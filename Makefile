# Makefile for the CS:APP Shell Lab

TEAM = NOBODY                # 队伍名（可修改为你的队伍名）
VERSION = 1                  # 版本号
HANDINDIR = /afs/cs/academic/class/15213-f02/L5/handin  # 提交作业的目录
DRIVER = ./sdriver.pl        # 测试驱动脚本
TSH = ./tsh                  # 你的 shell 程序可执行文件
TSHREF = ./tshref            # 参考 shell 程序可执行文件
TSHARGS = "-p"               # 传递给 shell 的参数
CC = gcc                     # C 语言编译器
CFLAGS = -Wall -O2           # 编译选项：显示所有警告，优化等级 2
FILES = $(TSH) ./myspin ./mysplit ./mystop ./myint  # 需要生成的可执行文件列表

all: $(FILES)                # 默认目标，生成所有可执行文件

##################
# Handin your work
##################
handin:                      # handin 目标：提交你的 tsh.c 文件
	cp tsh.c $(HANDINDIR)/$(TEAM)-$(VERSION)-tsh.c  # 拷贝 tsh.c 到提交目录，重命名为队伍名-版本号-tsh.c

##################
# Regression tests
##################

# Run tests using the student's shell program
test01:                      # 运行第 1 个测试用例，使用你的 shell
	$(DRIVER) -t trace01.txt -s $(TSH) -a $(TSHARGS)
test02:                      # 运行第 2 个测试用例
	$(DRIVER) -t trace02.txt -s $(TSH) -a $(TSHARGS)
test03:
	$(DRIVER) -t trace03.txt -s $(TSH) -a $(TSHARGS)
test04:
	$(DRIVER) -t trace04.txt -s $(TSH) -a $(TSHARGS)
test05:
	$(DRIVER) -t trace05.txt -s $(TSH) -a $(TSHARGS)
test06:
	$(DRIVER) -t trace06.txt -s $(TSH) -a $(TSHARGS)
test07:
	$(DRIVER) -t trace07.txt -s $(TSH) -a $(TSHARGS)
test08:
	$(DRIVER) -t trace08.txt -s $(TSH) -a $(TSHARGS)
test09:
	$(DRIVER) -t trace09.txt -s $(TSH) -a $(TSHARGS)
test10:
	$(DRIVER) -t trace10.txt -s $(TSH) -a $(TSHARGS)
test11:
	$(DRIVER) -t trace11.txt -s $(TSH) -a $(TSHARGS)
test12:
	$(DRIVER) -t trace12.txt -s $(TSH) -a $(TSHARGS)
test13:
	$(DRIVER) -t trace13.txt -s $(TSH) -a $(TSHARGS)
test14:
	$(DRIVER) -t trace14.txt -s $(TSH) -a $(TSHARGS)
test15:
	$(DRIVER) -t trace15.txt -s $(TSH) -a $(TSHARGS)
test16:
	$(DRIVER) -t trace16.txt -s $(TSH) -a $(TSHARGS)

# Run the tests using the reference shell program
rtest01:                     # 运行第 1 个测试用例，使用参考 shell
	$(DRIVER) -t trace01.txt -s $(TSHREF) -a $(TSHARGS)
rtest02:
	$(DRIVER) -t trace02.txt -s $(TSHREF) -a $(TSHARGS)
rtest03:
	$(DRIVER) -t trace03.txt -s $(TSHREF) -a $(TSHARGS)
rtest04:
	$(DRIVER) -t trace04.txt -s $(TSHREF) -a $(TSHARGS)
rtest05:
	$(DRIVER) -t trace05.txt -s $(TSHREF) -a $(TSHARGS)
rtest06:
	$(DRIVER) -t trace06.txt -s $(TSHREF) -a $(TSHARGS)
rtest07:
	$(DRIVER) -t trace07.txt -s $(TSHREF) -a $(TSHARGS)
rtest08:
	$(DRIVER) -t trace08.txt -s $(TSHREF) -a $(TSHARGS)
rtest09:
	$(DRIVER) -t trace09.txt -s $(TSHREF) -a $(TSHARGS)
rtest10:
	$(DRIVER) -t trace10.txt -s $(TSHREF) -a $(TSHARGS)
rtest11:
	$(DRIVER) -t trace11.txt -s $(TSHREF) -a $(TSHARGS)
rtest12:
	$(DRIVER) -t trace12.txt -s $(TSHREF) -a $(TSHARGS)
rtest13:
	$(DRIVER) -t trace13.txt -s $(TSHREF) -a $(TSHARGS)
rtest14:
	$(DRIVER) -t trace14.txt -s $(TSHREF) -a $(TSHARGS)
rtest15:
	$(DRIVER) -t trace15.txt -s $(TSHREF) -a $(TSHARGS)
rtest16:
	$(DRIVER) -t trace16.txt -s $(TSHREF) -a $(TSHARGS)

# clean up
clean:                       # 清理目标，删除所有生成的可执行文件、目标文件和临时文件
	rm -f $(FILES) *.o *~


