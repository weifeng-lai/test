//
// Credit to 李兆龙
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::vector;

int main() {
	// 硬盘上没空间了，所以只生成了10个文件，想要结果更准确且机器允许的话可以生成500个向上
    //
    // 记录几个有用的参数 for open()
    // O_DSYNC 告诉内核，当向文件写入数据的时候，只有当数据写到了磁盘时，写入操作才算完成(write才返回成功);
    // O_SYNC 比 O_DSYNC 更严格, 不仅要求数据已经写到了磁盘,
    // 而且对应的数据文件的属性(例如文件长度等)也需要更新完成才算 write 操作成功, 可见 O_SYNC 比 O_DSYNC 要多做一些操作;
    // O_RSYNC 表示文件读取时，该文件的 os cache 必须已经全部 flush 到磁盘了;
    // O_DIRECT 是 open 的参数, 若使用, 则读/写操作都会跳过 os cache, 直接在 device(disk) 上读/写,
    // 因为没有了 os cache, 所以会 O_DIRECT 降低文件的顺序读写的效率
    //
    // BTW, what's non-buffered I/O?
    // How program writes data be like:
    //     Application <-> Operation System Cache <-> File System/Volume Manager <-> Device
    // By set O_DIRECT flag in open(), command can directly access to data from device
    //
    // 记录几个有用的系统调用 for write()
    // fdatasync(), fsync(), sync()
    // write 后, 可以调用 fdatasync 来确保文件数据 flush 到了 disk 上
    // 当 fdatasync 返回成功后, 那么可以认为数据已经写到了磁盘上, 类似的系统调用有 fsync 和 sync
    // fdatasync 被认为是安全的, 因为在 MySQL 总会调用 fsync 来 flush 数据
    // 可以发现 O_DIRECT 和 fdatasync 很类似, 但是它会使用 O_DIRECT 来打开数据文件
    // 有数据表明, 如果是大量随机写入操作, O_DIRECT 会提升效率, w_await 指标提升很明显, 但是顺序写入和读取效率都会降低, 所以使用 O_DIRECT 需要谨慎
    //
    const size_t start_fd = 100;
    const size_t file_cnt = 100;
    vector<int> fds;
    fds.reserve(file_cnt);
    size_t len = 1 * 1024 * 1024;
    for (size_t i = start_fd; i < start_fd + file_cnt; ++i) {
        // 对比: 把 Direct IO 改成正常 IO, 也就是把 open 里面的 O_DIRECT 参数去掉, 然后内存分配改为malloc, 继续监测 IO,
        string filename = std::to_string(i);
        printf("opening file %s\n", filename.c_str());
        // int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_DIRECT, 0755);
        int fd = open(std::to_string(i).c_str(), O_RDWR | O_CREAT, 0755);
        fds.push_back(fd);
        char* buf;
        size_t buf_size = len * 2;
        // posix_memalign((void**)&buf, getpagesize(), buf_size);
        buf = (char*)malloc(sizeof(char) * buf_size);
        int ret = write(fd, buf, len);
        printf("fd: %d, real write: %d, expected write: %d\n", fd, ret, len);
        if(ret != len){
            cerr << "partially write!\n";
        }
        free(buf);
    }

    for(int x : fds) {
        close(x);
    }

    return  0;
}
