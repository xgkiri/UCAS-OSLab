# Project4 VM
### C core

# 双核 -O2 共享内存 快照

# Qemu上模拟单核运行
相当于全程绑核
见sched.c: line 520注释

# 换页
调整mm.h: line 37宏定义MAX_PAGE_NUM为32
然后命令行执行exec test_swap &

当前版本为qemu版，在img最后padding了1GB

# 快照
命令行执行exec test_sp &