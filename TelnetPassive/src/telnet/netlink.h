
class netlink {
 protected:
    int net;
    int psocket;
 public:
    netlink();
    ~netlink();

    int listenPort(int debug, int port);
    void close(int doshutdown);

    int setdebug(int debug);
    void oobinline();
    void nonblock(int onoff);

    int stilloob();

    int send(const char *buf, int len, int flags);

    int getfd();
};

extern netlink nlink;
