A daemon that executes a program when a certain input event is received,
handles power buttons, laptop lid switches, etc.

Dependencies

linux-headers
some libc (tested on musl)
sys/queue.h implementation (built-in to glibc, musl can extract glibc implementation)
